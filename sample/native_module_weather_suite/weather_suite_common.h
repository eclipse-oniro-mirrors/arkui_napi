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

#ifndef SAMPLE_NATIVE_MODULE_WEATHER_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_WEATHER_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int REQUIRED_ARGS_THREE = 3;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;
constexpr int ARG_INDEX_TWO = 2;

constexpr size_t WEATHER_MAX_ARRAY_LENGTH = 1024;
constexpr size_t WEATHER_EMPTY_ARRAY_SIZE = 0;
constexpr uint32_t WEATHER_UINT32_INDEX_ZERO = 0;

constexpr double WEATHER_MIN_TEMPERATURE_C = -100.0;
constexpr double WEATHER_MAX_TEMPERATURE_C = 100.0;
constexpr double WEATHER_MIN_HUMIDITY = 0.0;
constexpr double WEATHER_MAX_HUMIDITY = 100.0;
constexpr double WEATHER_HUMIDITY_PERCENT = 100.0;
constexpr double WEATHER_MIN_PRESSURE_HPA = 100.0;
constexpr double WEATHER_MAX_PRESSURE_HPA = 1100.0;
constexpr double WEATHER_MIN_WIND_KMH = 0.0;
constexpr double WEATHER_MAX_WIND_KMH = 500.0;
constexpr double WEATHER_MIN_UV_INDEX = 0.0;
constexpr double WEATHER_MAX_UV_INDEX = 20.0;
constexpr double WEATHER_MIN_ALTITUDE_M = -500.0;
constexpr double WEATHER_MAX_ALTITUDE_M = 11000.0;
constexpr double WEATHER_MIN_DEGREES = 0.0;
constexpr double WEATHER_MAX_DEGREES = 360.0;
constexpr double WEATHER_MIN_CLOUD_FRACTION = 0.0;
constexpr double WEATHER_MAX_CLOUD_FRACTION = 1.0;
constexpr double WEATHER_MIN_VISIBILITY_KM = 0.0;
constexpr double WEATHER_MAX_VISIBILITY_KM = 500.0;
constexpr double WEATHER_MIN_PRECIP_MMH = 0.0;
constexpr double WEATHER_MAX_PRECIP_MMH = 500.0;
constexpr double WEATHER_MIN_WATER_MM = 0.0;
constexpr double WEATHER_MAX_WATER_MM = 500.0;
constexpr double WEATHER_MIN_SNOW_CM = 0.0;
constexpr double WEATHER_MAX_SNOW_CM = 2000.0;
constexpr double WEATHER_MIN_STORM_SECONDS = 0.0;
constexpr double WEATHER_MAX_STORM_SECONDS = 3600.0;
constexpr double WEATHER_MIN_LATITUDE_DEG = -90.0;
constexpr double WEATHER_MAX_LATITUDE_DEG = 90.0;
constexpr int WEATHER_MIN_DAY_OF_YEAR = 1;
constexpr int WEATHER_MAX_DAY_OF_YEAR = 365;

constexpr double WEATHER_KELVIN_OFFSET_C = 273.15;
constexpr double WEATHER_C_TO_F_SCALE = 1.8;
constexpr double WEATHER_C_TO_F_OFFSET = 32.0;
constexpr double WEATHER_F_TO_C_SCALE = 5.0 / 9.0;
constexpr double WEATHER_KMH_TO_MS_DIVISOR = 3.6;
constexpr double WEATHER_MS_TO_KM_DIVISOR = 1000.0;
constexpr double WEATHER_HPA_TO_PA = 100.0;

constexpr double WEATHER_MAGNUS_A = 17.625;
constexpr double WEATHER_MAGNUS_B = 243.04;
constexpr double WEATHER_MAGNUS_ICE_A = 22.46;
constexpr double WEATHER_MAGNUS_ICE_B = 272.62;
constexpr double WEATHER_SATURATION_VP_COEFF_HPA = 6.112;

constexpr double WEATHER_HEAT_INDEX_F_INTERCEPT = -42.379;
constexpr double WEATHER_HEAT_INDEX_F_TEMP_COEFF = 2.04901523;
constexpr double WEATHER_HEAT_INDEX_F_HUMIDITY_COEFF = 10.14333127;
constexpr double WEATHER_HEAT_INDEX_F_CROSS_COEFF = -0.22475541;
constexpr double WEATHER_HEAT_INDEX_F_TEMP_SQ_COEFF = -0.00683783;
constexpr double WEATHER_HEAT_INDEX_F_HUMIDITY_SQ_COEFF = -0.05481717;
constexpr double WEATHER_HEAT_INDEX_F_TEMP_SQ_HUM_COEFF = 0.00122874;
constexpr double WEATHER_HEAT_INDEX_F_TEMP_HUM_SQ_COEFF = 0.00085282;
constexpr double WEATHER_HEAT_INDEX_F_TEMP_SQ_HUM_SQ_COEFF = -0.00000199;

