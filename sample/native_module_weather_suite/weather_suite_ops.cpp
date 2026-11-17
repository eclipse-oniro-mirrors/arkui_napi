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

#include "napi/native_node_api.h"
#include "weather_suite_common.h"

static double SaturationVaporPressureValue(double temperatureC)
{
    double denominator = WEATHER_MAGNUS_B + temperatureC;
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    double exponent = WEATHER_MAGNUS_A * temperatureC / safeDenominator;
    return WEATHER_SATURATION_VP_COEFF_HPA * std::exp(exponent);
}

static double ActualVaporPressureValue(double temperatureC, double humidityPercent)
{
    return SaturationVaporPressureValue(temperatureC) * humidityPercent / WEATHER_HUMIDITY_PERCENT;
}

static double WindChillValue(double temperatureC, double windSpeedKmh)
{
    double windFactor = std::pow(windSpeedKmh, WEATHER_WINDCHILL_WIND_EXPONENT);
    return WEATHER_WINDCHILL_INTERCEPT + WEATHER_WINDCHILL_TEMP_COEFF * temperatureC -
           WEATHER_WINDCHILL_WIND_COEFF * windFactor + WEATHER_WINDCHILL_CROSS_COEFF * temperatureC * windFactor;
}

static double HumidexValue(double temperatureC, double humidityPercent)
{
    double vaporPressure = ActualVaporPressureValue(temperatureC, humidityPercent);
    return temperatureC + WEATHER_HUMIDEX_VAPOR_COEFF * (vaporPressure - WEATHER_HUMIDEX_VAPOR_OFFSET);
}

static double SnowRatioValue(double temperatureC)
{
    double ratio =
        WEATHER_SNOW_RATIO_BASE + WEATHER_SNOW_RATIO_TEMP_COEFF * (WEATHER_SNOW_RATIO_BASE_TEMP_C - temperatureC);
    return std::max(ratio, WEATHER_SNOW_MIN_RATIO);
}

static double CircularMeanDegrees(const std::vector<double>& degrees)
{
    double sinSum = 0.0;
    double cosSum = 0.0;
    for (size_t i = 0; i < degrees.size(); ++i) {
        double radians = degrees[i] * WEATHER_DEG_TO_RAD;
        sinSum += std::sin(radians);
        cosSum += std::cos(radians);
    }
    double size = static_cast<double>(degrees.size());
    double safeSize = size > 0.0 ? size : 1.0;
    double meanRadians = std::atan2(sinSum / safeSize, cosSum / safeSize);
    double meanDegrees = meanRadians * WEATHER_RAD_TO_DEG;
    if (meanDegrees < WEATHER_MIN_DEGREES) {
        meanDegrees += WEATHER_FULL_CIRCLE_DEG;
    }
    return meanDegrees;
}

static bool ValidateArrayRange(napi_env env,
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

napi_value AverageWindDirection(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: degrees array");

    std::vector<double> degrees;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], degrees)) {
        return nullptr;
    }
    NAPI_ASSERT(env, degrees.size() > WEATHER_EMPTY_ARRAY_SIZE, "Degrees array must not be empty");
    if (!ValidateArrayRange(env, degrees, WEATHER_MIN_DEGREES, WEATHER_MAX_DEGREES, "Direction out of range")) {
        return nullptr;
    }

    return CreateDoubleValue(env, CircularMeanDegrees(degrees));
}

