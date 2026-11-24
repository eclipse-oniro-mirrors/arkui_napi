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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "napi/native_node_api.h"

#include "stats_extended_suite_common.h"

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

bool ExtractDataArray(napi_env env, napi_value value, std::vector<double>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array of numbers");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length < DATA_MIN_SIZE || length > DATA_MAX_SIZE) {
        napi_throw_error(env, nullptr, "Data size out of range");
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
        if (std::fabs(number) > DATA_MAX_ABS_VALUE) {
            napi_throw_error(env, nullptr, "Data value out of range");
            return false;
        }
        result.push_back(number);
    }
    return true;
}

bool ExtractTwoArraysSameSize(napi_env env, napi_value first, napi_value second,
                              std::vector<double>& left, std::vector<double>& right)
{
    if (!ExtractDataArray(env, first, left) || !ExtractDataArray(env, second, right)) {
        return false;
    }
    if (left.size() != right.size()) {
        napi_throw_error(env, nullptr, "Arrays must have the same size");
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

napi_value CreateDataArrayValue(napi_env env, const std::vector<double>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateDoubleValue(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

std::vector<double> SortedCopyOf(const std::vector<double>& values)
{
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

double SumValue(const std::vector<double>& values)
{
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        total += values[i];
    }
    return total;
}

double MeanValue(const std::vector<double>& values)
{
    return SumValue(values) / static_cast<double>(values.size());
}

double MedianValue(const std::vector<double>& values)
{
    std::vector<double> sorted = SortedCopyOf(values);
    size_t middle = sorted.size() / static_cast<size_t>(MIDPOINT_DIVISOR);
    if (sorted.size() % static_cast<size_t>(MIDPOINT_DIVISOR) != EMPTY_RESULT_SIZE) {
        return sorted[middle];
    }
    return (sorted[middle - INDEX_ONE] + sorted[middle]) / MIDPOINT_DIVISOR;
}

double DeviationSumSquare(const std::vector<double>& values, double mean)
{
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        double deviation = values[i] - mean;
        total += deviation * deviation;
    }
    return total;
}

double PopulationVarianceValue(const std::vector<double>& values)
{
    return DeviationSumSquare(values, MeanValue(values)) / static_cast<double>(values.size());
}

double RangeSpanValue(const std::vector<double>& values)
{
    std::vector<double> sorted = SortedCopyOf(values);
    return sorted.back() - sorted[INDEX_ZERO];
}

double CovarianceBetweenRanks(const std::vector<double>& left, const std::vector<double>& right)
{
    double leftMean = MeanValue(left);
    double rightMean = MeanValue(right);
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < left.size(); ++i) {
        total += (left[i] - leftMean) * (right[i] - rightMean);
    }
    return total / static_cast<double>(left.size());
}

napi_value VarianceOfGroup(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    return CreateDoubleValue(env, PopulationVarianceValue(values));
}

napi_value CovarianceBetweenPearson(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractTwoArraysSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Array extraction failed");
    NAPI_ASSERT(env, left.size() >= MIN_PAIRS_FOR_CORRELATION, "At least two pairs are required");

    return CreateDoubleValue(env, CovarianceBetweenRanks(left, right));
}

napi_value PearsonCorrelation(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractTwoArraysSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], left, right),
                "Array extraction failed");
    NAPI_ASSERT(env, left.size() >= MIN_PAIRS_FOR_CORRELATION, "At least two pairs are required");

    double denominator = std::sqrt(PopulationVarianceValue(left) * PopulationVarianceValue(right));
    NAPI_ASSERT(env, denominator > 0.0, "Constant input has undefined correlation");
    return CreateDoubleValue(env, CovarianceBetweenRanks(left, right) / denominator);
}

static double AverageRankOfTies(size_t low, size_t high)
{
    double firstPosition = static_cast<double>(low) + RANK_FIRST_POSITION;
    double lastPosition = static_cast<double>(high) + RANK_FIRST_POSITION;
    return (firstPosition + lastPosition) / MIDPOINT_DIVISOR;
}

static size_t FindTieRunEnd(const std::vector<double>& sorted, size_t position)
{
    size_t runEnd = position + INDEX_ONE;
    while (runEnd < sorted.size() && sorted[runEnd] == sorted[position]) {
        ++runEnd;
    }
    return runEnd;
}

static void AssignRankToMatches(std::vector<double>& ranks, const std::vector<double>& values,
                                double tieValue, double rank)
{
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        if (values[i] == tieValue) {
            ranks[i] = rank;
        }
    }
}

std::vector<double> ComputeRanksOf(const std::vector<double>& values)
{
    std::vector<double> sorted = SortedCopyOf(values);
    std::vector<double> ranks(values.size(), 0.0);
    size_t position = EMPTY_RESULT_SIZE;
    while (position < sorted.size()) {
        size_t runEnd = FindTieRunEnd(sorted, position);
        AssignRankToMatches(ranks, values, sorted[position], AverageRankOfTies(position, runEnd - INDEX_ONE));
        position = runEnd;
    }
    return ranks;
}

napi_value RankData(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    return CreateDataArrayValue(env, ComputeRanksOf(values));
}

napi_value WeightedVariance(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, weights");

    std::vector<double> values;
    std::vector<double> weights;
    NAPI_ASSERT(env, ExtractTwoArraysSameSize(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], values, weights),
                "Array extraction failed");

    double weightTotal = SumValue(weights);
    NAPI_ASSERT(env, weightTotal > 0.0, "Weights sum must be positive");

    double weightedMean = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        weightedMean += weights[i] * values[i];
    }
    weightedMean /= weightTotal;

    double spread = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        double deviation = values[i] - weightedMean;
        spread += weights[i] * deviation * deviation;
    }
    return CreateDoubleValue(env, spread / weightTotal);
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("varianceOfGroup", VarianceOfGroup),
        DECLARE_NAPI_FUNCTION("covarianceBetweenPearson", CovarianceBetweenPearson),
        DECLARE_NAPI_FUNCTION("pearsonCorrelation", PearsonCorrelation),
        DECLARE_NAPI_FUNCTION("rankData", RankData),
        DECLARE_NAPI_FUNCTION("weightedVariance", WeightedVariance),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterStatsExtendedFunctions(env, exports), nullptr);
    return exports;
}

static napi_module statsExtendedSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "statsExtendedSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void StatsExtendedSuiteRegisterModule(void)
{
    napi_module_register(&statsExtendedSuiteModule);
}
