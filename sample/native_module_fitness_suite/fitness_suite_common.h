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

#ifndef SAMPLE_NATIVE_MODULE_FITNESS_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_FITNESS_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int REQUIRED_ARGS_THREE = 3;
constexpr int REQUIRED_ARGS_FOUR = 4;
constexpr int REQUIRED_ARGS_FIVE = 5;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;
constexpr int ARG_INDEX_TWO = 2;
constexpr int ARG_INDEX_THREE = 3;
constexpr int ARG_INDEX_FOUR = 4;

constexpr size_t FITNESS_MAX_ARRAY_LENGTH = 1024;
constexpr size_t FITNESS_MAX_TEXT_LENGTH = 256;
constexpr size_t FITNESS_STRING_NUL_SIZE = 1;
constexpr uint32_t FITNESS_UINT32_INDEX_ZERO = 0;
constexpr size_t FITNESS_ARRAY_INDEX_FIRST = 0;
constexpr size_t FITNESS_PAIR_SIZE = 2;
constexpr size_t FITNESS_PAIR_INDEX_MIN = 0;
constexpr size_t FITNESS_PAIR_INDEX_MAX = 1;
constexpr size_t FITNESS_TRIPLE_SIZE = 3;

constexpr char FITNESS_CHAR_UPPER_A = 'A';
constexpr char FITNESS_CHAR_UPPER_Z = 'Z';
constexpr char FITNESS_CHAR_CASE_DELTA = 32;

constexpr size_t FITNESS_GENDER_TEXT_MAX_LENGTH = 10;
constexpr int FITNESS_GENDER_MALE = 0;
constexpr int FITNESS_GENDER_FEMALE = 1;
const char* const FITNESS_GENDER_MALE_TEXT = "male";
const char* const FITNESS_GENDER_FEMALE_TEXT = "female";

constexpr double FITNESS_MIN_WEIGHT_KG = 1.0;
constexpr double FITNESS_MAX_WEIGHT_KG = 500.0;
constexpr double FITNESS_MIN_HEIGHT_CM = 50.0;
constexpr double FITNESS_MAX_HEIGHT_CM = 280.0;
constexpr double FITNESS_MIN_AGE_YEARS = 1.0;
constexpr double FITNESS_MAX_AGE_YEARS = 120.0;
constexpr double FITNESS_MIN_BODY_FAT_PERCENT = 1.0;
constexpr double FITNESS_MAX_BODY_FAT_PERCENT = 70.0;
constexpr double FITNESS_MIN_HEART_RATE_BPM = 20.0;
constexpr double FITNESS_MAX_HEART_RATE_BPM = 250.0;
constexpr double FITNESS_MIN_MET = 0.0;
constexpr double FITNESS_MAX_MET = 25.0;
constexpr int FITNESS_MIN_REPS = 1;
constexpr int FITNESS_MAX_REPS = 50;
constexpr double FITNESS_MIN_WEIGHT_LIFTED_KG = 0.0;
constexpr double FITNESS_MAX_WEIGHT_LIFTED_KG = 1000.0;
constexpr double FITNESS_MIN_SPEED_KMH = 0.0;
constexpr double FITNESS_MAX_SPEED_KMH = 50.0;
constexpr double FITNESS_MIN_PACE_SECONDS = 0.0;
constexpr double FITNESS_MAX_PACE_SECONDS = 36000.0;
constexpr double FITNESS_MIN_DISTANCE_KM = 0.0;
constexpr double FITNESS_MAX_DISTANCE_KM = 10000.0;
constexpr double FITNESS_MIN_MINUTES = 0.0;
constexpr double FITNESS_MAX_MINUTES = 10000.0;
constexpr int FITNESS_MIN_STEPS = 0;
constexpr int FITNESS_MAX_STEPS = 1000000;
constexpr double FITNESS_MIN_GIRTH_CM = 10.0;
constexpr double FITNESS_MAX_GIRTH_CM = 250.0;
constexpr double FITNESS_MIN_POOL_LENGTH_M = 5.0;
constexpr double FITNESS_MAX_POOL_LENGTH_M = 100.0;
constexpr double FITNESS_MIN_WATTS = 0.0;
constexpr double FITNESS_MAX_WATTS = 2500.0;
constexpr double FITNESS_MIN_INCLINE_PERCENT = 0.0;
constexpr double FITNESS_MAX_INCLINE_PERCENT = 40.0;
constexpr double FITNESS_MIN_ELEVATION_M = -500.0;
constexpr double FITNESS_MAX_ELEVATION_M = 9000.0;

