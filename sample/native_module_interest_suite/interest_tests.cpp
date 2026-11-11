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

constexpr size_t K_INTEREST_CASE_COUNT = 20;
constexpr size_t K_INTEREST_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_FIRST_ARG_INDEX = 0;
constexpr int32_t K_MIN_PRINCIPAL = 1;
constexpr int32_t K_RATE_BASE_BP = 300;
constexpr int32_t K_RATE_STEP_BP = 50;
constexpr int32_t K_RATE_CYCLE = 9;
constexpr int32_t K_TERM_BASE_YEARS = 2;
constexpr int32_t K_TERM_CYCLE = 5;
constexpr int32_t K_BASIS_POINTS = 10000;
constexpr int32_t K_CENTS_SCALE = 100;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

constexpr std::array<const int32_t, 4> K_COMPOUNDING_PERIODS = {1, 2, 4, 12};

struct InterestCaseSpec {
    std::string name;
    int32_t rateBp;
    int32_t termYears;
    int32_t periodsPerYear;
};

struct InterestComputation {
    int32_t perPeriodRateBp;
    int32_t totalPeriods;
    int64_t simpleInterestCents;
    int64_t compoundInterestCents;
    int64_t simpleTotalCents;
    int64_t compoundTotalCents;
    bool compoundExceedsSimple;
};

std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(
            0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(data));
}

InterestCaseSpec GetInterestCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    const size_t periodIndex = caseIndex % K_COMPOUNDING_PERIODS.size();
    const int32_t ratePhase = static_cast<int32_t>(caseNumber % static_cast<size_t>(K_RATE_CYCLE));
    const int32_t termPhase = static_cast<int32_t>(caseNumber % static_cast<size_t>(K_TERM_CYCLE));
    return {
        BuildIndexedName("interestCase", caseNumber),
        K_RATE_BASE_BP + K_RATE_STEP_BP * ratePhase,
        K_TERM_BASE_YEARS + termPhase,
        K_COMPOUNDING_PERIODS[periodIndex],
    };
}

bool ReadPrincipal(napi_env env, napi_value value, int32_t* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_number) {
        napi_throw_type_error(env, nullptr, "principal must be a number");
        return false;
    }
    if (napi_get_value_int32(env, value, result) != napi_ok) {
        return false;
    }
    if (*result < K_MIN_PRINCIPAL) {
        napi_throw_error(env, nullptr, "principal must be positive");
        return false;
    }
    return true;
}

int64_t ComputeCompoundBalanceCents(int64_t principalCents, int32_t perPeriodRateBp, int32_t totalPeriods)
{
    int64_t balance = principalCents;
    const int64_t growthBp = static_cast<int64_t>(K_BASIS_POINTS + perPeriodRateBp);
    for (int32_t period = 0; period < totalPeriods; period++) {
        balance = balance * growthBp / static_cast<int64_t>(K_BASIS_POINTS);
    }
    return balance;
}

InterestComputation ComputeInterestComputation(int64_t principalCents, const InterestCaseSpec& spec)
{
    InterestComputation computation = {};
    computation.perPeriodRateBp = spec.rateBp / spec.periodsPerYear;
    computation.totalPeriods = spec.periodsPerYear * spec.termYears;
    computation.simpleInterestCents =
        principalCents * static_cast<int64_t>(spec.rateBp) * static_cast<int64_t>(spec.termYears) /
        static_cast<int64_t>(K_BASIS_POINTS);
    computation.compoundTotalCents =
        ComputeCompoundBalanceCents(principalCents, computation.perPeriodRateBp, computation.totalPeriods);
    computation.compoundInterestCents = computation.compoundTotalCents - principalCents;
    computation.simpleTotalCents = principalCents + computation.simpleInterestCents;
    computation.compoundExceedsSimple = computation.compoundInterestCents > computation.simpleInterestCents;
    return computation;
}

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedInt64(napi_env env, napi_value object, const char* name, int64_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int64(env, value, &napiValue) != napi_ok) {
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

bool SetNamedString(napi_env env, napi_value object, const char* name, const char* value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateInterestResultObject(napi_env env, int32_t principal, const InterestCaseSpec& spec,
    const InterestComputation& computation)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name.c_str());
    SetNamedInt32(env, result, "principal", principal);
    SetNamedInt32(env, result, "rateBp", spec.rateBp);
    SetNamedInt32(env, result, "termYears", spec.termYears);
    SetNamedInt32(env, result, "periodsPerYear", spec.periodsPerYear);
    SetNamedInt32(env, result, "perPeriodRateBp", computation.perPeriodRateBp);
    SetNamedInt32(env, result, "totalPeriods", computation.totalPeriods);
    SetNamedInt64(env, result, "simpleInterestCents", computation.simpleInterestCents);
    SetNamedInt64(env, result, "compoundInterestCents", computation.compoundInterestCents);
    SetNamedInt64(env, result, "simpleTotalCents", computation.simpleTotalCents);
    SetNamedInt64(env, result, "compoundTotalCents", computation.compoundTotalCents);
    SetNamedBool(env, result, "compoundExceedsSimple", computation.compoundExceedsSimple);
    return result;
}

static napi_value RunInterestCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_INTEREST_ARG_COUNT;
    napi_value args[K_INTEREST_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_INTEREST_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid interest case");
        return nullptr;
    }
    if (argc < K_INTEREST_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "one number is required");
        return nullptr;
    }

    int32_t principal = 0;
    if (!ReadPrincipal(env, args[K_FIRST_ARG_INDEX], &principal)) {
        return nullptr;
    }

    const auto spec = GetInterestCaseSpec(caseIndex);
    const int64_t principalCents = static_cast<int64_t>(principal) * static_cast<int64_t>(K_CENTS_SCALE);
    const InterestComputation computation = ComputeInterestComputation(principalCents, spec);
    return CreateInterestResultObject(env, principal, spec, computation);
}

}  // namespace

static napi_value InitInterestSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_INTEREST_CASE_COUNT);
    exportNames.reserve(K_INTEREST_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_INTEREST_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(
            BuildIndexedName("testInterestCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{
            exportNames.back().c_str(),
            nullptr,
            RunInterestCase,
            nullptr,
            nullptr,
            nullptr,
            napi_default,
            reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex)),
        };
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_interestSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitInterestSuite,
    .nm_modname = "interest_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterInterestSuiteModule(void)
{
    napi_module_register(&g_interestSuiteModule);
}
