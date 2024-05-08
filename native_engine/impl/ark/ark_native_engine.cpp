/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ark_native_engine.h"

#ifdef ENABLE_HITRACE
#include <sys/prctl.h>
#endif

#include <sstream>
#include "ark_native_deferred.h"
#if !defined(is_arkui_x) && defined(OHOS_PLATFORM)
#include "ark_native_hybrid_stack.h"
#endif
#include "ark_native_reference.h"
#include "native_engine/native_property.h"
#include "native_engine/native_utils.h"
#include "native_sendable.h"
#include "securec.h"
#include "utils/log.h"
#if !defined(PREVIEW) && !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)
#include "parameters.h"
#include <uv.h>
#endif
#ifdef ENABLE_CONTAINER_SCOPE
#include "core/common/container_scope.h"
#endif
#ifdef ENABLE_HITRACE
#include "hitrace/trace.h"
#include "hitrace_meter.h"
#include "parameter.h"
#include "musl_preinit_common.h"
#include "memory_trace.h"
struct alignas(8) HookJsConfig { // 8 is 8 bit
    int32_t jsStackReport = 0;
    uint8_t maxJsStackDepth = 0;
    bool jsFpUnwind = false;
    char filterNapiName[64] = { "" };
};

class BlockHookScope {
public:
    BlockHookScope()
    {
        previousState_ = __set_hook_flag(false);
    }
    ~BlockHookScope()
    {
        __set_hook_flag(previousState_);
    }
private:
    bool previousState_ {true};
};

static HookJsConfig* g_hookJsConfig = nullptr;
static std::once_flag g_hookOnceFlag;
static std::string JS_CALL_STACK_DEPTH_SEP = ","; // ',' is js call stack depth separator
static std::string JS_SYMBOL_FILEPATH_SEP = "|";  // '|' is js symbol and filepath separator
static constexpr uint64_t BUF_SIZE = 128;
#endif

using panda::JsiRuntimeCallInfo;
using panda::BooleanRef;
using panda::ObjectRef;
using panda::StringRef;
using panda::Global;
using panda::JSNApi;
using panda::FunctionRef;
using panda::PrimitiveRef;
using panda::ArrayBufferRef;
using panda::TypedArrayRef;
using panda::PromiseCapabilityRef;
using panda::PropertyAttribute;
using panda::NativePointerRef;
using panda::SymbolRef;
using panda::IntegerRef;
using panda::DateRef;
using panda::BigIntRef;
static constexpr auto PANDA_MAIN_FUNCTION = "_GLOBAL::func_main_0";
static constexpr auto PANDA_MODULE_NAME = "_GLOBAL_MODULE_NAME";
static constexpr auto PANDA_MODULE_NAME_LEN = 32;
static std::unordered_set<std::string> NATIVE_MODULE = {"system.app", "ohos.app", "system.router",
    "system.curves", "ohos.curves", "system.matrix4", "ohos.matrix4"};
static constexpr auto NATIVE_MODULE_PREFIX = "@native:";
static constexpr auto OHOS_MODULE_PREFIX = "@ohos:";
#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
struct JsHeapDumpWork {
    bool *isReady = nullptr;
    EcmaVM *vm;
    uv_work_t work;
    std::condition_variable *condition;
    size_t limitSize;
};
static constexpr uint32_t DEC_TO_INT = 100;
static size_t g_threshold = OHOS::system::GetUintParameter<size_t>("persist.ark.leak.threshold", 85);
static constexpr int TIME_OUT = 20;
static size_t g_checkInterval = OHOS::system::GetUintParameter<size_t>("persist.ark.leak.checkinterval", 60);
static constexpr int DEFAULT_SLEEP_TIME = 100000; //poll every 0.1s
static bool g_enableProperty = false;
static uint64_t g_lastHeapDumpTime = 0;
static bool g_debugLeak = OHOS::system::GetBoolParameter("debug.arkengine.tags.enableleak", false);
static constexpr uint64_t HEAP_DUMP_REPORT_INTERVAL = 24 * 3600 * 1000;
static constexpr uint64_t SEC_TO_MILSEC = 1000;
#endif
#ifdef ENABLE_HITRACE
constexpr auto NAPI_PROFILER_PARAM_SIZE = 10;
std::atomic<uint64_t> g_chainId = 0;
constexpr int NAPI_CALL_STACK = 2; // just for napi call stack
#endif

std::string ArkNativeEngine::tempModuleName_ {""};
bool ArkNativeEngine::napiProfilerEnabled {false};
bool ArkNativeEngine::napiProfilerParamReaded {false};
PermissionCheckCallback ArkNativeEngine::permissionCheckCallback_ {nullptr};

// This interface is using by ace_engine
napi_value LocalValueToLocalNapiValue(panda::Local<panda::JSValueRef> local)
{
    return JsValueFromLocalValue(local);
}

// This interface is using by ace_engine
panda::Local<panda::JSValueRef> NapiValueToLocalValue(napi_value v)
{
    return LocalValueFromJsValue(v);
}

#ifdef ENABLE_CONTAINER_SCOPE
void FunctionSetContainerId(const EcmaVM *vm, panda::Local<panda::JSValueRef> &value)
{
    if (!value->IsFunction()) {
        return;
    }
    panda::Local<panda::FunctionRef> funcValue(value);
    if (funcValue->IsNative(vm)) {
        return;
    }

    auto extraInfo = funcValue->GetData(vm);
    if (extraInfo != nullptr) {
        return;
    }

    NapiFunctionInfo *funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
        return;
    }
    funcInfo->scopeId = OHOS::Ace::ContainerScope::CurrentId();
    funcValue->SetData(vm, reinterpret_cast<void*>(funcInfo),
        [](void* env, void *externalPointer, void *data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
            if (info != nullptr) {
                delete info;
                info = nullptr;
            }
        }, true);
}
#endif

#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(IOS_PLATFORM)
bool IsFileNameFormat(char c)
{
    if (c >= '0' && c <= '9') {
        return false;
    }

    if (c >= 'a' && c <= 'z') {
        return false;
    }

    if (c >= 'A' && c <= 'Z') {
        return false;
    }

    if (c == '.' || c == '-' || c == '_') {
        return false;
    }

    return true;
}

std::string GetSelfProcName()
{
    constexpr uint16_t READ_SIZE = 128;
    std::ifstream fin;
    fin.open("/proc/self/comm", std::ifstream::in);
    if (!fin.is_open()) {
        HILOG_ERROR("fin.is_open() false");
        return "";
    }
    char readStr[READ_SIZE] = {'\0'};
    fin.getline(readStr, READ_SIZE - 1);
    fin.close();

    std::string ret = std::string(readStr);
    ret.erase(std::remove_if(ret.begin(), ret.end(), IsFileNameFormat), ret.end());
    return ret;
}

static bool IsInAppspawn()
{
    const char *env = getenv("LD_PRELOAD");
    if (env && strstr(env, "libclang_rt.asan.so") != nullptr) {
        return true;
    }
    if (GetSelfProcName().find("appspawn") != std::string::npos && getppid() == 1) {
        return true;
    }
    return false;
}

#endif

panda::Local<panda::JSValueRef> NapiDefineClass(napi_env env, const char* name, NapiNativeCallback callback,
    void* data, const NapiPropertyDescriptor* properties, size_t length)
{
    auto vm = const_cast<EcmaVM*>(reinterpret_cast<NativeEngine*>(env)->GetEcmaVm());
    std::string className(name);
    if (ArkNativeEngine::napiProfilerEnabled) {
        className = ArkNativeEngine::tempModuleName_ + "." + name;
    }

    NapiFunctionInfo* funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
        return panda::JSValueRef::Undefined(vm);
    }
    funcInfo->callback = callback;
    funcInfo->data = data;
#ifdef ENABLE_CONTAINER_SCOPE
    funcInfo->scopeId = OHOS::Ace::ContainerScope::CurrentId();
#endif

    Local<panda::FunctionRef> fn = panda::FunctionRef::NewClassFunction(vm, ArkNativeFunctionCallBack,
        [](void* env, void* externalPointer, void* data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
                if (info != nullptr) {
                    delete info;
                }
            },
        reinterpret_cast<void*>(funcInfo), true);
    Local<panda::ObjectRef> classPrototype = fn->GetFunctionPrototype(vm);
    Local<panda::StringRef> fnName = panda::StringRef::NewFromUtf8(vm, className.c_str());
    fn->SetName(vm, fnName);
    Local<panda::ObjectRef> fnObj = fn->ToObject(vm);
    for (size_t i = 0; i < length; i++) {
        if (properties[i].attributes & NATIVE_STATIC) {
            NapiDefineProperty(env, fnObj, properties[i]);
        } else {
            if (classPrototype->IsUndefined()) {
                HILOG_ERROR("ArkNativeEngineImpl::Class's prototype is null");
                continue;
            }
            reinterpret_cast<ArkNativeEngine*>(env)->SetModuleName(classPrototype, className);
            NapiDefineProperty(env, classPrototype, properties[i]);
        }
    }

    return fn;
}

Local<panda::JSValueRef> NapiNativeCreateSendableFunction(napi_env env,
                                                          const char* name,
                                                          NapiNativeCallback cb,
                                                          void* value)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    NapiFunctionInfo* funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
        return JSValueRef::Undefined(vm);
    }
    funcInfo->callback = cb;
    funcInfo->data = value;

    Local<panda::FunctionRef> fn = panda::FunctionRef::NewSendable(
        vm, ArkNativeFunctionCallBack,
        [](void* env, void* externalPointer, void* data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
            if (info != nullptr) {
                delete info;
            }
        },
        reinterpret_cast<void*>(funcInfo), true);
    return fn;
}

panda::Local<panda::JSValueRef> NapiDefineSendableClass(napi_env env,
                                                        const char* name,
                                                        NapiNativeCallback callback,
                                                        void* data,
                                                        const NapiPropertyDescriptor* properties,
                                                        size_t propertiesLength,
                                                        napi_value parent)
{
    NapiFunctionInfo* funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
    }
    funcInfo->callback = callback;
    funcInfo->data = data;

    std::string className(name);
    if (ArkNativeEngine::napiProfilerEnabled) {
        className = ArkNativeEngine::tempModuleName_ + "." + name;
    }

    const EcmaVM* vm = reinterpret_cast<NativeEngine*>(env)->GetEcmaVm();
    Local<panda::StringRef> fnName = panda::StringRef::NewFromUtf8(vm, className.c_str());
    Local<JSValueRef> localParent = JSValueRef::Null(vm);
    if (parent != nullptr) {
        localParent = LocalValueFromJsValue(parent);
    }

    auto infos = NativeSendable::CreateSendablePropertiesInfos(env, properties, propertiesLength);
    Local<panda::FunctionRef> fn = panda::FunctionRef::NewSendableClassFunction(
        vm, ArkNativeFunctionCallBack,
        [](void* env, void* externalPointer, void* data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
            if (info != nullptr) {
                delete info;
            }
        },
        reinterpret_cast<void*>(funcInfo), fnName, infos, localParent, true);

    return fn;
}

struct MoudleNameLocker {
    explicit MoudleNameLocker(std::string moduleName)
    {
        ArkNativeEngine::tempModuleName_ = moduleName;
    }
    ~MoudleNameLocker()
    {
        ArkNativeEngine::tempModuleName_ = "";
    }
};

void* ArkNativeEngine::GetNativePtrCallBack(void* data)
{
    if (data == nullptr) {
        HILOG_ERROR("data is nullptr");
        return nullptr;
    }
    auto info = reinterpret_cast<NapiFunctionInfo*>(data);
    auto cb = reinterpret_cast<void*>(info->callback);
    return cb;
}

