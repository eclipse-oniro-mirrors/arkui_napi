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

#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

// === Constants ===

// --- Argument Counts ---
constexpr size_t K_ARGS_ZERO = 0;

// --- Case Count ---
constexpr size_t K_FUNCTION_CASE_COUNT = 21;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Function Configuration ---
constexpr size_t K_FUNCTION_KIND_CYCLE = 7;
constexpr size_t K_FUNCTION_ARG_CYCLE = 3;
constexpr size_t K_FUNCTION_MAX_ARGS = 2;
constexpr size_t K_FUNCTION_PARITY_MOD = 2;

// --- Return Kind Cycle Indices ---
constexpr size_t K_KIND_INDEX_INT32 = 0;
constexpr size_t K_KIND_INDEX_DOUBLE = 1;
constexpr size_t K_KIND_INDEX_STRING = 2;
constexpr size_t K_KIND_INDEX_BOOL = 3;
constexpr size_t K_KIND_INDEX_UNDEFINED = 4;
constexpr size_t K_KIND_INDEX_OBJECT = 5;
constexpr size_t K_KIND_INDEX_NULL = 6;

// --- Return Kind Labels ---
const char* const K_RETURN_LABEL_INT32 = "int32";
const char* const K_RETURN_LABEL_DOUBLE = "double";
const char* const K_RETURN_LABEL_STRING = "string";
const char* const K_RETURN_LABEL_BOOL = "bool";
const char* const K_RETURN_LABEL_UNDEFINED = "undefined";
const char* const K_RETURN_LABEL_OBJECT = "object";
const char* const K_RETURN_LABEL_NULL = "null";

// --- Return Kind Descriptions ---
const char* const K_RETURN_DESC_INT32 = "callback returns napi_create_int32 value";
const char* const K_RETURN_DESC_DOUBLE = "callback returns napi_create_double value";
const char* const K_RETURN_DESC_STRING = "callback returns napi_create_string_utf8 value";
const char* const K_RETURN_DESC_BOOL = "callback returns napi_get_boolean value";
const char* const K_RETURN_DESC_UNDEFINED = "callback returns napi_get_undefined value";
const char* const K_RETURN_DESC_OBJECT = "callback returns napi_create_object value";
const char* const K_RETURN_DESC_NULL = "callback returns napi_get_null value";

// --- Typeof Labels ---
const char* const K_TYPEOF_NUMBER = "number";
const char* const K_TYPEOF_STRING = "string";
const char* const K_TYPEOF_BOOLEAN = "boolean";
const char* const K_TYPEOF_OBJECT = "object";
const char* const K_TYPEOF_UNDEFINED = "undefined";
const char* const K_TYPEOF_NULL = "null";
const char* const K_TYPEOF_FUNCTION = "function";
const char* const K_TYPEOF_UNKNOWN = "unknown";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class ReturnKind {
    INT32,
    DOUBLE,
    STRING,
    BOOL,
    UNDEFINED,
    OBJECT,
    NULL_VAL,
};

struct FunctionCaseSpec {
    std::string name;
    std::string funcName;
    ReturnKind returnKind;
    size_t argCount;
    int32_t seed;
};

// === Helpers: Naming & Case Lookup ===

std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(data));
}

FunctionCaseSpec GetFunctionCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    FunctionCaseSpec spec;
    spec.name = BuildIndexedName("functionCase", caseNumber);
    spec.funcName = BuildIndexedName("nativeFn", caseNumber);
    spec.seed = static_cast<int32_t>(caseNumber);
    spec.argCount = caseNumber % K_FUNCTION_ARG_CYCLE;
    const size_t kindCycle = caseNumber % K_FUNCTION_KIND_CYCLE;
    if (kindCycle == K_KIND_INDEX_INT32) {
        spec.returnKind = ReturnKind::INT32;
    } else if (kindCycle == K_KIND_INDEX_DOUBLE) {
        spec.returnKind = ReturnKind::DOUBLE;
    } else if (kindCycle == K_KIND_INDEX_STRING) {
        spec.returnKind = ReturnKind::STRING;
    } else if (kindCycle == K_KIND_INDEX_BOOL) {
        spec.returnKind = ReturnKind::BOOL;
    } else if (kindCycle == K_KIND_INDEX_UNDEFINED) {
        spec.returnKind = ReturnKind::UNDEFINED;
    } else if (kindCycle == K_KIND_INDEX_OBJECT) {
        spec.returnKind = ReturnKind::OBJECT;
    } else {
        spec.returnKind = ReturnKind::NULL_VAL;
    }
    return spec;
}

