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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "napi/native_node_api.h"

#include "number_theory_suite_common.h"

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

bool ExtractBoundedIntArg(napi_env env, napi_value value, int64_t low, int64_t high, int64_t& result)
{
    if (!ExtractInt64Arg(env, value, result)) {
        return false;
    }
    if (result < low || result > high) {
        napi_throw_error(env, nullptr, "Argument out of supported range");
        return false;
    }
    return true;
}

bool ExtractShortTextArg(napi_env env, napi_value value, std::string& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_string) {
        napi_throw_error(env, nullptr, "Argument must be a string");
        return false;
    }

    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, nullptr, 0, &length), false);
    if (length > DIGIT_COUNT_LIMIT * NT_BASE_MAX) {
        napi_throw_error(env, nullptr, "Text is too long");
        return false;
    }

    result.assign(length + 1, '\0');
    size_t copied = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, &result[0], length + 1, &copied), false);
    result.resize(copied);
    return true;
}

napi_value CreateInt64Value(napi_env env, int64_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int64(env, value, &result));
    return result;
}

napi_value CreateBoolValue(napi_env env, bool value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result));
    return result;
}

napi_value CreateStringValue(napi_env env, const std::string& value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, value.c_str(), value.size(), &result));
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

std::vector<int64_t> DigitsOfInBase(int64_t value, int64_t base)
{
    std::vector<int64_t> digits;
    if (base <= 0) {
        return digits;
    }
    if (value == 0) {
        digits.push_back(0);
        return digits;
    }

    int64_t remaining = value;
    while (remaining > 0 && base > 0 && digits.size() < DIGIT_COUNT_LIMIT) {
        digits.push_back(remaining % base);
        remaining /= base;
    }
    return digits;
}

int64_t DigitSumValue(int64_t value)
{
    int64_t total = 0;
    while (value > 0) {
        total += value % DECIMAL_BASE;
        value /= DECIMAL_BASE;
    }
    return total;
}

int64_t IntegerRootOf(int64_t value)
{
    if (value <= 0) {
        return 0;
    }
    int64_t low = 0;
    int64_t high = value;
    while (low < high) {
        int64_t middle = low + (high - low + UINT32_ONE) / BINARY_SHIFT_BASE;
        if (middle <= 0 || middle > value / middle) {
            high = middle - 1;
        } else {
            low = middle;
        }
    }
    return low;
}

std::vector<std::pair<int64_t, uint32_t>> PrimeFactorizationOf(int64_t value)
{
    std::vector<std::pair<int64_t, uint32_t>> factors;
    int64_t remaining = value;
    for (int64_t divisor = BINARY_SHIFT_BASE; divisor * divisor <= remaining; ++divisor) {
        if (remaining % divisor != 0) {
            continue;
        }

        uint32_t exponent = UINT32_INDEX_ZERO;
        while (remaining % divisor == 0) {
            remaining /= divisor;
            ++exponent;
        }
        factors.emplace_back(divisor, exponent);
    }
    if (remaining > 1) {
        factors.emplace_back(remaining, UINT32_ONE);
    }
    return factors;
}

int64_t SumOfDivisorsValue(int64_t value)
{
    if (value <= 0) {
        return 0;
    }
    int64_t root = IntegerRootOf(value);
    int64_t total = 0;
    for (int64_t divisor = 1; divisor <= root; ++divisor) {
        if (value % divisor != 0) {
            continue;
        }
        total += divisor;
        int64_t paired = value / divisor;
        total += paired == divisor ? 0 : paired;
    }
    return total;
}

static int64_t SquareSuffixEquals(const std::vector<int64_t>& squareDigits, const std::vector<int64_t>& digits)
{
    size_t tail = squareDigits.size() - digits.size();
    for (size_t i = EMPTY_RESULT_SIZE; i < digits.size(); ++i) {
        if (squareDigits[tail + i] != digits[digits.size() - INDEX_ONE - i]) {
            return 0;
        }
    }
    return 1;
}

napi_value IsAutomorphicNumber(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t square = n * n;
    std::vector<int64_t> squareDigits = DigitsOfInBase(square, DECIMAL_BASE);
    std::vector<int64_t> digits = DigitsOfInBase(n, DECIMAL_BASE);
    if (digits.size() > squareDigits.size()) {
        return CreateBoolValue(env, false);
    }

    return CreateBoolValue(env, SquareSuffixEquals(squareDigits, digits) == 1);
}

napi_value IsDudeneyNumber(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t digitSum = DigitSumValue(n);
    bool result = digitSum > 0 && digitSum * digitSum * digitSum == n;
    return CreateBoolValue(env, result);
}

