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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "fitness_suite_common.h"
#include "napi/native_node_api.h"

bool ExtractDoubleArg(napi_env env, napi_value value, double& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    double number = 0.0;
    NAPI_CALL_BASE(env, napi_get_value_double(env, value, &number), false);
    if (!std::isfinite(number)) {
        napi_throw_error(env, nullptr, "Argument must be a finite number");
        return false;
    }

    result = number;
    return true;
}

bool ExtractInt32Arg(napi_env env, napi_value value, int32_t& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    int32_t number = 0;
    NAPI_CALL_BASE(env, napi_get_value_int32(env, value, &number), false);
    result = number;
    return true;
}

bool ExtractStringArg(napi_env env, napi_value value, std::string& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_string) {
        napi_throw_error(env, nullptr, "Argument must be a string");
        return false;
    }

    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, nullptr, 0, &length), false);
    if (length > FITNESS_MAX_TEXT_LENGTH) {
        napi_throw_error(env, nullptr, "String exceeds maximum length");
        return false;
    }

    result.assign(length + FITNESS_STRING_NUL_SIZE, '\0');
    size_t copied = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, &result[0], length + FITNESS_STRING_NUL_SIZE, &copied),
                   false);
    result.resize(copied);
    return true;
}

static char LowerAscii(char c)
{
    if (c >= FITNESS_CHAR_UPPER_A && c <= FITNESS_CHAR_UPPER_Z) {
        return static_cast<char>(c + FITNESS_CHAR_CASE_DELTA);
    }
    return c;
}

static std::string LowerCopy(const std::string& input)
{
    std::string result = input;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = LowerAscii(result[i]);
    }
    return result;
}

bool ExtractGenderArg(napi_env env, napi_value value, int& result)
{
    std::string text;
    if (!ExtractStringArg(env, value, text)) {
        return false;
    }
    if (text.size() > FITNESS_GENDER_TEXT_MAX_LENGTH) {
        napi_throw_error(env, nullptr, "Gender text exceeds maximum length");
        return false;
    }

    std::string lowered = LowerCopy(text);
    if (lowered == FITNESS_GENDER_MALE_TEXT) {
        result = FITNESS_GENDER_MALE;
        return true;
    }
    if (lowered == FITNESS_GENDER_FEMALE_TEXT) {
        result = FITNESS_GENDER_FEMALE;
        return true;
    }
    napi_throw_error(env, nullptr, "Gender must be male or female");
    return false;
}

bool ExtractDoubleArray(napi_env env, napi_value value, std::vector<double>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > FITNESS_MAX_ARRAY_LENGTH) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = FITNESS_UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        double number = 0.0;
        if (!ExtractDoubleArg(env, element, number)) {
            return false;
        }
        result.push_back(number);
    }
    return true;
}

napi_value CreateDoubleValue(napi_env env, double value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_double(env, value, &result));
    return result;
}

napi_value CreateInt32Value(napi_env env, int32_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, value, &result));
    return result;
}

napi_value CreateBoolValue(napi_env env, bool value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result));
    return result;
}

napi_value CreateStringValue(napi_env env, const std::string& text)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, text.c_str(), text.size(), &result));
    return result;
}

napi_value CreateDoubleArrayValue(napi_env env, const std::vector<double>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateDoubleValue(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = nullptr;
        NAPI_CALL(env, napi_create_int64(env, values[i], &element));
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

static double MetersFromCm(double heightCm)
{
    return heightCm / FITNESS_HEIGHT_CM_PER_METER;
}

// Computes body mass index in kg/m2 from weight in kg and height in cm.
static double BmiValue(double weightKg, double heightCm)
{
    double heightM = MetersFromCm(heightCm);
    double safeHeightM = heightM > 0.0 ? heightM : 1.0;
    return weightKg / (safeHeightM * safeHeightM);
}

napi_value Bmi(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightKg, heightCm");

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    return CreateDoubleValue(env, BmiValue(weightKg, heightCm));
}

// Maps a body mass index to its WHO category name.
static std::string BmiCategoryFor(double bmi)
{
    if (bmi < FITNESS_BMI_UNDERWEIGHT_MAX) {
        return FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORY_INDEX_UNDERWEIGHT];
    }
    if (bmi < FITNESS_BMI_NORMAL_MAX) {
        return FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORY_INDEX_NORMAL];
    }
    if (bmi < FITNESS_BMI_OVERWEIGHT_MAX) {
        return FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORY_INDEX_OVERWEIGHT];
    }
    if (bmi < FITNESS_BMI_OBESE_CLASS_I_MAX) {
        return FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORY_INDEX_OBESE_I];
    }
    if (bmi < FITNESS_BMI_OBESE_CLASS_II_MAX) {
        return FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORY_INDEX_OBESE_II];
    }
    return FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORY_INDEX_OBESE_III];
}

