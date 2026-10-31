/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "test.h"
#include "test_common.h"
#include "gtest/gtest.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "native_api_internal.h"
#include "native_engine/native_utils.h"
#include "native_engine/impl/ark/ark_native_engine.h"
#include "napi/native_common.h"
#include "securec.h"
#include "utils/log.h"

static constexpr const char SHARED_MODULE_NAME[] = "test.shared.module";
static constexpr const char SHARED_MODULE_NAME_FAIL[] = "test.shared.module.fail";
static constexpr const char SHARED_MODULE_NAME_FAIL2[] = "test.shared.module.fail2";
static constexpr const char SHARED_MODULE_NAME_NONSENDABLE[] = "test.shared.module.nonsendable";
static constexpr const char SHARED_MODULE_NAME_THROW[] = "test.shared.module.throw";
static constexpr const char SHARED_MODULE_NAME_SLOW[] = "test.shared.module.slow";
static constexpr const char SHARED_MODULE_NAME_SLOWFAIL[] = "test.shared.module.slowfail";
static constexpr const char SHARED_MODULE_NAME_INNER[] = "test.shared.module.inner";
static constexpr const char SHARED_MODULE_NAME_OUTER[] = "test.shared.module.outer";
static constexpr const char SHARED_MODULE_NAME_MULTIVM[] = "test.shared.module.multivm";
static constexpr const char SHARED_MODULE_NAME_MULTIVM2[] = "test.shared.module.multivm2";

static std::atomic<int> g_initCount{0};

static std::mutex g_slowMutex;
static std::condition_variable g_slowCv;
static std::atomic<bool> g_slowStarted{false};
static std::atomic<bool> g_slowRelease{false};

static napi_value SharedModuleInit(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_value exports = nullptr;
    napi_status status = napi_create_sendable_object_with_properties(env, 0, nullptr, &exports);
    if (status != napi_ok) {
        return nullptr;
    }
    return exports;
}

static napi_value SharedModuleInitFail(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    return nullptr;
}

static napi_value SharedModuleInitNonSendable(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    return obj;
}

static napi_value SharedModuleInitThrow(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_throw_error(env, "100", "init threw");
    return nullptr;
}

static napi_value SharedModuleInitSlow(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    g_slowStarted.store(true);
    g_slowCv.notify_one();
    {
        std::unique_lock<std::mutex> lock(g_slowMutex);
        g_slowCv.wait(lock, [] { return g_slowRelease.load(); });
    }
    napi_value exports = nullptr;
    napi_create_sendable_object_with_properties(env, 0, nullptr, &exports);
    return exports;
}

static napi_value SharedModuleInitSlowFail(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    g_slowStarted.store(true);
    g_slowCv.notify_one();
    {
        std::unique_lock<std::mutex> lock(g_slowMutex);
        g_slowCv.wait(lock, [] { return g_slowRelease.load(); });
    }
    return nullptr;
}

static napi_value SharedModuleInitMultiVM(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_value exports = nullptr;
    napi_create_sendable_object_with_properties(env, 0, nullptr, &exports);
    return exports;
}

static EcmaVM* CreateWorkerVM()
{
    panda::RuntimeOption options;
    options.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::INFO);
    return panda::JSNApi::CreateJSVM(options);
}

static napi_value CallRequireNapi(napi_env env, const char* moduleName)
{
    napi_value global = nullptr;
    napi_get_global(env, &global);
    napi_value key = nullptr;
    napi_create_string_utf8(env, "requireNapi", NAPI_AUTO_LENGTH, &key);
    napi_value func = nullptr;
    napi_get_property(env, global, key, &func);
    napi_value arg = nullptr;
    napi_create_string_utf8(env, moduleName, NAPI_AUTO_LENGTH, &arg);
    napi_value result = nullptr;
    napi_call_function(env, global, func, 1, &arg, &result);
    return result;
}

static napi_value InnerModuleInit(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_value exports = nullptr;
    napi_create_sendable_object_with_properties(env, 0, nullptr, &exports);
    return exports;
}

static napi_value OuterModuleInit(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    CallRequireNapi(env, SHARED_MODULE_NAME_INNER);
    napi_value exports = nullptr;
    napi_create_sendable_object_with_properties(env, 0, nullptr, &exports);
    return exports;
}

static napi_module MakeModule(const char* modname, napi_addon_register_func regFunc)
{
    napi_module mod = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = __FILE__,
        .nm_register_func = regFunc,
        .nm_modname = modname,
        .nm_priv = nullptr,
        .reserved = { 0 },
    };
    return mod;
}

