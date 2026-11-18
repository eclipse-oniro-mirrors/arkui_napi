/**
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

#include "fitness_suite_common.h"
#include "napi/native_node_api.h"

// Checks every element of an array against an inclusive range and throws a descriptive error.
static bool ValidateArrayInRange(napi_env env,
                                 const std::vector<double>& values,
                                 double minValue,
                                 double maxValue,
                                 const char* message)
{
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < minValue || values[i] > maxValue) {
            napi_throw_error(env, nullptr, message);
            return false;
        }
    }
    return true;
}

napi_value WorkoutVolume(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightsArray, repsArray");

    std::vector<double> weights;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], weights)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, weights, FITNESS_MIN_WEIGHT_LIFTED_KG, FITNESS_MAX_WEIGHT_LIFTED_KG,
                              "Weight lifted out of range")) {
        return nullptr;
    }

    std::vector<double> reps;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ONE], reps)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, reps, static_cast<double>(FITNESS_MIN_REPS), static_cast<double>(FITNESS_MAX_REPS),
                              "Reps out of range")) {
        return nullptr;
    }

    NAPI_ASSERT(env, weights.size() == reps.size(), "Weights and reps arrays must have equal lengths");
    NAPI_ASSERT(env, !weights.empty(), "Weights array must not be empty");

    double totalVolumeKg = 0.0;
    for (size_t i = 0; i < weights.size(); ++i) {
        totalVolumeKg += weights[i] * reps[i];
    }
    return CreateDoubleValue(env, totalVolumeKg);
}

napi_value HeaviestSet(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: weightsArray");

    std::vector<double> weights;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], weights)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, weights, FITNESS_MIN_WEIGHT_LIFTED_KG, FITNESS_MAX_WEIGHT_LIFTED_KG,
                              "Weight lifted out of range")) {
        return nullptr;
    }
    NAPI_ASSERT(env, !weights.empty(), "Weights array must not be empty");

    double heaviestKg = weights[FITNESS_ARRAY_INDEX_FIRST];
    for (size_t i = 0; i < weights.size(); ++i) {
        heaviestKg = std::max(heaviestKg, weights[i]);
    }
    return CreateDoubleValue(env, heaviestKg);
}

napi_value TotalReps(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: repsArray");

    std::vector<double> reps;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], reps)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, reps, static_cast<double>(FITNESS_MIN_REPS), static_cast<double>(FITNESS_MAX_REPS),
                              "Reps out of range")) {
        return nullptr;
    }

    double totalReps = 0.0;
    for (size_t i = 0; i < reps.size(); ++i) {
        totalReps += reps[i];
    }
    return CreateInt32Value(env, static_cast<int32_t>(totalReps));
}

napi_value AverageWeightLifted(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: weightsArray");

    std::vector<double> weights;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], weights)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, weights, FITNESS_MIN_WEIGHT_LIFTED_KG, FITNESS_MAX_WEIGHT_LIFTED_KG,
                              "Weight lifted out of range")) {
        return nullptr;
    }

    double totalKg = 0.0;
    for (size_t i = 0; i < weights.size(); ++i) {
        totalKg += weights[i];
    }
    double safeSize = static_cast<double>(weights.size()) > 0.0 ? static_cast<double>(weights.size()) : 1.0;
    return CreateDoubleValue(env, totalKg / safeSize);
}

napi_value BestOneRepMaxEstimate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightsArray, repsArray");

    std::vector<double> weights;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], weights)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, weights, FITNESS_MIN_WEIGHT_LIFTED_KG, FITNESS_MAX_WEIGHT_LIFTED_KG,
                              "Weight lifted out of range")) {
        return nullptr;
    }

    std::vector<double> reps;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ONE], reps)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, reps, static_cast<double>(FITNESS_MIN_REPS), static_cast<double>(FITNESS_MAX_REPS),
                              "Reps out of range")) {
        return nullptr;
    }

    NAPI_ASSERT(env, weights.size() == reps.size(), "Weights and reps arrays must have equal lengths");
    NAPI_ASSERT(env, !weights.empty(), "Weights array must not be empty");

    double bestOneRepMaxKg = 0.0;
    for (size_t i = 0; i < weights.size(); ++i) {
        double epleyEstimateKg = weights[i] * (FITNESS_PERCENT_DIVISOR + reps[i]) / FITNESS_EPLEY_REP_DIVISOR;
        bestOneRepMaxKg = std::max(bestOneRepMaxKg, epleyEstimateKg);
    }
    return CreateDoubleValue(env, bestOneRepMaxKg);
}

napi_value PlateLoad(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: totalKg, barKg");

    double totalKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], totalKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, totalKg >= FITNESS_MIN_WEIGHT_LIFTED_KG && totalKg <= FITNESS_MAX_WEIGHT_LIFTED_KG,
                "Total weight out of range");

    double barKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], barKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, barKg >= FITNESS_MIN_WEIGHT_LIFTED_KG && barKg <= FITNESS_MAX_WEIGHT_LIFTED_KG,
                "Bar weight out of range");

    NAPI_ASSERT(env, totalKg >= barKg, "Total weight must not be below the bar weight");

    // Splits the loadable weight into two sides and greedily fills plate types, largest first.
    // The epsilon keeps gram-level floating point noise from dropping a whole plate; the final
    // remainder that no plate combination can express is tracked and dropped.
    double sideLoadKg = (totalKg - barKg) / FITNESS_PLATE_SIDES;
    double remainingKg = sideLoadKg;
    std::vector<int64_t> counts(FITNESS_PLATE_TYPE_COUNT, 0);
    for (int i = 0; i < FITNESS_PLATE_TYPE_COUNT; ++i) {
        double plateWeightKg = FITNESS_PLATE_WEIGHTS_KG[i];
        double safePlateWeightKg = plateWeightKg > 0.0 ? plateWeightKg : 1.0;
        double units = (remainingKg + FITNESS_PLATE_EPSILON_KG) / safePlateWeightKg;
        int64_t count = static_cast<int64_t>(std::floor(units));
        if (count < 0) {
            count = 0;
        }
        counts[i] = count;
        remainingKg -= static_cast<double>(count) * plateWeightKg;
        if (remainingKg < 0.0) {
            remainingKg = 0.0;
        }
    }
    return CreateInt64ArrayValue(env, counts);
}

napi_value WarmupSets(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: workWeightKg");

    double workWeightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], workWeightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, workWeightKg >= FITNESS_MIN_WEIGHT_LIFTED_KG && workWeightKg <= FITNESS_MAX_WEIGHT_LIFTED_KG,
                "Work weight out of range");

    std::vector<double> warmupWeights;
    warmupWeights.push_back(workWeightKg * FITNESS_WARMUP_RATIO_ONE);
    warmupWeights.push_back(workWeightKg * FITNESS_WARMUP_RATIO_TWO);
    warmupWeights.push_back(workWeightKg * FITNESS_WARMUP_RATIO_THREE);
    return CreateDoubleArrayValue(env, warmupWeights);
}

napi_value IntervalTrainingTime(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: intervals, workSeconds, restSeconds");

    int32_t intervals = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], intervals)) {
        return nullptr;
    }
    NAPI_ASSERT(env, intervals >= FITNESS_MIN_INTERVALS && intervals <= FITNESS_MAX_INTERVALS,
                "Intervals out of range");

    double workSeconds = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], workSeconds)) {
        return nullptr;
    }
    NAPI_ASSERT(env, workSeconds >= FITNESS_MIN_SECONDS && workSeconds <= FITNESS_MAX_SECONDS,
                "Work seconds out of range");

    double restSeconds = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], restSeconds)) {
        return nullptr;
    }
    NAPI_ASSERT(env, restSeconds >= FITNESS_MIN_SECONDS && restSeconds <= FITNESS_MAX_SECONDS,
                "Rest seconds out of range");

    double totalSeconds = static_cast<double>(intervals) * (workSeconds + restSeconds);
    return CreateDoubleValue(env, totalSeconds);
}

napi_value SessionLoad(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: rpe, minutes");

    double rpe = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], rpe)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rpe >= FITNESS_MIN_RPE && rpe <= FITNESS_MAX_RPE, "RPE out of range");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, minutes >= FITNESS_MIN_MINUTES && minutes <= FITNESS_MAX_MINUTES, "Minutes out of range");

    return CreateDoubleValue(env, rpe * minutes);
}

napi_value WeeklyLoad(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: rpeArray, minutesArray");

    std::vector<double> rpes;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], rpes)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, rpes, FITNESS_MIN_RPE, FITNESS_MAX_RPE, "RPE out of range")) {
        return nullptr;
    }

    std::vector<double> minutes;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ONE], minutes)) {
        return nullptr;
    }
    if (!ValidateArrayInRange(env, minutes, FITNESS_MIN_MINUTES, FITNESS_MAX_MINUTES, "Minutes out of range")) {
        return nullptr;
    }

    NAPI_ASSERT(env, rpes.size() == minutes.size(), "RPE and minutes arrays must have equal lengths");

    double totalLoad = 0.0;
    for (size_t i = 0; i < rpes.size(); ++i) {
        totalLoad += rpes[i] * minutes[i];
    }
    return CreateDoubleValue(env, totalLoad);
}

napi_value AcuteChronicRatio(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: acuteLoad, chronicLoad");

    double acuteLoad = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], acuteLoad)) {
        return nullptr;
    }
    NAPI_ASSERT(env, acuteLoad >= FITNESS_MIN_TRAINING_LOAD && acuteLoad <= FITNESS_MAX_TRAINING_LOAD,
                "Acute load out of range");

    double chronicLoad = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], chronicLoad)) {
        return nullptr;
    }
    NAPI_ASSERT(env, chronicLoad >= FITNESS_MIN_TRAINING_LOAD && chronicLoad <= FITNESS_MAX_TRAINING_LOAD,
                "Chronic load out of range");
    NAPI_ASSERT(env, chronicLoad > 0.0, "Chronic load must be positive");

    double safeChronicLoad = chronicLoad > 0.0 ? chronicLoad : 1.0;
    return CreateDoubleValue(env, acuteLoad / safeChronicLoad);
}

napi_value StepsCadence(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: steps, minutes");

    int32_t steps = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], steps)) {
        return nullptr;
    }
    NAPI_ASSERT(env, steps >= FITNESS_MIN_STEPS && steps <= FITNESS_MAX_STEPS, "Steps out of range");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, minutes >= FITNESS_MIN_MINUTES && minutes <= FITNESS_MAX_MINUTES, "Minutes out of range");

    double safeMinutes = minutes > 0.0 ? minutes : 1.0;
    double cadence = static_cast<double>(steps) / safeMinutes;
    return CreateDoubleValue(env, cadence);
}

napi_value DistanceToSteps(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: distanceKm, heightCm");

    double distanceKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], distanceKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, distanceKm >= FITNESS_MIN_DISTANCE_KM && distanceKm <= FITNESS_MAX_DISTANCE_KM,
                "Distance out of range");

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    double strideCm = heightCm * FITNESS_STRIDE_HEIGHT_RATIO;
    double safeStrideCm = strideCm > 0.0 ? strideCm : 1.0;
    double rawSteps = distanceKm * FITNESS_CM_PER_KM / safeStrideCm;
    return CreateInt32Value(env, static_cast<int32_t>(rawSteps));
}

napi_value HikingEffort(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: distanceKm, elevationGainM");

    double distanceKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], distanceKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, distanceKm >= FITNESS_MIN_DISTANCE_KM && distanceKm <= FITNESS_MAX_DISTANCE_KM,
                "Distance out of range");

    double elevationGainM = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], elevationGainM)) {
        return nullptr;
    }
    NAPI_ASSERT(env, elevationGainM >= FITNESS_MIN_ELEVATION_M && elevationGainM <= FITNESS_MAX_ELEVATION_M,
                "Elevation gain out of range");

    double effortScore = distanceKm + elevationGainM / FITNESS_HIKING_ELEVATION_DIVISOR;
    return CreateDoubleValue(env, effortScore);
}

napi_value TreadmillEquivalentPace(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: speedKmh, inclinePercent");

    double speedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], speedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, speedKmh >= FITNESS_MIN_SPEED_KMH && speedKmh <= FITNESS_MAX_SPEED_KMH, "Speed out of range");

    double inclinePercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], inclinePercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, inclinePercent >= FITNESS_MIN_INCLINE_PERCENT && inclinePercent <= FITNESS_MAX_INCLINE_PERCENT,
                "Incline percent out of range");

    double flatEquivalentKmh =
        speedKmh * (FITNESS_TREADMILL_INCLINE_BASE + inclinePercent / FITNESS_TREADMILL_INCLINE_DIVISOR);
    return CreateDoubleValue(env, flatEquivalentKmh);
}

napi_value CyclingCalories(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: watts, minutes");

    double watts = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], watts)) {
        return nullptr;
    }
    NAPI_ASSERT(env, watts >= FITNESS_MIN_WATTS && watts <= FITNESS_MAX_WATTS, "Watts out of range");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, minutes >= FITNESS_MIN_MINUTES && minutes <= FITNESS_MAX_MINUTES, "Minutes out of range");

    double energyJoules = watts * minutes * FITNESS_SECONDS_PER_MINUTE;
    double kilocalories = energyJoules / FITNESS_JOULES_PER_KCAL;
    return CreateDoubleValue(env, kilocalories);
}

napi_value PowerToWeight(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: watts, weightKg");

    double watts = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], watts)) {
        return nullptr;
    }
    NAPI_ASSERT(env, watts >= FITNESS_MIN_WATTS && watts <= FITNESS_MAX_WATTS, "Watts out of range");

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double safeWeightKg = weightKg > 0.0 ? weightKg : 1.0;
    return CreateDoubleValue(env, watts / safeWeightKg);
}

napi_value FunctionalThresholdPower(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: twentyMinutePowerWatts");

    double twentyMinutePowerWatts = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], twentyMinutePowerWatts)) {
        return nullptr;
    }
    NAPI_ASSERT(env, twentyMinutePowerWatts >= FITNESS_MIN_WATTS && twentyMinutePowerWatts <= FITNESS_MAX_WATTS,
                "Power out of range");

    return CreateDoubleValue(env, twentyMinutePowerWatts * FITNESS_FTP_RATIO);
}

napi_value SwimLapsToKm(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: laps, poolLengthM");

    int32_t laps = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], laps)) {
        return nullptr;
    }
    NAPI_ASSERT(env, laps >= FITNESS_MIN_LAPS && laps <= FITNESS_MAX_LAPS, "Laps out of range");

    double poolLengthM = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], poolLengthM)) {
        return nullptr;
    }
    NAPI_ASSERT(env, poolLengthM >= FITNESS_MIN_POOL_LENGTH_M && poolLengthM <= FITNESS_MAX_POOL_LENGTH_M,
                "Pool length out of range");

    double distanceKm = static_cast<double>(laps) * poolLengthM / FITNESS_METER_PER_KM;
    return CreateDoubleValue(env, distanceKm);
}

napi_value TriathlonTotalTime(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: swimMinutes, bikeMinutes, runMinutes");

    double swimMinutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], swimMinutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, swimMinutes >= FITNESS_MIN_MINUTES && swimMinutes <= FITNESS_MAX_MINUTES,
                "Swim minutes out of range");

    double bikeMinutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], bikeMinutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bikeMinutes >= FITNESS_MIN_MINUTES && bikeMinutes <= FITNESS_MAX_MINUTES,
                "Bike minutes out of range");

    double runMinutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], runMinutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, runMinutes >= FITNESS_MIN_MINUTES && runMinutes <= FITNESS_MAX_MINUTES,
                "Run minutes out of range");

    double totalMinutes = swimMinutes + bikeMinutes + runMinutes + FITNESS_TRANSITION_MINUTES;
    return CreateDoubleValue(env, totalMinutes);
}

napi_value RecoveryHeartRate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE,
                "Requires 3 arguments: restingHeartRate, peakHeartRate, afterOneMinuteHeartRate");

    double restingHeartRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], restingHeartRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, restingHeartRate >= FITNESS_MIN_HEART_RATE_BPM && restingHeartRate <= FITNESS_MAX_HEART_RATE_BPM,
                "Resting heart rate out of range");

    double peakHeartRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], peakHeartRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, peakHeartRate >= FITNESS_MIN_HEART_RATE_BPM && peakHeartRate <= FITNESS_MAX_HEART_RATE_BPM,
                "Peak heart rate out of range");

    double afterOneMinuteHeartRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], afterOneMinuteHeartRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env,
                afterOneMinuteHeartRate >= FITNESS_MIN_HEART_RATE_BPM &&
                    afterOneMinuteHeartRate <= FITNESS_MAX_HEART_RATE_BPM,
                "After-exercise heart rate out of range");

    NAPI_ASSERT(env, peakHeartRate >= restingHeartRate, "Peak heart rate must not be below resting heart rate");
    NAPI_ASSERT(env, afterOneMinuteHeartRate >= restingHeartRate && afterOneMinuteHeartRate <= peakHeartRate,
                "After-exercise heart rate must sit between resting and peak heart rate");

    double dropBpm = peakHeartRate - afterOneMinuteHeartRate;
    return CreateDoubleValue(env, dropBpm);
}

napi_value Vo2maxCooper(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: distanceMeters12Min");

    double distanceMeters12Min = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], distanceMeters12Min)) {
        return nullptr;
    }
    NAPI_ASSERT(env,
                distanceMeters12Min >= FITNESS_MIN_COOPER_DISTANCE_M &&
                    distanceMeters12Min <= FITNESS_MAX_COOPER_DISTANCE_M,
                "Cooper test distance out of range");

    double vo2max = (distanceMeters12Min - FITNESS_COOPER_OFFSET_METERS) / FITNESS_COOPER_DIVISOR;
    return CreateDoubleValue(env, vo2max);
}

// Predicts race duration from VO2max. Formula choice: the ACSM running metabolic equation
// VO2 = FITNESS_ACSM_SPEED_COEFF * v + FITNESS_VO2_REST_ML_PER_KG (speed v in m/min) is solved for
// the sustainable speed, then time = distance / speed. This linear cost model is preferred over a
// plain VO2max-per-MET lookup because it yields realistic running velocities directly.
static double RaceTimeFromVo2maxValue(double vo2max, double distanceKm)
{
    double speedMetersPerMinute = (vo2max - FITNESS_VO2_REST_ML_PER_KG) / FITNESS_ACSM_SPEED_COEFF;
    double safeSpeed = speedMetersPerMinute > 0.0 ? speedMetersPerMinute : 1.0;
    return distanceKm * FITNESS_METER_PER_KM / safeSpeed;
}

napi_value RaceTimeFromVo2max(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: vo2max, distanceKm");

    double vo2max = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], vo2max)) {
        return nullptr;
    }
    NAPI_ASSERT(env, vo2max >= FITNESS_MIN_VO2MAX && vo2max <= FITNESS_MAX_VO2MAX, "VO2max out of range");

    double distanceKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], distanceKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, distanceKm >= FITNESS_MIN_DISTANCE_KM && distanceKm <= FITNESS_MAX_DISTANCE_KM,
                "Distance out of range");

    return CreateDoubleValue(env, RaceTimeFromVo2maxValue(vo2max, distanceKm));
}

napi_status RegisterFitnessOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("workoutVolume", WorkoutVolume),
        DECLARE_NAPI_FUNCTION("heaviestSet", HeaviestSet),
        DECLARE_NAPI_FUNCTION("totalReps", TotalReps),
        DECLARE_NAPI_FUNCTION("averageWeightLifted", AverageWeightLifted),
        DECLARE_NAPI_FUNCTION("bestOneRepMaxEstimate", BestOneRepMaxEstimate),
        DECLARE_NAPI_FUNCTION("plateLoad", PlateLoad),
        DECLARE_NAPI_FUNCTION("warmupSets", WarmupSets),
        DECLARE_NAPI_FUNCTION("intervalTrainingTime", IntervalTrainingTime),
        DECLARE_NAPI_FUNCTION("sessionLoad", SessionLoad),
        DECLARE_NAPI_FUNCTION("weeklyLoad", WeeklyLoad),
        DECLARE_NAPI_FUNCTION("acuteChronicRatio", AcuteChronicRatio),
        DECLARE_NAPI_FUNCTION("stepsCadence", StepsCadence),
        DECLARE_NAPI_FUNCTION("distanceToSteps", DistanceToSteps),
        DECLARE_NAPI_FUNCTION("hikingEffort", HikingEffort),
        DECLARE_NAPI_FUNCTION("treadmillEquivalentPace", TreadmillEquivalentPace),
        DECLARE_NAPI_FUNCTION("cyclingCalories", CyclingCalories),
        DECLARE_NAPI_FUNCTION("powerToWeight", PowerToWeight),
        DECLARE_NAPI_FUNCTION("functionalThresholdPower", FunctionalThresholdPower),
        DECLARE_NAPI_FUNCTION("swimLapsToKm", SwimLapsToKm),
        DECLARE_NAPI_FUNCTION("triathlonTotalTime", TriathlonTotalTime),
        DECLARE_NAPI_FUNCTION("recoveryHeartRate", RecoveryHeartRate),
        DECLARE_NAPI_FUNCTION("vo2maxCooper", Vo2maxCooper),
        DECLARE_NAPI_FUNCTION("raceTimeFromVo2max", RaceTimeFromVo2max),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