napi_value WindDirectionSpread(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: degrees array");

    std::vector<double> degrees;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], degrees)) {
        return nullptr;
    }
    NAPI_ASSERT(env, degrees.size() > WEATHER_EMPTY_ARRAY_SIZE, "Degrees array must not be empty");
    if (!ValidateArrayRange(env, degrees, WEATHER_MIN_DEGREES, WEATHER_MAX_DEGREES, "Direction out of range")) {
        return nullptr;
    }

    double meanDegrees = CircularMeanDegrees(degrees);
    double spreadSum = 0.0;
    for (size_t i = 0; i < degrees.size(); ++i) {
        double delta = std::abs(degrees[i] - meanDegrees);
        if (delta > WEATHER_FULL_CIRCLE_DEG - delta) {
            delta = WEATHER_FULL_CIRCLE_DEG - delta;
        }
        spreadSum += delta;
    }
    double size = static_cast<double>(degrees.size());
    double safeSize = size > 0.0 ? size : 1.0;
    return CreateDoubleValue(env, spreadSum / safeSize);
}

napi_value WindGustFactor(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: mean speed, gust speed");

    double meanSpeedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], meanSpeedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, meanSpeedKmh > WEATHER_MIN_WIND_KMH && meanSpeedKmh <= WEATHER_MAX_WIND_KMH,
                "Mean wind speed must be positive");

    double gustSpeedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], gustSpeedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, gustSpeedKmh >= meanSpeedKmh && gustSpeedKmh <= WEATHER_MAX_WIND_KMH,
                "Gust speed must not be below the mean speed");

    double safeMean = meanSpeedKmh > 0.0 ? meanSpeedKmh : 1.0;
    return CreateDoubleValue(env, gustSpeedKmh / safeMean);
}

napi_value IsWindChillDangerous(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperature, wind speed");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_WINDCHILL_MAX_TEMP_C,
                "Temperature out of range for wind chill");

    double windSpeedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], windSpeedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, windSpeedKmh >= WEATHER_WINDCHILL_MIN_WIND_KMH && windSpeedKmh <= WEATHER_MAX_WIND_KMH,
                "Wind speed out of range for wind chill");

    double windChill = WindChillValue(temperatureC, windSpeedKmh);
    return CreateBoolValue(env, windChill <= WEATHER_WINDCHILL_DANGER_TEMP_C);
}

napi_value HeatWarningLevel(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperature, humidity");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double humidity = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], humidity)) {
        return nullptr;
    }
    NAPI_ASSERT(env, humidity >= WEATHER_MIN_HUMIDITY && humidity <= WEATHER_MAX_HUMIDITY, "Humidity out of range");

    double humidex = HumidexValue(temperatureC, humidity);
    int32_t level = 0;
    for (int i = 0; i < WEATHER_HEAT_WARNING_LEVEL_COUNT; ++i) {
        if (humidex >= WEATHER_HEAT_WARNING_THRESHOLDS[i]) {
            level = i + 1;
        }
    }
    return CreateInt32Value(env, level);
}

napi_value PrecipitationIntensity(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: precipitation rate");

    double mmPerHour = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], mmPerHour)) {
        return nullptr;
    }
    NAPI_ASSERT(env, mmPerHour >= WEATHER_MIN_PRECIP_MMH && mmPerHour <= WEATHER_MAX_PRECIP_MMH,
                "Precipitation rate out of range");

    if (mmPerHour < WEATHER_PRECIP_NONE_MAX_MMH) {
        return CreateStringValue(env, "None");
    }
    if (mmPerHour < WEATHER_PRECIP_LIGHT_MAX_MMH) {
        return CreateStringValue(env, "Light");
    }
    if (mmPerHour < WEATHER_PRECIP_MODERATE_MAX_MMH) {
        return CreateStringValue(env, "Moderate");
    }
    if (mmPerHour < WEATHER_PRECIP_HEAVY_MAX_MMH) {
        return CreateStringValue(env, "Heavy");
    }
    return CreateStringValue(env, "Violent");
}

napi_value SnowDepthEstimate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: water amount, temperature");

    double waterMm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], waterMm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, waterMm >= WEATHER_MIN_WATER_MM && waterMm <= WEATHER_MAX_WATER_MM, "Water amount out of range");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double snowRatio = SnowRatioValue(temperatureC);
    double snowCm = waterMm * snowRatio / WEATHER_SNOW_RATIO_TO_CM_DIVISOR;
    return CreateDoubleValue(env, snowCm);
}

