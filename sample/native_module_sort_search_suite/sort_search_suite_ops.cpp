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

static const double SQRT_INITIAL_GUESS = 2.0;
static const double SQRT_CONVERGENCE_EPSILON = 1e-12;
static const int SQRT_MAX_ITERATIONS = 64;

static bool IsInRange(int64_t value)
{
    return value >= INT64_MIN_INPUT && value <= INT64_MAX_INPUT;
}

static int64_t LowerBoundIndex(const std::vector<int64_t>& values, int64_t target)
{
    size_t low = 0;
    size_t high = values.size();
    while (low < high) {
        size_t middle = low + (high - low) / HALF_DIVISOR;
        if (values[middle] < target) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return static_cast<int64_t>(low);
}

static int64_t UpperBoundIndex(const std::vector<int64_t>& values, int64_t target)
{
    size_t low = 0;
    size_t high = values.size();
    while (low < high) {
        size_t middle = low + (high - low) / HALF_DIVISOR;
        if (values[middle] <= target) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return static_cast<int64_t>(low);
}

napi_value BinarySearchLowerBound(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, target");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }
    NAPI_ASSERT(env, IsSortedAscendingValue(values), "Values must be sorted ascending");

    int64_t target = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], target)) {
        return nullptr;
    }

    return CreateInt64Value(env, LowerBoundIndex(values, target));
}

napi_value BinarySearchUpperBound(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, target");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }
    NAPI_ASSERT(env, IsSortedAscendingValue(values), "Values must be sorted ascending");

    int64_t target = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], target)) {
        return nullptr;
    }

    return CreateInt64Value(env, UpperBoundIndex(values, target));
}

napi_value LinearSearchFrom(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, target");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t target = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], target)) {
        return nullptr;
    }

    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] == target) {
            return CreateInt64Value(env, static_cast<int64_t>(i));
        }
    }
    return CreateInt64Value(env, -1);
}

static napi_value CreateChunkArrays(napi_env env, const std::vector<std::vector<int64_t>>& chunks)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, chunks.size(), &result));
    for (size_t i = 0; i < chunks.size(); ++i) {
        napi_value chunk = CreateInt64ArrayValue(env, chunks[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), chunk));
    }
    return result;
}

napi_value ChunkArray(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, size");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t chunkSize = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], chunkSize)) {
        return nullptr;
    }
    NAPI_ASSERT(env, chunkSize >= static_cast<int64_t>(MIN_ARRAY_LENGTH), "Chunk size must be positive");

    std::vector<std::vector<int64_t>> chunks;
    for (size_t start = 0; start < values.size(); start += static_cast<size_t>(chunkSize)) {
        size_t end = std::min(start + static_cast<size_t>(chunkSize), values.size());
        chunks.push_back(std::vector<int64_t>(values.begin() + static_cast<ptrdiff_t>(start),
                                               values.begin() + static_cast<ptrdiff_t>(end)));
    }
    return CreateChunkArrays(env, chunks);
}

napi_value SplitIntoParts(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, parts");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t parts = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], parts)) {
        return nullptr;
    }
    NAPI_ASSERT(env, parts >= static_cast<int64_t>(MIN_ARRAY_LENGTH) &&
                         parts <= static_cast<int64_t>(values.size()),
                "Parts out of range");

    std::vector<std::vector<int64_t>> chunks;
    size_t baseSize = values.size() / static_cast<size_t>(parts);
    size_t remainder = values.size() % static_cast<size_t>(parts);
    size_t start = 0;
    for (int64_t p = 0; p < parts; ++p) {
        size_t extra = static_cast<size_t>(p) < remainder ? 1 : 0;
        size_t end = start + baseSize + extra;
        chunks.push_back(std::vector<int64_t>(values.begin() + static_cast<ptrdiff_t>(start),
                                              values.begin() + static_cast<ptrdiff_t>(end)));
        start = end;
    }
    return CreateChunkArrays(env, chunks);
}