napi_value BmiCategory(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: bmi");

    double bmi = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bmi)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bmi >= FITNESS_MIN_BMI && bmi <= FITNESS_MAX_BMI, "BMI out of range");

    return CreateStringValue(env, BmiCategoryFor(bmi));
}

napi_value HealthyWeightRange(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: heightCm");

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    double heightM = MetersFromCm(heightCm);
    std::vector<double> range;
    range.push_back(FITNESS_HEALTHY_BMI_MIN * heightM * heightM);
    range.push_back(FITNESS_HEALTHY_BMI_MAX * heightM * heightM);
    return CreateDoubleArrayValue(env, range);
}

// Computes basal metabolic rate with the Mifflin-St Jeor equation.
static double BmrMifflinValue(int gender, double weightKg, double heightCm, double ageYears)
{
    double base = FITNESS_MIFFLIN_WEIGHT_COEFF * weightKg + FITNESS_MIFFLIN_HEIGHT_COEFF * heightCm -
                  FITNESS_MIFFLIN_AGE_COEFF * ageYears;
    double offset = gender == FITNESS_GENDER_MALE ? FITNESS_MIFFLIN_MALE_OFFSET : FITNESS_MIFFLIN_FEMALE_OFFSET;
    return base + offset;
}

napi_value BmrMifflin(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FOUR;
    napi_value argv[REQUIRED_ARGS_FOUR] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FOUR, "Requires 4 arguments: gender, weightKg, heightCm, age");

    int gender = FITNESS_GENDER_MALE;
    if (!ExtractGenderArg(env, argv[ARG_INDEX_ZERO], gender)) {
        return nullptr;
    }

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    double ageYears = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_THREE], ageYears)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ageYears >= FITNESS_MIN_AGE_YEARS && ageYears <= FITNESS_MAX_AGE_YEARS, "Age out of range");

    return CreateDoubleValue(env, BmrMifflinValue(gender, weightKg, heightCm, ageYears));
}

// Computes basal metabolic rate with the Katch-McArdle equation from lean mass.
static double BmrKatchMcArdleValue(double weightKg, double bodyFatPercent)
{
    double leanMassKg = weightKg * (FITNESS_PERCENT_DIVISOR - bodyFatPercent) / FITNESS_PERCENT_DIVISOR;
    return FITNESS_KATCH_BASE_KCAL + FITNESS_KATCH_LEAN_COEFF * leanMassKg;
}

napi_value BmrKatchMcArdle(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightKg, bodyFatPercent");

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double bodyFatPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], bodyFatPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bodyFatPercent >= FITNESS_MIN_BODY_FAT_PERCENT && bodyFatPercent <= FITNESS_MAX_BODY_FAT_PERCENT,
                "Body fat percent out of range");

    return CreateDoubleValue(env, BmrKatchMcArdleValue(weightKg, bodyFatPercent));
}

napi_value Tdee(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: bmr, tdeeMultiplier");

    double bmr = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bmr)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bmr >= FITNESS_MIN_BMR && bmr <= FITNESS_MAX_BMR, "BMR out of range");

    double tdeeMultiplier = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], tdeeMultiplier)) {
        return nullptr;
    }
    NAPI_ASSERT(env, tdeeMultiplier >= FITNESS_MIN_TDEE_MULTIPLIER && tdeeMultiplier <= FITNESS_MAX_TDEE_MULTIPLIER,
                "TDEE multiplier out of range");

    return CreateDoubleValue(env, bmr * tdeeMultiplier);
}

napi_value MaxHeartRate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: age");

    double ageYears = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], ageYears)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ageYears >= FITNESS_MIN_AGE_YEARS && ageYears <= FITNESS_MAX_AGE_YEARS, "Age out of range");

    return CreateDoubleValue(env, FITNESS_MAX_HR_AGE_BASE - ageYears);
}