napi_value SnowWaterEquivalent(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: snow depth, temperature");

    double snowCm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], snowCm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, snowCm >= WEATHER_MIN_SNOW_CM && snowCm <= WEATHER_MAX_SNOW_CM, "Snow depth out of range");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double snowRatio = SnowRatioValue(temperatureC);
    double safeRatio = snowRatio > 0.0 ? snowRatio : 1.0;
    double waterMm = snowCm * WEATHER_SNOW_RATIO_TO_CM_DIVISOR / safeRatio;
    return CreateDoubleValue(env, waterMm);
}

napi_value FireRiskIndex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: temperature, humidity, wind speed");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double humidity = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], humidity)) {
        return nullptr;
    }
    NAPI_ASSERT(env, humidity >= WEATHER_MIN_HUMIDITY && humidity <= WEATHER_MAX_HUMIDITY, "Humidity out of range");

    double windSpeedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], windSpeedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, windSpeedKmh >= WEATHER_MIN_WIND_KMH && windSpeedKmh <= WEATHER_MAX_WIND_KMH,
                "Wind speed out of range");

    double tempScore = (temperatureC - WEATHER_FIRE_TEMP_BASE_C) / WEATHER_FIRE_TEMP_SPAN_C;
    tempScore = std::max(WEATHER_CLAMP_MIN, std::min(WEATHER_CLAMP_MAX, tempScore));
    double humidityScore = (WEATHER_FIRE_HUMIDITY_BASE - humidity) / WEATHER_FIRE_HUMIDITY_SPAN;
    humidityScore = std::max(WEATHER_CLAMP_MIN, std::min(WEATHER_CLAMP_MAX, humidityScore));
    double windScore = (windSpeedKmh - WEATHER_FIRE_WIND_BASE_KMH) / WEATHER_FIRE_WIND_SPAN_KMH;
    windScore = std::max(WEATHER_CLAMP_MIN, std::min(WEATHER_CLAMP_MAX, windScore));
    double weighted = WEATHER_FIRE_TEMP_WEIGHT * tempScore + WEATHER_FIRE_HUMIDITY_WEIGHT * humidityScore +
                      WEATHER_FIRE_WIND_WEIGHT * windScore;
    double score = weighted * WEATHER_FIRE_SCORE_SCALE;
    score = std::max(WEATHER_FIRE_MIN_SCORE, std::min(WEATHER_FIRE_MAX_SCORE, score));
    return CreateDoubleValue(env, score);
}

napi_value MouldRisk(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperature, humidity");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double humidity = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], humidity)) {
        return nullptr;
    }
    NAPI_ASSERT(env, humidity >= WEATHER_MIN_HUMIDITY && humidity <= WEATHER_MAX_HUMIDITY, "Humidity out of range");

    bool risky = humidity >= WEATHER_MOULD_MIN_HUMIDITY && temperatureC >= WEATHER_MOULD_MIN_TEMP_C &&
                 temperatureC <= WEATHER_MOULD_MAX_TEMP_C;
    return CreateBoolValue(env, risky);
}

napi_value GrowingDegreeDay(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: base, min, max temperature");

    double baseTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], baseTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, baseTempC >= WEATHER_MIN_TEMPERATURE_C && baseTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Base temperature out of range");

    double minTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], minTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, minTempC >= WEATHER_MIN_TEMPERATURE_C && minTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Minimum temperature out of range");

    double maxTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], maxTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, maxTempC >= WEATHER_MIN_TEMPERATURE_C && maxTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Maximum temperature out of range");
    NAPI_ASSERT(env, minTempC <= maxTempC, "Minimum temperature must not exceed the maximum");

    if (maxTempC < baseTempC) {
        return CreateDoubleValue(env, WEATHER_DEGREE_DAY_FLOOR);
    }
    double effectiveMin = std::max(minTempC, baseTempC);
    double meanTemp = (effectiveMin + maxTempC) / WEATHER_TEMP_MEAN_DIVISOR;
    double gdd = std::max(meanTemp - baseTempC, WEATHER_DEGREE_DAY_FLOOR);
    return CreateDoubleValue(env, gdd);
}