bool ArkNativeEngine::CheckArkApiAllowList(
    NativeModule* module, panda::ecmascript::ApiCheckContext context, panda::Local<panda::ObjectRef>& exportCopy)
{
    std::unique_ptr<ApiAllowListChecker>& apiAllowListChecker = module->apiAllowListChecker;
    if (apiAllowListChecker != nullptr) {
        const std::string apiPath = context.moduleName->ToString();
        if ((*apiAllowListChecker)(apiPath)) {
            CopyPropertyApiFilter(apiAllowListChecker, context.ecmaVm, context.exportObj, exportCopy, apiPath);
        }
        return true;
    }
    return false;
}

void ArkNativeEngine::CopyPropertyApiFilter(const std::unique_ptr<ApiAllowListChecker>& apiAllowListChecker,
    const EcmaVM* ecmaVm, const panda::Local<panda::ObjectRef> exportObj, panda::Local<panda::ObjectRef>& exportCopy,
    const std::string& apiPath)
{
    panda::Local<panda::ArrayRef> namesArrayRef = exportObj->GetAllPropertyNames(ecmaVm, NATIVE_DEFAULT);
    for (uint32_t i = 0; i < namesArrayRef->Length(ecmaVm); ++i) {
        const panda::Local<panda::JSValueRef> nameValue = panda::ArrayRef::GetValueAt(ecmaVm, namesArrayRef, i);
        const panda::Local<panda::JSValueRef> value = exportObj->Get(ecmaVm, nameValue);
        const std::string curPath = apiPath + "." + nameValue->ToString(ecmaVm)->ToString();
        if ((*apiAllowListChecker)(curPath)) {
            const std::string valueType = value->Typeof(ecmaVm)->ToString();
            if (valueType == "object") {
                panda::Local<panda::ObjectRef> subObject = ObjectRef::New(ecmaVm);
                CopyPropertyApiFilter(apiAllowListChecker, ecmaVm, value, subObject, curPath);
                exportCopy->Set(ecmaVm, nameValue, subObject);
                HILOG_DEBUG("Set the package '%{public}s' to the allow list", curPath.c_str());
            } else if (valueType == "function") {
                exportCopy->Set(ecmaVm, nameValue, value);
                HILOG_DEBUG("Set the function '%{public}s' to the allow list", curPath.c_str());
            } else {
                exportCopy->Set(ecmaVm, nameValue, value);
                HILOG_DEBUG("Set the element type is '%{public}s::%{public}s' to the allow list", valueType.c_str(),
                    curPath.c_str());
            }
        }
    }
}


ArkNativeEngine::ArkNativeEngine(EcmaVM* vm, void* jsEngine, bool isLimitedWorker) : NativeEngine(jsEngine),
                                                                                     vm_(vm),
                                                                                     topScope_(vm),
                                                                                     isLimitedWorker_(isLimitedWorker)
{
    HILOG_DEBUG("ArkNativeEngine::ArkNativeEngine");
    panda::JSNApi::SetEnv(vm, this);
#ifdef ENABLE_HITRACE
    if (!ArkNativeEngine::napiProfilerParamReaded) {
        char napiProfilerParam[NAPI_PROFILER_PARAM_SIZE] = {0};
        int ret = GetParameter("persist.hiviewdfx.napiprofiler.enabled", "false",
            napiProfilerParam, sizeof(napiProfilerParam));
        if (ret > 0 && strcmp(napiProfilerParam, "true") == 0) {
            ArkNativeEngine::napiProfilerEnabled = true;
        }
        ArkNativeEngine::napiProfilerParamReaded = true;
    }
#endif
    LocalScope scope(vm_);
    Local<StringRef> requireInternalName = StringRef::NewFromUtf8(vm, "requireInternal");
    void* requireData = static_cast<void*>(this);

    Local<FunctionRef> requireNapi =
        FunctionRef::New(
            vm,
            [](JsiRuntimeCallInfo *info) -> Local<JSValueRef> {
                EcmaVM *ecmaVm = info->GetVM();
                panda::EscapeLocalScope scope(ecmaVm);
                NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
                ArkNativeEngine* arkNativeEngine = static_cast<ArkNativeEngine*>(info->GetData());
                Local<StringRef> moduleName(info->GetCallArgRef(0));
                NativeModule* module = nullptr;
                bool isAppModule = false;
                std::string errInfo = "";
#ifdef IOS_PLATFORM
                module = moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, false,
                                                         errInfo, false, "", arkNativeEngine->isLimitedWorker_);
#else
                const uint32_t lengthMax = 2;
                if (info->GetArgsNumber() >= lengthMax) {
                    Local<BooleanRef> ret(info->GetCallArgRef(1));
                    isAppModule = ret->Value();
                }
                arkNativeEngine->isAppModule_ = isAppModule;
                if (info->GetArgsNumber() == 3) { // 3:Determine if the number of parameters is equal to 3
                    Local<StringRef> path(info->GetCallArgRef(2)); // 2:Take the second parameter
                    module =
                        moduleManager->LoadNativeModule(moduleName->ToString().c_str(), path->ToString().c_str(),
                            isAppModule, errInfo, false, "", arkNativeEngine->isLimitedWorker_);
                } else if (info->GetArgsNumber() == 4) { // 4:Determine if the number of parameters is equal to 4
                    Local<StringRef> path(info->GetCallArgRef(2)); // 2:Take the second parameter
                    Local<StringRef> relativePath(info->GetCallArgRef(3)); // 3:Take the second parameter
                    module =
                        moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, isAppModule,
                            errInfo, false, relativePath->ToString().c_str(), arkNativeEngine->isLimitedWorker_);
                } else {
                    module =
                        moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, isAppModule,
                            errInfo, false, "", arkNativeEngine->isLimitedWorker_);
                }
#endif
                Local<JSValueRef> exports(JSValueRef::Undefined(ecmaVm));
                if (module != nullptr) {
                    auto it = arkNativeEngine->loadedModules_.find(module);
                    if (it != arkNativeEngine->loadedModules_.end()) {
                        return scope.Escape(it->second.ToLocal(ecmaVm));
                    }
                    std::string strModuleName = moduleName->ToString();
                    moduleManager->SetNativeEngine(strModuleName, arkNativeEngine);
                    MoudleNameLocker nameLocker(strModuleName);

                    if (module->jsCode == nullptr && module->getABCCode != nullptr) {
                        module->getABCCode(&module->jsCode, &module->jsCodeLen);
                    }
                    if (module->jsABCCode != nullptr || module->jsCode != nullptr) {
                        char fileName[NAPI_PATH_MAX] = { 0 };
                        const char* name = module->name;
                        if (sprintf_s(fileName, sizeof(fileName), "lib%s.z.so/%s.js", name, name) == -1) {
                            HILOG_ERROR("sprintf_s file name failed");
                            return scope.Escape(exports);
                        }
                        HILOG_DEBUG("load js code from %{public}s", fileName);
                        const void *buffer = nullptr;
                        if (module->jsABCCode) {
                            buffer = static_cast<const void *>(module->jsABCCode);
                        } else {
                            buffer = static_cast<const void *>(module->jsCode);
                        }
                        auto exportObject = arkNativeEngine->LoadArkModule(buffer, module->jsCodeLen, fileName);
                        if (exportObject->IsUndefined()) {
                            HILOG_ERROR("load module failed");
                            return scope.Escape(exports);
                        } else {
                            exports = exportObject;
                            arkNativeEngine->loadedModules_[module] = Global<JSValueRef>(ecmaVm, exports);
                        }
                    } else if (module->registerCallback != nullptr) {
                        Local<ObjectRef> exportObj = ObjectRef::New(ecmaVm);
#ifdef ENABLE_HITRACE
                        StartTrace(HITRACE_TAG_ACE, "NAPI module init, name = " + std::string(module->name));
#endif
                        arkNativeEngine->SetModuleName(exportObj, module->name);
                        module->registerCallback(reinterpret_cast<napi_env>(arkNativeEngine),
                                                 JsValueFromLocalValue(exportObj));
#ifdef ENABLE_HITRACE
                        FinishTrace(HITRACE_TAG_ACE);
#endif
                        panda::Local<panda::ObjectRef> exportCopy = panda::ObjectRef::New(ecmaVm);
                        panda::ecmascript::ApiCheckContext context{moduleManager, ecmaVm, moduleName, exportObj, scope};
                        if (CheckArkApiAllowList(module, context, exportCopy)) {
                            return scope.Escape(exportCopy);
                        }
                        exports = exportObj;
                        arkNativeEngine->loadedModules_[module] = Global<JSValueRef>(ecmaVm, exports);
                    } else {
                        HILOG_ERROR("init module failed");
                        return scope.Escape(exports);
                    }
                }
                return scope.Escape(exports);
            },
            nullptr,
            requireData);

    Local<FunctionRef> requireInternal =
        FunctionRef::New(
            vm,
            [](JsiRuntimeCallInfo *info) -> Local<JSValueRef> {
                EcmaVM *ecmaVm = info->GetVM();
                panda::EscapeLocalScope scope(ecmaVm);
                NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
                ArkNativeEngine* arkNativeEngine = static_cast<ArkNativeEngine*>(info->GetData());
                Local<StringRef> moduleName(info->GetCallArgRef(0));
                std::string errInfo = "";
                NativeModule* module = moduleManager->LoadNativeModule(moduleName->ToString().c_str(),
                    nullptr, false, errInfo, false, "", arkNativeEngine->isLimitedWorker_);
                Local<JSValueRef> exports(JSValueRef::Undefined(ecmaVm));
                MoudleNameLocker nameLocker(moduleName->ToString());
                if (module != nullptr && arkNativeEngine) {
                    auto it = arkNativeEngine->loadedModules_.find(module);
                    if (it != arkNativeEngine->loadedModules_.end()) {
                        return scope.Escape(it->second.ToLocal(ecmaVm));
                    }
                    std::string strModuleName = moduleName->ToString();
                    moduleManager->SetNativeEngine(strModuleName, arkNativeEngine);
                    Local<ObjectRef> exportObj = ObjectRef::New(ecmaVm);
                    if (exportObj->IsObject()) {
                        arkNativeEngine->SetModuleName(exportObj, module->name);
                        module->registerCallback(reinterpret_cast<napi_env>(arkNativeEngine),
                                                 JsValueFromLocalValue(exportObj));
                        exports = exportObj;
                        arkNativeEngine->loadedModules_[module] = Global<JSValueRef>(ecmaVm, exports);
                    } else {
                        HILOG_ERROR("exportObject is nullptr");
                        return scope.Escape(exports);
                    }
                }
                return scope.Escape(exports);
            },
            nullptr,
            requireData);

    Local<ObjectRef> global = panda::JSNApi::GetGlobalObject(vm);
#if !defined(PREVIEW)
    Local<StringRef> requireName = StringRef::NewFromUtf8(vm, "requireNapi");
    global->Set(vm, requireName, requireNapi);
#else
    Local<StringRef> requireNapiPreview = StringRef::NewFromUtf8(vm, "requireNapiPreview");
    global->Set(vm, requireNapiPreview, requireNapi);
#endif
    global->Set(vm, requireInternalName, requireInternal);
    JSNApi::SetNativePtrGetter(vm, reinterpret_cast<void*>(ArkNativeEngine::GetNativePtrCallBack));
    // need to call init of base class.
    NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
    JSNApi::SetUnloadNativeModuleCallback(vm, std::bind(&NativeModuleManager::UnloadNativeModule,
        moduleManager, std::placeholders::_1));
    Init();
    panda::JSNApi::SetLoop(vm, loop_);
    panda::JSNApi::SetWeakFinalizeTaskCallback(vm, [this] () -> void {
        this->PostFinalizeTasks();
    });
}

