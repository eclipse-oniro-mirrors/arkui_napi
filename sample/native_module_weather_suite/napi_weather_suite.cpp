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

#include "napi/native_node_api.h"
#include "weather_suite_common.h"

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
    if (length > WEATHER_MAX_ARRAY_LENGTH) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = WEATHER_UINT32_INDEX_ZERO; i < length; ++i) {
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

static double DewPointValue(double temperatureC, double humidityPercent)
{
    double denominator = WEATHER_MAGNUS_B + temperatureC;
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    double gamma =
        std::log(humidityPercent / WEATHER_HUMIDITY_PERCENT) + WEATHER_MAGNUS_A * temperatureC / safeDenominator;
    double gammaDenominator = WEATHER_MAGNUS_A - gamma;
    double safeGammaDenominator = gammaDenominator > 0.0 ? gammaDenominator : 1.0;
    return WEATHER_MAGNUS_B * gamma / safeGammaDenominator;
}

static double FrostPointValue(double temperatureC, double humidityPercent)
{
    double denominator = WEATHER_MAGNUS_ICE_B + temperatureC;
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    double gamma =
        std::log(humidityPercent / WEATHER_HUMIDITY_PERCENT) + WEATHER_MAGNUS_ICE_A * temperatureC / safeDenominator;
    double gammaDenominator = WEATHER_MAGNUS_ICE_A - gamma;
    double safeGammaDenominator = gammaDenominator > 0.0 ? gammaDenominator : 1.0;
    return WEATHER_MAGNUS_ICE_B * gamma / safeGammaDenominator;
}

static double WindChillValue(double temperatureC, double windSpeedKmh)
{
    double windFactor = std::pow(windSpeedKmh, WEATHER_WINDCHILL_WIND_EXPONENT);
    return WEATHER_WINDCHILL_INTERCEPT + WEATHER_WINDCHILL_TEMP_COEFF * temperatureC -
           WEATHER_WINDCHILL_WIND_COEFF * windFactor + WEATHER_WINDCHILL_CROSS_COEFF * temperatureC * windFactor;
}

static double HeatIndexValue(double temperatureC, double humidityPercent)
{
    double temperatureF = WEATHER_C_TO_F_SCALE * temperatureC + WEATHER_C_TO_F_OFFSET;
    double temperatureFSq = temperatureF * temperatureF;
    double humiditySq = humidityPercent * humidityPercent;
    double indexF = WEATHER_HEAT_INDEX_F_INTERCEPT + WEATHER_HEAT_INDEX_F_TEMP_COEFF * temperatureF +
                    WEATHER_HEAT_INDEX_F_HUMIDITY_COEFF * humidityPercent +
                    WEATHER_HEAT_INDEX_F_CROSS_COEFF * temperatureF * humidityPercent +
                    WEATHER_HEAT_INDEX_F_TEMP_SQ_COEFF * temperatureFSq +
                    WEATHER_HEAT_INDEX_F_HUMIDITY_SQ_COEFF * humiditySq +
                    WEATHER_HEAT_INDEX_F_TEMP_SQ_HUM_COEFF * temperatureFSq * humidityPercent +
                    WEATHER_HEAT_INDEX_F_TEMP_HUM_SQ_COEFF * temperatureF * humiditySq +
                    WEATHER_HEAT_INDEX_F_TEMP_SQ_HUM_SQ_COEFF * temperatureFSq * humiditySq;
    return (indexF - WEATHER_C_TO_F_OFFSET) * WEATHER_F_TO_C_SCALE;
}

static double HumidexValue(double temperatureC, double humidityPercent)
{
    double vaporPressure = ActualVaporPressureValue(temperatureC, humidityPercent);
    return temperatureC + WEATHER_HUMIDEX_VAPOR_COEFF * (vaporPressure - WEATHER_HUMIDEX_VAPOR_OFFSET);
}

napi_value HeatIndexCelsius(napi_env env, napi_callback_info info)
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

    return CreateDoubleValue(env, HeatIndexValue(temperatureC, humidity));
}

napi_value WindChillCelsius(napi_env env, napi_callback_info info)
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

    return CreateDoubleValue(env, WindChillValue(temperatureC, windSpeedKmh));
}

napi_value DewPointCelsius(napi_env env, napi_callback_info info)
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
    NAPI_ASSERT(env, humidity > WEATHER_MIN_HUMIDITY, "Humidity must be positive for dew point");

    return CreateDoubleValue(env, DewPointValue(temperatureC, humidity));
}