napi_value CoolingDegreeDay(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: base, mean temperature");

    double baseTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], baseTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, baseTempC >= WEATHER_MIN_TEMPERATURE_C && baseTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Base temperature out of range");

    double meanTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], meanTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, meanTempC >= WEATHER_MIN_TEMPERATURE_C && meanTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Mean temperature out of range");

    return CreateDoubleValue(env, std::max(meanTempC - baseTempC, WEATHER_DEGREE_DAY_FLOOR));
}

napi_value HeatingDegreeDay(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: base, mean temperature");

    double baseTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], baseTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, baseTempC >= WEATHER_MIN_TEMPERATURE_C && baseTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Base temperature out of range");

    double meanTempC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], meanTempC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, meanTempC >= WEATHER_MIN_TEMPERATURE_C && meanTempC <= WEATHER_MAX_TEMPERATURE_C,
                "Mean temperature out of range");

    return CreateDoubleValue(env, std::max(baseTempC - meanTempC, WEATHER_DEGREE_DAY_FLOOR));
}

napi_value SkyConditionFromCloudiness(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: cloud fraction");

    double cloudFraction = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], cloudFraction)) {
        return nullptr;
    }
    NAPI_ASSERT(env, cloudFraction >= WEATHER_MIN_CLOUD_FRACTION && cloudFraction <= WEATHER_MAX_CLOUD_FRACTION,
                "Cloud fraction out of range");

    if (cloudFraction < WEATHER_CLOUD_SCATTERED_MIN) {
        return CreateStringValue(env, "Clear");
    }
    if (cloudFraction < WEATHER_CLOUD_BROKEN_MIN) {
        return CreateStringValue(env, "Scattered");
    }
    if (cloudFraction < WEATHER_CLOUD_OVERCAST_MIN) {
        return CreateStringValue(env, "Broken");
    }
    return CreateStringValue(env, "Overcast");
}

napi_value VisibilityCategory(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: visibility");

    double kilometers = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilometers)) {
        return nullptr;
    }
    NAPI_ASSERT(env, kilometers >= WEATHER_MIN_VISIBILITY_KM && kilometers <= WEATHER_MAX_VISIBILITY_KM,
                "Visibility out of range");

    if (kilometers < WEATHER_VIS_DENSE_FOG_MAX_KM) {
        return CreateStringValue(env, "Dense fog");
    }
    if (kilometers < WEATHER_VIS_FOG_MAX_KM) {
        return CreateStringValue(env, "Fog");
    }
    if (kilometers < WEATHER_VIS_MIST_MAX_KM) {
        return CreateStringValue(env, "Mist");
    }
    if (kilometers < WEATHER_VIS_VERY_CLEAR_MIN_KM) {
        return CreateStringValue(env, "Clear");
    }
    return CreateStringValue(env, "Very clear");
}

napi_value PressureTendency(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure readings");

    std::vector<double> readings;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], readings)) {
        return nullptr;
    }
    NAPI_ASSERT(env, static_cast<int>(readings.size()) >= WEATHER_PRESSURE_MIN_READINGS,
                "At least 3 pressure readings are required");
    if (!ValidateArrayRange(env, readings, WEATHER_MIN_PRESSURE_HPA, WEATHER_MAX_PRESSURE_HPA,
                            "Pressure out of range")) {
        return nullptr;
    }

    double delta = readings[readings.size() - 1] - readings[0];
    if (delta > WEATHER_PRESSURE_EPSILON_HPA) {
        return CreateStringValue(env, "Rising");
    }
    if (delta < -WEATHER_PRESSURE_EPSILON_HPA) {
        return CreateStringValue(env, "Falling");
    }
    return CreateStringValue(env, "Steady");
}

