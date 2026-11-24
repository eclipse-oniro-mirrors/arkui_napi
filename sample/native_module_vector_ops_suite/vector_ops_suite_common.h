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

#ifndef SAMPLE_NATIVE_MODULE_VECTOR_OPS_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_VECTOR_OPS_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

constexpr size_t VECTOR_MIN_SIZE = 1;
constexpr size_t VECTOR_MAX_SIZE = 256;
constexpr size_t VECTOR_3D_SIZE = 3;
constexpr size_t EMPTY_RESULT_SIZE = 0;
constexpr size_t INDEX_ZERO = 0;
constexpr size_t INDEX_ONE = 1;
constexpr size_t INDEX_TWO = 2;
constexpr uint32_t UINT32_INDEX_ZERO = 0;
constexpr double VECTOR_MAX_ABS_VALUE = 1e9;
constexpr double VECTOR_TOLERANCE_DEFAULT = 1e-9;
constexpr double UNIT_NORM_FLOOR = 1e-15;
constexpr double MIDPOINT_DIVISOR = 2.0;
constexpr double UNIT_TARGET_LENGTH = 1.0;

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
bool ExtractVectorArg(napi_env env, napi_value value, std::vector<double>& result);
bool ExtractVectorsSameSize(napi_env env, napi_value first, napi_value second,
                            std::vector<double>& left, std::vector<double>& right);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateVectorValue(napi_env env, const std::vector<double>& values);

double DotProductValue(const std::vector<double>& left, const std::vector<double>& right);
double NormL2Value(const std::vector<double>& values);
double NormL1Value(const std::vector<double>& values);
double NormInfinityValue(const std::vector<double>& values);
bool VectorsAlmostEqual(const std::vector<double>& left, const std::vector<double>& right, double tolerance);
std::vector<double> ScaleByValue(const std::vector<double>& values, double factor);

napi_value VectorNormSquared(napi_env env, napi_callback_info info);
napi_value VectorDistanceL2(napi_env env, napi_callback_info info);
napi_value CosineSimilarityOf(napi_env env, napi_callback_info info);
napi_value HadamardProductOf(napi_env env, napi_callback_info info);
napi_value SumOfComponents(napi_env env, napi_callback_info info);
napi_value NegateVectorAll(napi_env env, napi_callback_info info);
napi_value IsZeroVector(napi_env env, napi_callback_info info);
napi_value ComponentMaxIndex(napi_env env, napi_callback_info info);
napi_value VectorMeanValue(napi_env env, napi_callback_info info);
napi_value ScaleTowardsOrigin(napi_env env, napi_callback_info info);
napi_value AbsoluteComponentsOf(napi_env env, napi_callback_info info);
napi_value ClampComponentsInto(napi_env env, napi_callback_info info);
napi_value IsUnitLengthWithin(napi_env env, napi_callback_info info);
napi_value ComponentMinIndex(napi_env env, napi_callback_info info);
napi_value VectorSumSquaresOf(napi_env env, napi_callback_info info);
napi_value VectorRangeSpan(napi_env env, napi_callback_info info);
napi_value AngleBetweenVectorsRad(napi_env env, napi_callback_info info);
napi_value IsOrthogonalPair(napi_env env, napi_callback_info info);
napi_value LerpVector(napi_env env, napi_callback_info info);
napi_value MidpointVector(napi_env env, napi_callback_info info);
napi_value ProjectOntoVector(napi_env env, napi_callback_info info);
napi_value EpsilonEquals(napi_env env, napi_callback_info info);
napi_value DotWithOffset(napi_env env, napi_callback_info info);
napi_status RegisterVectorOpsFunctions(napi_env env, napi_value exports);

#endif  // SAMPLE_NATIVE_MODULE_VECTOR_OPS_SUITE_COMMON_H