const char* ReturnKindLabel(ReturnKind kind)
{
    if (kind == ReturnKind::INT32) {
        return K_RETURN_LABEL_INT32;
    }
    if (kind == ReturnKind::DOUBLE) {
        return K_RETURN_LABEL_DOUBLE;
    }
    if (kind == ReturnKind::STRING) {
        return K_RETURN_LABEL_STRING;
    }
    if (kind == ReturnKind::BOOL) {
        return K_RETURN_LABEL_BOOL;
    }
    if (kind == ReturnKind::UNDEFINED) {
        return K_RETURN_LABEL_UNDEFINED;
    }
    if (kind == ReturnKind::OBJECT) {
        return K_RETURN_LABEL_OBJECT;
    }
    return K_RETURN_LABEL_NULL;
}

const char* ReturnKindDescription(ReturnKind kind)
{
    if (kind == ReturnKind::INT32) {
        return K_RETURN_DESC_INT32;
    }
    if (kind == ReturnKind::DOUBLE) {
        return K_RETURN_DESC_DOUBLE;
    }
    if (kind == ReturnKind::STRING) {
        return K_RETURN_DESC_STRING;
    }
    if (kind == ReturnKind::BOOL) {
        return K_RETURN_DESC_BOOL;
    }
    if (kind == ReturnKind::UNDEFINED) {
        return K_RETURN_DESC_UNDEFINED;
    }
    if (kind == ReturnKind::OBJECT) {
        return K_RETURN_DESC_OBJECT;
    }
    return K_RETURN_DESC_NULL;
}

const char* TypeofLabel(napi_valuetype type)
{
    if (type == napi_undefined) {
        return K_TYPEOF_UNDEFINED;
    }
    if (type == napi_null) {
        return K_TYPEOF_NULL;
    }
    if (type == napi_boolean) {
        return K_TYPEOF_BOOLEAN;
    }
    if (type == napi_number) {
        return K_TYPEOF_NUMBER;
    }
    if (type == napi_string) {
        return K_TYPEOF_STRING;
    }
    if (type == napi_object) {
        return K_TYPEOF_OBJECT;
    }
    if (type == napi_function) {
        return K_TYPEOF_FUNCTION;
    }
    return K_TYPEOF_UNKNOWN;
}

// === Helpers: Object Property Setters ===

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Helpers: Return Value Construction ===

napi_value BuildReturnValue(napi_env env, const FunctionCaseSpec& spec)
{
    napi_value v = nullptr;
    if (spec.returnKind == ReturnKind::INT32) {
        NAPI_CALL(env, napi_create_int32(env, spec.seed, &v));
        return v;
    }
    if (spec.returnKind == ReturnKind::DOUBLE) {
        NAPI_CALL(env, napi_create_double(env, static_cast<double>(spec.seed), &v));
        return v;
    }
    if (spec.returnKind == ReturnKind::STRING) {
        NAPI_CALL(env, napi_create_string_utf8(env, spec.name.c_str(), spec.name.size(), &v));
        return v;
    }
    if (spec.returnKind == ReturnKind::BOOL) {
        const bool truthy = (static_cast<size_t>(spec.seed) % K_FUNCTION_PARITY_MOD == 0);
        NAPI_CALL(env, napi_get_boolean(env, truthy, &v));
        return v;
    }
    if (spec.returnKind == ReturnKind::UNDEFINED) {
        NAPI_CALL(env, napi_get_undefined(env, &v));
        return v;
    }
    if (spec.returnKind == ReturnKind::NULL_VAL) {
        NAPI_CALL(env, napi_get_null(env, &v));
        return v;
    }
    NAPI_CALL(env, napi_create_object(env, &v));
    SetNamedInt32(env, v, "seed", spec.seed);
    return v;
}

// === Helpers: Function Creation & Invocation ===

static napi_value CaseFunctionCallback(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_FUNCTION_MAX_ARGS;
    napi_value args[K_FUNCTION_MAX_ARGS] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_FUNCTION_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid function case");
        return nullptr;
    }
    const auto spec = GetFunctionCaseSpec(caseIndex);
    return BuildReturnValue(env, spec);
}

