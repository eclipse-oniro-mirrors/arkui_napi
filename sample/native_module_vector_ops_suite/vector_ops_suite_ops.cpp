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

static const double LERP_MIN_RATIO = 0.0;
static const double LERP_MAX_RATIO = 1.0;
static const double NEGATE_SCALE_FACTOR = 1.0;
static const double SHRINK_MIN_RATIO = 0.0;
static const double SHRINK_MAX_RATIO = 1.0;

napi_value VectorNormSquared(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    return CreateDoubleValue(env, DotProductValue(values, values));
}

napi_value VectorDistanceL2(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    std::vector<double> delta;
    delta.reserve(left.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        delta.push_back(left[i] - right[i]);
    }
    return CreateDoubleValue(env, NormL2Value(delta));
}

napi_value CosineSimilarityOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    double denominator = NormL2Value(left) * NormL2Value(right);
    NAPI_ASSERT(env, denominator > UNIT_NORM_FLOOR, "Vectors must be non-zero");
    return CreateDoubleValue(env, DotProductValue(left, right) / denominator);
}

napi_value HadamardProductOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    std::vector<double> product;
    product.reserve(left.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        product.push_back(left[i] * right[i]);
    }
    return CreateVectorValue(env, product);
}

napi_value SumOfComponents(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        total += values[i];
    }
    return CreateDoubleValue(env, total);
}

napi_value NegateVectorAll(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    return CreateVectorValue(env, ScaleByValue(values, -NEGATE_SCALE_FACTOR));
}

napi_value IsZeroVector(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    bool zero = NormInfinityValue(values) <= VECTOR_TOLERANCE_DEFAULT;
    return CreateBoolValue(env, zero);
}

napi_value ComponentMaxIndex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    size_t best = EMPTY_RESULT_SIZE;
    for (size_t i = INDEX_ONE; i < values.size(); ++i) {
        best = values[i] > values[best] ? i : best;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, static_cast<uint32_t>(best), &result));
    return result;
}

napi_value VectorMeanValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");
    NAPI_ASSERT(env, !values.empty(), "Vector must not be empty");

    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        total += values[i];
    }
    return CreateDoubleValue(env, total / static_cast<double>(values.size()));
}

napi_value ScaleTowardsOrigin(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: vector, shrinkRatio");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double ratio = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], ratio), "Ratio extraction failed");
    NAPI_ASSERT(env, ratio >= SHRINK_MIN_RATIO && ratio <= SHRINK_MAX_RATIO, "Ratio must be within [0, 1]");

    return CreateVectorValue(env, ScaleByValue(values, ratio));
}

napi_value AbsoluteComponentsOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    std::vector<double> absolutes;
    absolutes.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        absolutes.push_back(std::fabs(values[i]));
    }
    return CreateVectorValue(env, absolutes);
}

napi_value ClampComponentsInto(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: vector, bound");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double bound = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], bound), "Bound extraction failed");
    NAPI_ASSERT(env, bound > UNIT_NORM_FLOOR, "Bound must be positive");

    std::vector<double> clamped;
    clamped.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        clamped.push_back(std::min(bound, std::max(-bound, values[i])));
    }
    return CreateVectorValue(env, clamped);
}

napi_value IsUnitLengthWithin(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double deviation = std::fabs(NormL2Value(values) - UNIT_TARGET_LENGTH);
    bool unitLength = deviation <= VECTOR_TOLERANCE_DEFAULT;
    return CreateBoolValue(env, unitLength);
}

napi_value ComponentMinIndex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    size_t best = EMPTY_RESULT_SIZE;
    for (size_t i = INDEX_ONE; i < values.size(); ++i) {
        best = values[i] < values[best] ? i : best;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, static_cast<uint32_t>(best), &result));
    return result;
}

napi_value VectorSumSquaresOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    return CreateDoubleValue(env, DotProductValue(values, values));
}

napi_value VectorRangeSpan(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: vector");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double smallest = values[INDEX_ZERO];
    double largest = values[INDEX_ZERO];
    for (size_t i = INDEX_ONE; i < values.size(); ++i) {
        smallest = std::min(smallest, values[i]);
        largest = std::max(largest, values[i]);
    }
    return CreateDoubleValue(env, largest - smallest);
}

napi_value AngleBetweenVectorsRad(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    double leftNorm = NormL2Value(left);
    double rightNorm = NormL2Value(right);
    NAPI_ASSERT(env, leftNorm > UNIT_NORM_FLOOR && rightNorm > UNIT_NORM_FLOOR, "Vectors must be non-zero");

    double cosine = DotProductValue(left, right) / (leftNorm * rightNorm);
    cosine = std::min(1.0, std::max(-1.0, cosine));
    return CreateDoubleValue(env, std::acos(cosine));
}

