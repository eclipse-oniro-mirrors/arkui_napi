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
#include <vector>

#include "napi/native_node_api.h"

#include "number_theory_suite_common.h"

napi_value TriangularRootIsInteger(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t doubled = n * KAPRECAR_SPLIT_POWER;
    int64_t root = IntegerRootOf(doubled);
    bool triangular = root * (root + 1) == doubled;
    return CreateBoolValue(env, triangular);
}

napi_value SquareTriangularCheck(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t root = IntegerRootOf(n);
    bool square = root * root == n;
    int64_t doubled = n * KAPRECAR_SPLIT_POWER;
    int64_t triangularRoot = IntegerRootOf(doubled);
    bool triangular = triangularRoot * (triangularRoot + 1) == doubled;
    return CreateBoolValue(env, square && triangular);
}

napi_value PentagonalRootValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t low = 1;
    int64_t high = HAPPY_SQUARE_LIMIT;
    while (low <= high) {
        int64_t middle = low + (high - low) / BINARY_SHIFT_BASE;
        int64_t value = middle * (middle * KAPRECAR_SPLIT_POWER - 1) / KAPRECAR_SPLIT_POWER;
        if (value == n) {
            return CreateInt64Value(env, middle);
        }
        if (value < n) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return CreateInt64Value(env, NOT_FOUND_SENTINEL);
}

napi_value HexagonalNumberAt(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, PELL_MAX_N, n),
                "n must be within [1, 100000]");

    int64_t result = n * (KAPRECAR_SPLIT_POWER * n - 1);
    return CreateInt64Value(env, result);
}

napi_value IsCubeNumber(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t root = static_cast<int64_t>(std::round(std::cbrt(static_cast<double>(n))));
    bool cube = root * root * root == n || (root + 1) * (root + 1) * (root + 1) == n;
    return CreateBoolValue(env, cube);
}

napi_value FourthPowerRoot(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t squareRoot = IntegerRootOf(n);
    int64_t fourthRoot = IntegerRootOf(squareRoot);
    bool exact = fourthRoot * fourthRoot == squareRoot && squareRoot * squareRoot == n;
    return CreateInt64Value(env, exact ? fourthRoot : NOT_FOUND_SENTINEL);
}

napi_value NearestSquareOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t root = IntegerRootOf(n);
    int64_t lowerGap = n - root * root;
    int64_t upperGap = (root + 1) * (root + 1) - n;
    int64_t chosen = lowerGap <= upperGap ? root : root + 1;
    return CreateInt64Value(env, chosen * chosen);
}

napi_value NearestCubeOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: n");

    int64_t n = 0;
    NAPI_ASSERT(env, ExtractBoundedIntArg(env, argv[ARG_INDEX_ZERO], NT_MIN_INPUT, NT_MAX_INPUT, n),
                "n must be within [1, 1000000000]");

    int64_t root = static_cast<int64_t>(std::round(std::cbrt(static_cast<double>(n))));
    int64_t best = root * root * root;
    int64_t candidate = (root + 1) * (root + 1) * (root + 1);
    if (std::llabs(candidate - n) < std::llabs(best - n)) {
        best = candidate;
    }
    candidate = (root - 1) * (root - 1) * (root - 1);
    if (root > 1 && std::llabs(candidate - n) < std::llabs(best - n)) {
        best = candidate;
    }
    return CreateInt64Value(env, best);
}

napi_status RegisterNumberTheoryFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("factorialTrailingZeros", FactorialTrailingZeros),
        DECLARE_NAPI_FUNCTION("inBaseString", InBaseString),
        DECLARE_NAPI_FUNCTION("pellEquationMinSolution", PellEquationMinSolution),
        DECLARE_NAPI_FUNCTION("triangularRootIsInteger", TriangularRootIsInteger),
        DECLARE_NAPI_FUNCTION("squareTriangularCheck", SquareTriangularCheck),
        DECLARE_NAPI_FUNCTION("pentagonalRootValue", PentagonalRootValue),
        DECLARE_NAPI_FUNCTION("hexagonalNumberAt", HexagonalNumberAt),
        DECLARE_NAPI_FUNCTION("isCubeNumber", IsCubeNumber),
        DECLARE_NAPI_FUNCTION("fourthPowerRoot", FourthPowerRoot),
        DECLARE_NAPI_FUNCTION("nearestSquareOf", NearestSquareOf),
        DECLARE_NAPI_FUNCTION("nearestCubeOf", NearestCubeOf),
    };
    NAPI_CALL_BASE(env, RegisterExtraNumberTheoryFunctions(env, exports), napi_generic_failure);
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