// Computes a target heart rate with the Karvonen formula.
static double TargetHeartRateValue(double ageYears, double restingHeartRate, double intensityPercent)
{
    double reserve = FITNESS_MAX_HR_AGE_BASE - ageYears - restingHeartRate;
    return restingHeartRate + reserve * intensityPercent / FITNESS_PERCENT_DIVISOR;
}

napi_value TargetHeartRate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: age, restingHeartRate, intensityPercent");

    double ageYears = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], ageYears)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ageYears >= FITNESS_MIN_AGE_YEARS && ageYears <= FITNESS_MAX_AGE_YEARS, "Age out of range");

    double restingHeartRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], restingHeartRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, restingHeartRate >= FITNESS_MIN_HEART_RATE_BPM && restingHeartRate <= FITNESS_MAX_HEART_RATE_BPM,
                "Resting heart rate out of range");

    double intensityPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], intensityPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(
        env, intensityPercent >= FITNESS_KARVONEN_MIN_INTENSITY && intensityPercent <= FITNESS_KARVONEN_MAX_INTENSITY,
        "Intensity percent out of range");

    return CreateDoubleValue(env, TargetHeartRateValue(ageYears, restingHeartRate, intensityPercent));
}

napi_value HeartRateZone(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: age, restingHeartRate, zone");

    double ageYears = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], ageYears)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ageYears >= FITNESS_MIN_AGE_YEARS && ageYears <= FITNESS_MAX_AGE_YEARS, "Age out of range");

    double restingHeartRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], restingHeartRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, restingHeartRate >= FITNESS_MIN_HEART_RATE_BPM && restingHeartRate <= FITNESS_MAX_HEART_RATE_BPM,
                "Resting heart rate out of range");

    int32_t zone = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_TWO], zone)) {
        return nullptr;
    }
    NAPI_ASSERT(env, zone >= FITNESS_HR_ZONE_MIN && zone <= FITNESS_HR_ZONE_MAX, "Zone out of range");

    double lowPercent =
        FITNESS_HR_ZONE_START_PERCENT + FITNESS_HR_ZONE_SPAN_PERCENT * static_cast<double>(zone - FITNESS_HR_ZONE_MIN);
    double highPercent = lowPercent + FITNESS_HR_ZONE_SPAN_PERCENT;
    double lowBpm = TargetHeartRateValue(ageYears, restingHeartRate, lowPercent);
    double highBpm = TargetHeartRateValue(ageYears, restingHeartRate, highPercent);
    std::vector<double> bounds;
    bounds.push_back(lowBpm);
    bounds.push_back(highBpm);
    return CreateDoubleArrayValue(env, bounds);
}

// Estimates the one rep max with the Epley formula.
static double OneRepMaxEpleyValue(double weightLiftedKg, int32_t reps)
{
    return weightLiftedKg * (FITNESS_PERCENT_DIVISOR + static_cast<double>(reps)) / FITNESS_EPLEY_REP_DIVISOR;
}

napi_value OneRepMaxEpley(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightLiftedKg, reps");

    double weightLiftedKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], weightLiftedKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightLiftedKg >= FITNESS_MIN_WEIGHT_LIFTED_KG && weightLiftedKg <= FITNESS_MAX_WEIGHT_LIFTED_KG,
                "Weight lifted out of range");

    int32_t reps = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], reps)) {
        return nullptr;
    }
    NAPI_ASSERT(env, reps >= FITNESS_MIN_REPS && reps <= FITNESS_MAX_REPS, "Reps out of range");

    return CreateDoubleValue(env, OneRepMaxEpleyValue(weightLiftedKg, reps));
}

// Estimates the one rep max with the Brzycki formula.
static double OneRepMaxBrzyckiValue(double weightLiftedKg, int32_t reps)
{
    double denominator = FITNESS_BRZYCKI_DENOM_BASE - static_cast<double>(reps);
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    return weightLiftedKg * FITNESS_BRZYCKI_NUMERATOR / safeDenominator;
}

