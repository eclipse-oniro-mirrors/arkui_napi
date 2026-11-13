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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "napi/native_node_api.h"

#include "sort_search_suite_common.h"

bool ExtractInt64Arg(napi_env env, napi_value value, int64_t& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    double number = 0.0;
    NAPI_CALL_BASE(env, napi_get_value_double(env, value, &number), false);
    if (!std::isfinite(number) || std::floor(number) != number) {
        napi_throw_error(env, nullptr, "Argument must be a finite integer");
        return false;
    }

    result = static_cast<int64_t>(number);
    return true;
}

bool ExtractInt64Array(napi_env env, napi_value value, std::vector<int64_t>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > MAX_ARRAY_LENGTH) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        int64_t number = 0;
        if (!ExtractInt64Arg(env, element, number)) {
            return false;
        }
        result.push_back(number);
    }
    return true;
}

napi_value CreateInt64Value(napi_env env, int64_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int64(env, value, &result));
    return result;
}

napi_value CreateDoubleValue(napi_env env, double value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_double(env, value, &result));
    return result;
}

napi_value CreateBoolValue(napi_env env, bool value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result));
    return result;
}

napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateInt64Value(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

std::vector<int64_t> SortedCopy(const std::vector<int64_t>& values)
{
    std::vector<int64_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

bool IsSortedAscendingValue(const std::vector<int64_t>& values)
{
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i] < values[i - 1]) {
            return false;
        }
    }
    return true;
}

static void BubbleSortPass(std::vector<int64_t>& values)
{
    for (size_t i = 0; i + 1 < values.size(); ++i) {
        if (values[i + 1] < values[i]) {
            std::swap(values[i], values[i + 1]);
        }
    }
}

napi_value BubbleSortAsc(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    for (size_t pass = 0; pass < values.size(); ++pass) {
        BubbleSortPass(values);
    }
    return CreateInt64ArrayValue(env, values);
}

static void InsertionSortStep(std::vector<int64_t>& values, size_t position)
{
    int64_t key = values[position];
    size_t cursor = position;
    while (cursor > 0 && values[cursor - 1] < key) {
        values[cursor] = values[cursor - 1];
        --cursor;
    }
    values[cursor] = key;
}

napi_value InsertionSortDesc(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    for (size_t i = 1; i < values.size(); ++i) {
        InsertionSortStep(values, i);
    }
    return CreateInt64ArrayValue(env, values);
}

static size_t SmallestIndexFrom(std::vector<int64_t>& values, size_t start)
{
    size_t smallest = start;
    for (size_t i = start + 1; i < values.size(); ++i) {
        if (values[i] < values[smallest]) {
            smallest = i;
        }
    }
    return smallest;
}

napi_value SelectionSortAsc(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    for (size_t i = 0; i + 1 < values.size(); ++i) {
        size_t smallest = SmallestIndexFrom(values, i);
        std::swap(values[i], values[smallest]);
    }
    return CreateInt64ArrayValue(env, values);
}

int64_t MedianOfThreeValue(std::vector<int64_t>& values, size_t low, size_t high)
{
    size_t middle = low + (high - low) / HALF_DIVISOR;
    if (values[middle] < values[low]) {
        std::swap(values[low], values[middle]);
    }
    if (values[high] < values[low]) {
        std::swap(values[low], values[high]);
    }
    if (values[high] < values[middle]) {
        std::swap(values[middle], values[high]);
    }
    return values[middle];
}

size_t PartitionMedianPivot(std::vector<int64_t>& values, size_t low, size_t high)
{
    int64_t pivot = MedianOfThreeValue(values, low, high);
    size_t less = low;
    size_t equal = low;
    size_t greater = high + 1;
    while (equal < greater) {
        if (values[equal] < pivot) {
            std::swap(values[less], values[equal]);
            ++less;
            ++equal;
        } else if (values[equal] == pivot) {
            ++equal;
        } else {
            --greater;
            std::swap(values[equal], values[greater]);
        }
    }
    return less;
}

static void QuickSortRange(std::vector<int64_t>& values, size_t low, size_t high)
{
    if (low >= high) {
        return;
    }

    size_t lessEnd = PartitionMedianPivot(values, low, high);
    size_t greaterStart = lessEnd;
    while (greaterStart <= high && values[greaterStart] == values[lessEnd]) {
        ++greaterStart;
    }

    if (lessEnd > low) {
        QuickSortRange(values, low, lessEnd - 1);
    }
    if (greaterStart <= high) {
        QuickSortRange(values, greaterStart, high);
    }
}

napi_value QuickSortMedianAsc(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !values.empty(), "Array must not be empty");

    QuickSortRange(values, 0, values.size() - 1);
    return CreateInt64ArrayValue(env, values);
}

napi_value SortAscending(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    return CreateInt64ArrayValue(env, SortedCopy(values));
}

napi_value SortDescending(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    std::vector<int64_t> sorted = SortedCopy(values);
    std::reverse(sorted.begin(), sorted.end());
    return CreateInt64ArrayValue(env, sorted);
}

napi_value IsSortedAscending(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    return CreateBoolValue(env, IsSortedAscendingValue(values));
}

napi_value MergeTwoSorted(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<int64_t> left;
    std::vector<int64_t> right;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], left)) {
        return nullptr;
    }
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ONE], right)) {
        return nullptr;
    }
    NAPI_ASSERT(env, IsSortedAscendingValue(left) && IsSortedAscendingValue(right), "Inputs must be ascending");

    std::vector<int64_t> merged;
    merged.reserve(left.size() + right.size());
    std::merge(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(merged));
    return CreateInt64ArrayValue(env, merged);
}

napi_value NthLargestElement(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, n");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t rank = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], rank)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rank >= INT64_MIN_INPUT && rank <= INT64_MAX_INPUT, "Rank out of range");
    NAPI_ASSERT(env, rank >= static_cast<int64_t>(MIN_ARRAY_LENGTH) &&
                         rank <= static_cast<int64_t>(values.size()),
                "Rank exceeds array length");

    std::vector<int64_t> sorted = SortedCopy(values);
    size_t position = sorted.size() - static_cast<size_t>(rank);
    return CreateInt64Value(env, sorted[position]);
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("bubbleSortAsc", BubbleSortAsc),
        DECLARE_NAPI_FUNCTION("insertionSortDesc", InsertionSortDesc),
        DECLARE_NAPI_FUNCTION("selectionSortAsc", SelectionSortAsc),
        DECLARE_NAPI_FUNCTION("quickSortMedianAsc", QuickSortMedianAsc),
        DECLARE_NAPI_FUNCTION("sortAscending", SortAscending),
        DECLARE_NAPI_FUNCTION("sortDescending", SortDescending),
        DECLARE_NAPI_FUNCTION("isSortedAscending", IsSortedAscending),
        DECLARE_NAPI_FUNCTION("mergeTwoSorted", MergeTwoSorted),
        DECLARE_NAPI_FUNCTION("nthLargestElement", NthLargestElement),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterSortSearchOpsFunctions(env, exports), nullptr);
    return exports;
}

static napi_module sortSearchSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "sortSearchSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void SortSearchSuiteRegisterModule(void)
{
    napi_module_register(&sortSearchSuiteModule);
}
