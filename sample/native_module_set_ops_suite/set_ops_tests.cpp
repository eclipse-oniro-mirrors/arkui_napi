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
constexpr size_t K_SET_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Set Configuration ---
constexpr size_t K_SET_OP_CYCLE = 3;
constexpr size_t K_SET_MAX = 8;
constexpr size_t K_SET_MAX_RESULT = K_SET_MAX * 2;
constexpr size_t K_SET_VALUE_RANGE = 10;
constexpr size_t K_SET_OFFSET = 7;
constexpr size_t K_SET_ZERO = 0;
constexpr size_t K_SET_OP_INDEX_UNION = 0;
constexpr size_t K_SET_OP_INDEX_INTERSECT = 1;
constexpr size_t K_SET_OP_INDEX_DIFFERENCE = 2;

// --- Test Values (Integers) ---
constexpr int32_t K_INT_VALUE_ZERO = 0;

// --- Op Labels ---
const char* const K_SET_OP_LABEL_UNION = "union";
const char* const K_SET_OP_LABEL_INTERSECT = "intersect";
const char* const K_SET_OP_LABEL_DIFFERENCE = "difference";

// --- Op Descriptions ---
const char* const K_SET_OP_DESC_UNION = "union of setA and setB with duplicates removed";
const char* const K_SET_OP_DESC_INTERSECT = "intersection of setA and setB preserving setA order";
const char* const K_SET_OP_DESC_DIFFERENCE = "elements in setA but not in setB preserving order";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class SetOp {
    UNION,
    INTERSECT,
    DIFFERENCE,
};

struct SetCaseSpec {
    std::string name;
    SetOp op;
    size_t sizeA;
    size_t sizeB;
    int32_t setA[K_SET_MAX];
    int32_t setB[K_SET_MAX];
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

void FillSetA(SetCaseSpec& spec, size_t caseNumber)
{
    for (size_t i = 0; i < spec.sizeA; i++) {
        spec.setA[i] = static_cast<int32_t>((caseNumber + i) % K_SET_VALUE_RANGE);
    }
}

void FillSetB(SetCaseSpec& spec, size_t caseNumber)
{
    for (size_t i = 0; i < spec.sizeB; i++) {
        spec.setB[i] = static_cast<int32_t>((caseNumber * K_SET_OFFSET + i) % K_SET_VALUE_RANGE);
    }
}

SetCaseSpec GetSetCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    SetCaseSpec spec;
    spec.name = BuildIndexedName("setOpsCase", caseNumber);
    spec.sizeA = caseNumber % K_SET_MAX + 1;
    spec.sizeB = caseNumber % K_SET_MAX + 1;
    const size_t opIndex = caseNumber % K_SET_OP_CYCLE;
    if (opIndex == K_SET_OP_INDEX_UNION) {
        spec.op = SetOp::UNION;
    } else if (opIndex == K_SET_OP_INDEX_INTERSECT) {
        spec.op = SetOp::INTERSECT;
    } else {
        spec.op = SetOp::DIFFERENCE;
    }
    FillSetA(spec, caseNumber);
    FillSetB(spec, caseNumber);
    return spec;
}

const char* OpLabel(SetOp op)
{
    if (op == SetOp::UNION) {
        return K_SET_OP_LABEL_UNION;
    }
    if (op == SetOp::INTERSECT) {
        return K_SET_OP_LABEL_INTERSECT;
    }
    return K_SET_OP_LABEL_DIFFERENCE;
}

const char* OpDescription(SetOp op)
{
    if (op == SetOp::UNION) {
        return K_SET_OP_DESC_UNION;
    }
    if (op == SetOp::INTERSECT) {
        return K_SET_OP_DESC_INTERSECT;
    }
    return K_SET_OP_DESC_DIFFERENCE;
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

// === Helpers: Set Membership & Operations ===

bool SetContains(const int32_t* arr, size_t n, int32_t value)
{
    for (size_t i = 0; i < n; i++) {
        if (arr[i] == value) {
            return true;
        }
    }
    return false;
}

size_t SetUnion(const int32_t* a, size_t na, const int32_t* b, size_t nb, int32_t* out)
{
    size_t count = K_SET_ZERO;
    for (size_t i = 0; i < na; i++) {
        if (!SetContains(out, count, a[i])) {
            out[count] = a[i];
            count++;
        }
    }
    for (size_t i = 0; i < nb; i++) {
        if (!SetContains(out, count, b[i])) {
            out[count] = b[i];
            count++;
        }
    }
    return count;
}

size_t SetIntersect(const int32_t* a, size_t na, const int32_t* b, size_t nb, int32_t* out)
{
    size_t count = K_SET_ZERO;
    for (size_t i = 0; i < na; i++) {
        if (SetContains(b, nb, a[i]) && !SetContains(out, count, a[i])) {
            out[count] = a[i];
            count++;
        }
    }
    return count;
}

size_t SetDifference(const int32_t* a, size_t na, const int32_t* b, size_t nb, int32_t* out)
{
    size_t count = K_SET_ZERO;
    for (size_t i = 0; i < na; i++) {
        if (!SetContains(b, nb, a[i]) && !SetContains(out, count, a[i])) {
            out[count] = a[i];
            count++;
        }
    }
    return count;
}

// === Helpers: Result Statistics ===

int32_t ComputeInt32Sum(const int32_t* values, size_t count)
{
    int32_t sum = K_INT_VALUE_ZERO;
    for (size_t i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum;
}

int32_t ComputeInt32Min(const int32_t* values, size_t count)
{
    if (count == K_SET_ZERO) {
        return K_INT_VALUE_ZERO;
    }
    int32_t minVal = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] < minVal) {
            minVal = values[i];
        }
    }
    return minVal;
}