napi_value FrostPointCelsius(napi_env env, napi_callback_info info)
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
    NAPI_ASSERT(env, humidity > WEATHER_MIN_HUMIDITY, "Humidity must be positive for frost point");

    return CreateDoubleValue(env, FrostPointValue(temperatureC, humidity));
}

napi_value SaturationVaporPressureHpa(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: temperature");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    return CreateDoubleValue(env, SaturationVaporPressureValue(temperatureC));
}

napi_value ActualVaporPressureHpa(napi_env env, napi_callback_info info)
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

    return CreateDoubleValue(env, ActualVaporPressureValue(temperatureC, humidity));
}

napi_value AbsoluteHumidity(napi_env env, napi_callback_info info)
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

    double vaporPressure = ActualVaporPressureValue(temperatureC, humidity);
    double kelvin = temperatureC + WEATHER_KELVIN_OFFSET_C;
    double safeKelvin = kelvin > 0.0 ? kelvin : 1.0;
    double humidityGPerM3 = WEATHER_ABS_HUMIDITY_COEFF * vaporPressure / safeKelvin;
    return CreateDoubleValue(env, humidityGPerM3);
}

napi_value SpecificHumidity(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: temperature, humidity, pressure");

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

    double pressureHpa = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], pressureHpa)) {
        return nullptr;
    }
    NAPI_ASSERT(env, pressureHpa >= WEATHER_MIN_PRESSURE_HPA && pressureHpa <= WEATHER_MAX_PRESSURE_HPA,
                "Pressure out of range");

    double vaporPressure = ActualVaporPressureValue(temperatureC, humidity);
    double denominator = pressureHpa - WEATHER_SPECIFIC_HUMIDITY_VAPOR_FACTOR * vaporPressure;
    NAPI_ASSERT(env, denominator > 0.0, "Pressure must dominate the vapor pressure");
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    double humidityGPerKg = WEATHER_MOLAR_WEIGHT_RATIO * vaporPressure / safeDenominator;
    return CreateDoubleValue(env, humidityGPerKg);
}

napi_value MixingRatio(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: temperature, humidity, pressure");

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

    double pressureHpa = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], pressureHpa)) {
        return nullptr;
    }
    NAPI_ASSERT(env, pressureHpa >= WEATHER_MIN_PRESSURE_HPA && pressureHpa <= WEATHER_MAX_PRESSURE_HPA,
                "Pressure out of range");

    double vaporPressure = ActualVaporPressureValue(temperatureC, humidity);
    double denominator = pressureHpa - vaporPressure;
    NAPI_ASSERT(env, denominator > 0.0, "Pressure must exceed the vapor pressure");
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    double ratioGPerKg = WEATHER_MOLAR_WEIGHT_RATIO * vaporPressure / safeDenominator;
    return CreateDoubleValue(env, ratioGPerKg);
}

napi_value RelativeHumidityFromDew(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperature, dew point");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double dewPointC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], dewPointC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, dewPointC >= WEATHER_MIN_TEMPERATURE_C && dewPointC <= WEATHER_MAX_TEMPERATURE_C,
                "Dew point out of range");

    double saturationAtTemp = SaturationVaporPressureValue(temperatureC);
    double saturationAtDew = SaturationVaporPressureValue(dewPointC);
    double safeSaturation = saturationAtTemp > 0.0 ? saturationAtTemp : 1.0;
    double humidityPercent = WEATHER_HUMIDITY_PERCENT * saturationAtDew / safeSaturation;
    return CreateDoubleValue(env, humidityPercent);
}

napi_value ApparentTemperatureCelsius(napi_env env, napi_callback_info info)
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

    double vaporPressure = ActualVaporPressureValue(temperatureC, humidity);
    double windMs = windSpeedKmh / WEATHER_KMH_TO_MS_DIVISOR;
    double apparent =
        temperatureC + WEATHER_AT_VAPOR_COEFF * vaporPressure - WEATHER_AT_WIND_COEFF * windMs + WEATHER_AT_CONSTANT;
    return CreateDoubleValue(env, apparent);
}

napi_value HumidexCelsius(napi_env env, napi_callback_info info)
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

    return CreateDoubleValue(env, HumidexValue(temperatureC, humidity));
}

napi_value WetBulbTemperature(napi_env env, napi_callback_info info)
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

    double humiditySqrt =
        WEATHER_STULL_SQRT_COEFF * std::pow(humidity + WEATHER_STULL_HUM_OFFSET, WEATHER_STULL_HUM_POWER);
    double wetBulb = temperatureC * std::atan(humiditySqrt) + std::atan(temperatureC + humidity) -
                     std::atan(humidity - WEATHER_STULL_SUB_HUM_OFFSET) +
                     WEATHER_STULL_TERM4_COEFF * std::pow(humidity, WEATHER_STULL_TERM4_POWER) *
                         std::atan(WEATHER_STULL_TERM4_ATAN_COEFF * humidity) -
                     WEATHER_STULL_OFFSET;
    return CreateDoubleValue(env, wetBulb);
}