ArkNativeEngine::~ArkNativeEngine()
{
    HILOG_DEBUG("ArkNativeEngine::~ArkNativeEngine");
    Deinit();
    // Free cached objects
    for (auto&& [module, exportObj] : loadedModules_) {
        exportObj.FreeGlobalHandleAddr();
    }
    // Free callbackRef
    if (promiseRejectCallbackRef_ != nullptr) {
        delete promiseRejectCallbackRef_;
    }
    if (checkCallbackRef_ != nullptr) {
        delete checkCallbackRef_;
    }
    StopMonitorJSHeapUsage();
}

#ifdef ENABLE_HITRACE
static inline bool CheckHookConfig(const std::string &nameRef)
{
    if (g_hookJsConfig == nullptr || g_hookJsConfig->jsStackReport <= 0 ||
        g_hookJsConfig->maxJsStackDepth == 0 || !g_hookJsConfig->jsFpUnwind) {
        return false;
    } else if (g_hookJsConfig->filterNapiName[0] != '\0' &&
        nameRef.find(g_hookJsConfig->filterNapiName) != std::string::npos) {
        return false;
    }
    return true;
}
#endif

static inline uint64_t StartNapiProfilerTrace(panda::JsiRuntimeCallInfo* runtimeInfo, void* cb)
{
#ifdef ENABLE_HITRACE
    if (ArkNativeEngine::napiProfilerEnabled) {
        EcmaVM *vm = runtimeInfo->GetVM();
        LocalScope scope(vm);
        Local<panda::FunctionRef> fn = runtimeInfo->GetFunctionRef();
        Local<panda::StringRef> nameRef = fn->GetName(vm);
        char threadName[BUF_SIZE];
        prctl(PR_GET_NAME, threadName);
        StartTraceArgs(HITRACE_TAG_ACE, "Napi called:%s, tname:%s", nameRef->ToString().c_str(), threadName);
    }
    bool hookFlag = __get_hook_flag() && __get_global_hook_flag();
    if (!hookFlag) {
        return 0;
    }
    EcmaVM* vm = runtimeInfo->GetVM();
    LocalScope scope(vm);
    Local<panda::FunctionRef> fn = runtimeInfo->GetFunctionRef();
    Local<panda::StringRef> nameRef = fn->GetName(vm);
    if (g_hookJsConfig == nullptr) {
        std::call_once(g_hookOnceFlag, []() { g_hookJsConfig = (HookJsConfig*)__get_hook_config(); });
    }
    // add memtrace function
    if (g_hookJsConfig->jsStackReport == NAPI_CALL_STACK && !g_hookJsConfig->jsFpUnwind) {
        OHOS::HiviewDFX::HiTraceChain::ClearId();
        std::unique_ptr<OHOS::HiviewDFX::HiTraceId> arkCallBackTraceId = std::make_unique<OHOS::HiviewDFX::HiTraceId>(
            OHOS::HiviewDFX::HiTraceChain::Begin("New ArkCallBackTrace", 0));
        char buffer[256] = {0}; // 256 : buffer size of tag name
        (void)sprintf_s(buffer, sizeof(buffer), "napi:0x%x:%s", arkCallBackTraceId->GetChainId(),
                        nameRef->ToString().c_str());
        uint64_t addr = reinterpret_cast<uint64_t>(cb);
        ++g_chainId;
        (void)memtrace(reinterpret_cast<void*>(addr + g_chainId), 8, buffer, true); // 8: the size of addr
        return 0;
    }
    if (!CheckHookConfig(nameRef->ToString())) {
        return 0;
    }
    BlockHookScope blockHook; // block hook
    std::string rawStack;
    std::vector<JsFrameInfo> jsFrames;
    uint64_t nestChainId = 0;
    jsFrames.reserve(g_hookJsConfig->maxJsStackDepth);
    auto env = reinterpret_cast<napi_env>(JSNApi::GetEnv(vm));
    auto engine = reinterpret_cast<NativeEngine*>(env);
    engine->BuildJsStackInfoListWithCustomDepth(jsFrames, g_hookJsConfig->maxJsStackDepth);
    std::stringstream ssRawStack;
    for (size_t i = 0; i < jsFrames.size(); i++) {
        ssRawStack << jsFrames[i].functionName << JS_SYMBOL_FILEPATH_SEP << jsFrames[i].fileName << ":" <<
            jsFrames[i].pos;
        if (i < jsFrames.size() - 1) {
            ssRawStack << JS_CALL_STACK_DEPTH_SEP;
        }
    }
    rawStack = ssRawStack.str();
    OHOS::HiviewDFX::HiTraceChain::Begin("ArkNativeFunctionCallBack", 0);
    OHOS::HiviewDFX::HiTraceId hitraceId = OHOS::HiviewDFX::HiTraceChain::GetId();
    // resolve nested calls to napi and ts
    if (hitraceId.IsValid()) {
        nestChainId = hitraceId.GetChainId();
    }
    uint64_t chainId = ++g_chainId;
    hitraceId.SetChainId(chainId);
    OHOS::HiviewDFX::HiTraceChain::SetId(hitraceId);
    __send_hook_misc_data(chainId, rawStack.c_str(), rawStack.size() + 1, 1);
    return nestChainId;
        
#endif
    return 0;
}

static inline void FinishNapiProfilerTrace(uint64_t value)
{
#ifdef ENABLE_HITRACE
    if (ArkNativeEngine::napiProfilerEnabled) {
        FinishTrace(HITRACE_TAG_ACE);
    }
    bool hookFlag = __get_hook_flag() && __get_global_hook_flag();
    if (!hookFlag) {
        return;
    }
    OHOS::HiviewDFX::HiTraceId hitraceId = OHOS::HiviewDFX::HiTraceChain::GetId();
    if (hitraceId.IsValid()) {
        OHOS::HiviewDFX::HiTraceChain::End(hitraceId);
        OHOS::HiviewDFX::HiTraceChain::ClearId();
    }
    // resolve nested calls to napi and ts
    if (value) {
        hitraceId.SetChainId(value);
        OHOS::HiviewDFX::HiTraceChain::SetId(hitraceId);
    }
    
#endif
}

template <bool changeState>
panda::JSValueRef ArkNativeFunctionCallBack(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();
    panda::LocalScope scope(vm);
    bool getStackBeforeCallNapiSuccess = false;
    JSNApi::GetStackBeforeCallNapiSuccess(vm, getStackBeforeCallNapiSuccess);
    auto info = reinterpret_cast<NapiFunctionInfo*>(runtimeInfo->GetData());
    auto env = reinterpret_cast<napi_env>(JSNApi::GetEnv(vm));
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto cb = info->callback;
    if (engine == nullptr) {
        HILOG_ERROR("native engine is null");
        return **JSValueRef::Undefined(vm);
    }

    uint64_t nestChainId = StartNapiProfilerTrace(runtimeInfo, reinterpret_cast<void *>(cb));

    if (JSNApi::IsMixedDebugEnabled(vm)) {
        JSNApi::NotifyNativeCalling(vm, reinterpret_cast<void *>(cb));
    }

    napi_value result = nullptr;
    if (cb != nullptr) {
        if constexpr (changeState) {
            panda::JsiNativeScope nativeScope(vm);
            result = cb(env, runtimeInfo);
        } else {
            result = cb(env, runtimeInfo);
        }
    }

    if (JSNApi::IsMixedDebugEnabled(vm)) {
        JSNApi::NotifyNativeReturn(vm, reinterpret_cast<void *>(cb));
    }

    Local<panda::JSValueRef> localRet = panda::JSValueRef::Undefined(vm);
    if (result != nullptr) {
        localRet = LocalValueFromJsValue(result);
    }

    FinishNapiProfilerTrace(nestChainId);
    // Fixme: Rethrow error to engine while clear lastException_
    if (!engine->lastException_.IsEmpty()) {
        engine->lastException_.Empty();
    }

    if (localRet.IsEmpty()) {
        return **JSValueRef::Undefined(vm);
    }
    if (getStackBeforeCallNapiSuccess) {
        JSNApi::GetStackAfterCallNapi(vm);
    }
    return **localRet;
}

static Local<panda::JSValueRef> NapiNativeCreateFunction(napi_env env, const char* name,
                                                         NapiNativeCallback cb, void* value)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    NapiFunctionInfo* funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
        return JSValueRef::Undefined(vm);
    }
    funcInfo->callback = cb;
    funcInfo->data = value;
#ifdef ENABLE_CONTAINER_SCOPE
    funcInfo->scopeId = OHOS::Ace::ContainerScope::CurrentId();
#endif

    Local<panda::FunctionRef> fn = panda::FunctionRef::NewConcurrent(
        vm, ArkNativeFunctionCallBack,
        [](void* env, void* externalPointer, void* data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
            if (info != nullptr) {
                delete info;
            }
        },
        reinterpret_cast<void*>(funcInfo), true
    );
    Local<panda::StringRef> fnName = panda::StringRef::NewFromUtf8(vm, name);
    fn->SetName(vm, fnName);
    return fn;
}

Local<JSValueRef> GetProperty(EcmaVM* vm, Local<panda::ObjectRef> &obj, const char* name)
{
    Local<StringRef> key = StringRef::NewFromUtf8(vm, name);
    Local<JSValueRef> val = obj->Get(vm, key);
    return val;
}

void GetCString(EcmaVM* vm, Local<StringRef> str, char* buffer, size_t size, size_t* length)
{
    if (length == nullptr) {
        return;
    }
    if (buffer == nullptr) {
        *length = str->Utf8Length(vm) - 1;
    } else if (size != 0) {
        int copied = str->WriteUtf8(buffer, size - 1, true) - 1;
        buffer[copied] = '\0';
        *length = copied;
    } else {
        *length = 0;
    }
}

std::string NapiGetModuleName(napi_env env, Local<panda::ObjectRef> &obj)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    std::string moduleName("");
    auto nativeModuleName = GetProperty(vm, obj, PANDA_MODULE_NAME);
    if (nativeModuleName->IsString()) {
        char arrayName[PANDA_MODULE_NAME_LEN] = {0};
        size_t len = 0;
        GetCString(vm, nativeModuleName, arrayName, PANDA_MODULE_NAME_LEN, &len);
        moduleName += arrayName;
        moduleName += ".";
    }
    return moduleName;
}

void NapiDefinePropertyInner(napi_env env,
                             Local<panda::ObjectRef> &obj,
                             NapiPropertyDescriptor &propertyDescriptor,
                             Local<panda::JSValueRef> &propertyName,
                             bool &result)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = engine->GetEcmaVm();
    bool writable = (propertyDescriptor.attributes & NATIVE_WRITABLE) != 0;
    bool enumable = (propertyDescriptor.attributes & NATIVE_ENUMERABLE) != 0;
    bool configable = (propertyDescriptor.attributes & NATIVE_CONFIGURABLE) != 0;
    std::string fullName("");
    if (propertyDescriptor.getter != nullptr || propertyDescriptor.setter != nullptr) {
#ifdef ENABLE_HITRACE
        fullName += NapiGetModuleName(env, obj);
#endif
        Local<panda::JSValueRef> localGetter = panda::JSValueRef::Undefined(vm);
        Local<panda::JSValueRef> localSetter = panda::JSValueRef::Undefined(vm);

        if (propertyDescriptor.getter != nullptr) {
            fullName += "getter";
            localGetter = NapiNativeCreateFunction(env, fullName.c_str(),
                                                   propertyDescriptor.getter, propertyDescriptor.data);
        }
        if (propertyDescriptor.setter != nullptr) {
            fullName += "setter";
            localSetter = NapiNativeCreateFunction(env, fullName.c_str(),
                                                   propertyDescriptor.setter, propertyDescriptor.data);
        }

        PropertyAttribute attr(panda::JSValueRef::Undefined(vm), false, enumable, configable);
        result = obj->SetAccessorProperty(vm, propertyName, localGetter, localSetter, attr);
    } else if (propertyDescriptor.method != nullptr) {
#ifdef ENABLE_HITRACE
        fullName += NapiGetModuleName(env, obj);
#endif
        if (propertyDescriptor.utf8name != nullptr) {
            fullName += propertyDescriptor.utf8name;
        } else {
            fullName += propertyName->IsString()
                        ? Local<panda::StringRef>(propertyName)->ToString()
                        : Local<panda::SymbolRef>(propertyName)->GetDescription(vm)->ToString();
        }
        Local<panda::JSValueRef> cbObj = NapiNativeCreateFunction(env, fullName.c_str(),
                                                                  propertyDescriptor.method, propertyDescriptor.data);
        PropertyAttribute attr(cbObj, writable, enumable, configable);
        result = obj->DefineProperty(vm, propertyName, attr);
    } else {
        Local<panda::JSValueRef> val = LocalValueFromJsValue(propertyDescriptor.value);

        PropertyAttribute attr(val, writable, enumable, configable);
        result = obj->DefineProperty(vm, propertyName, attr);
    }
}