napi_value PressureDelta(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure readings");

    std::vector<double> readings;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], readings)) {
        return nullptr;
    }
    NAPI_ASSERT(env, static_cast<int>(readings.size()) >= WEATHER_PRESSURE_DELTA_MIN_READINGS,
                "At least 2 pressure readings are required");
    if (!ValidateArrayRange(env, readings, WEATHER_MIN_PRESSURE_HPA, WEATHER_MAX_PRESSURE_HPA,
                            "Pressure out of range")) {
        return nullptr;
    }

    return CreateDoubleValue(env, readings[readings.size() - 1] - readings[0]);
}

napi_value StormDistanceKm(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: seconds since the lightning");

    double seconds = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], seconds)) {
        return nullptr;
    }
    NAPI_ASSERT(env, seconds >= WEATHER_MIN_STORM_SECONDS && seconds <= WEATHER_MAX_STORM_SECONDS,
                "Seconds out of range");

    double meters = seconds * WEATHER_SPEED_OF_SOUND_MS;
    return CreateDoubleValue(env, meters / WEATHER_MS_TO_KM_DIVISOR);
}

napi_value IsStormApproaching(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure readings");

    std::vector<double> readings;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], readings)) {
        return nullptr;
    }
    NAPI_ASSERT(env, static_cast<int>(readings.size()) >= WEATHER_STORM_MIN_READINGS,
                "At least 3 pressure readings are required");
    if (!ValidateArrayRange(env, readings, WEATHER_MIN_PRESSURE_HPA, WEATHER_MAX_PRESSURE_HPA,
                            "Pressure out of range")) {
        return nullptr;
    }

    for (size_t i = 1; i < readings.size(); ++i) {
        if (readings[i] >= readings[i - 1]) {
            return CreateBoolValue(env, false);
        }
    }
    return CreateBoolValue(env, true);
}

napi_value DayLengthEstimate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: latitude, day of year");

    double latitudeDeg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], latitudeDeg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, latitudeDeg >= WEATHER_MIN_LATITUDE_DEG && latitudeDeg <= WEATHER_MAX_LATITUDE_DEG,
                "Latitude out of range");

    double dayValue = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], dayValue)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::floor(dayValue) == dayValue, "Day of year must be an integer");
    NAPI_ASSERT(env,
                dayValue >= static_cast<double>(WEATHER_MIN_DAY_OF_YEAR) &&
                    dayValue <= static_cast<double>(WEATHER_MAX_DAY_OF_YEAR),
                "Day of year out of range");

    double latitudeRad = latitudeDeg * WEATHER_DEG_TO_RAD;
    double declinationDeg =
        WEATHER_SOLAR_DECLINATION_DEG * std::sin(WEATHER_DEG_TO_RAD * WEATHER_DECLINATION_PERIOD_DEG *
                                                 (WEATHER_DECLINATION_PHASE_DAYS + dayValue) / WEATHER_DAYS_PER_YEAR);
    double declinationRad = declinationDeg * WEATHER_DEG_TO_RAD;
    double cosHourAngle = -std::tan(latitudeRad) * std::tan(declinationRad);
    if (cosHourAngle <= WEATHER_ACOS_CLAMP_MIN) {
        return CreateDoubleValue(env, WEATHER_POLAR_DAY_LENGTH_H);
    }
    if (cosHourAngle >= WEATHER_ACOS_CLAMP_MAX) {
        return CreateDoubleValue(env, WEATHER_POLAR_NIGHT_LENGTH_H);
    }
    double hourAngleDeg = std::acos(cosHourAngle) * WEATHER_RAD_TO_DEG;
    double dayLength = WEATHER_DAY_LENGTH_FACTOR * hourAngleDeg / WEATHER_HOUR_ANGLE_DIVISOR;
    return CreateDoubleValue(env, dayLength);
}