int32_t ComputeInt32Max(const int32_t* values, size_t count)
{
    if (count == K_SET_ZERO) {
        return K_INT_VALUE_ZERO;
    }
    int32_t maxVal = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] > maxVal) {
            maxVal = values[i];
        }
    }
    return maxVal;
}

// === Helpers: Distinct Count & Result Bounds ===

size_t CountDistinct(const int32_t* arr, size_t n)
{
    size_t count = K_SET_ZERO;
    for (size_t i = 0; i < n; i++) {
        if (!SetContains(arr, i, arr[i])) {
            count++;
        }
    }
    return count;
}

size_t ExpectedResultUpperBound(SetOp op, size_t sizeA, size_t sizeB)
{
    if (op == SetOp::UNION) {
        return sizeA + sizeB;
    }
    if (op == SetOp::INTERSECT) {
        return (sizeA < sizeB) ? sizeA : sizeB;
    }
    return sizeA;
}

// === Helpers: Validation & Result Dispatch ===

bool ValidateSpec(const SetCaseSpec& spec)
{
    if (spec.sizeA > K_SET_MAX) {
        return false;
    }
    if (spec.sizeB > K_SET_MAX) {
        return false;
    }
    return true;
}

size_t ComputeSetResult(const SetCaseSpec& spec, int32_t* out)
{
    if (spec.op == SetOp::UNION) {
        return SetUnion(spec.setA, spec.sizeA, spec.setB, spec.sizeB, out);
    }
    if (spec.op == SetOp::INTERSECT) {
        return SetIntersect(spec.setA, spec.sizeA, spec.setB, spec.sizeB, out);
    }
    return SetDifference(spec.setA, spec.sizeA, spec.setB, spec.sizeB, out);
}

// === Helpers: Array Building ===

napi_value BuildInt32Array(napi_env env, const int32_t* values, size_t count)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &array));
    for (size_t i = 0; i < count; i++) {
        napi_value elem = nullptr;
        NAPI_CALL(env, napi_create_int32(env, values[i], &elem));
        NAPI_CALL(env, napi_set_element(env, array, static_cast<uint32_t>(i), elem));
    }
    return array;
}

// === Result Building ===

napi_value BuildSetResult(napi_env env, const SetCaseSpec& spec, const int32_t* result, size_t resultSize,
    int32_t resultSum, int32_t resultMin, int32_t resultMax, size_t distinctA, size_t distinctB,
    size_t expectedMax, bool boundsValid)
{
    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));
    SetNamedString(env, obj, "name", spec.name);
    SetNamedString(env, obj, "op", OpLabel(spec.op));
    SetNamedString(env, obj, "description", OpDescription(spec.op));
    SetNamedInt32(env, obj, "sizeA", static_cast<int32_t>(spec.sizeA));
    SetNamedInt32(env, obj, "sizeB", static_cast<int32_t>(spec.sizeB));
    SetNamedInt32(env, obj, "resultSize", static_cast<int32_t>(resultSize));
    SetNamedInt32(env, obj, "distinctA", static_cast<int32_t>(distinctA));
    SetNamedInt32(env, obj, "distinctB", static_cast<int32_t>(distinctB));
    SetNamedInt32(env, obj, "expectedMax", static_cast<int32_t>(expectedMax));
    SetNamedInt32(env, obj, "resultSum", resultSum);
    SetNamedInt32(env, obj, "resultMin", resultMin);
    SetNamedInt32(env, obj, "resultMax", resultMax);
    SetNamedBool(env, obj, "boundsValid", boundsValid);
    napi_value setAArray = BuildInt32Array(env, spec.setA, spec.sizeA);
    NAPI_CALL(env, napi_set_named_property(env, obj, "setA", setAArray));
    napi_value setBArray = BuildInt32Array(env, spec.setB, spec.sizeB);
    NAPI_CALL(env, napi_set_named_property(env, obj, "setB", setBArray));
    napi_value resultArray = BuildInt32Array(env, result, resultSize);
    NAPI_CALL(env, napi_set_named_property(env, obj, "resultArray", resultArray));
    return obj;
}

// === Test Entry ===

static napi_value RunSetCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_SET_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid set ops case");
        return nullptr;
    }
    const auto spec = GetSetCaseSpec(caseIndex);
    if (!ValidateSpec(spec)) {
        napi_throw_error(env, nullptr, "invalid set ops spec");
        return nullptr;
    }
    int32_t result[K_SET_MAX_RESULT] = {K_INT_VALUE_ZERO};
    const size_t resultSize = ComputeSetResult(spec, result);
    const int32_t resultSum = ComputeInt32Sum(result, resultSize);
    const int32_t resultMin = ComputeInt32Min(result, resultSize);
    const int32_t resultMax = ComputeInt32Max(result, resultSize);
    const size_t distinctA = CountDistinct(spec.setA, spec.sizeA);
    const size_t distinctB = CountDistinct(spec.setB, spec.sizeB);
    const size_t expectedMax = ExpectedResultUpperBound(spec.op, spec.sizeA, spec.sizeB);
    const bool boundsValid = (resultSize <= expectedMax);
    return BuildSetResult(env, spec, result, resultSize, resultSum, resultMin, resultMax, distinctA,
        distinctB, expectedMax, boundsValid);
}

}  // namespace

static napi_value InitSetOpsSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_SET_CASE_COUNT);
    exportNames.reserve(K_SET_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_SET_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testSetOpsCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunSetCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_setOpsSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitSetOpsSuite,
    .nm_modname = "set_ops_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterSetOpsSuiteModule(void)
{
    napi_module_register(&g_setOpsSuiteModule);
}
