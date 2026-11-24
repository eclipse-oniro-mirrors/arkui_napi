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

#include "stats_extended_suite_common.h"

static bool TrimBothSides(const std::vector<double>& values, double ratio, std::vector<double>& trimmed)
{
    std::vector<double> sorted = SortedCopyOf(values);
    size_t trimCount = static_cast<size_t>(static_cast<double>(sorted.size()) * ratio);
    if (trimCount * INDEX_TWO >= sorted.size()) {
        return false;
    }
    trimmed.assign(sorted.begin() + static_cast<ptrdiff_t>(trimCount),
                   sorted.end() - static_cast<ptrdiff_t>(trimCount));
    return true;
}

napi_value TrimmedVariance(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, trimRatio");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double ratio = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], ratio), "Ratio extraction failed");
    NAPI_ASSERT(env, ratio >= TRIM_RATIO_MIN && ratio <= TRIM_RATIO_MAX, "Trim ratio must be within [0, 0.45]");

    std::vector<double> trimmed;
    NAPI_ASSERT(env, TrimBothSides(values, ratio, trimmed), "Trim removes every sample");
    NAPI_ASSERT(env, trimmed.size() >= MIN_SAMPLES_FOR_VARIANCE, "Too few samples remain after trimming");

    return CreateDoubleValue(env, PopulationVarianceValue(trimmed));
}

napi_value PooledVariance(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    std::vector<double> right;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], left), "First group extraction failed");
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ONE], right), "Second group extraction failed");
    NAPI_ASSERT(env, left.size() >= MIN_SAMPLES_FOR_VARIANCE && right.size() >= MIN_SAMPLES_FOR_VARIANCE,
                "Each group needs at least two samples");

    double leftCount = static_cast<double>(left.size());
    double rightCount = static_cast<double>(right.size());
    double leftSpread = DeviationSumSquare(left, MeanValue(left));
    double rightSpread = DeviationSumSquare(right, MeanValue(right));
    double degreesOfFreedom = leftCount + rightCount - MIDPOINT_DIVISOR;

    double pooled = (leftSpread + rightSpread) / degreesOfFreedom;
    return CreateDoubleValue(env, pooled);
}

napi_value RunningMean(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    std::vector<double> means;
    means.reserve(values.size());
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        total += values[i];
        means.push_back(total / static_cast<double>(i + INDEX_ONE));
    }
    return CreateDataArrayValue(env, means);
}

napi_value ExponentialMovingAverage(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, alpha");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double alpha = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], alpha), "Alpha extraction failed");
    NAPI_ASSERT(env, alpha >= EMA_ALPHA_MIN && alpha <= EMA_ALPHA_MAX, "Alpha must be within [0, 1]");

    std::vector<double> averages;
    averages.reserve(values.size());
    double previous = values[INDEX_ZERO];
    averages.push_back(previous);
    for (size_t i = INDEX_ONE; i < values.size(); ++i) {
        previous = alpha * values[i] + (EMA_ALPHA_MAX - alpha) * previous;
        averages.push_back(previous);
    }
    return CreateDataArrayValue(env, averages);
}

napi_value CumulativeProduct(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    std::vector<double> products;
    products.reserve(values.size());
    double running = LOG_SHIFT_UNIT;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        running *= values[i];
        products.push_back(running);
    }
    return CreateDataArrayValue(env, products);
}

napi_value PartialSums(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    std::vector<double> sums;
    sums.reserve(values.size());
    double running = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        running += values[i];
        sums.push_back(running);
    }
    return CreateDataArrayValue(env, sums);
}

napi_value SecondDifferences(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");
    NAPI_ASSERT(env, values.size() >= static_cast<size_t>(MIN_SAMPLES_FOR_VARIANCE) + INDEX_ONE,
                "At least three values are required");

    std::vector<double> differences;
    differences.reserve(values.size() - MIN_SAMPLES_FOR_VARIANCE);
    for (size_t i = INDEX_TWO; i < values.size(); ++i) {
        differences.push_back(values[i] - MULTIPLIER_TWO * values[i - INDEX_ONE] + values[i - INDEX_TWO]);
    }
    return CreateDataArrayValue(env, differences);
}

napi_value SafeLogTransform(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    std::vector<double> transformed;
    transformed.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        transformed.push_back(std::log(values[i] + LOG_SHIFT_UNIT));
    }
    return CreateDataArrayValue(env, transformed);
}

napi_value PowerTransform(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, exponent");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double exponent = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], exponent), "Exponent extraction failed");
    NAPI_ASSERT(env, exponent >= POWER_EXPONENT_MIN && exponent <= POWER_EXPONENT_MAX,
                "Exponent out of supported range");

    std::vector<double> transformed;
    transformed.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        NAPI_ASSERT(env, values[i] > LOG_FLOOR_VALUE || exponent == EXPONENT_LINEAR,
                    "Negative base requires integer-friendly exponent");
        transformed.push_back(std::pow(values[i], exponent));
    }
    return CreateDataArrayValue(env, transformed);
}

napi_value InterquartileSpreadValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    std::vector<double> sorted = SortedCopyOf(values);
    size_t halfSize = sorted.size() / static_cast<size_t>(MIDPOINT_DIVISOR);
    size_t upperStart = (sorted.size() + INDEX_ONE) / static_cast<size_t>(MIDPOINT_DIVISOR);
    std::vector<double> lowerHalf(sorted.begin(), sorted.begin() + static_cast<ptrdiff_t>(halfSize));
    std::vector<double> upperHalf(sorted.begin() + static_cast<ptrdiff_t>(upperStart), sorted.end());
    return CreateDoubleValue(env, MedianValue(upperHalf) - MedianValue(lowerHalf));
}