napi_value OneRepMaxBrzycki(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightLiftedKg, reps");

    double weightLiftedKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], weightLiftedKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightLiftedKg >= FITNESS_MIN_WEIGHT_LIFTED_KG && weightLiftedKg <= FITNESS_MAX_WEIGHT_LIFTED_KG,
                "Weight lifted out of range");

    int32_t reps = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], reps)) {
        return nullptr;
    }
    NAPI_ASSERT(env, reps >= FITNESS_MIN_REPS && reps <= FITNESS_MAX_REPS, "Reps out of range");

    return CreateDoubleValue(env, OneRepMaxBrzyckiValue(weightLiftedKg, reps));
}

// Inverts the Epley formula to find the load for a rep target.
static double WeightForRepsValue(double oneRepMaxKg, int32_t repTarget)
{
    double denominator = FITNESS_PERCENT_DIVISOR + static_cast<double>(repTarget);
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    return oneRepMaxKg * FITNESS_EPLEY_REP_DIVISOR / safeDenominator;
}

napi_value WeightForReps(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: oneRepMaxKg, repTarget");

    double oneRepMaxKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], oneRepMaxKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, oneRepMaxKg >= FITNESS_MIN_WEIGHT_LIFTED_KG && oneRepMaxKg <= FITNESS_MAX_WEIGHT_LIFTED_KG,
                "One rep max out of range");

    int32_t repTarget = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], repTarget)) {
        return nullptr;
    }
    NAPI_ASSERT(env, repTarget >= FITNESS_MIN_REPS && repTarget <= FITNESS_MAX_REPS, "Rep target out of range");

    return CreateDoubleValue(env, WeightForRepsValue(oneRepMaxKg, repTarget));
}

napi_value RunningPace(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: distanceKm, minutes");

    double distanceKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], distanceKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, distanceKm >= FITNESS_MIN_DISTANCE_KM && distanceKm <= FITNESS_MAX_DISTANCE_KM,
                "Distance out of range");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, minutes >= FITNESS_MIN_MINUTES && minutes <= FITNESS_MAX_MINUTES, "Minutes out of range");

    double safeDistanceKm = distanceKm > 0.0 ? distanceKm : 1.0;
    double secondsPerKm = minutes * FITNESS_SECONDS_PER_MINUTE / safeDistanceKm;
    return CreateDoubleValue(env, secondsPerKm);
}

napi_value PaceToSpeed(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: paceSecondsPerKm");

    double paceSecondsPerKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], paceSecondsPerKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, paceSecondsPerKm >= FITNESS_MIN_PACE_SECONDS && paceSecondsPerKm <= FITNESS_MAX_PACE_SECONDS,
                "Pace out of range");

    double safePaceSeconds = paceSecondsPerKm > 0.0 ? paceSecondsPerKm : 1.0;
    double speedKmh = FITNESS_SECONDS_PER_HOUR / safePaceSeconds;
    return CreateDoubleValue(env, speedKmh);
}

napi_value SpeedToPace(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speedKmh");

    double speedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], speedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, speedKmh >= FITNESS_MIN_SPEED_KMH && speedKmh <= FITNESS_MAX_SPEED_KMH, "Speed out of range");

    double safeSpeedKmh = speedKmh > 0.0 ? speedKmh : 1.0;
    double paceSecondsPerKm = FITNESS_SECONDS_PER_HOUR / safeSpeedKmh;
    return CreateDoubleValue(env, paceSecondsPerKm);
}

// Predicts race times with the Riegel endurance model.
static double RacePredictorValue(double fromDistanceKm, double fromMinutes, double toDistanceKm)
{
    double safeFromDistance = fromDistanceKm > 0.0 ? fromDistanceKm : 1.0;
    double distanceRatio = toDistanceKm / safeFromDistance;
    return fromMinutes * std::pow(distanceRatio, FITNESS_RIEGEL_EXPONENT);
}

napi_value RacePredictor(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: fromDistanceKm, fromMinutes, toDistanceKm");

    double fromDistanceKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], fromDistanceKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fromDistanceKm >= FITNESS_MIN_DISTANCE_KM && fromDistanceKm <= FITNESS_MAX_DISTANCE_KM,
                "From-distance out of range");

    double fromMinutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], fromMinutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fromMinutes >= FITNESS_MIN_MINUTES && fromMinutes <= FITNESS_MAX_MINUTES,
                "From-minutes out of range");

    double toDistanceKm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], toDistanceKm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, toDistanceKm >= FITNESS_MIN_DISTANCE_KM && toDistanceKm <= FITNESS_MAX_DISTANCE_KM,
                "To-distance out of range");

    return CreateDoubleValue(env, RacePredictorValue(fromDistanceKm, fromMinutes, toDistanceKm));
}