bool NapiDefineProperty(napi_env env, Local<panda::ObjectRef> &obj, NapiPropertyDescriptor propertyDescriptor)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = engine->GetEcmaVm();
    bool result = false;
    Local<panda::JSValueRef> propertyName;
    if (propertyDescriptor.utf8name != nullptr) {
        propertyName = panda::StringRef::NewFromUtf8(vm, propertyDescriptor.utf8name);
    } else {
        propertyName = LocalValueFromJsValue(propertyDescriptor.name);
    }
    if (obj->IsJSShared()) {
        NativeSendable::NapiDefineSendabledProperty(env, obj, propertyDescriptor, propertyName, result);
    } else {
        NapiDefinePropertyInner(env, obj, propertyDescriptor, propertyName, result);
    }
    Local<panda::ObjectRef> excep = panda::JSNApi::GetUncaughtException(vm);
    if (!excep.IsNull()) {
        HILOG_DEBUG("ArkNativeObject::DefineProperty occur Exception");
        panda::JSNApi::GetAndClearUncaughtException(vm);
    }
    return result;
}

panda::Local<panda::ObjectRef> NapiCreateObjectWithProperties(napi_env env, size_t propertyCount,
                                                              const napi_property_descriptor *properties,
                                                              Local<panda::JSValueRef> *keys,
                                                              PropertyAttribute *attrs)
{
    auto vm = reinterpret_cast<NativeEngine*>(env)->GetEcmaVm();
    panda::EscapeLocalScope scope(vm);
    for (size_t i = 0; i < propertyCount; ++i) {
        const napi_property_descriptor &property = properties[i];

        const char* utf8name = property.utf8name;
        uint32_t attributes = property.attributes;
        bool writable = (attributes & NATIVE_WRITABLE) != 0;
        bool enumable = (attributes & NATIVE_ENUMERABLE) != 0;
        bool configable = (attributes & NATIVE_CONFIGURABLE) != 0;
        NapiNativeCallback method = reinterpret_cast<NapiNativeCallback>(property.method);
        NapiNativeCallback getter = reinterpret_cast<NapiNativeCallback>(property.getter);
        NapiNativeCallback setter = reinterpret_cast<NapiNativeCallback>(property.setter);
        napi_value value = property.value;
        void *data = property.data;

        Local<panda::JSValueRef> val = panda::JSValueRef::Undefined(vm);

        std::string fullName("");
        if (getter != nullptr || setter != nullptr) {
            Local<panda::JSValueRef> localGetter = panda::JSValueRef::Undefined(vm);
            Local<panda::JSValueRef> localSetter = panda::JSValueRef::Undefined(vm);

            if (getter != nullptr) {
                fullName += "getter";
                localGetter = NapiNativeCreateFunction(env, fullName.c_str(), getter, data);
            }
            if (setter != nullptr) {
                fullName += "setter";
                localSetter = NapiNativeCreateFunction(env, fullName.c_str(), setter, data);
            }

            val = panda::ObjectRef::CreateAccessorData(vm, localGetter, localSetter);
            writable = false;
        } else if (method != nullptr) {
            fullName += utf8name;
            val = NapiNativeCreateFunction(env, fullName.c_str(), method, data);
        } else {
            val = LocalValueFromJsValue(value);
        }
        new (reinterpret_cast<void *>(&attrs[i])) PropertyAttribute(val, writable, enumable, configable);
        keys[i] = panda::StringRef::NewFromUtf8(vm, utf8name);
    }
    Local<panda::ObjectRef> object = panda::ObjectRef::NewWithProperties(vm, propertyCount, keys, attrs);
    return scope.Escape(object);
}

panda::Local<panda::ObjectRef> ArkNativeEngine::GetModuleFromName(
    const std::string& moduleName, bool isAppModule, const std::string& id, const std::string& param,
    const std::string& instanceName, void** instance)
{
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> exports(JSValueRef::Undefined(vm_));
    NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
    std::string errInfo = "";
    NativeModule* module = moduleManager->LoadNativeModule(moduleName.c_str(), nullptr, isAppModule, errInfo);
    if (module != nullptr) {
        Local<StringRef> idStr = StringRef::NewFromUtf8(vm_, id.c_str(), id.size());
        napi_value idValue = JsValueFromLocalValue(idStr);
        Local<StringRef> paramStr = StringRef::NewFromUtf8(vm_, param.c_str(), param.size());
        napi_value paramValue = JsValueFromLocalValue(paramStr);
        Local<ObjectRef> exportObj = ObjectRef::New(vm_);
        NapiPropertyDescriptor idProperty, paramProperty;
        idProperty.utf8name = "id";
        idProperty.value = idValue;
        paramProperty.utf8name = "param";
        paramProperty.value = paramValue;
        SetModuleName(exportObj, module->name);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, idProperty);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, paramProperty);
        MoudleNameLocker nameLocker(module->name);
        module->registerCallback(reinterpret_cast<napi_env>(this), JsValueFromLocalValue(exportObj));
        napi_value nExport = JsValueFromLocalValue(exportObj);
        napi_value exportInstance = nullptr;
        napi_status status = napi_get_named_property(
            reinterpret_cast<napi_env>(this), nExport, instanceName.c_str(), &exportInstance);
        if (status != napi_ok) {
            HILOG_ERROR("GetModuleFromName napi_get_named_property status != napi_ok");
        }

        status = napi_unwrap(reinterpret_cast<napi_env>(this), exportInstance, reinterpret_cast<void**>(instance));
        if (status != napi_ok) {
            HILOG_ERROR("GetModuleFromName napi_unwrap status != napi_ok");
        }
        exports = exportObj;
    }
    return scope.Escape(exports);
}

napi_value ArkNativeEngine::CreatePromise(NativeDeferred** deferred)
{
    panda::EscapeLocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    *deferred = new ArkNativeDeferred(this, capability);
    Local<JSValueRef> promiseValue = capability->GetPromise(vm_);
    return JsValueFromLocalValue(scope.Escape(promiseValue));
}

panda::Local<panda::ObjectRef> ArkNativeEngine::LoadModuleByName(const std::string& moduleName, bool isAppModule,
    const std::string& param, const std::string& instanceName, void* instance, const std::string& path)
{
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> exports(JSValueRef::Undefined(vm_));
    NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
    std::string errInfo = "";
    NativeModule* module = moduleManager->LoadNativeModule(moduleName.c_str(),
        path.empty() ? nullptr : path.c_str(), isAppModule, errInfo);
    if (module != nullptr) {
        Local<ObjectRef> exportObj = ObjectRef::New(vm_);
        NapiPropertyDescriptor paramProperty, instanceProperty;
        Local<StringRef> paramStr = StringRef::NewFromUtf8(vm_, param.c_str(), param.size());
        napi_value paramValue = JsValueFromLocalValue(paramStr);
        paramProperty.utf8name = "param";
        paramProperty.value = paramValue;
        Local<ObjectRef> instanceValue = ObjectRef::New(vm_);
        Local<StringRef> key = StringRef::GetNapiWrapperString(vm_);
        if (instance == nullptr && instanceValue->Has(vm_, key)) {
            Local<ObjectRef> wrapper = instanceValue->Get(vm_, key);
            auto ref = reinterpret_cast<NativeReference*>(wrapper->GetNativePointerField(0));
            wrapper->SetNativePointerField(vm_, 0, nullptr, nullptr, nullptr, 0);
            instanceValue->Delete(vm_, key);
            delete ref;
        } else {
            Local<ObjectRef> object = ObjectRef::New(vm_);
            NativeReference* ref = nullptr;
            Local<JSValueRef> value(instanceValue);
            ref = new ArkNativeReference(this, vm_, value, 0, true, nullptr, instance, nullptr);

            object->SetNativePointerFieldCount(vm_, 1);
            object->SetNativePointerField(vm_, 0, ref, nullptr, nullptr, 0);
            PropertyAttribute attr(object, true, false, true);
            instanceValue->DefineProperty(vm_, key, attr);
        }
        instanceProperty.utf8name = instanceName.c_str();
        instanceProperty.value = JsValueFromLocalValue(instanceValue);
        SetModuleName(exportObj, module->name);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, paramProperty);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, instanceProperty);

        MoudleNameLocker nameLocker(module->name);
        module->registerCallback(reinterpret_cast<napi_env>(this), JsValueFromLocalValue(exportObj));
        exports = exportObj;
    }
    return scope.Escape(exports);
}

void ArkNativeEngine::Loop(LoopMode mode, bool needSync)
{
    LocalScope scope(vm_);
    NativeEngine::Loop(mode, needSync);
    panda::JSNApi::ExecutePendingJob(vm_);
}

void ArkNativeEngine::SetModuleName(Local<ObjectRef> &nativeObj, std::string moduleName)
{
#ifdef ENABLE_HITRACE
    if (ArkNativeEngine::napiProfilerEnabled) {
        Local<StringRef> moduleNameStr = StringRef::NewFromUtf8(vm_, moduleName.c_str(), moduleName.size());
        Local<StringRef> key = StringRef::NewFromUtf8(vm_, PANDA_MODULE_NAME);
        nativeObj->Set(vm_, key, moduleNameStr);
    }
#endif
}

static void ConcurrentCallbackFunc(Local<JSValueRef> result, bool success, void *taskInfo, void *data)
{
    if (data == nullptr) {
        return;
    }
    auto engine = static_cast<ArkNativeEngine *>(data);
    auto concurrentCallbackFunc = engine->GetConcurrentCallbackFunc();
    if (concurrentCallbackFunc == nullptr) {
        return;
    }
    napi_env env = reinterpret_cast<napi_env>(engine);
    concurrentCallbackFunc(env, ArkNativeEngine::ArkValueToNapiValue(env, result), success, taskInfo);
}

bool ArkNativeEngine::InitTaskPoolThread(NativeEngine* engine, NapiConcurrentCallback callback)
{
    concurrentCallbackFunc_ = callback;
    return JSNApi::InitForConcurrentThread(vm_, ConcurrentCallbackFunc, static_cast<void *>(this));
}

bool ArkNativeEngine::InitTaskPoolThread(napi_env env, NapiConcurrentCallback callback)
{
    concurrentCallbackFunc_ = callback;
    return JSNApi::InitForConcurrentThread(vm_, ConcurrentCallbackFunc, static_cast<void *>(this));
}

bool ArkNativeEngine::InitTaskPoolFunc(napi_env env, napi_value func, void* taskInfo)
{
    LocalScope scope(vm_);
    Local<JSValueRef> function = LocalValueFromJsValue(func);
    return JSNApi::InitForConcurrentFunction(vm_, function, taskInfo);
}

