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

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// --- Case Count ---
constexpr size_t K_ROUNDTRIP_CASE_COUNT = 16;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Test Values (Int32) ---
constexpr int32_t K_INT32_ZERO = 0;
constexpr int32_t K_INT32_ONE = 1;
constexpr int32_t K_INT32_NEG_ONE = -1;
constexpr int32_t K_INT32_FORTY_TWO = 42;
constexpr int32_t K_INT32_ONE_HUNDRED = 100;
constexpr int32_t K_INT32_NEG_FIFTY = -50;
constexpr int32_t K_INT32_MAX = 2147483647;
constexpr int32_t K_INT32_MIN = (-2147483647 - 1);

// --- Test Values (Double) ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;
constexpr double K_DOUBLE_NEG_ONE = -1.0;
constexpr double K_DOUBLE_PI = 3.14159265358979;
constexpr double K_DOUBLE_E = 2.71828182845904;
constexpr double K_DOUBLE_LARGE = 123456.789;

// === Scalar Type Enum ===
enum class ScalarType : int32_t {
    INT32,
    DOUBLE,
    BOOL,
};

// === Spec Structures ===
struct RoundtripCaseSpec {
    std::string name;
    ScalarType type;
    int32_t int32Value;
    double doubleValue;
    bool boolValue;
};

// === Helper Functions - Name Building & Case Index ===
std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data) { return static_cast<size_t>(reinterpret_cast<uintptr_t>(data)); }

// === Helper Functions - Case Specifications ===
const RoundtripCaseSpec& GetRoundtripCaseSpec(size_t caseIndex)
{
    static const RoundtripCaseSpec specs[K_ROUNDTRIP_CASE_COUNT] = {
        { "roundtripCase01", ScalarType::INT32, K_INT32_ZERO, K_DOUBLE_ZERO, false },
        { "roundtripCase02", ScalarType::INT32, K_INT32_ONE, K_DOUBLE_ZERO, false },
        { "roundtripCase03", ScalarType::INT32, K_INT32_NEG_ONE, K_DOUBLE_ZERO, false },
        { "roundtripCase04", ScalarType::INT32, K_INT32_FORTY_TWO, K_DOUBLE_ZERO, false },
        { "roundtripCase05", ScalarType::INT32, K_INT32_ONE_HUNDRED, K_DOUBLE_ZERO, false },
        { "roundtripCase06", ScalarType::INT32, K_INT32_NEG_FIFTY, K_DOUBLE_ZERO, false },
        { "roundtripCase07", ScalarType::INT32, K_INT32_MAX, K_DOUBLE_ZERO, false },
        { "roundtripCase08", ScalarType::INT32, K_INT32_MIN, K_DOUBLE_ZERO, false },
        { "roundtripCase09", ScalarType::DOUBLE, K_INT32_ZERO, K_DOUBLE_ZERO, false },
        { "roundtripCase10", ScalarType::DOUBLE, K_INT32_ZERO, K_DOUBLE_ONE, false },
        { "roundtripCase11", ScalarType::DOUBLE, K_INT32_ZERO, K_DOUBLE_NEG_ONE, false },
        { "roundtripCase12", ScalarType::DOUBLE, K_INT32_ZERO, K_DOUBLE_PI, false },
        { "roundtripCase13", ScalarType::DOUBLE, K_INT32_ZERO, K_DOUBLE_E, false },
        { "roundtripCase14", ScalarType::DOUBLE, K_INT32_ZERO, K_DOUBLE_LARGE, false },
        { "roundtripCase15", ScalarType::BOOL, K_INT32_ZERO, K_DOUBLE_ZERO, true },
        { "roundtripCase16", ScalarType::BOOL, K_INT32_ZERO, K_DOUBLE_ZERO, false },
    };
    return specs[caseIndex];
}

// === Helper Functions - Object Property ===
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

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Helper Functions - Result Object ===
napi_value MakeRoundtripResult(napi_env env, const RoundtripCaseSpec& spec, bool passed)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedBool(env, result, "passed", passed);
    SetNamedInt32(env, result, "type", static_cast<int32_t>(spec.type));
    return result;
}

// === Test Functions ===
napi_value RunInt32(napi_env env, const RoundtripCaseSpec& spec)
{
    napi_value value = nullptr;
    NAPI_CALL(env, napi_create_int32(env, spec.int32Value, &value));
    int32_t actual = K_INT32_ZERO;
    NAPI_CALL(env, napi_get_value_int32(env, value, &actual));
    const bool passed = (actual == spec.int32Value);
    return MakeRoundtripResult(env, spec, passed);
}

napi_value RunDouble(napi_env env, const RoundtripCaseSpec& spec)
{
    napi_value value = nullptr;
    NAPI_CALL(env, napi_create_double(env, spec.doubleValue, &value));
    double actual = K_DOUBLE_ZERO;
    NAPI_CALL(env, napi_get_value_double(env, value, &actual));
    const bool passed = (actual == spec.doubleValue);
    return MakeRoundtripResult(env, spec, passed);
}

napi_value RunBool(napi_env env, const RoundtripCaseSpec& spec)
{
    napi_value value = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, spec.boolValue, &value));
    bool actual = false;
    NAPI_CALL(env, napi_get_value_bool(env, value, &actual));
    const bool passed = (actual == spec.boolValue);
    return MakeRoundtripResult(env, spec, passed);
}

static napi_value RunRoundtripCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_ROUNDTRIP_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid roundtrip case");
        return nullptr;
    }
    const RoundtripCaseSpec& spec = GetRoundtripCaseSpec(caseIndex);
    switch (spec.type) {
        case ScalarType::INT32:  return RunInt32(env, spec);
        case ScalarType::DOUBLE: return RunDouble(env, spec);
        case ScalarType::BOOL:   return RunBool(env, spec);
        default:
            napi_throw_error(env, nullptr, "unknown scalar type");
            return nullptr;
    }
}

}  // namespace

// === Module Registration ===
static napi_value InitRoundtripSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_ROUNDTRIP_CASE_COUNT);
    exportNames.reserve(K_ROUNDTRIP_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_ROUNDTRIP_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testRoundtripCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunRoundtripCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_roundtripSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitRoundtripSuite,
    .nm_modname = "roundtrip_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterRoundtripSuiteModule(void)
{
    napi_module_register(&g_roundtripSuiteModule);
}