napi_value MovingAverageWindow(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, window");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t window = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], window)) {
        return nullptr;
    }
    NAPI_ASSERT(env, window >= static_cast<int64_t>(MIN_ARRAY_LENGTH) &&
                         window <= static_cast<int64_t>(values.size()),
                "Window out of range");

    std::vector<double> averages;
    size_t width = static_cast<size_t>(window);
    double runningTotal = 0.0;
    for (size_t i = 0; i < values.size(); ++i) {
        runningTotal += static_cast<double>(values[i]);
        if (i >= width) {
            runningTotal -= static_cast<double>(values[i - width]);
        }
        if (i + 1 >= width) {
            averages.push_back(runningTotal / static_cast<double>(width));
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, averages.size(), &result));
    for (size_t i = 0; i < averages.size(); ++i) {
        napi_value element = CreateDoubleValue(env, averages[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

napi_value MedianOfThreeValues(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }
    NAPI_ASSERT(env, values.size() == MEDIAN_OF_THREE_REQUIRED_SIZE, "Requires exactly three values");

    std::vector<int64_t> sorted = SortedCopy(values);
    return CreateInt64Value(env, sorted[MEDIAN_INDEX]);
}

static double NewtonSqrtStep(double value, double guess)
{
    if (guess == 0.0) {
        return 0.0;
    }
    return (guess + value / guess) / SQRT_INITIAL_GUESS;
}

napi_value NewtonsSqrt(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: value");

    double number = 0.0;
    NAPI_CALL(env, napi_get_value_double(env, argv[ARG_INDEX_ZERO], &number));
    NAPI_ASSERT(env, std::isfinite(number) && number >= 0.0, "Value must be a non-negative finite number");

    if (number == 0.0) {
        return CreateDoubleValue(env, 0.0);
    }

    double guess = number;
    for (int iteration = 0; iteration < SQRT_MAX_ITERATIONS; ++iteration) {
        double next = NewtonSqrtStep(number, guess);
        if (std::fabs(next - guess) < SQRT_CONVERGENCE_EPSILON) {
            guess = next;
            break;
        }
        guess = next;
    }
    return CreateDoubleValue(env, guess);
}

static int64_t IntegerSqrtValue(int64_t value)
{
    if (value <= 0) {
        return 0;
    }
    int64_t low = 0;
    int64_t high = value;
    while (low < high) {
        int64_t middle = low + (high - low + 1) / static_cast<int64_t>(HALF_DIVISOR);
        if (middle > value / middle) {
            high = middle - 1;
        } else {
            low = middle;
        }
    }
    return low;
}

napi_value IntegerSqrt(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: value");

    int64_t value = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], value)) {
        return nullptr;
    }
    NAPI_ASSERT(env, IsInRange(value) && value >= 0, "Value must be in [0, 1000000000]");

    return CreateInt64Value(env, IntegerSqrtValue(value));
}

napi_value PerfectSquareCheck(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: value");

    int64_t value = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], value)) {
        return nullptr;
    }
    NAPI_ASSERT(env, IsInRange(value) && value >= 0, "Value must be in [0, 1000000000]");

    int64_t root = IntegerSqrtValue(value);
    bool result = root * root == value;
    return CreateBoolValue(env, result);
}

napi_value CountingSortRange(napi_env env, napi_callback_info info)
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

    int64_t minimum = values[0];
    int64_t maximum = values[0];
    for (size_t i = 1; i < values.size(); ++i) {
        minimum = std::min(minimum, values[i]);
        maximum = std::max(maximum, values[i]);
    }

    int64_t span = maximum - minimum + static_cast<int64_t>(SPAN_EXTRA_SLOTS);
    NAPI_ASSERT(env, static_cast<uint64_t>(span) <= COUNTING_SORT_SPAN_LIMIT, "Value span too large");

    std::vector<uint32_t> counts(static_cast<size_t>(span), UINT32_INDEX_ZERO);
    for (size_t i = 0; i < values.size(); ++i) {
        ++counts[static_cast<size_t>(values[i] - minimum)];
    }

    std::vector<int64_t> sorted;
    sorted.reserve(values.size());
    for (size_t slot = 0; slot < counts.size(); ++slot) {
        for (uint32_t k = 0; k < counts[slot]; ++k) {
            sorted.push_back(minimum + static_cast<int64_t>(slot));
        }
    }
    return CreateInt64ArrayValue(env, sorted);
}

napi_status RegisterSortSearchOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("binarySearchLowerBound", BinarySearchLowerBound),
        DECLARE_NAPI_FUNCTION("binarySearchUpperBound", BinarySearchUpperBound),
        DECLARE_NAPI_FUNCTION("linearSearchFrom", LinearSearchFrom),
        DECLARE_NAPI_FUNCTION("chunkArray", ChunkArray),
        DECLARE_NAPI_FUNCTION("splitIntoParts", SplitIntoParts),
        DECLARE_NAPI_FUNCTION("movingAverageWindow", MovingAverageWindow),
        DECLARE_NAPI_FUNCTION("medianOfThreeValues", MedianOfThreeValues),
        DECLARE_NAPI_FUNCTION("newtonsSqrt", NewtonsSqrt),
        DECLARE_NAPI_FUNCTION("integerSqrt", IntegerSqrt),
        DECLARE_NAPI_FUNCTION("perfectSquareCheck", PerfectSquareCheck),
        DECLARE_NAPI_FUNCTION("countingSortRange", CountingSortRange),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
