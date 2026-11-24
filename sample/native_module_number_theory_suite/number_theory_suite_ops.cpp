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
#include <vector>

#include "napi/native_node_api.h"

#include "number_theory_suite_common.h"


#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
static int64_t DigitSumRepeated(int64_t value, int64_t base)
{
    int64_t total = 0;
    if (base <= 0) {
        return total;
    }
    while (value > 0 && base > 0) {
        total += value % base;
        value /= base;
    }
    return total;
}

napi_value DigitalRootInBase(napi_env env, napi_callback_info info)
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
    while (current >= base) {
        current = DigitSumRepeated(current, base);
    }
    return CreateInt64Value(env, current);
}

napi_value IsNarcissisticNumber(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    std::vector<int64_t> digits = DigitsOfInBase(n, DECIMAL_BASE);
    int64_t power = static_cast<int64_t>(digits.size());
    int64_t total = 0;
    for (size_t i = EMPTY_RESULT_SIZE; i < digits.size(); ++i) {
        total += static_cast<int64_t>(std::pow(static_cast<double>(digits[i]), static_cast<double>(power)));
    }
    return CreateBoolValue(env, total == n);
}

napi_value DivisorCountOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t root = IntegerRootOf(n);
    int64_t count = 0;
    for (int64_t divisor = 1; divisor <= root; ++divisor) {
        if (n % divisor != 0) {
            continue;
        }
        count += divisor * divisor == n ? UINT32_ONE : LUCKY_WINDOW_STRIDE;
    }
    return CreateInt64Value(env, count);
}

napi_value EulerTotientUnder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: limit");

    int64_t limit = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, PELL_MAX_N, limit),
                "limit must be within [1, 100000]");

    std::vector<int64_t> totients(limit + INDEX_ONE, 0);
    for (int64_t index = 1; index <= limit; ++index) {
        totients[index] = index;
    }

    for (int64_t prime = BINARY_SHIFT_BASE; prime <= limit; ++prime) {
        if (totients[prime] != prime) {
            continue;
        }
        for (int64_t multiple = prime; multiple <= limit; multiple += prime) {
            totients[multiple] = totients[multiple] / prime * (prime - 1);
        }
    }

    int64_t total = 0;
    for (int64_t index = 1; index <= limit; ++index) {
        total += totients[index];
    }
    return CreateInt64Value(env, total);
}

napi_value SumOfDivisorsOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    return CreateInt64Value(env, SumOfDivisorsValue(n));
}

napi_status RegisterExtraNumberTheoryFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("digitalRootInBase", DigitalRootInBase),
        DECLARE_NAPI_FUNCTION("isNarcissisticNumber", IsNarcissisticNumber),
        DECLARE_NAPI_FUNCTION("divisorCountOf", DivisorCountOf),
        DECLARE_NAPI_FUNCTION("eulerTotientUnder", EulerTotientUnder),
        DECLARE_NAPI_FUNCTION("sumOfDivisorsOf", SumOfDivisorsOf),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