constexpr double FITNESS_HEIGHT_CM_PER_METER = 100.0;
constexpr double FITNESS_METER_PER_KM = 1000.0;
constexpr double FITNESS_SECONDS_PER_MINUTE = 60.0;
constexpr double FITNESS_SECONDS_PER_HOUR = 3600.0;
constexpr double FITNESS_MINUTES_PER_HOUR = 60.0;
constexpr double FITNESS_PERCENT_DIVISOR = 100.0;

constexpr double FITNESS_BMI_UNDERWEIGHT_MAX = 18.5;
constexpr double FITNESS_BMI_NORMAL_MAX = 25.0;
constexpr double FITNESS_BMI_OVERWEIGHT_MAX = 30.0;
constexpr double FITNESS_BMI_OBESE_CLASS_I_MAX = 35.0;
constexpr double FITNESS_BMI_OBESE_CLASS_II_MAX = 40.0;
constexpr int FITNESS_BMI_CATEGORIES_COUNT = 6;
const char* const FITNESS_BMI_CATEGORIES[FITNESS_BMI_CATEGORIES_COUNT] = {
    "Underweight", "Normal", "Overweight", "Obese class I", "Obese class II", "Obese class III",
};
constexpr double FITNESS_MIN_BMI = 5.0;
constexpr double FITNESS_MAX_BMI = 100.0;
constexpr int FITNESS_BMI_CATEGORY_INDEX_UNDERWEIGHT = 0;
constexpr int FITNESS_BMI_CATEGORY_INDEX_NORMAL = 1;
constexpr int FITNESS_BMI_CATEGORY_INDEX_OVERWEIGHT = 2;
constexpr int FITNESS_BMI_CATEGORY_INDEX_OBESE_I = 3;
constexpr int FITNESS_BMI_CATEGORY_INDEX_OBESE_II = 4;
constexpr int FITNESS_BMI_CATEGORY_INDEX_OBESE_III = 5;
constexpr double FITNESS_HEALTHY_BMI_MIN = 18.5;
constexpr double FITNESS_HEALTHY_BMI_MAX = 24.9;

constexpr double FITNESS_MIFFLIN_WEIGHT_COEFF = 10.0;
constexpr double FITNESS_MIFFLIN_HEIGHT_COEFF = 6.25;
constexpr double FITNESS_MIFFLIN_AGE_COEFF = 5.0;
constexpr double FITNESS_MIFFLIN_MALE_OFFSET = 5.0;
constexpr double FITNESS_MIFFLIN_FEMALE_OFFSET = -161.0;
constexpr double FITNESS_KATCH_BASE_KCAL = 370.0;
constexpr double FITNESS_KATCH_LEAN_COEFF = 21.6;
constexpr double FITNESS_MIN_BMR = 0.0;
constexpr double FITNESS_MAX_BMR = 20000.0;
constexpr double FITNESS_MIN_TDEE_MULTIPLIER = 1.0;
constexpr double FITNESS_MAX_TDEE_MULTIPLIER = 2.5;

constexpr double FITNESS_MAX_HR_AGE_BASE = 220.0;
constexpr double FITNESS_KARVONEN_MIN_INTENSITY = 0.3;
constexpr double FITNESS_KARVONEN_MAX_INTENSITY = 1.0;
constexpr int FITNESS_HR_ZONE_MIN = 1;
constexpr int FITNESS_HR_ZONE_MAX = 5;
constexpr double FITNESS_HR_ZONE_START_PERCENT = 50.0;
constexpr double FITNESS_HR_ZONE_SPAN_PERCENT = 10.0;