bool ArkNativeEngine::HasPendingJob() const
{
    return JSNApi::HasPendingJob(vm_);
}

bool ArkNativeEngine::IsProfiling() const
{
    return JSNApi::IsProfiling(vm_);
}

bool ArkNativeEngine::IsExecutingPendingJob() const
{
    return panda::JSNApi::IsExecutingPendingJob(vm_);
}

void* ArkNativeEngine::GetCurrentTaskInfo() const
{
    return JSNApi::GetCurrentTaskInfo(vm_);
}

void ArkNativeEngine::ClearCurrentTaskInfo()
{
    JSNApi::ClearCurrentTaskInfo(vm_);
}

void ArkNativeEngine::TerminateExecution() const
{
    DFXJSNApi::TerminateExecution(vm_);
}

napi_value ArkNativeEngine::CallFunction(
    napi_value thisVar, napi_value function, napi_value const* argv, size_t argc)
{
    if (function == nullptr) {
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<JSValueRef> thisObj = JSValueRef::Undefined(vm_);
    if (thisVar != nullptr) {
        thisObj = LocalValueFromJsValue(thisVar);
    }
    Local<FunctionRef> funcObj = LocalValueFromJsValue(function);
    std::vector<Local<JSValueRef>> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++) {
        if (argv[i] != nullptr) {
            args.emplace_back(LocalValueFromJsValue(argv[i]));
        } else {
            args.emplace_back(JSValueRef::Undefined(vm_));
        }
    }

    Local<JSValueRef> value = funcObj->Call(vm_, thisObj, args.data(), argc);
    if (panda::JSNApi::HasPendingException(vm_)) {
        HILOG_ERROR("pending exception when js function called");
        HILOG_ERROR("print exception info: ");
        panda::JSNApi::PrintExceptionInfo(vm_);
        return nullptr;
    }

    return JsValueFromLocalValue(scope.Escape(value));
}

bool ArkNativeEngine::Napi_Judge_TypedArray(NativeTypedArrayType typedArrayType, Local<panda::TypedArrayRef> typedArray,
                                            const EcmaVM* vm, Local<panda::ArrayBufferRef> arrayBuf,
                                            size_t byte_offset, size_t length, napi_value* result)
{
    switch (typedArrayType) {
        case NATIVE_INT8_ARRAY:
            typedArray = panda::Int8ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_UINT8_ARRAY:
            typedArray = panda::Uint8ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_UINT8_CLAMPED_ARRAY:
            typedArray = panda::Uint8ClampedArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_INT16_ARRAY:
            typedArray = panda::Int16ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_UINT16_ARRAY:
            typedArray = panda::Uint16ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_INT32_ARRAY:
            typedArray = panda::Int32ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_UINT32_ARRAY:
            typedArray = panda::Uint32ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_FLOAT32_ARRAY:
            typedArray = panda::Float32ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_FLOAT64_ARRAY:
            typedArray = panda::Float64ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_BIGINT64_ARRAY:
            typedArray = panda::BigInt64ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        case NATIVE_BIGUINT64_ARRAY:
            typedArray = panda::BigUint64ArrayRef::New(vm, arrayBuf, byte_offset, length);
            break;
        default:
            *result = nullptr;
            return false;
    }
    *result = JsValueFromLocalValue(typedArray);
    return true;
}

bool ArkNativeEngine::RunScriptPath(const char* path)
{
    panda::JSExecutionScope executionScope(vm_);
    LocalScope scope(vm_);
    [[maybe_unused]] bool ret = panda::JSNApi::Execute(vm_, path, PANDA_MAIN_FUNCTION);
    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return false;
    }
    return true;
}

/*
 * Before: input: 1. @ohos.hilog
                  2. @system.app (NATIVE_MODULE contains this name)
 * After: return: 1.@ohos:hilog
 *                2.@native:system.app
 */
std::string ArkNativeEngine::GetOhmurl(std::string path)
{
    const std::regex reg("@(ohos|system)\\.(\\S+)");
    path.erase(0, path.find_first_not_of(" "));
    path.erase(path.find_last_not_of(" ") + 1);
    bool ret = std::regex_match(path, reg);
    if (!ret) {
        HILOG_ERROR("ArkNativeEngine:The module name doesn't comply with the naming rules");
        return "";
    }
    std::string systemModule = path.substr(1);
    if (NATIVE_MODULE.count(systemModule)) {
        return NATIVE_MODULE_PREFIX + systemModule;
    } else {
        int pos = static_cast<int>(path.find('.'));
        std::string systemKey = path.substr(pos + 1, systemModule.size());
        return OHOS_MODULE_PREFIX + systemKey;
    }
}

Local<JSValueRef> ArkNativeEngine::NapiLoadNativeModule(std::string path)
{
    std::string key = GetOhmurl(path);
    if (key.size() == 0) {
        return JSValueRef::Undefined(vm_);
    }
    return panda::JSNApi::ExecuteNativeModule(vm_, key);
}

ModuleTypes ArkNativeEngine::CheckLoadType(const std::string &path)
{
    if (path[0] == '@') {
        return ModuleTypes::NATIVE_MODULE;
    } else if (path.find("ets/") == 0) { // ets/xxx/xxx
        return ModuleTypes::MODULE_INNER_FILE;
    }
    return ModuleTypes::UNKNOWN;
}

napi_value ArkNativeEngine::NapiLoadModule(const char* path, const char* module_info)
{
    if (path == nullptr) {
        HILOG_ERROR("ArkNativeEngine:The module name is empty");
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    Local<ObjectRef> exportObj(undefObj);
    std::string inputPath(path);
    std::string modulePath;
    if (module_info != nullptr) {
        modulePath = module_info;
    }
    switch (CheckLoadType(inputPath)) {
        case ModuleTypes::NATIVE_MODULE: {
            exportObj = NapiLoadNativeModule(inputPath);
            break;
        }
        case ModuleTypes::MODULE_INNER_FILE: {
            exportObj = panda::JSNApi::GetModuleNameSpaceFromFile(vm_, inputPath, modulePath);
            break;
        }
        default: {
            std::string msg = "ArkNativeEngine:NapiLoadModule input path:" + inputPath + " is invalid.";
            ThrowException(msg.c_str());
        }
    }
    if (!exportObj->IsObject()) {
        ThrowException("ArkNativeEngine:NapiLoadModule failed.");
        return JsValueFromLocalValue(scope.Escape(undefObj));
    }
    return JsValueFromLocalValue(scope.Escape(exportObj));
}

napi_value ArkNativeEngine::NapiLoadModuleWithInfo(const char* path, const char* module_info)
{
    if (path == nullptr) {
        HILOG_ERROR("ArkNativeEngine:The module name is empty");
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    Local<ObjectRef> exportObj(undefObj);
    std::string inputPath(path);
    std::string modulePath;
    if (module_info != nullptr) {
        modulePath = module_info;
        exportObj = panda::JSNApi::GetModuleNameSpaceWithModuleInfo(vm_, inputPath, modulePath);
    } else {
        exportObj = NapiLoadNativeModule(inputPath);
    }

    if (!exportObj->IsObject()) {
        ThrowException("ArkNativeEngine:NapiLoadModuleWithInfo failed.");
        return JsValueFromLocalValue(scope.Escape(undefObj));
    }
    return JsValueFromLocalValue(scope.Escape(exportObj));
}

bool ArkNativeEngine::SuspendVMById(uint32_t tid)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    return DFXJSNApi::SuspendVMById(vm_, tid);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

void ArkNativeEngine::ResumeVMById(uint32_t tid)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    DFXJSNApi::ResumeVMById(vm_, tid);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return;
#endif
}

// The security interface needs to be modified accordingly.
napi_value ArkNativeEngine::RunScriptBuffer(const char* path, std::vector<uint8_t>& buffer, bool isBundle)
{
    panda::EscapeLocalScope scope(vm_);
    [[maybe_unused]] bool ret = false;
    if (isBundle) {
        ret = panda::JSNApi::Execute(vm_, buffer.data(), buffer.size(), PANDA_MAIN_FUNCTION, path);
    } else {
        ret = panda::JSNApi::ExecuteModuleBuffer(vm_, buffer.data(), buffer.size(), path);
    }

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return nullptr;
    }
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    return JsValueFromLocalValue(scope.Escape(undefObj));
}

bool ArkNativeEngine::RunScriptBuffer(const std::string& path, uint8_t* buffer, size_t size, bool isBundle)
{
    panda::JSExecutionScope executionScope(vm_);
    LocalScope scope(vm_);
    bool ret = false;
    if (isBundle) {
        ret = panda::JSNApi::ExecuteSecure(vm_, buffer, size, PANDA_MAIN_FUNCTION, path);
    } else {
        ret = panda::JSNApi::ExecuteModuleBufferSecure(vm_, buffer, size, path);
    }

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return false;
    }
    return ret;
}

void ArkNativeEngine::SetPackagePath(const std::string appLibPathKey, const std::vector<std::string>& packagePath)
{
    auto moduleManager = NativeModuleManager::GetInstance();
    if (moduleManager && !packagePath.empty()) {
        moduleManager->SetAppLibPath(appLibPathKey, packagePath);
    }
}

napi_value ArkNativeEngine::CreateInstance(napi_value constructor, napi_value const *argv, size_t argc)
{
    if (constructor == nullptr) {
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<FunctionRef> value = LocalValueFromJsValue(constructor);
    std::vector<Local<JSValueRef>> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++) {
        if (argv[i] != nullptr) {
            args.emplace_back(LocalValueFromJsValue(argv[i]));
        } else {
            args.emplace_back(JSValueRef::Undefined(vm_));
        }
    }
    Local<JSValueRef> instance = value->Constructor(vm_, args.data(), argc);
    Local<ObjectRef> excep = panda::JSNApi::GetUncaughtException(vm_);
    if (!excep.IsNull()) {
        HILOG_ERROR("ArkNativeEngineImpl::CreateInstance occur Exception");
        return nullptr;
    }
    return JsValueFromLocalValue(scope.Escape(instance));
}

NativeReference* ArkNativeEngine::CreateReference(napi_value value, uint32_t initialRefcount,
    bool flag, NapiNativeFinalize callback, void* data, void* hint)
{
    return new ArkNativeReference(this, this->GetEcmaVm(), value, initialRefcount, flag, callback, data, hint);
}

NativeReference* ArkNativeEngine::CreateAsyncReference(napi_value value, uint32_t initialRefcount,
    bool flag, NapiNativeFinalize callback, void* data, void* hint)
{
    return new ArkNativeReference(this, this->GetEcmaVm(), value, initialRefcount, flag, callback, data, hint, true);
}

