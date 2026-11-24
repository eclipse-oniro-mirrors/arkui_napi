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

#include "vector_ops_suite_common.h"

bool ExtractDoubleArg(napi_env env, napi_value value, double& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    NAPI_CALL_BASE(env, napi_get_value_double(env, value, &result), false);
    if (!std::isfinite(result)) {
        napi_throw_error(env, nullptr, "Number must be finite");
        return false;
    }
    return true;
}

bool ExtractVectorArg(napi_env env, napi_value value, std::vector<double>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be a vector array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length < VECTOR_MIN_SIZE || length > VECTOR_MAX_SIZE) {
        napi_throw_error(env, nullptr, "Vector size out of range");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        double number = 0.0;
        if (!ExtractDoubleArg(env, element, number)) {
            return false;
        }
        if (std::fabs(number) > VECTOR_MAX_ABS_VALUE) {
            napi_throw_error(env, nullptr, "Vector component out of range");
            return false;
        }
        result.push_back(number);
    }
    return true;
}

bool ExtractVectorsSameSize(napi_env env, napi_value first, napi_value second,
                            std::vector<double>& left, std::vector<double>& right)
{
    if (!ExtractVectorArg(env, first, left) || !ExtractVectorArg(env, second, right)) {
        return false;
    }
    if (left.size() != right.size()) {
        napi_throw_error(env, nullptr, "Vectors must have the same size");
        return false;
    }
    return true;
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

napi_value CreateVectorValue(napi_env env, const std::vector<double>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateDoubleValue(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

double DotProductValue(const std::vector<double>& left, const std::vector<double>& right)
{
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        total += left[i] * right[i];
    }
    return total;
}

double NormL2Value(const std::vector<double>& values)
{
    return std::sqrt(DotProductValue(values, values));
}

double NormL1Value(const std::vector<double>& values)
{
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        total += std::fabs(values[i]);
    }
    return total;
}

double NormInfinityValue(const std::vector<double>& values)
{
    double largest = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        largest = std::max(largest, std::fabs(values[i]));
    }
    return largest;
}

bool VectorsAlmostEqual(const std::vector<double>& left, const std::vector<double>& right, double tolerance)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        if (std::fabs(left[i] - right[i]) > tolerance) {
            return false;
        }
    }
    return true;
}

std::vector<double> ScaleByValue(const std::vector<double>& values, double factor)
{
    std::vector<double> scaled;
    scaled.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        scaled.push_back(values[i] * factor);
    }
    return scaled;
}

napi_value VectorAddition(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    std::vector<double> sum;
    sum.reserve(left.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        sum.push_back(left[i] + right[i]);
    }
    return CreateVectorValue(env, sum);
}

napi_value VectorSubtraction(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    std::vector<double> difference;
    difference.reserve(left.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        difference.push_back(left[i] - right[i]);
    }
    return CreateVectorValue(env, difference);
}

napi_value ScalarTimesVector(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: scalar, vector");

    double scalar = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], scalar), "Scalar extraction failed");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ONE], values), "Vector extraction failed");

    return CreateVectorValue(env, ScaleByValue(values, scalar));
}

napi_value VectorDotProduct(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    return CreateDoubleValue(env, DotProductValue(left, right));
}

napi_value VectorCrossProduct3d(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");
    NAPI_ASSERT(env, left.size() == VECTOR_3D_SIZE, "Cross product requires 3-dimensional vectors");

    std::vector<double> crossed;
    crossed.push_back(left[INDEX_ONE] * right[INDEX_TWO] - left[INDEX_TWO] * right[INDEX_ONE]);
    crossed.push_back(left[INDEX_TWO] * right[INDEX_ZERO] - left[INDEX_ZERO] * right[INDEX_TWO]);
    crossed.push_back(left[INDEX_ZERO] * right[INDEX_ONE] - left[INDEX_ONE] * right[INDEX_ZERO]);
    return CreateVectorValue(env, crossed);
}

napi_value VectorNormL1(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    return CreateDoubleValue(env, NormL1Value(values));
}

napi_value VectorNormInfinity(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    return CreateDoubleValue(env, NormInfinityValue(values));
}

napi_value NormalizeUnitVector(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double norm = NormL2Value(values);
    NAPI_ASSERT(env, norm > UNIT_NORM_FLOOR, "Cannot normalize a zero vector");
    double scale = (norm > 0.0) ? 1.0 / norm : 0.0;
    return CreateVectorValue(env, ScaleByValue(values, scale));
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("vectorAddition", VectorAddition),
        DECLARE_NAPI_FUNCTION("vectorSubtraction", VectorSubtraction),
        DECLARE_NAPI_FUNCTION("scalarTimesVector", ScalarTimesVector),
        DECLARE_NAPI_FUNCTION("vectorDotProduct", VectorDotProduct),
        DECLARE_NAPI_FUNCTION("vectorCrossProduct3d", VectorCrossProduct3d),
        DECLARE_NAPI_FUNCTION("vectorNormL1", VectorNormL1),
        DECLARE_NAPI_FUNCTION("vectorNormInfinity", VectorNormInfinity),
        DECLARE_NAPI_FUNCTION("normalizeUnitVector", NormalizeUnitVector),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterVectorOpsFunctions(env, exports), nullptr);
    return exports;
}

static napi_module vectorOpsSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "vectorOpsSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void VectorOpsSuiteRegisterModule(void)
{
    napi_module_register(&vectorOpsSuiteModule);
}