constexpr double FITNESS_EPLEY_REP_DIVISOR = 30.0;
constexpr double FITNESS_BRZYCKI_NUMERATOR = 36.0;
constexpr double FITNESS_BRZYCKI_DENOM_BASE = 37.0;

constexpr double FITNESS_RIEGEL_EXPONENT = 1.06;
constexpr double FITNESS_STRIDE_HEIGHT_RATIO = 0.415;
constexpr double FITNESS_CM_PER_KM = 100000.0;

constexpr double FITNESS_NAVY_MALE_TERM = 1.0324;
constexpr double FITNESS_NAVY_MALE_GIRTH_COEFF = 0.19077;
constexpr double FITNESS_NAVY_MALE_HEIGHT_COEFF = 0.15456;
constexpr double FITNESS_NAVY_FEMALE_TERM = 1.29579;
constexpr double FITNESS_NAVY_FEMALE_GIRTH_COEFF = 0.35004;
constexpr double FITNESS_NAVY_FEMALE_HEIGHT_COEFF = 0.22100;
constexpr double FITNESS_NAVY_SCALE = 495.0;
constexpr double FITNESS_NAVY_OFFSET = 450.0;

constexpr double FITNESS_DEVINE_MALE_BASE_KG = 50.0;
constexpr double FITNESS_DEVINE_FEMALE_BASE_KG = 45.5;
constexpr double FITNESS_DEVINE_INCH_COEFF = 2.3;
constexpr double FITNESS_DEVINE_BASE_HEIGHT_CM = 152.4;
constexpr double FITNESS_DEVINE_CM_PER_INCH = 2.54;

constexpr double FITNESS_WATER_PER_KG_LITERS = 0.033;
constexpr double FITNESS_WATER_PER_BLOCK_LITERS = 0.35;
constexpr double FITNESS_WATER_BLOCK_MINUTES = 30.0;

constexpr int FITNESS_PLATE_TYPE_COUNT = 7;
const double FITNESS_PLATE_WEIGHTS_KG[FITNESS_PLATE_TYPE_COUNT] = {
    25.0, 20.0, 15.0, 10.0, 5.0, 2.5, 1.25,
};
constexpr double FITNESS_PLATE_SIDES = 2.0;
constexpr double FITNESS_PLATE_EPSILON_KG = 0.001;

constexpr double FITNESS_WARMUP_RATIO_ONE = 0.4;
constexpr double FITNESS_WARMUP_RATIO_TWO = 0.6;
constexpr double FITNESS_WARMUP_RATIO_THREE = 0.8;
constexpr int FITNESS_MIN_INTERVALS = 1;
constexpr int FITNESS_MAX_INTERVALS = 1000;
constexpr double FITNESS_MIN_SECONDS = 0.0;
constexpr double FITNESS_MAX_SECONDS = 36000.0;
constexpr double FITNESS_MIN_RPE = 1.0;
constexpr double FITNESS_MAX_RPE = 10.0;
constexpr double FITNESS_MIN_TRAINING_LOAD = 0.0;
constexpr double FITNESS_MAX_TRAINING_LOAD = 100000.0;

constexpr double FITNESS_HIKING_ELEVATION_DIVISOR = 100.0;
constexpr double FITNESS_TREADMILL_INCLINE_DIVISOR = 15.0;
constexpr double FITNESS_TREADMILL_INCLINE_BASE = 1.0;
constexpr double FITNESS_JOULES_PER_KCAL = 4184.0;
constexpr double FITNESS_FTP_RATIO = 0.95;
constexpr int FITNESS_MIN_LAPS = 0;
constexpr int FITNESS_MAX_LAPS = 10000;
constexpr double FITNESS_TRANSITION_MINUTES = 5.0;