void ArkNativeEngine::PostFinalizeTasks()
{
    if (!pendingAsyncFinalizers_.empty()) {
        uv_work_t *asynWork = new uv_work_t;
        std::vector<RefFinalizer> *asyncFinalizers = new std::vector<RefFinalizer>();
        asyncFinalizers->swap(pendingAsyncFinalizers_);
        asynWork->data = reinterpret_cast<void *>(asyncFinalizers);

        int ret = uv_queue_work(GetUVLoop(), asynWork, [](uv_work_t *asynWork) {
            std::vector<RefFinalizer> *finalizers = reinterpret_cast<std::vector<RefFinalizer> *>(asynWork->data);
            for (auto iter : (*finalizers)) {
                std::tuple<NativeEngine*, void*, void*> &param = iter.second;
                (iter.first)(reinterpret_cast<napi_env>(std::get<0>(param)),
                            std::get<1>(param), std::get<2>(param)); // 2 is the param.
            }
            HILOG_DEBUG("uv_queue_work async running ");
            delete finalizers;
        }, [](uv_work_t *asynWork, int32_t) {
            delete asynWork;
        });
        if (ret != 0) {
            HILOG_ERROR("uv_queue_work fail ret '%{public}d'", ret);
            for (auto iter : (*asyncFinalizers)) {
                std::tuple<NativeEngine*, void*, void*> &param = iter.second;
                (iter.first)(reinterpret_cast<napi_env>(std::get<0>(param)),
                            std::get<1>(param), std::get<2>(param)); // 2 is the param.
            }
            delete asynWork;
            delete asyncFinalizers;
        }
    }
    if (pendingFinalizers_.empty()) {
        return;
    }
    uv_work_t *syncWork = new uv_work_t;
    std::vector<RefFinalizer> *syncFinalizers = new std::vector<RefFinalizer>();
    syncFinalizers->swap(pendingFinalizers_);
    syncWork->data = reinterpret_cast<void *>(syncFinalizers);

    int ret = uv_queue_work(GetUVLoop(), syncWork, [](uv_work_t *) {}, [](uv_work_t *syncWork, int32_t) {
        std::vector<RefFinalizer> *finalizers = reinterpret_cast<std::vector<RefFinalizer> *>(syncWork->data);
        for (auto iter : (*finalizers)) {
            std::tuple<NativeEngine*, void*, void*> &param = iter.second;
            (iter.first)(reinterpret_cast<napi_env>(std::get<0>(param)),
                         std::get<1>(param), std::get<2>(param)); // 2 is the param.
        }
        HILOG_DEBUG("uv_queue_work running");
        delete syncWork;
        delete finalizers;
    });
    if (ret != 0) {
        HILOG_ERROR("uv_queue_work fail ret '%{public}d'", ret);
        for (auto iter : (*syncFinalizers)) {
            std::tuple<NativeEngine*, void*, void*> &param = iter.second;
            (iter.first)(reinterpret_cast<napi_env>(std::get<0>(param)),
                         std::get<1>(param), std::get<2>(param)); // 2 is the param.
        }
        delete syncWork;
        delete syncFinalizers;
    }
}

NativeEngine* ArkNativeEngine::CreateRuntimeFunc(NativeEngine* engine, void* jsEngine, bool isLimitedWorker)
{
    panda::RuntimeOption option;
#if defined(OHOS_PLATFORM) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
    int arkProperties = OHOS::system::GetIntParameter<int>("persist.ark.properties", -1);
    std::string bundleName = OHOS::system::GetParameter("persist.ark.arkbundlename", "");
    std::string memConfigProperty = OHOS::system::GetParameter("persist.ark.mem_config_property", "");
    size_t gcThreadNum = OHOS::system::GetUintParameter<size_t>("persist.ark.gcthreads", 7);
    size_t longPauseTime = OHOS::system::GetUintParameter<size_t>("persist.ark.longpausetime", 40);
    bool asmInterpreterEnabled = OHOS::system::GetBoolParameter("persist.ark.asminterpreter", true);
    std::string asmOpcodeDisableRange = OHOS::system::GetParameter("persist.ark.asmopcodedisablerange", "");
    bool builtinsLazyEnabled = OHOS::system::GetBoolParameter("persist.ark.enablebuiltinslazy", true);
    option.SetEnableBuiltinsLazy(builtinsLazyEnabled);
    option.SetArkProperties(arkProperties);
    option.SetArkBundleName(bundleName);
    option.SetMemConfigProperty(memConfigProperty);
    option.SetGcThreadNum(gcThreadNum);
    option.SetLongPauseTime(longPauseTime);
    option.SetEnableAsmInterpreter(asmInterpreterEnabled);
    option.SetAsmOpcodeDisableRange(asmOpcodeDisableRange);
    option.SetIsWorker();
    option.SetIsRestrictedWorker(isLimitedWorker);
    HILOG_DEBUG("ArkNativeEngineImpl::CreateRuntimeFunc ark properties = %{public}d, bundlename = %{public}s",
        arkProperties, bundleName.c_str());
#endif
    option.SetGcType(panda::RuntimeOption::GC_TYPE::GEN_GC);
    const int64_t poolSize = 0x1000000;
    option.SetGcPoolSize(poolSize);
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    option.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::INFO);
#endif
    option.SetDebuggerLibraryPath("");
    EcmaVM* vm = panda::JSNApi::CreateJSVM(option);
    if (vm == nullptr) {
        return nullptr;
    }
    // worker adaptation mergeabc
    const panda::ecmascript::EcmaVM* hostVM = reinterpret_cast<ArkNativeEngine*>(engine)->GetEcmaVm();
    panda::JSNApi::SynchronizVMInfo(vm, hostVM);
    ArkNativeEngine* arkEngine = new ArkNativeEngine(vm, jsEngine, isLimitedWorker);
    // init callback
    arkEngine->RegisterWorkerFunction(engine);
    arkEngine->SetHostEngine(engine);
    // sync apiVersion
    arkEngine->SetApiVersion(engine->GetApiVersion());

    auto cleanEnv = [vm]() {
        if (vm != nullptr) {
            HILOG_DEBUG("cleanEnv is called");
            panda::JSNApi::DestroyJSVM(vm);
        }
    };
    arkEngine->SetCleanEnv(cleanEnv);
    if (hostVM != nullptr) {
        panda::JSNApi::AddWorker(const_cast<EcmaVM*>(hostVM), vm);
    }
    return arkEngine;
}

void* ArkNativeEngine::CreateRuntime(bool isLimitedWorker)
{
    return ArkNativeEngine::CreateRuntimeFunc(this, jsEngine_, isLimitedWorker);
}

void ArkNativeEngine::StartCpuProfiler(const std::string& fileName)
{
    JSNApi::SetNativePtrGetter(vm_, reinterpret_cast<void*>(ArkNativeEngine::GetNativePtrCallBack));
    DFXJSNApi::StartCpuProfilerForFile(vm_, fileName);
}

void ArkNativeEngine::StopCpuProfiler()
{
    DFXJSNApi::StopCpuProfilerForFile(vm_);
    JSNApi::SetNativePtrGetter(vm_, nullptr);
}

void ArkNativeEngine::ResumeVM()
{
    DFXJSNApi::ResumeVM(vm_);
}

bool ArkNativeEngine::SuspendVM()
{
    return DFXJSNApi::SuspendVM(vm_);
}

bool ArkNativeEngine::IsSuspended()
{
    return DFXJSNApi::IsSuspended(vm_);
}

bool ArkNativeEngine::CheckSafepoint()
{
    return DFXJSNApi::CheckSafepoint(vm_);
}

napi_value ArkNativeEngine::RunBufferScript(std::vector<uint8_t>& buffer)
{
    panda::EscapeLocalScope scope(vm_);
    [[maybe_unused]] bool ret = panda::JSNApi::Execute(vm_, buffer.data(), buffer.size(), PANDA_MAIN_FUNCTION);

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return nullptr;
    }
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    return JsValueFromLocalValue(scope.Escape(undefObj));
}

#define EXECUTE_BUFFER(functionName)                                                                 \
    if (panda::JSNApi::IsBundle(vm_)) {                                                              \
        /* FA doesn't enable securemem */                                                            \
        ret = panda::JSNApi::Execute(vm_, buffer, bufferSize, PANDA_MAIN_FUNCTION, desc);            \
    } else if (bufferSize != 0) {                                                                    \
        if (entryPoint == nullptr) {                                                                 \
            HILOG_DEBUG("Input entryPoint is nullptr, please input entryPoint for merged ESModule"); \
            /* this path for bundle and abc compiled by single module js */                          \
            ret = panda::JSNApi::functionName(vm_, buffer, bufferSize, PANDA_MAIN_FUNCTION, desc);   \
        } else {                                                                                     \
            /* this path for mergeabc with specific entryPoint */                                    \
            /* entryPoint: bundleName/moduleName/xxx/xxx */                                          \
            panda::JSNApi::SetModuleInfo(vm_, desc, std::string(entryPoint));                        \
            if (panda::JSNApi::HasPendingException(vm_)) {                                           \
                HandleUncaughtException();                                                           \
                return nullptr;                                                                      \
            }                                                                                        \
            ret = panda::JSNApi::functionName(vm_, buffer, bufferSize, entryPoint, desc);            \
        }                                                                                            \
    } else {                                                                                         \
        /* this path for worker */                                                                   \
        ret = panda::JSNApi::Execute(vm_, desc, PANDA_MAIN_FUNCTION);                                \
    }

napi_value ArkNativeEngine::RunActor(uint8_t* buffer, size_t bufferSize, const char* descriptor, char* entryPoint)
{
    panda::EscapeLocalScope scope(vm_);
    std::string desc(descriptor);
    [[maybe_unused]] bool ret = false;
    // if apiVersion > API11, use secure path.
    if (IsApplicationApiVersionAPI11Plus()) {
        EXECUTE_BUFFER(ExecuteSecure);
    } else {
        EXECUTE_BUFFER(Execute);
    }
    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return nullptr;
    }
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    return JsValueFromLocalValue(scope.Escape(undefObj));
}

void ArkNativeEngine::GetCurrentModuleInfo(std::string& moduleName, std::string& fileName, bool needRecordName)
{
    LocalScope scope(vm_);
    std::pair<std::string, std::string> moduleInfo = panda::JSNApi::GetCurrentModuleInfo(vm_, needRecordName);
    moduleName = moduleInfo.first; // if needRecordName is true, then moduleName is recordName.
    fileName = moduleInfo.second;
}

bool ArkNativeEngine::GetIsBundle()
{
    LocalScope scope(vm_);
    return panda::JSNApi::IsBundle(vm_);
}

std::string ArkNativeEngine::GetBundleName()
{
    LocalScope scope(vm_);
    return panda::JSNApi::GetBundleName(vm_);
}

bool ArkNativeEngine::IsExecuteModuleInAbcFile(std::string bundleName, std::string moduleName, std::string ohmurl)
{
    LocalScope scope(vm_);
    return panda::JSNApi::IsExecuteModuleInAbcFile(vm_, bundleName, moduleName, ohmurl);
}

panda::Local<panda::ObjectRef> ArkNativeEngine::LoadArkModule(const void* buffer,
    int32_t len, const std::string& fileName)
{
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> undefObj(JSValueRef::Undefined(vm_));
    if (buffer == nullptr || len <= 0 || fileName.empty()) {
        HILOG_ERROR("fileName is nullptr or source code is nullptr");
        return scope.Escape(undefObj);
    }

    bool res = JSNApi::ExecuteModuleFromBuffer(vm_, buffer, len, fileName);
    if (!res) {
        HILOG_ERROR("Execute module failed");
        return scope.Escape(undefObj);
    }

    Local<ObjectRef> exportObj = JSNApi::GetExportObjectFromBuffer(vm_, fileName, "default");
    if (exportObj->IsNull()) {
        HILOG_ERROR("Get export object failed");
        return scope.Escape(undefObj);
    }

    HILOG_DEBUG("ArkNativeEngineImpl::LoadModule end");
    return scope.Escape(exportObj);
}

napi_value ArkNativeEngine::ValueToNapiValue(JSValueWrapper& value)
{
    Global<JSValueRef> arkValue = value;
    return JsValueFromLocalValue(arkValue.ToLocal(vm_));
}

napi_value ArkNativeEngine::ArkValueToNapiValue(napi_env env, Local<JSValueRef> value)
{
    return JsValueFromLocalValue(value);
}

std::string ArkNativeEngine::GetSourceCodeInfo(napi_value value, ErrorPos pos)
{
    if (value == nullptr || pos.first == 0) {
        return "";
    }

    LocalScope scope(vm_);
    Local<panda::FunctionRef> func = LocalValueFromJsValue(value);
    uint32_t line = pos.first;
    uint32_t column = pos.second;
    Local<panda::StringRef> sourceCode = func->GetSourceCode(vm_, line);
    std::string sourceCodeStr = sourceCode->ToString();
    if (sourceCodeStr.empty()) {
        return "";
    }
    std::string sourceCodeInfo = "SourceCode:\n";
    sourceCodeInfo.append(sourceCodeStr).append("\n");
    for (uint32_t k = 1; k < column; k++) {
        sourceCodeInfo.push_back(' ');
    }
    sourceCodeInfo.append("^\n");
    return sourceCodeInfo;
}

