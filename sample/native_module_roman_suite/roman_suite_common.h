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

#ifndef SAMPLE_NATIVE_MODULE_ROMAN_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_ROMAN_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

// Roman numerals of this suite range from I (1) to MMMCMXCIX (3999).
constexpr int32_t ROMAN_MIN_VALUE = 1;
constexpr int32_t ROMAN_MAX_VALUE = 3999;
constexpr int32_t ROMAN_INVALID_VALUE = -1;
constexpr int32_t ROMAN_ZERO = 0;
constexpr int ROMAN_MODULE_VERSION = 1;

constexpr int32_t COMPARE_LESS = -1;
constexpr int32_t COMPARE_EQUAL = 0;
constexpr int32_t COMPARE_GREATER = 1;

// Seven basic symbols I, V, X, L, C, D, M.
constexpr size_t ROMAN_SYMBOL_COUNT = 7;
// Seven basic symbols plus six subtractive pairs (CM, CD, XC, XL, IX, IV).
constexpr size_t ROMAN_PAIR_COUNT = 13;
constexpr size_t ROMAN_SINGLE_SYMBOL_LENGTH = 1;
constexpr size_t NEXT_SYMBOL_OFFSET = 1;
constexpr size_t PARSE_STEP_SINGLE = 1;
constexpr size_t PARSE_STEP_PAIR = 2;
constexpr size_t STRING_NULL_TERMINATOR = 1;

const char* const ROMAN_SYMBOL_CHARS = "IVXLCDM";
constexpr int32_t ROMAN_SYMBOL_VALUES[ROMAN_SYMBOL_COUNT] = { 1, 5, 10, 50, 100, 500, 1000 };
const char* const ROMAN_PAIR_SYMBOLS[ROMAN_PAIR_COUNT] = {
    "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I",
};
constexpr int32_t ROMAN_PAIR_VALUES[ROMAN_PAIR_COUNT] = {
    1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1,
};

int32_t RomanCharToValue(char symbol, bool& valid);
void ParseRomanString(const std::string& text, bool& valid, int32_t& value);
void ParseRomanInRange(const std::string& text, bool& valid, int32_t& value);
std::string EmitRomanString(int32_t value);
napi_status ExtractInt32Arg(napi_env env, napi_value value, int32_t& result);
napi_status ExtractStringArg(napi_env env, napi_value value, std::string& result);
napi_status ExtractRomanArg(napi_env env, napi_value value, int32_t& result);
napi_status CreateStringValue(napi_env env, const std::string& text, napi_value& result);
napi_status CreateRomanResult(napi_env env, int32_t value, napi_value& result);

napi_value ToRoman(napi_env env, napi_callback_info info);
napi_value FromRoman(napi_env env, napi_callback_info info);
napi_value IsRoman(napi_env env, napi_callback_info info);
napi_value RomanCharValue(napi_env env, napi_callback_info info);
napi_value RomanMinValue(napi_env env, napi_callback_info info);
napi_value RomanMaxValue(napi_env env, napi_callback_info info);
napi_value RomanSymbols(napi_env env, napi_callback_info info);

napi_value RomanAdd(napi_env env, napi_callback_info info);
napi_value RomanSubtract(napi_env env, napi_callback_info info);
napi_value RomanMultiply(napi_env env, napi_callback_info info);
napi_value RomanDivide(napi_env env, napi_callback_info info);
napi_value RomanMod(napi_env env, napi_callback_info info);
napi_value RomanCompare(napi_env env, napi_callback_info info);
napi_value RomanMin(napi_env env, napi_callback_info info);
napi_value RomanMax(napi_env env, napi_callback_info info);
napi_value RomanNormalize(napi_env env, napi_callback_info info);
napi_value RomanSumList(napi_env env, napi_callback_info info);

napi_status RegisterRomanOpsFunctions(napi_env env, napi_value exports);

#endif  // SAMPLE_NATIVE_MODULE_ROMAN_SUITE_COMMON_H