napi_value StepsToDistanceKm(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: steps, heightCm");

    int32_t steps = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], steps)) {
        return nullptr;
    }
    NAPI_ASSERT(env, steps >= FITNESS_MIN_STEPS && steps <= FITNESS_MAX_STEPS, "Steps out of range");

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    double strideCm = heightCm * FITNESS_STRIDE_HEIGHT_RATIO;
    double distanceKm = static_cast<double>(steps) * strideCm / FITNESS_CM_PER_KM;
    return CreateDoubleValue(env, distanceKm);
}

// Estimates body fat percent with the US Navy circumference method.
static double BodyFatNavyValue(int gender, double heightCm, double girthTermCm)
{
    double term = gender == FITNESS_GENDER_MALE ? FITNESS_NAVY_MALE_TERM : FITNESS_NAVY_FEMALE_TERM;
    double girthCoeff = gender == FITNESS_GENDER_MALE ? FITNESS_NAVY_MALE_GIRTH_COEFF : FITNESS_NAVY_FEMALE_GIRTH_COEFF;
    double heightCoeff =
        gender == FITNESS_GENDER_MALE ? FITNESS_NAVY_MALE_HEIGHT_COEFF : FITNESS_NAVY_FEMALE_HEIGHT_COEFF;
    double denominator = term - girthCoeff * std::log10(girthTermCm) + heightCoeff * std::log10(heightCm);
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    return FITNESS_NAVY_SCALE / safeDenominator - FITNESS_NAVY_OFFSET;
}

napi_value BodyFatNavy(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FIVE;
    napi_value argv[REQUIRED_ARGS_FIVE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FIVE, "Requires 5 arguments: gender, heightCm, neckCm, waistCm, hipCm");

    int gender = FITNESS_GENDER_MALE;
    if (!ExtractGenderArg(env, argv[ARG_INDEX_ZERO], gender)) {
        return nullptr;
    }

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    double neckCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], neckCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, neckCm >= FITNESS_MIN_GIRTH_CM && neckCm <= FITNESS_MAX_GIRTH_CM, "Neck girth out of range");

    double waistCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_THREE], waistCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, waistCm >= FITNESS_MIN_GIRTH_CM && waistCm <= FITNESS_MAX_GIRTH_CM, "Waist girth out of range");

    double hipCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_FOUR], hipCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, hipCm >= FITNESS_MIN_GIRTH_CM && hipCm <= FITNESS_MAX_GIRTH_CM, "Hip girth out of range");

    double girthTermCm = gender == FITNESS_GENDER_MALE ? waistCm - neckCm : waistCm + hipCm - neckCm;
    NAPI_ASSERT(env, girthTermCm > 0.0, "Girth combination must stay positive for the Navy formula");

    return CreateDoubleValue(env, BodyFatNavyValue(gender, heightCm, girthTermCm));
}

napi_value LeanBodyMass(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightKg, bodyFatPercent");

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double bodyFatPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], bodyFatPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bodyFatPercent >= FITNESS_MIN_BODY_FAT_PERCENT && bodyFatPercent <= FITNESS_MAX_BODY_FAT_PERCENT,
                "Body fat percent out of range");

    double leanMassKg = weightKg * (FITNESS_PERCENT_DIVISOR - bodyFatPercent) / FITNESS_PERCENT_DIVISOR;
    return CreateDoubleValue(env, leanMassKg);
}

// Computes the Devine ideal body weight from height and gender.
static double IdealWeightDevineValue(int gender, double heightCm)
{
    double extraCm = heightCm - FITNESS_DEVINE_BASE_HEIGHT_CM;
    double inchesOverFiveFeet = extraCm > 0.0 ? extraCm / FITNESS_DEVINE_CM_PER_INCH : 0.0;
    double baseKg = gender == FITNESS_GENDER_MALE ? FITNESS_DEVINE_MALE_BASE_KG : FITNESS_DEVINE_FEMALE_BASE_KG;
    return baseKg + FITNESS_DEVINE_INCH_COEFF * inchesOverFiveFeet;
}