napi_value BeaufortLevel(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: wind speed");

    double windSpeedKmh = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], windSpeedKmh)) {
        return nullptr;
    }
    NAPI_ASSERT(env, windSpeedKmh >= WEATHER_MIN_WIND_KMH && windSpeedKmh <= WEATHER_MAX_WIND_KMH,
                "Wind speed out of range");

    int32_t level = 0;
    for (int i = 0; i < WEATHER_BEAUFORT_THRESHOLD_COUNT; ++i) {
        if (windSpeedKmh >= WEATHER_BEAUFORT_THRESHOLDS_KMH[i]) {
            level = i + 1;
        }
    }
    return CreateInt32Value(env, level);
}

napi_value BeaufortDescription(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: beaufort level");

    double levelValue = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], levelValue)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::floor(levelValue) == levelValue, "Beaufort level must be an integer");
    NAPI_ASSERT(env,
                levelValue >= static_cast<double>(WEATHER_BEAUFORT_MIN_LEVEL) &&
                    levelValue < static_cast<double>(WEATHER_BEAUFORT_LEVEL_COUNT),
                "Beaufort level out of range");

    int32_t level = static_cast<int32_t>(levelValue);
    return CreateStringValue(env, WEATHER_BEAUFORT_DESCRIPTIONS[level]);
}

napi_value UvIndexCategory(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: uv index");

    double uvIndex = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], uvIndex)) {
        return nullptr;
    }
    NAPI_ASSERT(env, uvIndex >= WEATHER_MIN_UV_INDEX && uvIndex <= WEATHER_MAX_UV_INDEX, "Uv index out of range");

    if (uvIndex < WEATHER_UV_MODERATE_MIN) {
        return CreateStringValue(env, "Low");
    }
    if (uvIndex < WEATHER_UV_HIGH_MIN) {
        return CreateStringValue(env, "Moderate");
    }
    if (uvIndex < WEATHER_UV_VERY_HIGH_MIN) {
        return CreateStringValue(env, "High");
    }
    if (uvIndex < WEATHER_UV_EXTREME_MIN) {
        return CreateStringValue(env, "Very high");
    }
    return CreateStringValue(env, "Extreme");
}

napi_value TemperatureHumidityIndex(napi_env env, napi_callback_info info)
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

    double thi = temperatureC - (WEATHER_THI_HUMIDITY_BASE - WEATHER_THI_HUMIDITY_COEFF * humidity) *
                                    (temperatureC - WEATHER_THI_REFERENCE_TEMP_C);
    return CreateDoubleValue(env, thi);
}

napi_value CloudBaseMeters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperature, dew point");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double dewPointC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], dewPointC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, dewPointC >= WEATHER_MIN_TEMPERATURE_C && dewPointC <= WEATHER_MAX_TEMPERATURE_C,
                "Dew point out of range");

    double spread = std::max(temperatureC - dewPointC, WEATHER_SPREAD_FLOOR_C);
    return CreateDoubleValue(env, spread * WEATHER_CLOUD_BASE_SPREAD_FACTOR);
}

napi_value AirDensity(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: temperature, pressure");

    double temperatureC = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], temperatureC)) {
        return nullptr;
    }
    NAPI_ASSERT(env, temperatureC >= WEATHER_MIN_TEMPERATURE_C && temperatureC <= WEATHER_MAX_TEMPERATURE_C,
                "Temperature out of range");

    double pressureHpa = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], pressureHpa)) {
        return nullptr;
    }
    NAPI_ASSERT(env, pressureHpa >= WEATHER_MIN_PRESSURE_HPA && pressureHpa <= WEATHER_MAX_PRESSURE_HPA,
                "Pressure out of range");

    double kelvin = temperatureC + WEATHER_KELVIN_OFFSET_C;
    double safeKelvin = kelvin > 0.0 ? kelvin : 1.0;
    double denominator = WEATHER_DRY_AIR_GAS_CONSTANT * safeKelvin;
    double safeDenominator = denominator > 0.0 ? denominator : 1.0;
    double density = pressureHpa * WEATHER_HPA_TO_PA / safeDenominator;
    return CreateDoubleValue(env, density);
}

