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

#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

// === Constants ===

// --- Argument Counts ---
constexpr size_t K_ARGS_ZERO = 0;

// --- Case Count ---
constexpr size_t K_TEMP_CASE_COUNT = 27;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Temperature Operation Cycle ---
constexpr size_t K_TEMP_OP_CYCLE = 3;
constexpr size_t K_TEMP_OP_INDEX_TO_FAHRENHEIT = 0;
constexpr size_t K_TEMP_OP_INDEX_TO_CELSIUS = 1;
constexpr size_t K_TEMP_OP_INDEX_TO_KELVIN = 2;

// --- Temperature Value Derivation ---
constexpr double K_TEMP_STEP = 5.0;
constexpr double K_TEMP_OFFSET = 45.0;

// --- Conversion Coefficients ---
constexpr double K_TEMP_SCALE_9 = 9.0;
constexpr double K_TEMP_SCALE_5 = 5.0;
constexpr double K_TEMP_FROZEN_32 = 32.0;
constexpr double K_TEMP_KELVIN_OFFSET = 273.15;

// --- Reference Points ---
constexpr double K_TEMP_FREEZING_WATER_CELSIUS = 0.0;
constexpr double K_TEMP_BOILING_WATER_CELSIUS = 100.0;
constexpr double K_TEMP_ABSOLUTE_ZERO_CELSIUS = -273.15;
constexpr double K_TEMP_BODY_CELSIUS = 37.0;
constexpr double K_TEMP_ROOM_CELSIUS = 20.0;
constexpr double K_TEMP_FREEZING_WATER_KELVIN = K_TEMP_FREEZING_WATER_CELSIUS + K_TEMP_KELVIN_OFFSET;
constexpr double K_TEMP_BOILING_WATER_KELVIN = K_TEMP_BOILING_WATER_CELSIUS + K_TEMP_KELVIN_OFFSET;
constexpr double K_TEMP_ABSOLUTE_ZERO_KELVIN = K_TEMP_ABSOLUTE_ZERO_CELSIUS + K_TEMP_KELVIN_OFFSET;

// --- Round-trip Tolerance ---
constexpr double K_TEMP_ROUNDTRIP_TOLERANCE = 1e-9;

// --- Test Values (Doubles) ---
constexpr double K_DOUBLE_ZERO = 0.0;

// --- Unit Labels ---
const char* const K_TEMP_UNIT_CELSIUS = "celsius";
const char* const K_TEMP_UNIT_FAHRENHEIT = "fahrenheit";
const char* const K_TEMP_UNIT_KELVIN = "kelvin";

// --- Operation Labels ---
const char* const K_TEMP_OP_LABEL_TO_FAHRENHEIT = "celsius_to_fahrenheit";
const char* const K_TEMP_OP_LABEL_TO_CELSIUS = "fahrenheit_to_celsius";
const char* const K_TEMP_OP_LABEL_TO_KELVIN = "celsius_to_kelvin";

// --- Operation Descriptions ---
const char* const K_TEMP_OP_DESC_TO_FAHRENHEIT = "convert celsius to fahrenheit via c*9/5+32";
const char* const K_TEMP_OP_DESC_TO_CELSIUS = "convert fahrenheit to celsius via (f-32)*5/9";
const char* const K_TEMP_OP_DESC_TO_KELVIN = "convert celsius to kelvin via c+273.15";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class TempOp {
    CELSIUS_TO_FAHRENHEIT,
    FAHRENHEIT_TO_CELSIUS,
    CELSIUS_TO_KELVIN,
};

struct TemperatureCaseSpec {
    std::string name;
    TempOp op;
    size_t caseNumber;
    double value;
};

// === Helpers: Naming & Case Lookup ===

std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(data));
}

TemperatureCaseSpec GetTemperatureCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    TemperatureCaseSpec spec;
    spec.name = BuildIndexedName("temperatureCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.value = static_cast<double>(caseNumber) * K_TEMP_STEP - K_TEMP_OFFSET;
    const size_t cycle = caseNumber % K_TEMP_OP_CYCLE;
    if (cycle == K_TEMP_OP_INDEX_TO_FAHRENHEIT) {
        spec.op = TempOp::CELSIUS_TO_FAHRENHEIT;
    } else if (cycle == K_TEMP_OP_INDEX_TO_CELSIUS) {
        spec.op = TempOp::FAHRENHEIT_TO_CELSIUS;
    } else {
        spec.op = TempOp::CELSIUS_TO_KELVIN;
    }
    return spec;
}

const char* TempOpLabel(TempOp op)
{
    if (op == TempOp::CELSIUS_TO_FAHRENHEIT) {
        return K_TEMP_OP_LABEL_TO_FAHRENHEIT;
    }
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return K_TEMP_OP_LABEL_TO_CELSIUS;
    }
    return K_TEMP_OP_LABEL_TO_KELVIN;
}