static void RegisterBasicModules()
{
    napi_module modValid = MakeModule(SHARED_MODULE_NAME, SharedModuleInit);
    napi_shared_module_register(&modValid);

    napi_module modFail = MakeModule(SHARED_MODULE_NAME_FAIL, SharedModuleInitFail);
    napi_shared_module_register(&modFail);

    napi_module modFail2 = MakeModule(SHARED_MODULE_NAME_FAIL2, SharedModuleInitFail);
    napi_shared_module_register(&modFail2);

    napi_module modNonSendable = MakeModule(SHARED_MODULE_NAME_NONSENDABLE, SharedModuleInitNonSendable);
    napi_shared_module_register(&modNonSendable);

    napi_module modThrow = MakeModule(SHARED_MODULE_NAME_THROW, SharedModuleInitThrow);
    napi_shared_module_register(&modThrow);
}

static void RegisterAdvancedModules()
{
    napi_module modSlow = MakeModule(SHARED_MODULE_NAME_SLOW, SharedModuleInitSlow);
    napi_shared_module_register(&modSlow);

    napi_module modSlowFail = MakeModule(SHARED_MODULE_NAME_SLOWFAIL, SharedModuleInitSlowFail);
    napi_shared_module_register(&modSlowFail);

    napi_module modInner = MakeModule(SHARED_MODULE_NAME_INNER, InnerModuleInit);
    napi_shared_module_register(&modInner);

    napi_module modOuter = MakeModule(SHARED_MODULE_NAME_OUTER, OuterModuleInit);
    napi_shared_module_register(&modOuter);
}

static void RegisterMultiEngineModules()
{
    napi_module modMultiVM = MakeModule(SHARED_MODULE_NAME_MULTIVM, SharedModuleInitMultiVM);
    napi_shared_module_register(&modMultiVM);

    napi_module modMultiVM2 = MakeModule(SHARED_MODULE_NAME_MULTIVM2, SharedModuleInitMultiVM);
    napi_shared_module_register(&modMultiVM2);
}

static void RegisterSharedModules()
{
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    RegisterBasicModules();
    RegisterAdvancedModules();
    RegisterMultiEngineModules();
}

class NapiSharedModuleTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "NapiSharedModuleTest SetUpTestCase";
        RegisterSharedModules();
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "NapiSharedModuleTest TearDownTestCase";
    }

    void SetUp() override
    {
        napi_env env = reinterpret_cast<napi_env>(engine_);
        ASSERT_NE(env, nullptr);
        g_initCount.store(0);
    }

    void TearDown() override {}
};

/* ===== Group A: napi_shared_module_register ===== */

/* TC-A2: napi_shared_module_register(nullptr) -> HILOG_ERROR, no crash */
HWTEST_F(NapiSharedModuleTest, SharedModule_Register_NullMod, testing::ext::TestSize.Level1)
{
    LoggerCollector collector;
    napi_shared_module_register(nullptr);
    ASSERT_TRUE(collector.Includes("mod is nullptr"));
}

/* ===== Group D: LoadSharedNativeModule — first call init ===== */

/* TC-D1: callback returns valid sendable object -> root stored, returned non-null */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_ValidSendableRoot, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result, nullptr);
    bool isSendable = false;
    napi_is_sendable(env, result, &isSendable);
    EXPECT_TRUE(isSendable);
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-D3: callback returns nullptr -> undefined returned, HILOG_ERROR */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_CallbackReturnsNull, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    LoggerCollector collector;
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_FAIL);
    ASSERT_NE(result, nullptr);
    napi_valuetype type;
    napi_typeof(env, result, &type);
    EXPECT_EQ(type, napi_undefined);
    ASSERT_TRUE(collector.Includes("callback returned null"));
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-D4: callback returns plain Object -> undefined, HILOG_ERROR "non-sendable" */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_CallbackReturnsPlainObject, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    LoggerCollector collector;
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_NONSENDABLE);
    ASSERT_NE(result, nullptr);
    napi_valuetype type;
    napi_typeof(env, result, &type);
    EXPECT_EQ(type, napi_undefined);
    ASSERT_TRUE(collector.Includes("non-sendable root"));
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-D7: callback throws exception -> pending exception propagates,
   errored_ set permanently, result is nullptr */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_CallbackThrows, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    LoggerCollector collector;
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_THROW);
    EXPECT_EQ(result, nullptr);
    ASSERT_TRUE(collector.Includes("init failed"));
    EXPECT_EQ(g_initCount.load(), 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_NE(exception, nullptr);
}

/* ===== Group E: cache hit / errored ===== */