napi_value TemperatureAnomaly(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperatures array, value");

    std::vector<double> temperatures;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], temperatures)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatures.size() > WEATHER_EMPTY_ARRAY_SIZE, "Temperatures array must not be empty");
    if (!ValidateArrayRange(env, temperatures, WEATHER_MIN_TEMPERATURE_C, WEATHER_MAX_TEMPERATURE_C,
                            "Temperature out of range")) {
        return nullptr;
    }

    double value = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], value)) {
        return nullptr;
    }
    NAPI_ASSERT(env, value >= WEATHER_MIN_TEMPERATURE_C && value <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double sum = 0.0;
    for (size_t i = 0; i < temperatures.size(); ++i) {
        sum += temperatures[i];
    }
    double size = static_cast<double>(temperatures.size());
    double safeSize = size > 0.0 ? size : 1.0;
    double mean = sum / safeSize;
    return CreateDoubleValue(env, value - mean);
}

napi_value DiurnalRange(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: temperatures array");

    std::vector<double> temperatures;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], temperatures)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatures.size() > WEATHER_EMPTY_ARRAY_SIZE, "Temperatures array must not be empty");
    if (!ValidateArrayRange(env, temperatures, WEATHER_MIN_TEMPERATURE_C, WEATHER_MAX_TEMPERATURE_C,
                            "Temperature out of range")) {
        return nullptr;
    }

    double minimum = temperatures[0];
    double maximum = temperatures[0];
    for (size_t i = 0; i < temperatures.size(); ++i) {
        minimum = std::min(minimum, temperatures[i]);
        maximum = std::max(maximum, temperatures[i]);
    }
    return CreateDoubleValue(env, maximum - minimum);
}

napi_status RegisterWeatherOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("averageWindDirection", AverageWindDirection),
        DECLARE_NAPI_FUNCTION("windDirectionSpread", WindDirectionSpread),
        DECLARE_NAPI_FUNCTION("windGustFactor", WindGustFactor),
        DECLARE_NAPI_FUNCTION("isWindChillDangerous", IsWindChillDangerous),
        DECLARE_NAPI_FUNCTION("heatWarningLevel", HeatWarningLevel),
        DECLARE_NAPI_FUNCTION("precipitationIntensity", PrecipitationIntensity),
        DECLARE_NAPI_FUNCTION("snowDepthEstimate", SnowDepthEstimate),
        DECLARE_NAPI_FUNCTION("snowWaterEquivalent", SnowWaterEquivalent),
        DECLARE_NAPI_FUNCTION("fireRiskIndex", FireRiskIndex),
        DECLARE_NAPI_FUNCTION("mouldRisk", MouldRisk),
        DECLARE_NAPI_FUNCTION("growingDegreeDay", GrowingDegreeDay),
        DECLARE_NAPI_FUNCTION("coolingDegreeDay", CoolingDegreeDay),
        DECLARE_NAPI_FUNCTION("heatingDegreeDay", HeatingDegreeDay),
        DECLARE_NAPI_FUNCTION("skyConditionFromCloudiness", SkyConditionFromCloudiness),
        DECLARE_NAPI_FUNCTION("visibilityCategory", VisibilityCategory),
        DECLARE_NAPI_FUNCTION("pressureTendency", PressureTendency),
        DECLARE_NAPI_FUNCTION("pressureDelta", PressureDelta),
        DECLARE_NAPI_FUNCTION("stormDistanceKm", StormDistanceKm),
        DECLARE_NAPI_FUNCTION("isStormApproaching", IsStormApproaching),
        DECLARE_NAPI_FUNCTION("dayLengthEstimate", DayLengthEstimate),
        DECLARE_NAPI_FUNCTION("temperatureAnomaly", TemperatureAnomaly),
        DECLARE_NAPI_FUNCTION("diurnalRange", DiurnalRange),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