constexpr double WEATHER_WINDCHILL_INTERCEPT = 13.12;
constexpr double WEATHER_WINDCHILL_TEMP_COEFF = 0.6215;
constexpr double WEATHER_WINDCHILL_WIND_COEFF = 11.37;
constexpr double WEATHER_WINDCHILL_CROSS_COEFF = 0.3965;
constexpr double WEATHER_WINDCHILL_WIND_EXPONENT = 0.16;
constexpr double WEATHER_WINDCHILL_MAX_TEMP_C = 10.0;
constexpr double WEATHER_WINDCHILL_MIN_WIND_KMH = 4.8;
constexpr double WEATHER_WINDCHILL_DANGER_TEMP_C = -27.0;

constexpr double WEATHER_ABS_HUMIDITY_COEFF = 216.7;
constexpr double WEATHER_MOLAR_WEIGHT_RATIO = 622.0;
constexpr double WEATHER_SPECIFIC_HUMIDITY_VAPOR_FACTOR = 0.378;
constexpr double WEATHER_AT_VAPOR_COEFF = 0.33;
constexpr double WEATHER_AT_WIND_COEFF = 0.70;
constexpr double WEATHER_AT_CONSTANT = -4.0;
constexpr double WEATHER_HUMIDEX_VAPOR_COEFF = 0.5555;
constexpr double WEATHER_HUMIDEX_VAPOR_OFFSET = 10.0;

constexpr double WEATHER_STULL_SQRT_COEFF = 0.151977;
constexpr double WEATHER_STULL_HUM_OFFSET = 8.313659;
constexpr double WEATHER_STULL_HUM_POWER = 0.5;
constexpr double WEATHER_STULL_SUB_HUM_OFFSET = 1.676331;
constexpr double WEATHER_STULL_TERM4_COEFF = 0.00391838;
constexpr double WEATHER_STULL_TERM4_POWER = 1.5;
constexpr double WEATHER_STULL_TERM4_ATAN_COEFF = 0.023101;
constexpr double WEATHER_STULL_OFFSET = 4.686035;

constexpr int WEATHER_BEAUFORT_MIN_LEVEL = 0;
constexpr int WEATHER_BEAUFORT_LEVEL_COUNT = 13;
constexpr int WEATHER_BEAUFORT_THRESHOLD_COUNT = 12;
const double WEATHER_BEAUFORT_THRESHOLDS_KMH[WEATHER_BEAUFORT_THRESHOLD_COUNT] = {
    1.0, 6.0, 12.0, 20.0, 29.0, 38.0, 49.0, 61.0, 74.0, 88.0, 102.0, 118.0,
};
const char* const WEATHER_BEAUFORT_DESCRIPTIONS[WEATHER_BEAUFORT_LEVEL_COUNT] = {
    "Calm",      "Light air", "Light breeze", "Gentle breeze", "Moderate breeze", "Fresh breeze", "Strong breeze",
    "Near gale", "Gale",      "Strong gale",  "Storm",         "Violent storm",   "Hurricane",
};

constexpr double WEATHER_UV_MODERATE_MIN = 3.0;
constexpr double WEATHER_UV_HIGH_MIN = 6.0;
constexpr double WEATHER_UV_VERY_HIGH_MIN = 8.0;
constexpr double WEATHER_UV_EXTREME_MIN = 11.0;

constexpr double WEATHER_THI_HUMIDITY_BASE = 0.55;
constexpr double WEATHER_THI_HUMIDITY_COEFF = 0.0055;
constexpr double WEATHER_THI_REFERENCE_TEMP_C = 14.5;

constexpr double WEATHER_CLOUD_BASE_SPREAD_FACTOR = 125.0;
constexpr double WEATHER_SPREAD_FLOOR_C = 0.0;
constexpr double WEATHER_DRY_AIR_GAS_CONSTANT = 287.05;

constexpr double WEATHER_ISA_SEA_LEVEL_PRESSURE_HPA = 1013.25;
constexpr double WEATHER_ISA_LAPSE_FACTOR = 0.0000225577;
constexpr double WEATHER_ISA_PRESSURE_EXPONENT = 5.25588;
constexpr double WEATHER_ISA_UNIT_RATIO = 1.0;