napi_value StandardErrorOfMeanValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");
    NAPI_ASSERT(env, values.size() >= MIN_SAMPLES_FOR_VARIANCE, "At least two samples are required");

    double spread = PopulationVarianceValue(values);
    double standardError = std::sqrt(spread / static_cast<double>(values.size()));
    return CreateDoubleValue(env, standardError);
}

napi_value GeometricMeanPositive(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double logTotal = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        NAPI_ASSERT(env, values[i] > LOG_FLOOR_VALUE, "All values must be positive");
        logTotal += std::log(values[i]);
    }

    double result = std::exp(logTotal / static_cast<double>(values.size()));
    return CreateDoubleValue(env, result);
}

napi_value SpearmanCorrelation(napi_env env, napi_callback_info info)
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

    std::vector<double> leftRanks = ComputeRanksOf(left);
    std::vector<double> rightRanks = ComputeRanksOf(right);
    double denominator = std::sqrt(PopulationVarianceValue(leftRanks) * PopulationVarianceValue(rightRanks));
    NAPI_ASSERT(env, denominator > 0.0, "Constant input has undefined correlation");
    return CreateDoubleValue(env, CovarianceBetweenRanks(leftRanks, rightRanks) / denominator);
}

napi_value DeviationsFromMean(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double mean = MeanValue(values);
    std::vector<double> deviations;
    deviations.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        deviations.push_back(values[i] - mean);
    }
    return CreateDataArrayValue(env, deviations);
}

napi_value NormalizedByRange(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double span = RangeSpanValue(values);
    NAPI_ASSERT(env, span > 0.0, "All values are equal; range is zero");

    std::vector<double> sorted = SortedCopyOf(values);
    double smallest = sorted[INDEX_ZERO];
    std::vector<double> normalized;
    normalized.reserve(values.size());
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        normalized.push_back((values[i] - smallest) / span);
    }
    return CreateDataArrayValue(env, normalized);
}

napi_value MovingSumWindow(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: values, window");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double rawWindow = 0.0;
    NAPI_ASSERT(env, ExtractDoubleArg(env, argv[ARG_INDEX_ONE], rawWindow), "Window extraction failed");
    NAPI_ASSERT(env, rawWindow >= RANK_FIRST_POSITION && rawWindow <= static_cast<double>(values.size()),
                "Window out of range");

    size_t window = static_cast<size_t>(rawWindow);
    std::vector<double> sums;
    sums.reserve(values.size() - window + INDEX_ONE);
    double running = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        running += values[i];
        if (i >= window) {
            running -= values[i - window];
        }
        if (i + INDEX_ONE >= window) {
            sums.push_back(running);
        }
    }
    return CreateDataArrayValue(env, sums);
}

napi_value SumOfSquaredDeviations(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    return CreateDoubleValue(env, DeviationSumSquare(values, MeanValue(values)));
}

napi_value MeanAbsoluteDeviationValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: values");

    std::vector<double> values;
    NAPI_ASSERT(env, ExtractDataArray(env, argv[ARG_INDEX_ZERO], values), "Values extraction failed");

    double median = MedianValue(values);
    double total = 0.0;
    for (size_t i = EMPTY_RESULT_SIZE; i < values.size(); ++i) {
        total += std::fabs(values[i] - median);
    }
    return CreateDoubleValue(env, total / static_cast<double>(values.size()));
}

napi_status RegisterStatsExtendedFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("spearmanCorrelation", SpearmanCorrelation),
        DECLARE_NAPI_FUNCTION("deviationsFromMean", DeviationsFromMean),
        DECLARE_NAPI_FUNCTION("normalizedByRange", NormalizedByRange),
        DECLARE_NAPI_FUNCTION("movingSumWindow", MovingSumWindow),
        DECLARE_NAPI_FUNCTION("sumOfSquaredDeviations", SumOfSquaredDeviations),
        DECLARE_NAPI_FUNCTION("meanAbsoluteDeviationValue", MeanAbsoluteDeviationValue),
        DECLARE_NAPI_FUNCTION("interquartileSpreadValue", InterquartileSpreadValue),
        DECLARE_NAPI_FUNCTION("standardErrorOfMeanValue", StandardErrorOfMeanValue),
        DECLARE_NAPI_FUNCTION("geometricMeanPositive", GeometricMeanPositive),
        DECLARE_NAPI_FUNCTION("trimmedVariance", TrimmedVariance),
        DECLARE_NAPI_FUNCTION("pooledVariance", PooledVariance),
        DECLARE_NAPI_FUNCTION("runningMean", RunningMean),
        DECLARE_NAPI_FUNCTION("exponentialMovingAverage", ExponentialMovingAverage),
        DECLARE_NAPI_FUNCTION("cumulativeProduct", CumulativeProduct),
        DECLARE_NAPI_FUNCTION("partialSums", PartialSums),
        DECLARE_NAPI_FUNCTION("secondDifferences", SecondDifferences),
        DECLARE_NAPI_FUNCTION("safeLogTransform", SafeLogTransform),
        DECLARE_NAPI_FUNCTION("powerTransform", PowerTransform),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