const char* TempOpDescription(TempOp op)
{
    if (op == TempOp::CELSIUS_TO_FAHRENHEIT) {
        return K_TEMP_OP_DESC_TO_FAHRENHEIT;
    }
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return K_TEMP_OP_DESC_TO_CELSIUS;
    }
    return K_TEMP_OP_DESC_TO_KELVIN;
}

const char* TempOpInputUnit(TempOp op)
{
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return K_TEMP_UNIT_FAHRENHEIT;
    }
    return K_TEMP_UNIT_CELSIUS;
}

const char* TempOpOutputUnit(TempOp op)
{
    if (op == TempOp::CELSIUS_TO_KELVIN) {
        return K_TEMP_UNIT_KELVIN;
    }
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return K_TEMP_UNIT_CELSIUS;
    }
    return K_TEMP_UNIT_FAHRENHEIT;
}

// === Helpers: Temperature Conversions ===

double CelsiusToFahrenheit(double celsius)
{
    return celsius * K_TEMP_SCALE_9 / K_TEMP_SCALE_5 + K_TEMP_FROZEN_32;
}

double FahrenheitToCelsius(double fahrenheit)
{
    return (fahrenheit - K_TEMP_FROZEN_32) * K_TEMP_SCALE_5 / K_TEMP_SCALE_9;
}

double CelsiusToKelvin(double celsius)
{
    return celsius + K_TEMP_KELVIN_OFFSET;
}

double KelvinToCelsius(double kelvin)
{
    return kelvin - K_TEMP_KELVIN_OFFSET;
}

double KelvinToFahrenheit(double kelvin)
{
    return CelsiusToFahrenheit(KelvinToCelsius(kelvin));
}

double FahrenheitToKelvin(double fahrenheit)
{
    return CelsiusToKelvin(FahrenheitToCelsius(fahrenheit));
}

double KelvinToRankine(double kelvin)
{
    return kelvin * K_TEMP_SCALE_9 / K_TEMP_SCALE_5;
}

double RankineToKelvin(double rankine)
{
    return rankine * K_TEMP_SCALE_5 / K_TEMP_SCALE_9;
}

double EvaluateConversion(const TemperatureCaseSpec& spec)
{
    if (spec.op == TempOp::CELSIUS_TO_FAHRENHEIT) {
        return CelsiusToFahrenheit(spec.value);
    }
    if (spec.op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return FahrenheitToCelsius(spec.value);
    }
    return CelsiusToKelvin(spec.value);
}

double ReverseConversion(TempOp op, double output)
{
    if (op == TempOp::CELSIUS_TO_FAHRENHEIT) {
        return FahrenheitToCelsius(output);
    }
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return CelsiusToFahrenheit(output);
    }
    return KelvinToCelsius(output);
}

bool IsRoundtripConsistent(const TemperatureCaseSpec& spec, double output)
{
    const double reversed = ReverseConversion(spec.op, output);
    const double delta = reversed - spec.value;
    const double magnitude = (delta < K_DOUBLE_ZERO) ? -delta : delta;
    return magnitude < K_TEMP_ROUNDTRIP_TOLERANCE;
}

double KelvinEquivalent(TempOp op, double output)
{
    if (op == TempOp::CELSIUS_TO_KELVIN) {
        return output;
    }
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return CelsiusToKelvin(output);
    }
    return CelsiusToKelvin(FahrenheitToCelsius(output));
}

double FreezingPointDelta(TempOp op, double output)
{
    if (op == TempOp::CELSIUS_TO_KELVIN) {
        return output - K_TEMP_KELVIN_OFFSET;
    }
    if (op == TempOp::FAHRENHEIT_TO_CELSIUS) {
        return output - K_TEMP_FREEZING_WATER_CELSIUS;
    }
    return output - K_TEMP_FROZEN_32;
}

bool IsAboveFreezing(double freezingDelta)
{
    return freezingDelta > K_DOUBLE_ZERO;
}

bool IsAboveAbsoluteZero(double kelvinValue)
{
    return kelvinValue > K_TEMP_ABSOLUTE_ZERO_KELVIN;
}

bool IsFreezing(double kelvinValue)
{
    return kelvinValue <= K_TEMP_FREEZING_WATER_KELVIN;
}

bool IsBoiling(double kelvinValue)
{
    return kelvinValue >= K_TEMP_BOILING_WATER_KELVIN;
}

bool IsAboveRoomTemp(double kelvinValue)
{
    return KelvinToCelsius(kelvinValue) > K_TEMP_ROOM_CELSIUS;
}