napi_value CreateCaseFunction(napi_env env, const FunctionCaseSpec& spec, size_t caseIndex)
{
    napi_value fn = nullptr;
    NAPI_CALL(env, napi_create_function(env, spec.funcName.c_str(), spec.funcName.size(),
        CaseFunctionCallback, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex)), &fn));
    return fn;
}

bool CheckFunctionType(napi_env env, napi_value fn)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, fn, &type) != napi_ok) {
        return false;
    }
    if (type != napi_function) {
        napi_throw_type_error(env, nullptr, "created value must be a function");
        return false;
    }
    return true;
}

bool BuildCallArgs(napi_env env, const FunctionCaseSpec& spec, napi_value* args)
{
    for (size_t i = 0; i < spec.argCount; i++) {
        if (napi_create_int32(env, spec.seed + static_cast<int32_t>(i), &args[i]) != napi_ok) {
            return false;
        }
    }
    return true;
}

napi_value CallCaseFunction(napi_env env, const FunctionCaseSpec& spec, napi_value fn)
{
    napi_value args[K_FUNCTION_MAX_ARGS] = {nullptr};
    if (!BuildCallArgs(env, spec, args)) {
        return nullptr;
    }
    napi_value undefined = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));
    napi_value callResult = nullptr;
    NAPI_CALL(env, napi_call_function(env, undefined, fn, spec.argCount, args, &callResult));
    return callResult;
}

bool ReturnKindMatches(napi_env env, napi_value callResult, ReturnKind kind)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, callResult, &type) != napi_ok) {
        return false;
    }
    if (kind == ReturnKind::UNDEFINED) {
        return type == napi_undefined;
    }
    if (kind == ReturnKind::NULL_VAL) {
        return type == napi_null;
    }
    if (kind == ReturnKind::BOOL) {
        return type == napi_boolean;
    }
    if (kind == ReturnKind::STRING) {
        return type == napi_string;
    }
    if (kind == ReturnKind::OBJECT) {
        return type == napi_object;
    }
    // INT32 and DOUBLE both surface as napi_number.
    return type == napi_number;
}

// === Result Building ===

napi_value BuildFunctionResult(napi_env env, const FunctionCaseSpec& spec, napi_value fn,
    napi_value callResult, bool isFunction, bool returnMatches)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "funcName", spec.funcName);
    SetNamedString(env, result, "returnKind", ReturnKindLabel(spec.returnKind));
    SetNamedString(env, result, "description", ReturnKindDescription(spec.returnKind));
    SetNamedInt32(env, result, "seed", spec.seed);
    SetNamedInt32(env, result, "argCount", static_cast<int32_t>(spec.argCount));
    SetNamedBool(env, result, "isFunction", isFunction);
    SetNamedBool(env, result, "returnMatches", returnMatches);
    napi_valuetype returnType = napi_undefined;
    if (napi_typeof(env, callResult, &returnType) == napi_ok) {
        SetNamedString(env, result, "returnValueType", TypeofLabel(returnType));
    }
    NAPI_CALL(env, napi_set_named_property(env, result, "function", fn));
    NAPI_CALL(env, napi_set_named_property(env, result, "returnValue", callResult));
    return result;
}

// === Test Entry ===

static napi_value RunFunctionCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_FUNCTION_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid function case");
        return nullptr;
    }
    const auto spec = GetFunctionCaseSpec(caseIndex);
    napi_value fn = CreateCaseFunction(env, spec, caseIndex);
    if (fn == nullptr) {
        return nullptr;
    }
    const bool isFunction = CheckFunctionType(env, fn);
    if (!isFunction) {
        return nullptr;
    }
    napi_value callResult = CallCaseFunction(env, spec, fn);
    if (callResult == nullptr) {
        return nullptr;
    }
    const bool returnMatches = ReturnKindMatches(env, callResult, spec.returnKind);
    return BuildFunctionResult(env, spec, fn, callResult, isFunction, returnMatches);
}

}  // namespace

static napi_value InitFunctionSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_FUNCTION_CASE_COUNT);
    exportNames.reserve(K_FUNCTION_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_FUNCTION_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testFunctionCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunFunctionCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_functionSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitFunctionSuite,
    .nm_modname = "function_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterFunctionSuiteModule(void)
{
    napi_module_register(&g_functionSuiteModule);
}