bool ArkNativeEngine::ExecuteJsBin(const std::string& fileName)
{
    panda::JSExecutionScope executionScope(vm_);
    LocalScope scope(vm_);
    bool ret = JSNApi::Execute(vm_, fileName, PANDA_MAIN_FUNCTION);
    return ret;
}

bool ArkNativeEngine::TriggerFatalException(napi_value error)
{
    return true;
}

bool ArkNativeEngine::AdjustExternalMemory(int64_t ChangeInBytes, int64_t* AdjustedValue)
{
    return true;
}

void ArkNativeEngine::SetPromiseRejectCallback(NativeReference* rejectCallbackRef, NativeReference* checkCallbackRef)
{
    if (rejectCallbackRef == nullptr || checkCallbackRef == nullptr) {
        HILOG_ERROR("rejectCallbackRef or checkCallbackRef is nullptr");
        return;
    }
    promiseRejectCallbackRef_ = rejectCallbackRef;
    checkCallbackRef_ = checkCallbackRef;
    JSNApi::SetHostPromiseRejectionTracker(vm_, reinterpret_cast<void*>(PromiseRejectCallback),
                                           reinterpret_cast<void*>(this));
}

void ArkNativeEngine::PromiseRejectCallback(void* info)
{
    panda::PromiseRejectInfo* promiseRejectInfo = reinterpret_cast<panda::PromiseRejectInfo*>(info);
    ArkNativeEngine* env = reinterpret_cast<ArkNativeEngine*>(promiseRejectInfo->GetData());

    if (env == nullptr) {
        HILOG_ERROR("engine is nullptr");
        return;
    }

    if (env->promiseRejectCallbackRef_ == nullptr || env->checkCallbackRef_ == nullptr) {
        HILOG_ERROR("promiseRejectCallbackRef or checkCallbackRef is nullptr");
        return;
    }
    panda::ecmascript::EcmaVM* vm = const_cast<EcmaVM*>(env->GetEcmaVm());
    LocalScope scope(vm);
    Local<JSValueRef> promise = promiseRejectInfo->GetPromise();
    Local<JSValueRef> reason = promiseRejectInfo->GetReason();
    panda::PromiseRejectInfo::PROMISE_REJECTION_EVENT operation = promiseRejectInfo->GetOperation();

    Local<JSValueRef> type(IntegerRef::New(vm, static_cast<int32_t>(operation)));

    Local<JSValueRef> args[] = {type, promise, reason};

    napi_value promiseNapiRejectCallback = env->promiseRejectCallbackRef_->Get();
    Local<FunctionRef> promiseRejectCallback = LocalValueFromJsValue(promiseNapiRejectCallback);
    if (!promiseRejectCallback.IsEmpty()) {
        promiseRejectCallback->Call(vm, JSValueRef::Undefined(vm), args, 3); // 3 args size
    }

    if (operation == panda::PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT) {
        Local<JSValueRef> checkCallback = LocalValueFromJsValue(env->checkCallbackRef_->Get());
        if (!checkCallback.IsEmpty()) {
            JSNApi::SetHostEnqueueJob(vm, checkCallback);
        }
    }
}

void ArkNativeEngine::DumpHeapSnapshot(const std::string& path, bool isVmMode, DumpFormat dumpFormat)
{
    if (dumpFormat == DumpFormat::JSON) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 0, path, isVmMode);
    }
    if (dumpFormat == DumpFormat::BINARY) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 1, path, isVmMode);
    }
    if (dumpFormat == DumpFormat::OTHER) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 2, path, isVmMode); // 2:enum is 2
    }
}

void ArkNativeEngine::DumpCpuProfile(bool isVmMode, DumpFormat dumpFormat, bool isPrivate, bool isFullGC)
{
    if (dumpFormat == DumpFormat::JSON) {
        DFXJSNApi::DumpCpuProfile(vm_, 0, isVmMode, isPrivate, false, isFullGC);
    }
    if (dumpFormat == DumpFormat::BINARY) {
        DFXJSNApi::DumpCpuProfile(vm_, 1, isVmMode, isPrivate);
    }
    if (dumpFormat == DumpFormat::OTHER) {
        DFXJSNApi::DumpCpuProfile(vm_, 2, isVmMode, isPrivate); // 2:enum is 2
    }
}

void ArkNativeEngine::DumpHeapSnapshot(bool isVmMode, DumpFormat dumpFormat, bool isPrivate, bool isFullGC)
{
    if (dumpFormat == DumpFormat::JSON) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 0, isVmMode, isPrivate, false, isFullGC);
    }
    if (dumpFormat == DumpFormat::BINARY) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 1, isVmMode, isPrivate);
    }
    if (dumpFormat == DumpFormat::OTHER) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 2, isVmMode, isPrivate); // 2:enum is 2
    }
}

bool ArkNativeEngine::BuildNativeAndJsStackTrace(std::string& stackTraceStr)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    return DFXJSNApi::BuildNativeAndJsStackTrace(vm_, stackTraceStr);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool ArkNativeEngine::BuildJsStackTrace(std::string& stackTraceStr)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    return DFXJSNApi::BuildJsStackTrace(vm_, stackTraceStr);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool ArkNativeEngine::BuildJsStackInfoList(uint32_t tid, std::vector<JsFrameInfo>& jsFrames)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    std::vector<JsFrameInfo> arkJsFrames;
    bool sign = DFXJSNApi::BuildJsStackInfoList(vm_, tid, arkJsFrames);
    return sign;
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool ArkNativeEngine::BuildJsStackInfoListWithCustomDepth(std::vector<JsFrameInfo>& jsFrames, uint8_t maxJsStackDepth)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    std::vector<JsFrameInfo> arkJsFrames;
    bool sign = DFXJSNApi::BuildJsStackInfoList(vm_, gettid(), arkJsFrames);
    for (size_t i = 0; i < arkJsFrames.size() && i < maxJsStackDepth; i++) {
        JsFrameInfo jsframe;
        jsframe.fileName = arkJsFrames[i].fileName;
        jsframe.functionName = arkJsFrames[i].functionName;
        jsframe.pos = arkJsFrames[i].pos;
        jsframe.nativePointer = arkJsFrames[i].nativePointer;
        jsFrames.emplace_back(jsframe);
    }
    return sign;
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool ArkNativeEngine::DeleteWorker(NativeEngine* workerEngine)
{
    if (workerEngine != nullptr) {
#if !defined(PREVIEW)
        const panda::ecmascript::EcmaVM* workerVM = reinterpret_cast<ArkNativeEngine*>(workerEngine)->GetEcmaVm();
        if (workerVM != nullptr) {
            return panda::JSNApi::DeleteWorker(vm_, const_cast<EcmaVM*>(workerVM));
        }
#else
        HILOG_WARN("ARK does not support dfx on windows");
#endif
        return false;
        }
    return false;
}

bool ArkNativeEngine::StartHeapTracking(double timeInterval, bool isVmMode)
{
    return DFXJSNApi::StartHeapTracking(vm_, timeInterval, isVmMode);
}

bool ArkNativeEngine::StopHeapTracking(const std::string &filePath)
{
    return DFXJSNApi::StopHeapTracking(vm_, filePath);
}

#if !defined(PREVIEW)
void ArkNativeEngine::PrintStatisticResult()
{
    DFXJSNApi::PrintStatisticResult(vm_);
}

void ArkNativeEngine::StartRuntimeStat()
{
    DFXJSNApi::StartRuntimeStat(vm_);
}

void ArkNativeEngine::StopRuntimeStat()
{
    DFXJSNApi::StopRuntimeStat(vm_);
}

size_t ArkNativeEngine::GetArrayBufferSize()
{
    return DFXJSNApi::GetArrayBufferSize(vm_);
}

size_t ArkNativeEngine::GetHeapTotalSize()
{
    return DFXJSNApi::GetHeapTotalSize(vm_);
}

size_t ArkNativeEngine::GetHeapUsedSize()
{
    return DFXJSNApi::GetHeapUsedSize(vm_);
}

size_t ArkNativeEngine::GetHeapObjectSize()
{
    return DFXJSNApi::GetHeapObjectSize(vm_);
}

size_t ArkNativeEngine::GetHeapLimitSize()
{
    return DFXJSNApi::GetHeapLimitSize(vm_);
}

void ArkNativeEngine::NotifyApplicationState(bool inBackground)
{
    DFXJSNApi::NotifyApplicationState(vm_, inBackground);
}

void ArkNativeEngine::NotifyIdleStatusControl(std::function<void(bool)> callback)
{
    DFXJSNApi::NotifyIdleStatusControl(vm_, callback);
}

void ArkNativeEngine::NotifyIdleTime(int idleMicroSec)
{
    DFXJSNApi::NotifyIdleTime(vm_, idleMicroSec);
}

void ArkNativeEngine::NotifyMemoryPressure(bool inHighMemoryPressure)
{
    DFXJSNApi::NotifyMemoryPressure(vm_, inHighMemoryPressure);
}

void ArkNativeEngine::NotifyForceExpandState(int32_t value)
{
    switch (ForceExpandState(value)) {
        case ForceExpandState::FINISH_COLD_START:
            DFXJSNApi::NotifyFinishColdStart(vm_, true);
            break;
        case ForceExpandState::START_HIGH_SENSITIVE:
            DFXJSNApi::NotifyHighSensitive(vm_, true);
            break;
        case ForceExpandState::FINISH_HIGH_SENSITIVE:
            DFXJSNApi::NotifyHighSensitive(vm_, false);
            break;
        default:
            HILOG_ERROR("Invalid Force Expand State: %{public}d.", value);
            break;
    }
}
#else
void ArkNativeEngine::PrintStatisticResult()
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void ArkNativeEngine::StartRuntimeStat()
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void ArkNativeEngine::StopRuntimeStat()
{
    HILOG_WARN("ARK does not support dfx on windows");
}

size_t ArkNativeEngine::GetArrayBufferSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t ArkNativeEngine::GetHeapTotalSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t ArkNativeEngine::GetHeapUsedSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t ArkNativeEngine::GetHeapObjectSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t ArkNativeEngine::GetHeapLimitSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

void ArkNativeEngine::NotifyApplicationState([[maybe_unused]] bool inBackground)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void ArkNativeEngine::NotifyIdleStatusControl([[maybe_unused]] std::function<void(bool)> callback)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void ArkNativeEngine::NotifyIdleTime([[maybe_unused]] int idleMicroSec)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void ArkNativeEngine::NotifyMemoryPressure([[maybe_unused]] bool inHighMemoryPressure)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void ArkNativeEngine::NotifyForceExpandState([[maybe_unused]] int32_t value)
{
    HILOG_WARN("ARK does not support dfx on windows");
}
#endif

void ArkNativeEngine::SetMockModuleList(const std::map<std::string, std::string> &list)
{
    JSNApi::SetMockModuleList(vm_, list);
}

void ArkNativeEngine::RegisterNapiUncaughtExceptionHandler(NapiUncaughtExceptionCallback callback)
{
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    napiUncaughtExceptionCallback_ = callback;
}

void ArkNativeEngine::HandleUncaughtException()
{
    if (napiUncaughtExceptionCallback_ == nullptr) {
        return;
    }
    LocalScope scope(vm_);
    lastException_.Empty();
    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(vm_);
    if (!exception.IsEmpty() && !exception->IsHole()) {
        if (napiUncaughtExceptionCallback_ != nullptr) {
            napiUncaughtExceptionCallback_(ArkValueToNapiValue(reinterpret_cast<napi_env>(this), exception));
        }
    }
}