bool IsCrossScaleConsistent(double kelvinValue)
{
    const double fahrenheitRoundtrip = FahrenheitToKelvin(KelvinToFahrenheit(kelvinValue));
    const double rankineRoundtrip = RankineToKelvin(KelvinToRankine(kelvinValue));
    const double fahrenheitDelta = fahrenheitRoundtrip - kelvinValue;
    const double rankineDelta = rankineRoundtrip - kelvinValue;
    const double fahrenheitMagnitude = (fahrenheitDelta < K_DOUBLE_ZERO) ? -fahrenheitDelta : fahrenheitDelta;
    const double rankineMagnitude = (rankineDelta < K_DOUBLE_ZERO) ? -rankineDelta : rankineDelta;
    return (fahrenheitMagnitude < K_TEMP_ROUNDTRIP_TOLERANCE) &&
        (rankineMagnitude < K_TEMP_ROUNDTRIP_TOLERANCE);
}

// === Helpers: Object Property Setters ===

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedDouble(napi_env env, napi_value object, const char* name, double value)
{
    napi_value napiValue = nullptr;
    if (napi_create_double(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Result Building ===

napi_value BuildTemperatureResult(napi_env env, const TemperatureCaseSpec& spec, double output,
    bool roundtripConsistent, double kelvinEquivalent, double freezingDelta, bool aboveFreezing,
    double rankineValue, double fahrenheitEquivalent, bool isAboveAbsoluteZero, bool isFreezing,
    bool isBoiling, bool isAboveRoomTemp, bool crossScaleConsistent)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", TempOpLabel(spec.op));
    SetNamedString(env, result, "description", TempOpDescription(spec.op));
    SetNamedInt32(env, result, "caseNumber", static_cast<int32_t>(spec.caseNumber));
    SetNamedDouble(env, result, "input", spec.value);
    SetNamedDouble(env, result, "output", output);
    SetNamedString(env, result, "inputUnit", TempOpInputUnit(spec.op));
    SetNamedString(env, result, "outputUnit", TempOpOutputUnit(spec.op));
    SetNamedBool(env, result, "roundtripConsistent", roundtripConsistent);
    SetNamedDouble(env, result, "kelvinEquivalent", kelvinEquivalent);
    SetNamedDouble(env, result, "freezingDelta", freezingDelta);
    SetNamedBool(env, result, "aboveFreezing", aboveFreezing);
    SetNamedDouble(env, result, "rankineValue", rankineValue);
    SetNamedDouble(env, result, "fahrenheitEquivalent", fahrenheitEquivalent);
    SetNamedBool(env, result, "isAboveAbsoluteZero", isAboveAbsoluteZero);
    SetNamedBool(env, result, "isFreezing", isFreezing);
    SetNamedBool(env, result, "isBoiling", isBoiling);
    SetNamedBool(env, result, "isAboveRoomTemp", isAboveRoomTemp);
    SetNamedBool(env, result, "crossScaleConsistent", crossScaleConsistent);
    return result;
}

// === Test Entry ===

static napi_value RunTemperatureCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_TEMP_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid temperature case");
        return nullptr;
    }
    const auto spec = GetTemperatureCaseSpec(caseIndex);
    const double output = EvaluateConversion(spec);
    const bool roundtripConsistent = IsRoundtripConsistent(spec, output);
    const double kelvinEquivalent = KelvinEquivalent(spec.op, output);
    const double freezingDelta = FreezingPointDelta(spec.op, output);
    const bool aboveFreezing = IsAboveFreezing(freezingDelta);
    const double rankineValue = KelvinToRankine(kelvinEquivalent);
    const double fahrenheitEquivalent = KelvinToFahrenheit(kelvinEquivalent);
    const bool isAboveAbsoluteZero = IsAboveAbsoluteZero(kelvinEquivalent);
    const bool isFreezing = IsFreezing(kelvinEquivalent);
    const bool isBoiling = IsBoiling(kelvinEquivalent);
    const bool isAboveRoomTemp = IsAboveRoomTemp(kelvinEquivalent);
    const bool crossScaleConsistent = IsCrossScaleConsistent(kelvinEquivalent);
    return BuildTemperatureResult(env, spec, output, roundtripConsistent, kelvinEquivalent,
        freezingDelta, aboveFreezing, rankineValue, fahrenheitEquivalent, isAboveAbsoluteZero,
        isFreezing, isBoiling, isAboveRoomTemp, crossScaleConsistent);
}

}  // namespace

static napi_value InitTemperatureSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_TEMP_CASE_COUNT);
    exportNames.reserve(K_TEMP_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_TEMP_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testTemperatureCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunTemperatureCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_temperatureSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitTemperatureSuite,
    .nm_modname = "temperature_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterTemperatureSuiteModule(void)
{
    napi_module_register(&g_temperatureSuiteModule);
}