napi_value AltitudePressureHpa(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: altitude");

    double altitudeM = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], altitudeM)) {
        return nullptr;
    }
    NAPI_ASSERT(env, altitudeM >= WEATHER_MIN_ALTITUDE_M && altitudeM <= WEATHER_MAX_ALTITUDE_M,
                "Altitude out of range");

    double lapseRatio = WEATHER_ISA_LAPSE_FACTOR * altitudeM;
    double pressure = WEATHER_ISA_SEA_LEVEL_PRESSURE_HPA *
                      std::pow(WEATHER_ISA_UNIT_RATIO - lapseRatio, WEATHER_ISA_PRESSURE_EXPONENT);
    return CreateDoubleValue(env, pressure);
}

napi_value PressureAltitudeMeters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure");

    double pressureHpa = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], pressureHpa)) {
        return nullptr;
    }
    NAPI_ASSERT(env, pressureHpa >= WEATHER_MIN_PRESSURE_HPA && pressureHpa <= WEATHER_MAX_PRESSURE_HPA,
                "Pressure out of range");

    double pressureRatio = pressureHpa / WEATHER_ISA_SEA_LEVEL_PRESSURE_HPA;
    double pressureTerm = std::pow(pressureRatio, WEATHER_ISA_UNIT_RATIO / WEATHER_ISA_PRESSURE_EXPONENT);
    double altitudeM = (WEATHER_ISA_UNIT_RATIO - pressureTerm) / WEATHER_ISA_LAPSE_FACTOR;
    return CreateDoubleValue(env, altitudeM);
}

napi_value ComfortScore(napi_env env, napi_callback_info info)
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

    double tempPenalty = WEATHER_COMFORT_TEMP_PENALTY * std::abs(temperatureC - WEATHER_COMFORT_IDEAL_TEMP_C);
    double humidityPenalty = WEATHER_COMFORT_HUMIDITY_PENALTY * std::abs(humidity - WEATHER_COMFORT_IDEAL_HUMIDITY);
    double score = WEATHER_COMFORT_MAX_SCORE - tempPenalty - humidityPenalty;
    score = std::max(WEATHER_COMFORT_MIN_SCORE, std::min(WEATHER_COMFORT_MAX_SCORE, score));
    return CreateDoubleValue(env, score);
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("heatIndexCelsius", HeatIndexCelsius),
        DECLARE_NAPI_FUNCTION("windChillCelsius", WindChillCelsius),
        DECLARE_NAPI_FUNCTION("dewPointCelsius", DewPointCelsius),
        DECLARE_NAPI_FUNCTION("frostPointCelsius", FrostPointCelsius),
        DECLARE_NAPI_FUNCTION("saturationVaporPressureHpa", SaturationVaporPressureHpa),
        DECLARE_NAPI_FUNCTION("actualVaporPressureHpa", ActualVaporPressureHpa),
        DECLARE_NAPI_FUNCTION("absoluteHumidity", AbsoluteHumidity),
        DECLARE_NAPI_FUNCTION("specificHumidity", SpecificHumidity),
        DECLARE_NAPI_FUNCTION("mixingRatio", MixingRatio),
        DECLARE_NAPI_FUNCTION("relativeHumidityFromDew", RelativeHumidityFromDew),
        DECLARE_NAPI_FUNCTION("apparentTemperatureCelsius", ApparentTemperatureCelsius),
        DECLARE_NAPI_FUNCTION("humidexCelsius", HumidexCelsius),
        DECLARE_NAPI_FUNCTION("wetBulbTemperature", WetBulbTemperature),
        DECLARE_NAPI_FUNCTION("beaufortLevel", BeaufortLevel),
        DECLARE_NAPI_FUNCTION("beaufortDescription", BeaufortDescription),
        DECLARE_NAPI_FUNCTION("uvIndexCategory", UvIndexCategory),
        DECLARE_NAPI_FUNCTION("temperatureHumidityIndex", TemperatureHumidityIndex),
        DECLARE_NAPI_FUNCTION("cloudBaseMeters", CloudBaseMeters),
        DECLARE_NAPI_FUNCTION("airDensity", AirDensity),
        DECLARE_NAPI_FUNCTION("altitudePressureHpa", AltitudePressureHpa),
        DECLARE_NAPI_FUNCTION("pressureAltitudeMeters", PressureAltitudeMeters),
        DECLARE_NAPI_FUNCTION("comfortScore", ComfortScore),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterWeatherOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module weatherSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "weatherSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void WeatherSuiteRegisterModule(void)
{
    napi_module_register(&weatherSuiteModule);
}