napi_value IsOrthogonalPair(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    double tolerance = NormL1Value(left) * NormL1Value(right) * VECTOR_TOLERANCE_DEFAULT + VECTOR_TOLERANCE_DEFAULT;
    bool orthogonal = std::fabs(DotProductValue(left, right)) <= tolerance;
    return CreateBoolValue(env, orthogonal);
}

napi_value LerpVector(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: vector, ratio");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], values), "Vector extraction failed");

    double ratio = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], ratio), "Ratio extraction failed");
    NAPI_ASSERT(env, ratio >= LERP_MIN_RATIO && ratio <= LERP_MAX_RATIO, "Ratio must be within [0, 1]");

    std::vector<double> result;
    result.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        result.push_back(values[i] * ratio);
    }
    return CreateVectorValue(env, result);
}

napi_value MidpointVector(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    std::vector<double> midpoint;
    midpoint.reserve(left.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        midpoint.push_back((left[i] + right[i]) / MIDPOINT_DIVISOR);
    }
    return CreateVectorValue(env, midpoint);
}

static std::vector<double> ProjectOntoAxis(const std::vector<double>& values, const std::vector<double>& axis)
{
    double axisNormSquared = DotProductValue(axis, axis);
    if (axisNormSquared <= 0.0) {
        return std::vector<double>(values.size(), 0.0);
    }
    double scale = (axisNormSquared > 0.0) ? DotProductValue(values, axis) / axisNormSquared : 0.0;
    return ScaleByValue(axis, scale);
}

napi_value ProjectOntoVector(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: vector, axis");

    std::vector<double> values;
    std::vector<double> axis;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], values, axis),
                "Vector extraction failed");
    NAPI_ASSERT(env, DotProductValue(axis, axis) > UNIT_NORM_FLOOR, "Axis must be non-zero");

    return CreateVectorValue(env, ProjectOntoAxis(values, axis));
}

napi_value EpsilonEquals(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ZERO], left), "First vector extraction failed");
    NAPI_ASSERT(env, ExtractVectorArg(env, argv[ARG_INDEX_ONE], right), "Second vector extraction failed");

    bool equals = VectorsAlmostEqual(left, right, VECTOR_TOLERANCE_DEFAULT);
    return CreateBoolValue(env, equals);
}

napi_value DotWithOffset(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractVectorsSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Vector extraction failed");

    size_t offset = left.size() / static_cast<size_t>(MIDPOINT_DIVISOR);
    double total = 0.0;
    for (size_t i = offset; i < left.size(); ++i) {
        total += left[i] * right[i - offset];
    }
    return CreateDoubleValue(env, total);
}

napi_status RegisterVectorOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("vectorNormSquared", VectorNormSquared),
        DECLARE_NAPI_FUNCTION("vectorDistanceL2", VectorDistanceL2),
        DECLARE_NAPI_FUNCTION("cosineSimilarityOf", CosineSimilarityOf),
        DECLARE_NAPI_FUNCTION("hadamardProductOf", HadamardProductOf),
        DECLARE_NAPI_FUNCTION("sumOfComponents", SumOfComponents),
        DECLARE_NAPI_FUNCTION("negateVectorAll", NegateVectorAll),
        DECLARE_NAPI_FUNCTION("isZeroVector", IsZeroVector),
        DECLARE_NAPI_FUNCTION("componentMaxIndex", ComponentMaxIndex),
        DECLARE_NAPI_FUNCTION("vectorMeanValue", VectorMeanValue),
        DECLARE_NAPI_FUNCTION("scaleTowardsOrigin", ScaleTowardsOrigin),
        DECLARE_NAPI_FUNCTION("absoluteComponentsOf", AbsoluteComponentsOf),
        DECLARE_NAPI_FUNCTION("clampComponentsInto", ClampComponentsInto),
        DECLARE_NAPI_FUNCTION("isUnitLengthWithin", IsUnitLengthWithin),
        DECLARE_NAPI_FUNCTION("componentMinIndex", ComponentMinIndex),
        DECLARE_NAPI_FUNCTION("vectorSumSquaresOf", VectorSumSquaresOf),
        DECLARE_NAPI_FUNCTION("vectorRangeSpan", VectorRangeSpan),
        DECLARE_NAPI_FUNCTION("angleBetweenVectorsRad", AngleBetweenVectorsRad),
        DECLARE_NAPI_FUNCTION("isOrthogonalPair", IsOrthogonalPair),
        DECLARE_NAPI_FUNCTION("lerpVector", LerpVector),
        DECLARE_NAPI_FUNCTION("midpointVector", MidpointVector),
        DECLARE_NAPI_FUNCTION("projectOntoVector", ProjectOntoVector),
        DECLARE_NAPI_FUNCTION("epsilonEquals", EpsilonEquals),
        DECLARE_NAPI_FUNCTION("dotWithOffset", DotWithOffset),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