constexpr double FITNESS_MIN_COOPER_DISTANCE_M = 0.0;
constexpr double FITNESS_MAX_COOPER_DISTANCE_M = 10000.0;
constexpr double FITNESS_COOPER_OFFSET_METERS = 504.9;
constexpr double FITNESS_COOPER_DIVISOR = 44.73;
constexpr double FITNESS_MIN_VO2MAX = 10.0;
constexpr double FITNESS_MAX_VO2MAX = 90.0;
constexpr double FITNESS_VO2_REST_ML_PER_KG = 3.5;
constexpr double FITNESS_ACSM_SPEED_COEFF = 0.2;

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
bool ExtractInt32Arg(napi_env env, napi_value value, int32_t& result);
bool ExtractStringArg(napi_env env, napi_value value, std::string& result);
bool ExtractGenderArg(napi_env env, napi_value value, int& result);
bool ExtractDoubleArray(napi_env env, napi_value value, std::vector<double>& result);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateInt32Value(napi_env env, int32_t value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateStringValue(napi_env env, const std::string& text);
napi_value CreateDoubleArrayValue(napi_env env, const std::vector<double>& values);
napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values);

napi_value Bmi(napi_env env, napi_callback_info info);
napi_value BmiCategory(napi_env env, napi_callback_info info);
napi_value HealthyWeightRange(napi_env env, napi_callback_info info);
napi_value BmrMifflin(napi_env env, napi_callback_info info);
napi_value BmrKatchMcArdle(napi_env env, napi_callback_info info);
napi_value Tdee(napi_env env, napi_callback_info info);
napi_value MaxHeartRate(napi_env env, napi_callback_info info);
napi_value TargetHeartRate(napi_env env, napi_callback_info info);
napi_value HeartRateZone(napi_env env, napi_callback_info info);
napi_value OneRepMaxEpley(napi_env env, napi_callback_info info);
napi_value OneRepMaxBrzycki(napi_env env, napi_callback_info info);
napi_value WeightForReps(napi_env env, napi_callback_info info);
napi_value RunningPace(napi_env env, napi_callback_info info);
napi_value PaceToSpeed(napi_env env, napi_callback_info info);
napi_value SpeedToPace(napi_env env, napi_callback_info info);
napi_value RacePredictor(napi_env env, napi_callback_info info);
napi_value StepsToDistanceKm(napi_env env, napi_callback_info info);
napi_value BodyFatNavy(napi_env env, napi_callback_info info);
napi_value LeanBodyMass(napi_env env, napi_callback_info info);
napi_value IdealWeightDevine(napi_env env, napi_callback_info info);
napi_value WaterIntakeLiters(napi_env env, napi_callback_info info);
napi_value CaloriesBurned(napi_env env, napi_callback_info info);

napi_value WorkoutVolume(napi_env env, napi_callback_info info);
napi_value HeaviestSet(napi_env env, napi_callback_info info);
napi_value TotalReps(napi_env env, napi_callback_info info);
napi_value AverageWeightLifted(napi_env env, napi_callback_info info);
napi_value BestOneRepMaxEstimate(napi_env env, napi_callback_info info);
napi_value PlateLoad(napi_env env, napi_callback_info info);
napi_value WarmupSets(napi_env env, napi_callback_info info);
napi_value IntervalTrainingTime(napi_env env, napi_callback_info info);
napi_value SessionLoad(napi_env env, napi_callback_info info);
napi_value WeeklyLoad(napi_env env, napi_callback_info info);
napi_value AcuteChronicRatio(napi_env env, napi_callback_info info);
napi_value StepsCadence(napi_env env, napi_callback_info info);
napi_value DistanceToSteps(napi_env env, napi_callback_info info);
napi_value HikingEffort(napi_env env, napi_callback_info info);
napi_value TreadmillEquivalentPace(napi_env env, napi_callback_info info);
napi_value CyclingCalories(napi_env env, napi_callback_info info);
napi_value PowerToWeight(napi_env env, napi_callback_info info);
napi_value FunctionalThresholdPower(napi_env env, napi_callback_info info);
napi_value SwimLapsToKm(napi_env env, napi_callback_info info);
napi_value TriathlonTotalTime(napi_env env, napi_callback_info info);
napi_value RecoveryHeartRate(napi_env env, napi_callback_info info);
napi_value Vo2maxCooper(napi_env env, napi_callback_info info);
napi_value RaceTimeFromVo2max(napi_env env, napi_callback_info info);

napi_status RegisterFitnessOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_FITNESS_SUITE_COMMON_H