napi_value IdealWeightDevine(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: gender, heightCm");

    int gender = FITNESS_GENDER_MALE;
    if (!ExtractGenderArg(env, argv[ARG_INDEX_ZERO], gender)) {
        return nullptr;
    }

    double heightCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], heightCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, heightCm >= FITNESS_MIN_HEIGHT_CM && heightCm <= FITNESS_MAX_HEIGHT_CM, "Height out of range");

    return CreateDoubleValue(env, IdealWeightDevineValue(gender, heightCm));
}

napi_value WaterIntakeLiters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: weightKg, exerciseMinutes");

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double exerciseMinutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], exerciseMinutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, exerciseMinutes >= FITNESS_MIN_MINUTES && exerciseMinutes <= FITNESS_MAX_MINUTES,
                "Exercise minutes out of range");

    double exerciseBlocks = exerciseMinutes / FITNESS_WATER_BLOCK_MINUTES;
    double liters = FITNESS_WATER_PER_KG_LITERS * weightKg + FITNESS_WATER_PER_BLOCK_LITERS * exerciseBlocks;
    return CreateDoubleValue(env, liters);
}

napi_value CaloriesBurned(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: metValue, weightKg, minutes");

    double metValue = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], metValue)) {
        return nullptr;
    }
    NAPI_ASSERT(env, metValue >= FITNESS_MIN_MET && metValue <= FITNESS_MAX_MET, "MET value out of range");

    double weightKg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], weightKg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weightKg >= FITNESS_MIN_WEIGHT_KG && weightKg <= FITNESS_MAX_WEIGHT_KG, "Weight out of range");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, minutes >= FITNESS_MIN_MINUTES && minutes <= FITNESS_MAX_MINUTES, "Minutes out of range");

    double hours = minutes / FITNESS_MINUTES_PER_HOUR;
    double kilocalories = metValue * weightKg * hours;
    return CreateDoubleValue(env, kilocalories);
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("bmi", Bmi),
        DECLARE_NAPI_FUNCTION("bmiCategory", BmiCategory),
        DECLARE_NAPI_FUNCTION("healthyWeightRange", HealthyWeightRange),
        DECLARE_NAPI_FUNCTION("bmrMifflin", BmrMifflin),
        DECLARE_NAPI_FUNCTION("bmrKatchMcArdle", BmrKatchMcArdle),
        DECLARE_NAPI_FUNCTION("tdee", Tdee),
        DECLARE_NAPI_FUNCTION("maxHeartRate", MaxHeartRate),
        DECLARE_NAPI_FUNCTION("targetHeartRate", TargetHeartRate),
        DECLARE_NAPI_FUNCTION("heartRateZone", HeartRateZone),
        DECLARE_NAPI_FUNCTION("oneRepMaxEpley", OneRepMaxEpley),
        DECLARE_NAPI_FUNCTION("oneRepMaxBrzycki", OneRepMaxBrzycki),
        DECLARE_NAPI_FUNCTION("weightForReps", WeightForReps),
        DECLARE_NAPI_FUNCTION("runningPace", RunningPace),
        DECLARE_NAPI_FUNCTION("paceToSpeed", PaceToSpeed),
        DECLARE_NAPI_FUNCTION("speedToPace", SpeedToPace),
        DECLARE_NAPI_FUNCTION("racePredictor", RacePredictor),
        DECLARE_NAPI_FUNCTION("stepsToDistanceKm", StepsToDistanceKm),
        DECLARE_NAPI_FUNCTION("bodyFatNavy", BodyFatNavy),
        DECLARE_NAPI_FUNCTION("leanBodyMass", LeanBodyMass),
        DECLARE_NAPI_FUNCTION("idealWeightDevine", IdealWeightDevine),
        DECLARE_NAPI_FUNCTION("waterIntakeLiters", WaterIntakeLiters),
        DECLARE_NAPI_FUNCTION("caloriesBurned", CaloriesBurned),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterFitnessOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module fitnessSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "fitnessSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void FitnessSuiteRegisterModule(void)
{
    napi_module_register(&fitnessSuiteModule);
}
