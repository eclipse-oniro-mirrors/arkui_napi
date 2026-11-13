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

#ifndef SAMPLE_NATIVE_MODULE_SORT_SEARCH_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_SORT_SEARCH_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

constexpr int64_t INT64_MIN_INPUT = -1000000000LL;
constexpr int64_t INT64_MAX_INPUT = 1000000000LL;
constexpr size_t MAX_ARRAY_LENGTH = 1024;
constexpr size_t MIN_ARRAY_LENGTH = 1;
constexpr size_t COUNTING_SORT_SPAN_LIMIT = 65536;
constexpr size_t SPAN_EXTRA_SLOTS = 1;
constexpr size_t HALF_DIVISOR = 2;
constexpr size_t EMPTY_ARRAY_SIZE = 0;
constexpr size_t MEDIAN_OF_THREE_REQUIRED_SIZE = 3;
constexpr size_t MEDIAN_INDEX = 1;
constexpr uint32_t UINT32_INDEX_ZERO = 0;

bool ExtractInt64Arg(napi_env env, napi_value value, int64_t& result);
bool ExtractInt64Array(napi_env env, napi_value value, std::vector<int64_t>& result);
napi_value CreateInt64Value(napi_env env, int64_t value);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values);
std::vector<int64_t> SortedCopy(const std::vector<int64_t>& values);
bool IsSortedAscendingValue(const std::vector<int64_t>& values);
int64_t MedianOfThreeValue(std::vector<int64_t>& values, size_t low, size_t high);
size_t PartitionMedianPivot(std::vector<int64_t>& values, size_t low, size_t high);

napi_value BubbleSortAsc(napi_env env, napi_callback_info info);
napi_value InsertionSortDesc(napi_env env, napi_callback_info info);
napi_value SelectionSortAsc(napi_env env, napi_callback_info info);
napi_value QuickSortMedianAsc(napi_env env, napi_callback_info info);
napi_value CountingSortRange(napi_env env, napi_callback_info info);
napi_value SortAscending(napi_env env, napi_callback_info info);
napi_value SortDescending(napi_env env, napi_callback_info info);
napi_value IsSortedAscending(napi_env env, napi_callback_info info);
napi_value MergeTwoSorted(napi_env env, napi_callback_info info);
napi_value NthLargestElement(napi_env env, napi_callback_info info);

napi_value BinarySearchLowerBound(napi_env env, napi_callback_info info);
napi_value BinarySearchUpperBound(napi_env env, napi_callback_info info);
napi_value LinearSearchFrom(napi_env env, napi_callback_info info);
napi_value ChunkArray(napi_env env, napi_callback_info info);
napi_value SplitIntoParts(napi_env env, napi_callback_info info);
napi_value MovingAverageWindow(napi_env env, napi_callback_info info);
napi_value MedianOfThreeValues(napi_env env, napi_callback_info info);
napi_value NewtonsSqrt(napi_env env, napi_callback_info info);
napi_value IntegerSqrt(napi_env env, napi_callback_info info);
napi_value PerfectSquareCheck(napi_env env, napi_callback_info info);
napi_status RegisterSortSearchOpsFunctions(napi_env env, napi_value exports);

#endif  // SAMPLE_NATIVE_MODULE_SORT_SEARCH_SUITE_COMMON_H
