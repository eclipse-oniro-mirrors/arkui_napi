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

#include <cstddef>
#include <cstdint>
#include <string>

#include "napi/native_node_api.h"
#include "roman_suite_common.h"

napi_value RomanAdd(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t sum = left + right;
    NAPI_ASSERT(env, sum <= ROMAN_MAX_VALUE, "Sum exceeds roman range");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, sum, result));
    return result;
}

napi_value RomanSubtract(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t difference = left - right;
    NAPI_ASSERT(env, difference >= ROMAN_MIN_VALUE, "Difference must stay within roman range");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, difference, result));
    return result;
}

napi_value RomanMultiply(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int64_t product = static_cast<int64_t>(left) * static_cast<int64_t>(right);
    NAPI_ASSERT(env, product <= ROMAN_MAX_VALUE, "Product exceeds roman range");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, static_cast<int32_t>(product), result));
    return result;
}

napi_value RomanDivide(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t quotient = left / right;
    NAPI_ASSERT(env, quotient >= ROMAN_MIN_VALUE, "Dividend must not be smaller than divisor");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, quotient, result));
    return result;
}

// Integer remainder, returned as a plain number because zero has no roman symbol.
napi_value RomanMod(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t remainder = left % right;

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, remainder, &result));
    return result;
}

napi_value RomanCompare(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t order = COMPARE_EQUAL;
    if (left < right) {
        order = COMPARE_LESS;
    } else if (right < left) {
        order = COMPARE_GREATER;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, order, &result));
    return result;
}

napi_value RomanMin(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t value = (left < right) ? left : right;

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, value, result));
    return result;
}

napi_value RomanMax(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: romanA, romanB");

    int32_t left = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ZERO], left));

    int32_t right = ROMAN_ZERO;
    NAPI_CALL(env, ExtractRomanArg(env, argv[ARG_INDEX_ONE], right));

    int32_t value = (right < left) ? left : right;

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, value, result));
    return result;
}

// Re-emits the canonical form of a lenient-parseable numeral, e.g. "IIII" -> "IV".
napi_value RomanNormalize(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    bool valid = false;
    int32_t value = ROMAN_ZERO;
    ParseRomanInRange(text, valid, value);
    NAPI_ASSERT(env, valid, "Argument must be a roman numeral in range");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, value, result));
    return result;
}

// Sums every numeral of an array and returns the canonical form of the total.
napi_value RomanSumList(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: romanList");

    bool isArray = false;
    NAPI_CALL(env, napi_is_array(env, argv[ARG_INDEX_ZERO], &isArray));
    NAPI_ASSERT(env, isArray, "Roman list must be an array");

    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[ARG_INDEX_ZERO], &length));

    int64_t total = ROMAN_ZERO;
    for (uint32_t i = 0; i < length; ++i) {
        napi_value item = nullptr;
        NAPI_CALL(env, napi_get_element(env, argv[ARG_INDEX_ZERO], i, &item));
        int32_t value = ROMAN_ZERO;
        NAPI_CALL(env, ExtractRomanArg(env, item, value));
        total += value;
    }

    NAPI_ASSERT(env, total >= ROMAN_MIN_VALUE && total <= ROMAN_MAX_VALUE, "Sum must be within roman range");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, static_cast<int32_t>(total), result));
    return result;
}

napi_status RegisterRomanOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("romanAdd", RomanAdd),
        DECLARE_NAPI_FUNCTION("romanSubtract", RomanSubtract),
        DECLARE_NAPI_FUNCTION("romanMultiply", RomanMultiply),
        DECLARE_NAPI_FUNCTION("romanDivide", RomanDivide),
        DECLARE_NAPI_FUNCTION("romanMod", RomanMod),
        DECLARE_NAPI_FUNCTION("romanCompare", RomanCompare),
        DECLARE_NAPI_FUNCTION("romanMin", RomanMin),
        DECLARE_NAPI_FUNCTION("romanMax", RomanMax),
        DECLARE_NAPI_FUNCTION("romanNormalize", RomanNormalize),
        DECLARE_NAPI_FUNCTION("romanSumList", RomanSumList),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