bool ArkNativeEngine::HasPendingException()
{
    return panda::JSNApi::HasPendingException(vm_);
}

void ArkNativeEngine::RegisterPermissionCheck(PermissionCheckCallback callback)
{
    if (permissionCheckCallback_ == nullptr) {
        permissionCheckCallback_ = callback;
    }
}

bool ArkNativeEngine::ExecutePermissionCheck()
{
    if (permissionCheckCallback_ != nullptr) {
        return permissionCheckCallback_();
    } else {
        HILOG_INFO("permissionCheckCallback_ is still nullptr when executing permission check!");
        return true;
    }
}

void ArkNativeEngine::RegisterTranslateBySourceMap(SourceMapCallback callback)
{
    if (SourceMapCallback_ == nullptr) {
        SourceMapCallback_ = callback;
    }
    // regedit SourceMapCallback to ark_js_runtime
    panda::JSNApi::SetSourceMapCallback(vm_, callback);
}

void ArkNativeEngine::RegisterSourceMapTranslateCallback(SourceMapTranslateCallback callback)
{
    panda::JSNApi::SetSourceMapTranslateCallback(vm_, callback);
}

std::string ArkNativeEngine::ExecuteTranslateBySourceMap(const std::string& rawStack)
{
    if (SourceMapCallback_ != nullptr) {
        return SourceMapCallback_(rawStack);
    } else {
        HILOG_WARN("SourceMapCallback_ is nullptr.");
        return "";
    }
}

bool ArkNativeEngine::IsMixedDebugEnabled()
{
    return JSNApi::IsMixedDebugEnabled(vm_);
}

void ArkNativeEngine::NotifyNativeCalling(const void *nativeAddress)
{
    JSNApi::NotifyNativeCalling(vm_, nativeAddress);
}

void ArkNativeEngine::AllowCrossThreadExecution() const
{
    JSNApi::AllowCrossThreadExecution(vm_);
}

#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
uint64_t ArkNativeEngine::GetCurrentTickMillseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void AsyncAfterWorkCallback(uv_work_t* req, int status)
{
    if (req == nullptr) {
        HILOG_ERROR("dumpheapSnapshot AsyncAfterWorkCallback req is nullptr");
        return;
    }
    if (status) {
        HILOG_ERROR("dumpheapSnapshot AsyncAfterWorkCallback failed");
    }
    auto work = reinterpret_cast<JsHeapDumpWork*>(req->data);
    if (work == nullptr) {
        HILOG_ERROR("dumpheapSnapshot JsHeapDumpWork is nullptr");
        return;
    }
    size_t heapUsedSize = DFXJSNApi::GetHeapUsedSize(work->vm);
    size_t nowPrecent = heapUsedSize * DEC_TO_INT / (work->limitSize);
    HILOG_INFO("dumpheapSnapshot nowPrecent is %{public}zu", nowPrecent);
    if (nowPrecent >= g_threshold) {
        auto vm = work->vm;
        DFXJSNApi::GetHeapPrepare(vm);
        DFXJSNApi::SetOverLimit(vm, true);
        HILOG_INFO("dumpheapSnapshot GetHeapPrepare done");
    }
    *(work->isReady) = true;
    work->condition->notify_all();
    delete work;
}

bool ArkNativeEngine::JudgmentDumpExecuteTask(int pid)
{
    // VM destructed but syncTask is executed.
    if (needStop_ || !DFXJSNApi::isOverLimit(vm_)) {
        return false;
    }
    DFXJSNApi::SetOverLimit(vm_, false);
    HILOG_INFO("dumpheapSnapshot ready");
    time_t startTime = time(nullptr);
    g_lastHeapDumpTime = GetCurrentTickMillseconds();
    if ((pid = fork()) < 0) {
        HILOG_ERROR("ready dumpheapSnapshot Fork error, err:%{public}d", errno);
        sleep(g_checkInterval);
        return false;
    }
    if (pid == 0) {
        AllowCrossThreadExecution();
        DumpHeapSnapshot(true, DumpFormat::JSON, false, false);
        HILOG_INFO("dumpheapSnapshot successful, now you can check some file");
        _exit(0);
    }
    while (true) {
        int status = 0;
        pid_t p = waitpid(pid, &status, 0);
        if (p < 0) {
            HILOG_ERROR("dumpheapSnapshot Waitpid return p=%{public}d, err:%{public}d", p, errno);
            break;
        }
        if (p == pid) {
            HILOG_ERROR("dumpheapSnapshot dump process exited status is %{public}d", status);
            break;
        }
        if (time(nullptr) > startTime + TIME_OUT) {
            HILOG_ERROR("time out to wait child process, killing forkpid %{public}d", pid);
            kill(pid, SIGKILL);
            break;
        }
        usleep(DEFAULT_SLEEP_TIME);
    }
    return true;
}

void ArkNativeEngine::JudgmentDump(size_t limitSize)
{
    if (!limitSize) {
        return;
    }
    size_t nowPrecent = GetHeapObjectSize() * DEC_TO_INT / limitSize;
    if (g_debugLeak || (nowPrecent >= g_threshold && (g_lastHeapDumpTime == 0 ||
        GetCurrentTickMillseconds() - g_lastHeapDumpTime > HEAP_DUMP_REPORT_INTERVAL))) {
        int pid = -1;
        std::unique_lock<std::mutex> lock(lock_);
        dumpWork_ = new(std::nothrow) JsHeapDumpWork;
        dumpWork_->vm = vm_;
        dumpWork_->work.data = dumpWork_;
        dumpWork_->condition = &condition_;
        dumpWork_->limitSize = limitSize;
        dumpWork_->isReady = &isReady_;
        uv_loop_t *loop = GetUVLoop();
        int status = uv_queue_work(loop, &(dumpWork_->work), [](uv_work_t *) {}, AsyncAfterWorkCallback);
        if (status != 0) {
            HILOG_ERROR("In JudgmentDump, uv_queue_work failed");
            delete dumpWork_;
            return;
        }
        while (!isReady_) {
            condition_.wait(lock);
        }
        isReady_ = false;
        if (JudgmentDumpExecuteTask(pid) == false) {
            return;
        }
    }
}

void ArkNativeEngine::JsHeapStart()
{
    if (pthread_setname_np(pthread_self(), "OS_JsHeapThread") != 0) {
        HILOG_ERROR("Failed to set threadName for JsHeap, errno:%d", errno);
    }
    while (!needStop_) {
        {
            std::unique_lock<std::mutex> lock(lock_);
            condition_.wait_for(lock, std::chrono::milliseconds(g_checkInterval * SEC_TO_MILSEC));
            if (needStop_) {
                return;
            }
        }
        size_t limitSize = GetHeapLimitSize();
        JudgmentDump(limitSize);
    }
}
#endif

void ArkNativeEngine::StartMonitorJSHeapUsage()
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
    if (!g_enableProperty) {
        return;
    }
    if (threadJsHeap_ == nullptr && !IsInAppspawn()) {
        threadJsHeap_ = std::make_unique<std::thread>(&ArkNativeEngine::JsHeapStart, this);
        HILOG_DEBUG("JsHeapStart is OK");
    }
#else
    HILOG_ERROR("StartMonitorJSHeapUsage does not support dfx");
#endif
}

void ArkNativeEngine::StopMonitorJSHeapUsage()
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
    if (threadJsHeap_ != nullptr) {
        // Free threadJsHeap_
        needStop_ = true;
        condition_.notify_all();
        if (threadJsHeap_->joinable()) {
            threadJsHeap_->join();
            threadJsHeap_ = nullptr;
        }
    }
#else
    HILOG_ERROR("StopMonitorJSHeapUsage does not support dfx");
#endif
}

#if !defined(is_arkui_x) && defined(OHOS_PLATFORM)
std::string HybridStackDumper::PrintJsFrame(const JsFrameInfo& jsFrame)
{
    return "    at " + jsFrame.functionName + " (" + jsFrame.fileName + ":" + jsFrame.pos + ")\n";
}

std::string HybridStackDumper::MatchJsFrameByFlag(const std::vector<JsFrameInfo>& jsFrames, uint32_t& jsIndex)
{
    std::string jsStack = "";
    if (jsIndex == jsFrames.size()) {
        return jsStack;
    }

    do {
        jsStack += PrintJsFrame(jsFrames[jsIndex]);
        jsIndex++;
    } while (jsIndex < jsFrames.size() && jsFrames[jsIndex].nativePointer == nullptr);

    return jsStack;
}

void HybridStackDumper::BuildJsNativeMixStack(std::vector<JsFrameInfo>& jsFrames,
    std::vector<DfxFrame>& nativeFrames)
{
    uint32_t jsIndex = 0;
    std::string hybridStackStr = "";
    if (nativeFrames.empty()) {
        for (auto jsFrame : jsFrames) {
            hybridStackStr += PrintJsFrame(jsFrame);
        }
        Write(hybridStackStr);
        Write("\n");
        return;
    }

    std::string nativeStacksStr = Unwinder::GetFramesStr(nativeFrames);
    std::stringstream ss(nativeStacksStr);
    std::string nativeStackStr;
    while (std::getline(ss, nativeStackStr)) {
        hybridStackStr += nativeStackStr;
        hybridStackStr += "\n";
        if (nativeStackStr.find(flag_) != std::string::npos) {
            hybridStackStr += MatchJsFrameByFlag(jsFrames, jsIndex);
        }
    }

    Write(hybridStackStr);
    Write("\n");
}

bool HybridStackDumper::DumpMixFrame(const EcmaVM* vm)
{
    unwinder_ = std::make_shared<Unwinder>();
    std::vector<DfxFrame> nativeFrames;
    unwinder_->IgnoreMixstack(true);
    if (unwinder_->UnwindLocal(false, false, DEFAULT_MAX_FRAME_NUM, skipframes_)) {
        nativeFrames = unwinder_->GetFrames();
    } else {
        HILOG_ERROR("Failed to unwind local");
    }
    std::vector<JsFrameInfo> jsFrames;
    DFXJSNApi::BuildJsStackInfoList(vm, gettid(), jsFrames);
    BuildJsNativeMixStack(jsFrames, nativeFrames);
    return true;
}

void HybridStackDumper::Write(const std::string& outStr)
{
    stack_.append(outStr);
}

std::string HybridStackDumper::DumpMixStackLocked(const EcmaVM* vm)
{
    stack_.clear();
    DumpMixFrame(vm);

    return stack_;
}

std::string HybridStackDumper::GetMixStack(const EcmaVM* vm)
{
    HybridStackDumper stackDumper;
    return stackDumper.DumpMixStackLocked(vm);
}
#endif

void ArkNativeEngine::WrapSendableObj(napi_env env,
                                      napi_value js_object,
                                      void* native_object,
                                      panda::NativePointerCallback finalize_cb)
{
    Local<ObjectRef> nativeObject = LocalValueFromJsValue(js_object);
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = engine->GetEcmaVm();
    nativeObject->SetNativePointerFieldCount(vm, 1);
    nativeObject->SetNativePointerField(vm, 0, native_object, finalize_cb, nullptr, 0);
}

void ArkNativeEngine::UnwrapSendableObj(napi_env env, napi_value js_object, void** result)
{
    Local<ObjectRef> nativeObject = LocalValueFromJsValue(js_object);
    auto native_object = nativeObject->GetNativePointerField(0);
    *result = native_object;
}

int32_t ArkNativeEngine::GetObjectHash(napi_env env, napi_value src)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = engine->GetEcmaVm();
    auto nativeValue = LocalValueFromJsValue(src);
    return DFXJSNApi::GetObjectHash(vm, nativeValue);
}