/* TC-E1: second call after success -> find hits, no re-init */
HWTEST_F(NapiSharedModuleTest, LoadShared_SecondCall_AfterSuccess, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    int countBefore = g_initCount.load();

    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result1, nullptr);
    bool isSendable1 = false;
    napi_is_sendable(env, result1, &isSendable1);
    EXPECT_TRUE(isSendable1);

    napi_value result2 = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result2, nullptr);
    bool isSendable2 = false;
    napi_is_sendable(env, result2, &isSendable2);
    EXPECT_TRUE(isSendable2);

    /* Second call must not re-run the init callback (cache hit). */
    EXPECT_EQ(g_initCount.load(), countBefore);

    bool equals = false;
    napi_strict_equals(env, result1, result2, &equals);
    EXPECT_TRUE(equals);
}

/* TC-E2: second call after failure -> errored_ permanent, no retry */
HWTEST_F(NapiSharedModuleTest, LoadShared_SecondCall_AfterFailure, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    int countBefore = g_initCount.load();

    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME_FAIL2);
    ASSERT_NE(result1, nullptr);
    napi_valuetype type1;
    napi_typeof(env, result1, &type1);
    EXPECT_EQ(type1, napi_undefined);
    EXPECT_EQ(g_initCount.load(), countBefore + 1);

    napi_value result2 = CallRequireNapi(env, SHARED_MODULE_NAME_FAIL2);
    ASSERT_NE(result2, nullptr);
    napi_valuetype type2;
    napi_typeof(env, result2, &type2);
    EXPECT_EQ(type2, napi_undefined);
    EXPECT_EQ(g_initCount.load(), countBefore + 1);
}

/* ===== Group F: multi-engine sharing ===== */

/* TC-F1: main engine and worker engine import same shared module -> both get sendable root, init == 1 */
HWTEST_F(NapiSharedModuleTest, MultiVM_ConcurrentFirstImport, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME_MULTIVM);
    ASSERT_NE(result1, nullptr);
    bool isSendable = false;
    napi_is_sendable(env, result1, &isSendable);
    EXPECT_TRUE(isSendable);
    EXPECT_EQ(g_initCount.load(), 1);

    EcmaVM* workerVM = CreateWorkerVM();
    ASSERT_NE(workerVM, nullptr);
    ArkNativeEngine* workerEngine = new ArkNativeEngine(workerVM, nullptr);
    ASSERT_NE(workerEngine, nullptr);

    napi_env workerEnv = reinterpret_cast<napi_env>(workerEngine);
    napi_value result2 = CallRequireNapi(workerEnv, SHARED_MODULE_NAME_MULTIVM);
    ASSERT_NE(result2, nullptr);
    bool isSendable2 = false;
    napi_is_sendable(workerEnv, result2, &isSendable2);
    EXPECT_TRUE(isSendable2);

    EXPECT_EQ(g_initCount.load(), 1);

    delete workerEngine;
    panda::JSNApi::DestroyJSVM(workerVM);
}

/* TC-F2: main engine inits first, worker engine imports later -> both get sendable root, init stays 1 */
HWTEST_F(NapiSharedModuleTest, MultiVM_SecondVMAfterInit, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME_MULTIVM2);
    ASSERT_NE(result1, nullptr);
    bool isSendable1 = false;
    napi_is_sendable(env, result1, &isSendable1);
    EXPECT_TRUE(isSendable1);
    EXPECT_EQ(g_initCount.load(), 1);

    EcmaVM* workerVM = CreateWorkerVM();
    ASSERT_NE(workerVM, nullptr);
    ArkNativeEngine* workerEngine = new ArkNativeEngine(workerVM, nullptr);
    ASSERT_NE(workerEngine, nullptr);

    napi_env workerEnv = reinterpret_cast<napi_env>(workerEngine);
    napi_value result2 = CallRequireNapi(workerEnv, SHARED_MODULE_NAME_MULTIVM2);
    ASSERT_NE(result2, nullptr);
    bool isSendable2 = false;
    napi_is_sendable(workerEnv, result2, &isSendable2);
    EXPECT_TRUE(isSendable2);

    EXPECT_EQ(g_initCount.load(), 1);

    delete workerEngine;
    panda::JSNApi::DestroyJSVM(workerVM);
}

/* ===== Group F-extra: two-level locking cv.wait path ===== */

/* TC-F3: VM-A slow init, VM-B arrives while init in progress -> VM-B waits on cv,
   both get sendable root, init == 1 (cv.wait + notify_all path) */
