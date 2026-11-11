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

constexpr size_t K_INTERVAL_CASE_COUNT = 20;
constexpr size_t K_INTERVAL_ARG_COUNT = 2;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_FIRST_ARG_INDEX = 0;
constexpr size_t K_SECOND_ARG_INDEX = 1;
constexpr int32_t K_FIXED_LOWER_BASE = -20;
constexpr int32_t K_FIXED_LOWER_STEP = 3;
constexpr int32_t K_FIXED_LOWER_CYCLE = 9;
constexpr int32_t K_FIXED_WIDTH_BASE = 5;
constexpr int32_t K_FIXED_WIDTH_CYCLE = 7;
constexpr int32_t K_NO_OVERLAP_WIDTH = 0;
constexpr int32_t K_NO_GAP = 0;
constexpr int32_t K_TOUCHING_WIDTH = 0;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

struct IntervalCaseSpec {
    std::string name;
    int32_t fixedLower;
    int32_t fixedUpper;
};

struct IntervalRelation {
    int32_t userLower;
    int32_t userUpper;
    int32_t overlapLower;
    int32_t overlapUpper;
    int32_t overlapWidth;
    int32_t mergedLower;
    int32_t mergedUpper;
    int32_t mergedWidth;
    int32_t gap;
    bool hasOverlap;
    bool userInsideFixed;
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

IntervalCaseSpec GetIntervalCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    const int32_t lowerPhase = static_cast<int32_t>(caseNumber % static_cast<size_t>(K_FIXED_LOWER_CYCLE));
    const int32_t widthPhase = static_cast<int32_t>(caseNumber % static_cast<size_t>(K_FIXED_WIDTH_CYCLE));
    const int32_t fixedLower = K_FIXED_LOWER_BASE + K_FIXED_LOWER_STEP * lowerPhase;
    const int32_t fixedWidth = K_FIXED_WIDTH_BASE + widthPhase;
    return {BuildIndexedName("intervalCase", caseNumber), fixedLower, fixedLower + fixedWidth};
}

int32_t MinInt32(int32_t left, int32_t right)
{
    return left < right ? left : right;
}

int32_t MaxInt32(int32_t left, int32_t right)
{
    return left > right ? left : right;
}

void ComputeIntervalRelation(const IntervalCaseSpec& spec, int32_t first, int32_t second,
    IntervalRelation* relation)
{
    relation->userLower = MinInt32(first, second);
    relation->userUpper = MaxInt32(first, second);
    relation->overlapLower = MaxInt32(spec.fixedLower, relation->userLower);
    relation->overlapUpper = MinInt32(spec.fixedUpper, relation->userUpper);
    relation->hasOverlap = relation->overlapLower <= relation->overlapUpper;
    relation->overlapWidth = relation->hasOverlap ? relation->overlapUpper - relation->overlapLower
        : K_NO_OVERLAP_WIDTH;
    relation->mergedLower = MinInt32(spec.fixedLower, relation->userLower);
    relation->mergedUpper = MaxInt32(spec.fixedUpper, relation->userUpper);
    relation->mergedWidth = relation->mergedUpper - relation->mergedLower;
    relation->gap = relation->hasOverlap ? K_NO_GAP : (relation->userUpper < spec.fixedLower
        ? spec.fixedLower - relation->userUpper
        : relation->userLower - spec.fixedUpper);
    relation->userInsideFixed = relation->userLower >= spec.fixedLower &&
        relation->userUpper <= spec.fixedUpper;
}

const char* GetRelationLabel(const IntervalRelation& relation)
{
    if (!relation.hasOverlap) {
        return "disjoint";
    }
    return relation.overlapWidth == K_TOUCHING_WIDTH ? "touching" : "overlap";
}

bool ReadInt32(napi_env env, napi_value value, const char* message, int32_t* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_number) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }
    return napi_get_value_int32(env, value, result) == napi_ok;
}

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
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

napi_value CreateIntervalResultObject(napi_env env, const IntervalCaseSpec& spec,
    const IntervalRelation& relation)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name.c_str());
    SetNamedInt32(env, result, "fixedLower", spec.fixedLower);
    SetNamedInt32(env, result, "fixedUpper", spec.fixedUpper);
    SetNamedInt32(env, result, "userLower", relation.userLower);
    SetNamedInt32(env, result, "userUpper", relation.userUpper);
    SetNamedBool(env, result, "hasOverlap", relation.hasOverlap);
    SetNamedInt32(env, result, "overlapWidth", relation.overlapWidth);
    SetNamedInt32(env, result, "mergedWidth", relation.mergedWidth);
    SetNamedInt32(env, result, "gap", relation.gap);
    SetNamedBool(env, result, "userInsideFixed", relation.userInsideFixed);
    SetNamedString(env, result, "relation", GetRelationLabel(relation));
    return result;
}

static napi_value RunIntervalCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_INTERVAL_ARG_COUNT;
    napi_value args[K_INTERVAL_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_INTERVAL_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid interval case");
        return nullptr;
    }
    if (argc < K_INTERVAL_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "two numbers are required");
        return nullptr;
    }

    int32_t first = 0;
    int32_t second = 0;
    if (!ReadInt32(env, args[K_FIRST_ARG_INDEX], "first must be a number", &first)) {
        return nullptr;
    }
    if (!ReadInt32(env, args[K_SECOND_ARG_INDEX], "second must be a number", &second)) {
        return nullptr;
    }

    const auto spec = GetIntervalCaseSpec(caseIndex);
    IntervalRelation relation = {};
    ComputeIntervalRelation(spec, first, second, &relation);
    return CreateIntervalResultObject(env, spec, relation);
}

}  // namespace

static napi_value InitIntervalSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_INTERVAL_CASE_COUNT);
    exportNames.reserve(K_INTERVAL_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_INTERVAL_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(
            BuildIndexedName("testIntervalCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{
            exportNames.back().c_str(),
            nullptr,
            RunIntervalCase,
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

static napi_module g_intervalSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitIntervalSuite,
    .nm_modname = "interval_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterIntervalSuiteModule(void)
{
    napi_module_register(&g_intervalSuiteModule);
}