static bool KaprekarSplitMatches(int64_t n, int64_t square, int64_t splitPower)
{
    int64_t rightModulus = 1;
    for (int64_t i = 0; i < splitPower; ++i) {
        rightModulus *= DECIMAL_BASE;
    }

    if (rightModulus <= 0) {
        return false;
    }
    int64_t rightPart = square % rightModulus;
    int64_t leftPart = square / rightModulus;
    return leftPart + rightPart == n && rightPart != 0;
}

napi_value IsKaprekarNumber(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t square = n * n;
    int64_t digitCount = static_cast<int64_t>(DigitsOfInBase(square, DECIMAL_BASE).size());
    for (int64_t splitPower = 1; splitPower <= digitCount; ++splitPower) {
        if (KaprekarSplitMatches(n, square, splitPower)) {
            return CreateBoolValue(env, true);
        }
    }
    return CreateBoolValue(env, false);
}

static int64_t FactorDigitSum(const std::vector<std::pair<int64_t, uint32_t>>& factors)
{
    int64_t total = 0;
    for (size_t i = EMPTY_RESULT_SIZE; i < factors.size(); ++i) {
        int64_t prime = factors[i].first;
        uint32_t exponent = factors[i].second;
        for (uint32_t k = 0; k < exponent; ++k) {
            total += DigitSumValue(prime);
        }
    }
    return total;
}

napi_value IsSmithNumber(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], SMITH_DIGIT_MIN, NT_MAX_INPUT, n),
                "n must be within [2, 1000000000]");

    std::vector<std::pair<int64_t, uint32_t>> factors = PrimeFactorizationOf(n);
    bool composite = factors.size() > INDEX_ONE || factors[INDEX_ZERO].second > UINT32_ONE;
    bool smith = composite && FactorDigitSum(factors) == DigitSumValue(n);
    return CreateBoolValue(env, smith);
}

static int64_t HappyStepInBase(int64_t value, int64_t base)
{
    int64_t total = 0;
    if (base <= 0) {
        return total;
    }
    while (value > 0 && base > 0) {
        int64_t digit = value % base;
        total += digit * digit;
        value /= base;
    }
    return total;
}

napi_value IsHappyNumberInBase(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: n, base");

    int64_t n = 0;
    int64_t base = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ONE], NT_BASE_MIN, NT_BASE_MAX, base),
                "base must be within [2, 36]");

    int64_t current = n;
    for (int64_t step = 0; step < HAPPY_SQUARE_LIMIT; ++step) {
        if (current == 1) {
            return CreateBoolValue(env, true);
        }
        current = HappyStepInBase(current, base);
    }
    return CreateBoolValue(env, false);
}

napi_value SumOfProperDivisors(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    return CreateInt64Value(env, SumOfDivisorsValue(n) - n);
}

napi_value AliquotChainLength(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t current = n;
    int64_t length = 0;
    while (current > 1 && length < HAPPY_SQUARE_LIMIT) {
        current = SumOfDivisorsValue(current) - current;
        ++length;
    }
    return CreateInt64Value(env, length);
}

static bool IsSquareFreeValue(int64_t value)
{
    for (int64_t divisor = BINARY_SHIFT_BASE; divisor * divisor <= value; ++divisor) {
        if (value % (divisor * divisor) == 0) {
            return false;
        }
    }
    return true;
}

napi_value NextPerfectSquareFree(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t candidate = n + 1;
    while (!IsSquareFreeValue(candidate)) {
        ++candidate;
    }
    return CreateInt64Value(env, candidate);
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("isAutomorphicNumber", IsAutomorphicNumber),
        DECLARE_NAPI_FUNCTION("isDudeneyNumber", IsDudeneyNumber),
        DECLARE_NAPI_FUNCTION("isKaprekarNumber", IsKaprekarNumber),
        DECLARE_NAPI_FUNCTION("isSmithNumber", IsSmithNumber),
        DECLARE_NAPI_FUNCTION("isHappyNumberInBase", IsHappyNumberInBase),
        DECLARE_NAPI_FUNCTION("sumOfProperDivisors", SumOfProperDivisors),
        DECLARE_NAPI_FUNCTION("aliquotChainLength", AliquotChainLength),
        DECLARE_NAPI_FUNCTION("nextPerfectSquareFree", NextPerfectSquareFree),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterNumberTheoryFunctions(env, exports), nullptr);
    return exports;
}

static napi_module numberTheorySuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "numberTheorySuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void NumberTheorySuiteRegisterModule(void)
{
    napi_module_register(&numberTheorySuiteModule);
}