HWTEST_F(NapiSharedModuleTest, MultiVM_ConcurrentWaitForInit, testing::ext::TestSize.Level1)
{
    g_slowStarted.store(false);
    g_slowRelease.store(false);
    g_initCount.store(0);

    std::atomic<bool> ok1{false};
    std::atomic<bool> ok2{false};

    // Thread 1: worker VM, becomes init party (slow init blocks inside callback)
    std::thread t1([&ok1]() {
        EcmaVM* vm = CreateWorkerVM();
        auto* engine = new ArkNativeEngine(vm, nullptr);
        napi_env env = reinterpret_cast<napi_env>(engine);
        napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_SLOW);
        bool isSendable = false;
        napi_is_sendable(env, result, &isSendable);
        ok1.store(result != nullptr && isSendable);
        delete engine;
        panda::JSNApi::DestroyJSVM(vm);
    });

    // Wait for thread 1 to enter init (initializing=true, entry.mtx released)
    {
        std::unique_lock<std::mutex> lock(g_slowMutex);
        g_slowCv.wait(lock, [] { return g_slowStarted.load(); });
    }

    // Thread 2: another worker VM, should hit initializing=true -> cv.wait
    std::thread t2([&ok2]() {
        EcmaVM* vm = CreateWorkerVM();
        auto* engine = new ArkNativeEngine(vm, nullptr);
        napi_env env = reinterpret_cast<napi_env>(engine);
        napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_SLOW);
        bool isSendable = false;
        napi_is_sendable(env, result, &isSendable);
        ok2.store(result != nullptr && isSendable);
        delete engine;
        panda::JSNApi::DestroyJSVM(vm);
    });

    // Give thread 2 time to reach cv.wait
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Release the slow init -> thread 1 finishes -> notify_all -> thread 2 wakes
    g_slowRelease.store(true);
    g_slowCv.notify_one();

    t1.join();
    t2.join();

    EXPECT_TRUE(ok1.load());
    EXPECT_TRUE(ok2.load());
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-F5: VM-A slow init fails, VM-B waits on cv -> VM-B sees errored_,
   returns Undefined, no retry (cv.wait failure path) */
HWTEST_F(NapiSharedModuleTest, MultiVM_ConcurrentInitFail_WaitingVMGetsUndefined, testing::ext::TestSize.Level1)
{
    g_slowStarted.store(false);
    g_slowRelease.store(false);
    g_initCount.store(0);

    std::atomic<bool> ok1{false};
    std::atomic<bool> ok2{false};

    // Thread 1: worker VM, becomes init party (slow init blocks then fails)
    std::thread t1([&ok1]() {
        EcmaVM* vm = CreateWorkerVM();
        auto* engine = new ArkNativeEngine(vm, nullptr);
        napi_env env = reinterpret_cast<napi_env>(engine);
        napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_SLOWFAIL);
        ok1.store(result == nullptr || !LocalValueFromJsValue(result)->IsJSShared(vm));
        delete engine;
        panda::JSNApi::DestroyJSVM(vm);
    });

    // Wait for thread 1 to enter init (initializing=true, entry.mtx_ released)
    {
        std::unique_lock<std::mutex> lock(g_slowMutex);
        g_slowCv.wait(lock, [] { return g_slowStarted.load(); });
    }

    // Thread 2: another worker VM, should hit initializing=true -> cv.wait
    std::thread t2([&ok2]() {
        EcmaVM* vm = CreateWorkerVM();
        auto* engine = new ArkNativeEngine(vm, nullptr);
        napi_env env = reinterpret_cast<napi_env>(engine);
        napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_SLOWFAIL);
        // VM-B wakes up, sees errored_ -> Undefined, no retry
        napi_valuetype type = napi_undefined;
        napi_typeof(env, result, &type);
        ok2.store(type == napi_undefined);
        delete engine;
        panda::JSNApi::DestroyJSVM(vm);
    });

    // Give thread 2 time to reach cv.wait
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Release the slow init -> thread 1 finishes with failure -> notify_all -> thread 2 wakes
    g_slowRelease.store(true);
    g_slowCv.notify_one();

    t1.join();
    t2.join();

    EXPECT_TRUE(ok1.load());
    EXPECT_TRUE(ok2.load());
    // Only VM-A ran callback, VM-B did not retry
    EXPECT_EQ(g_initCount.load(), 1);
}

/* ===== Group F-reentry: cross-module reentry (no deadlock) ===== */

/* TC-F4: outer module's callback calls requireNapi(inner) -> no deadlock,
   both modules init successfully (two-level locking: entry lock released before
   CreateSendableRoot, inner uses different entry lock) */
HWTEST_F(NapiSharedModuleTest, LoadShared_CrossModuleReentry_NoDeadlock, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_OUTER);
    ASSERT_NE(result, nullptr);
    bool isSendable = false;
    napi_is_sendable(env, result, &isSendable);
    EXPECT_TRUE(isSendable);
    // outer init + inner init = 2
    EXPECT_EQ(g_initCount.load(), 2);
}
