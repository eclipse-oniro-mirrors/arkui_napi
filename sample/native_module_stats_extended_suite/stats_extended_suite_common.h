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

#ifndef SAMPLE_NATIVE_MODULE_STATS_EXTENDED_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_STATS_EXTENDED_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

constexpr size_t DATA_MIN_SIZE = 1;
constexpr size_t DATA_MAX_SIZE = 512;
constexpr size_t MIN_PAIRS_FOR_CORRELATION = 2;
constexpr size_t MIN_SAMPLES_FOR_VARIANCE = 2;
constexpr size_t EMPTY_RESULT_SIZE = 0;
constexpr size_t INDEX_ZERO = 0;
constexpr size_t INDEX_ONE = 1;
constexpr size_t INDEX_TWO = 2;
constexpr uint32_t UINT32_INDEX_ZERO = 0;
constexpr double DATA_MAX_ABS_VALUE = 1e9;
constexpr double TRIM_RATIO_MIN = 0.0;
constexpr double TRIM_RATIO_MAX = 0.45;
constexpr double EMA_ALPHA_MIN = 0.0;
constexpr double EMA_ALPHA_MAX = 1.0;
constexpr double LOG_FLOOR_VALUE = 0.0;
constexpr double POWER_EXPONENT_MIN = -8.0;
constexpr double POWER_EXPONENT_MAX = 8.0;
constexpr double MIDPOINT_DIVISOR = 2.0;
constexpr double LOG_SHIFT_UNIT = 1.0;
constexpr double EXPONENT_LINEAR = 0.0;
constexpr double MULTIPLIER_TWO = 2.0;
constexpr double RANK_FIRST_POSITION = 1.0;

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
bool ExtractDataArray(napi_env env, napi_value value, std::vector<double>& result);
bool ExtractTwoArraysSameSize(napi_env env, napi_value first, napi_value second,
                              std::vector<double>& left, std::vector<double>& right);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateDataArrayValue(napi_env env, const std::vector<double>& values);
std::vector<double> SortedCopyOf(const std::vector<double>& values);
double MeanValue(const std::vector<double>& values);
double SumValue(const std::vector<double>& values);
double MedianValue(const std::vector<double>& values);
double PopulationVarianceValue(const std::vector<double>& values);
double DeviationSumSquare(const std::vector<double>& values, double mean);
double RangeSpanValue(const std::vector<double>& values);
std::vector<double> ComputeRanksOf(const std::vector<double>& values);
double CovarianceBetweenRanks(const std::vector<double>& left, const std::vector<double>& right);

napi_value VarianceOfGroup(napi_env env, napi_callback_info info);
napi_value CovarianceBetweenPearson(napi_env env, napi_callback_info info);
napi_value PearsonCorrelation(napi_env env, napi_callback_info info);
napi_value RankData(napi_env env, napi_callback_info info);
napi_value WeightedVariance(napi_env env, napi_callback_info info);
napi_status RegisterStatsExtendedFunctions(napi_env env, napi_value exports);

napi_value SpearmanCorrelation(napi_env env, napi_callback_info info);
napi_value DeviationsFromMean(napi_env env, napi_callback_info info);
napi_value NormalizedByRange(napi_env env, napi_callback_info info);
napi_value MovingSumWindow(napi_env env, napi_callback_info info);
napi_value SumOfSquaredDeviations(napi_env env, napi_callback_info info);
napi_value MeanAbsoluteDeviationValue(napi_env env, napi_callback_info info);
napi_value InterquartileSpreadValue(napi_env env, napi_callback_info info);
napi_value StandardErrorOfMeanValue(napi_env env, napi_callback_info info);
napi_value GeometricMeanPositive(napi_env env, napi_callback_info info);
napi_value TrimmedVariance(napi_env env, napi_callback_info info);
napi_value PooledVariance(napi_env env, napi_callback_info info);
napi_value RunningMean(napi_env env, napi_callback_info info);
napi_value ExponentialMovingAverage(napi_env env, napi_callback_info info);
napi_value CumulativeProduct(napi_env env, napi_callback_info info);
napi_value PartialSums(napi_env env, napi_callback_info info);
napi_value SecondDifferences(napi_env env, napi_callback_info info);
napi_value SafeLogTransform(napi_env env, napi_callback_info info);
napi_value PowerTransform(napi_env env, napi_callback_info info);

#endif  // SAMPLE_NATIVE_MODULE_STATS_EXTENDED_SUITE_COMMON_H