constexpr double WEATHER_COMFORT_IDEAL_TEMP_C = 22.0;
constexpr double WEATHER_COMFORT_IDEAL_HUMIDITY = 45.0;
constexpr double WEATHER_COMFORT_TEMP_PENALTY = 4.0;
constexpr double WEATHER_COMFORT_HUMIDITY_PENALTY = 0.5;
constexpr double WEATHER_COMFORT_MIN_SCORE = 0.0;
constexpr double WEATHER_COMFORT_MAX_SCORE = 100.0;

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
bool ExtractDoubleArray(napi_env env, napi_value value, std::vector<double>& result);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateInt32Value(napi_env env, int32_t value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateStringValue(napi_env env, const std::string& text);
napi_value CreateDoubleArrayValue(napi_env env, const std::vector<double>& values);

constexpr double WEATHER_DEG_TO_RAD = 0.017453292519943295;
constexpr double WEATHER_RAD_TO_DEG = 57.29577951308232;
constexpr double WEATHER_FULL_CIRCLE_DEG = 360.0;

constexpr int WEATHER_HEAT_WARNING_LEVEL_COUNT = 4;
const double WEATHER_HEAT_WARNING_THRESHOLDS[WEATHER_HEAT_WARNING_LEVEL_COUNT] = {
    30.0,
    40.0,
    45.0,
    54.0,
};

constexpr double WEATHER_PRECIP_NONE_MAX_MMH = 0.1;
constexpr double WEATHER_PRECIP_LIGHT_MAX_MMH = 2.5;
constexpr double WEATHER_PRECIP_MODERATE_MAX_MMH = 7.6;
constexpr double WEATHER_PRECIP_HEAVY_MAX_MMH = 50.0;

constexpr double WEATHER_SNOW_RATIO_BASE = 10.0;
constexpr double WEATHER_SNOW_RATIO_BASE_TEMP_C = 0.0;
constexpr double WEATHER_SNOW_RATIO_TEMP_COEFF = 0.5;
constexpr double WEATHER_SNOW_MIN_RATIO = 1.0;
constexpr double WEATHER_SNOW_RATIO_TO_CM_DIVISOR = 10.0;

constexpr double WEATHER_FIRE_TEMP_WEIGHT = 0.4;
constexpr double WEATHER_FIRE_HUMIDITY_WEIGHT = 0.35;
constexpr double WEATHER_FIRE_WIND_WEIGHT = 0.25;
constexpr double WEATHER_FIRE_TEMP_BASE_C = 20.0;
constexpr double WEATHER_FIRE_TEMP_SPAN_C = 30.0;
constexpr double WEATHER_FIRE_HUMIDITY_BASE = 30.0;
constexpr double WEATHER_FIRE_HUMIDITY_SPAN = 40.0;
constexpr double WEATHER_FIRE_WIND_BASE_KMH = 10.0;
constexpr double WEATHER_FIRE_WIND_SPAN_KMH = 40.0;
constexpr double WEATHER_FIRE_SCORE_SCALE = 100.0;
constexpr double WEATHER_FIRE_MIN_SCORE = 0.0;
constexpr double WEATHER_FIRE_MAX_SCORE = 100.0;
constexpr double WEATHER_CLAMP_MIN = 0.0;
constexpr double WEATHER_CLAMP_MAX = 1.0;

constexpr double WEATHER_MOULD_MIN_HUMIDITY = 70.0;
constexpr double WEATHER_MOULD_MIN_TEMP_C = 10.0;
constexpr double WEATHER_MOULD_MAX_TEMP_C = 35.0;

constexpr double WEATHER_TEMP_MEAN_DIVISOR = 2.0;
constexpr double WEATHER_DEGREE_DAY_FLOOR = 0.0;

constexpr double WEATHER_CLOUD_SCATTERED_MIN = 0.25;
constexpr double WEATHER_CLOUD_BROKEN_MIN = 0.5;
constexpr double WEATHER_CLOUD_OVERCAST_MIN = 0.875;

constexpr double WEATHER_VIS_DENSE_FOG_MAX_KM = 0.2;
constexpr double WEATHER_VIS_FOG_MAX_KM = 1.0;
constexpr double WEATHER_VIS_MIST_MAX_KM = 5.0;
constexpr double WEATHER_VIS_VERY_CLEAR_MIN_KM = 20.0;

constexpr int WEATHER_PRESSURE_MIN_READINGS = 3;
constexpr int WEATHER_PRESSURE_DELTA_MIN_READINGS = 2;
constexpr double WEATHER_PRESSURE_EPSILON_HPA = 0.1;
constexpr int WEATHER_STORM_MIN_READINGS = 3;
constexpr double WEATHER_SPEED_OF_SOUND_MS = 343.0;

constexpr double WEATHER_SOLAR_DECLINATION_DEG = 23.44;
constexpr double WEATHER_DECLINATION_PERIOD_DEG = 360.0;
constexpr double WEATHER_DAYS_PER_YEAR = 365.0;
constexpr double WEATHER_DECLINATION_PHASE_DAYS = 284.0;
constexpr double WEATHER_HOUR_ANGLE_DIVISOR = 15.0;
constexpr double WEATHER_DAY_LENGTH_FACTOR = 2.0;
constexpr double WEATHER_ACOS_CLAMP_MIN = -1.0;
constexpr double WEATHER_ACOS_CLAMP_MAX = 1.0;
constexpr double WEATHER_POLAR_DAY_LENGTH_H = 24.0;
constexpr double WEATHER_POLAR_NIGHT_LENGTH_H = 0.0;

napi_value HeatIndexCelsius(napi_env env, napi_callback_info info);
napi_value WindChillCelsius(napi_env env, napi_callback_info info);
napi_value DewPointCelsius(napi_env env, napi_callback_info info);
napi_value FrostPointCelsius(napi_env env, napi_callback_info info);
napi_value SaturationVaporPressureHpa(napi_env env, napi_callback_info info);
napi_value ActualVaporPressureHpa(napi_env env, napi_callback_info info);
napi_value AbsoluteHumidity(napi_env env, napi_callback_info info);
napi_value SpecificHumidity(napi_env env, napi_callback_info info);
napi_value MixingRatio(napi_env env, napi_callback_info info);
napi_value RelativeHumidityFromDew(napi_env env, napi_callback_info info);
napi_value ApparentTemperatureCelsius(napi_env env, napi_callback_info info);
napi_value HumidexCelsius(napi_env env, napi_callback_info info);
napi_value WetBulbTemperature(napi_env env, napi_callback_info info);
napi_value BeaufortLevel(napi_env env, napi_callback_info info);
napi_value BeaufortDescription(napi_env env, napi_callback_info info);
napi_value UvIndexCategory(napi_env env, napi_callback_info info);
napi_value TemperatureHumidityIndex(napi_env env, napi_callback_info info);
napi_value CloudBaseMeters(napi_env env, napi_callback_info info);
napi_value AirDensity(napi_env env, napi_callback_info info);
napi_value AltitudePressureHpa(napi_env env, napi_callback_info info);
napi_value PressureAltitudeMeters(napi_env env, napi_callback_info info);
napi_value ComfortScore(napi_env env, napi_callback_info info);

napi_value AverageWindDirection(napi_env env, napi_callback_info info);
napi_value WindDirectionSpread(napi_env env, napi_callback_info info);
napi_value WindGustFactor(napi_env env, napi_callback_info info);
napi_value IsWindChillDangerous(napi_env env, napi_callback_info info);
napi_value HeatWarningLevel(napi_env env, napi_callback_info info);
napi_value PrecipitationIntensity(napi_env env, napi_callback_info info);
napi_value SnowDepthEstimate(napi_env env, napi_callback_info info);
napi_value SnowWaterEquivalent(napi_env env, napi_callback_info info);
napi_value FireRiskIndex(napi_env env, napi_callback_info info);
napi_value MouldRisk(napi_env env, napi_callback_info info);
napi_value GrowingDegreeDay(napi_env env, napi_callback_info info);
napi_value CoolingDegreeDay(napi_env env, napi_callback_info info);
napi_value HeatingDegreeDay(napi_env env, napi_callback_info info);
napi_value SkyConditionFromCloudiness(napi_env env, napi_callback_info info);
napi_value VisibilityCategory(napi_env env, napi_callback_info info);
napi_value PressureTendency(napi_env env, napi_callback_info info);
napi_value PressureDelta(napi_env env, napi_callback_info info);
napi_value StormDistanceKm(napi_env env, napi_callback_info info);
napi_value IsStormApproaching(napi_env env, napi_callback_info info);
napi_value DayLengthEstimate(napi_env env, napi_callback_info info);
napi_value TemperatureAnomaly(napi_env env, napi_callback_info info);
napi_value DiurnalRange(napi_env env, napi_callback_info info);

napi_status RegisterWeatherOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_WEATHER_SUITE_COMMON_H
