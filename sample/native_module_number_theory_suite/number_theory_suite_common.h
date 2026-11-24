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

#ifndef SAMPLE_NATIVE_MODULE_NUMBER_THEORY_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_NUMBER_THEORY_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

constexpr int64_t NT_MIN_INPUT = 1;
constexpr int64_t NT_MAX_INPUT = 1000000000LL;
constexpr int64_t NT_BASE_MIN = 2;
constexpr int64_t NT_BASE_MAX = 36;
constexpr size_t DIGIT_COUNT_LIMIT = 16;
constexpr size_t EMPTY_RESULT_SIZE = 0;
constexpr size_t INDEX_ZERO = 0;
constexpr size_t INDEX_ONE = 1;
constexpr uint32_t UINT32_INDEX_ZERO = 0;
constexpr uint32_t UINT32_ONE = 1;
constexpr int64_t DECIMAL_BASE = 10;
constexpr int64_t BINARY_SHIFT_BASE = 2;
constexpr int64_t KAPRECAR_SPLIT_POWER = 2;
constexpr int64_t HAPPY_SQUARE_LIMIT = 100;
constexpr int64_t SMITH_DIGIT_MIN = 2;
constexpr int64_t PELL_MAX_N = 100000;
constexpr int64_t LUCKY_WINDOW_STRIDE = 2;
constexpr int64_t NOT_FOUND_SENTINEL = -1;

bool ExtractInt64Arg(napi_env env, napi_value value, int64_t& result);
bool ExtractBoundedIntArg(napi_env env, napi_value value, int64_t low, int64_t high, int64_t& result);
bool ExtractShortTextArg(napi_env env, napi_value value, std::string& result);
napi_value CreateInt64Value(napi_env env, int64_t value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateStringValue(napi_env env, const std::string& value);
napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values);
std::vector<int64_t> DigitsOfInBase(int64_t value, int64_t base);
int64_t DigitSumValue(int64_t value);
int64_t IntegerRootOf(int64_t value);
std::vector<std::pair<int64_t, uint32_t>> PrimeFactorizationOf(int64_t value);
int64_t SumOfDivisorsValue(int64_t value);

napi_value IsAutomorphicNumber(napi_env env, napi_callback_info info);
napi_value IsDudeneyNumber(napi_env env, napi_callback_info info);
napi_value IsKaprekarNumber(napi_env env, napi_callback_info info);
napi_value IsSmithNumber(napi_env env, napi_callback_info info);
napi_value IsHappyNumberInBase(napi_env env, napi_callback_info info);
napi_value SumOfProperDivisors(napi_env env, napi_callback_info info);
napi_value AliquotChainLength(napi_env env, napi_callback_info info);
napi_value NextPerfectSquareFree(napi_env env, napi_callback_info info);
napi_status RegisterNumberTheoryFunctions(napi_env env, napi_value exports);

napi_value FactorialTrailingZeros(napi_env env, napi_callback_info info);
napi_value InBaseString(napi_env env, napi_callback_info info);
napi_value PellEquationMinSolution(napi_env env, napi_callback_info info);
napi_value DigitalRootInBase(napi_env env, napi_callback_info info);
napi_value IsNarcissisticNumber(napi_env env, napi_callback_info info);
napi_value DivisorCountOf(napi_env env, napi_callback_info info);
napi_value EulerTotientUnder(napi_env env, napi_callback_info info);
napi_value SumOfDivisorsOf(napi_env env, napi_callback_info info);
napi_status RegisterExtraNumberTheoryFunctions(napi_env env, napi_value exports);

napi_value TriangularRootIsInteger(napi_env env, napi_callback_info info);
napi_value SquareTriangularCheck(napi_env env, napi_callback_info info);
napi_value PentagonalRootValue(napi_env env, napi_callback_info info);
napi_value HexagonalNumberAt(napi_env env, napi_callback_info info);
napi_value IsCubeNumber(napi_env env, napi_callback_info info);
napi_value FourthPowerRoot(napi_env env, napi_callback_info info);
napi_value NearestSquareOf(napi_env env, napi_callback_info info);
napi_value NearestCubeOf(napi_env env, napi_callback_info info);

#endif  // SAMPLE_NATIVE_MODULE_NUMBER_THEORY_SUITE_COMMON_H
