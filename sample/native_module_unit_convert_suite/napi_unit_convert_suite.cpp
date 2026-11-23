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

#include "napi/native_node_api.h"
#include "unit_convert_suite_common.h"

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

napi_value CreateDoubleValue(napi_env env, double value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_double(env, value, &result));
    return result;
}

static double MetersToFeetValue(double meters)
{
    return meters * UNIT_FEET_PER_METER;
}

// Converts meters to feet using the international foot definition.
napi_value MetersToFeet(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: length in meters");

    double meters = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], meters)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(meters) >= UNIT_MIN_ABS_INPUT && std::abs(meters) <= UNIT_MAX_ABS_INPUT,
                "Length out of range");

    return CreateDoubleValue(env, MetersToFeetValue(meters));
}

static double FeetToMetersValue(double feet)
{
    return feet * UNIT_METERS_PER_FOOT;
}

// Converts feet to meters using the international foot definition.
napi_value FeetToMeters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: length in feet");

    double feet = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], feet)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(feet) >= UNIT_MIN_ABS_INPUT && std::abs(feet) <= UNIT_MAX_ABS_INPUT,
                "Length out of range");

    return CreateDoubleValue(env, FeetToMetersValue(feet));
}

static double KilometersToMilesValue(double kilometers)
{
    return kilometers * UNIT_MILES_PER_KILOMETER;
}

// Converts kilometers to miles using the international mile definition.
napi_value KilometersToMiles(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: distance in kilometers");

    double kilometers = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilometers)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(kilometers) >= UNIT_MIN_ABS_INPUT && std::abs(kilometers) <= UNIT_MAX_ABS_INPUT,
                "Distance out of range");

    return CreateDoubleValue(env, KilometersToMilesValue(kilometers));
}

static double MilesToKilometersValue(double miles)
{
    return miles * UNIT_KILOMETERS_PER_MILE;
}

// Converts miles to kilometers using the international mile definition.
napi_value MilesToKilometers(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: distance in miles");

    double miles = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], miles)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(miles) >= UNIT_MIN_ABS_INPUT && std::abs(miles) <= UNIT_MAX_ABS_INPUT,
                "Distance out of range");

    return CreateDoubleValue(env, MilesToKilometersValue(miles));
}

static double CentimetersToInchesValue(double centimeters)
{
    return centimeters * UNIT_INCHES_PER_CENTIMETER;
}

// Converts centimeters to inches using the international inch definition.
napi_value CentimetersToInches(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: length in centimeters");

    double centimeters = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], centimeters)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(centimeters) >= UNIT_MIN_ABS_INPUT && std::abs(centimeters) <= UNIT_MAX_ABS_INPUT,
                "Length out of range");

    return CreateDoubleValue(env, CentimetersToInchesValue(centimeters));
}

static double InchesToCentimetersValue(double inches)
{
    return inches * UNIT_CENTIMETERS_PER_INCH;
}

// Converts inches to centimeters using the international inch definition.
napi_value InchesToCentimeters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: length in inches");

    double inches = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], inches)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(inches) >= UNIT_MIN_ABS_INPUT && std::abs(inches) <= UNIT_MAX_ABS_INPUT,
                "Length out of range");

    return CreateDoubleValue(env, InchesToCentimetersValue(inches));
}

static double KilogramsToPoundsValue(double kilograms)
{
    return kilograms * UNIT_POUNDS_PER_KILOGRAM;
}

// Converts kilograms to pounds using the avoirdupois pound definition.
napi_value KilogramsToPounds(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: mass in kilograms");

    double kilograms = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilograms)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(kilograms) >= UNIT_MIN_ABS_INPUT && std::abs(kilograms) <= UNIT_MAX_ABS_INPUT,
                "Mass out of range");

    return CreateDoubleValue(env, KilogramsToPoundsValue(kilograms));
}

static double PoundsToKilogramsValue(double pounds)
{
    return pounds * UNIT_KILOGRAMS_PER_POUND;
}

// Converts pounds to kilograms using the avoirdupois pound definition.
napi_value PoundsToKilograms(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: mass in pounds");

    double pounds = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], pounds)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(pounds) >= UNIT_MIN_ABS_INPUT && std::abs(pounds) <= UNIT_MAX_ABS_INPUT,
                "Mass out of range");

    return CreateDoubleValue(env, PoundsToKilogramsValue(pounds));
}

static double GramsToOuncesValue(double grams)
{
    return grams * UNIT_OUNCES_PER_GRAM;
}

// Converts grams to ounces using the avoirdupois ounce definition.
napi_value GramsToOunces(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: mass in grams");

    double grams = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], grams)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(grams) >= UNIT_MIN_ABS_INPUT && std::abs(grams) <= UNIT_MAX_ABS_INPUT,
                "Mass out of range");

    return CreateDoubleValue(env, GramsToOuncesValue(grams));
}

static double OuncesToGramsValue(double ounces)
{
    return ounces * UNIT_GRAMS_PER_OUNCE;
}

// Converts ounces to grams using the avoirdupois ounce definition.
napi_value OuncesToGrams(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: mass in ounces");

    double ounces = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], ounces)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(ounces) >= UNIT_MIN_ABS_INPUT && std::abs(ounces) <= UNIT_MAX_ABS_INPUT,
                "Mass out of range");

    return CreateDoubleValue(env, OuncesToGramsValue(ounces));
}

static double LitersToGallonsValue(double liters)
{
    return liters * UNIT_GALLONS_PER_LITER;
}

// Converts liters to US liquid gallons.
napi_value LitersToGallons(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: volume in liters");

    double liters = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], liters)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(liters) >= UNIT_MIN_ABS_INPUT && std::abs(liters) <= UNIT_MAX_ABS_INPUT,
                "Volume out of range");

    return CreateDoubleValue(env, LitersToGallonsValue(liters));
}

static double GallonsToLitersValue(double gallons)
{
    return gallons * UNIT_LITERS_PER_GALLON;
}

// Converts US liquid gallons to liters.
napi_value GallonsToLiters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: volume in gallons");

    double gallons = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], gallons)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(gallons) >= UNIT_MIN_ABS_INPUT && std::abs(gallons) <= UNIT_MAX_ABS_INPUT,
                "Volume out of range");

    return CreateDoubleValue(env, GallonsToLitersValue(gallons));
}

static double CelsiusToFahrenheitValue(double celsius)
{
    return celsius * UNIT_FAHRENHEIT_SCALE + UNIT_FAHRENHEIT_OFFSET;
}

// Converts celsius to fahrenheit using the standard conversion formula.
napi_value CelsiusToFahrenheit(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: temperature in celsius");

    double celsius = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], celsius)) {
        return nullptr;
    }

    return CreateDoubleValue(env, CelsiusToFahrenheitValue(celsius));
}

static double FahrenheitToCelsiusValue(double fahrenheit)
{
    return (fahrenheit - UNIT_FAHRENHEIT_OFFSET) * UNIT_CELSIUS_SCALE;
}

// Converts fahrenheit to celsius using the standard conversion formula.
napi_value FahrenheitToCelsius(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: temperature in fahrenheit");

    double fahrenheit = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], fahrenheit)) {
        return nullptr;
    }

    return CreateDoubleValue(env, FahrenheitToCelsiusValue(fahrenheit));
}

static double CelsiusToKelvinValue(double celsius)
{
    return celsius + UNIT_KELVIN_OFFSET;
}

// Converts celsius to kelvin by adding the absolute zero offset.
napi_value CelsiusToKelvin(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: temperature in celsius");

    double celsius = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], celsius)) {
        return nullptr;
    }

    return CreateDoubleValue(env, CelsiusToKelvinValue(celsius));
}

static double KelvinToCelsiusValue(double kelvin)
{
    return kelvin - UNIT_KELVIN_OFFSET;
}

// Converts kelvin to celsius by subtracting the absolute zero offset.
napi_value KelvinToCelsius(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: temperature in kelvin");

    double kelvin = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kelvin)) {
        return nullptr;
    }
    NAPI_ASSERT(env, kelvin >= UNIT_MIN_KELVIN, "Kelvin must not be negative");

    return CreateDoubleValue(env, KelvinToCelsiusValue(kelvin));
}

static double MetersPerSecondToKmPerHourValue(double metersPerSecond)
{
    return metersPerSecond * UNIT_KMPH_PER_MPS;
}

// Converts meters per second to kilometers per hour.
napi_value MetersPerSecondToKmPerHour(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speed in meters per second");

    double metersPerSecond = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], metersPerSecond)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(metersPerSecond) >= UNIT_MIN_ABS_INPUT && std::abs(metersPerSecond) <= UNIT_MAX_ABS_INPUT,
                "Speed out of range");

    return CreateDoubleValue(env, MetersPerSecondToKmPerHourValue(metersPerSecond));
}

static double KmPerHourToMetersPerSecondValue(double kilometersPerHour)
{
    return kilometersPerHour * UNIT_MPS_PER_KMPH;
}

// Converts kilometers per hour to meters per second.
napi_value KmPerHourToMetersPerSecond(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speed in kilometers per hour");

    double kilometersPerHour = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilometersPerHour)) {
        return nullptr;
    }
    NAPI_ASSERT(env,
                std::abs(kilometersPerHour) >= UNIT_MIN_ABS_INPUT && std::abs(kilometersPerHour) <= UNIT_MAX_ABS_INPUT,
                "Speed out of range");

    return CreateDoubleValue(env, KmPerHourToMetersPerSecondValue(kilometersPerHour));
}

static double KilometersPerHourToMilesPerHourValue(double kilometersPerHour)
{
    return kilometersPerHour * UNIT_MILES_PER_KILOMETER;
}

// Converts kilometers per hour to miles per hour.
napi_value KilometersPerHourToMilesPerHour(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speed in kilometers per hour");

    double kilometersPerHour = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilometersPerHour)) {
        return nullptr;
    }
    NAPI_ASSERT(env,
                std::abs(kilometersPerHour) >= UNIT_MIN_ABS_INPUT && std::abs(kilometersPerHour) <= UNIT_MAX_ABS_INPUT,
                "Speed out of range");

    return CreateDoubleValue(env, KilometersPerHourToMilesPerHourValue(kilometersPerHour));
}

static double MilesPerHourToKilometersPerHourValue(double milesPerHour)
{
    return milesPerHour * UNIT_KILOMETERS_PER_MILE;
}

// Converts miles per hour to kilometers per hour.
napi_value MilesPerHourToKilometersPerHour(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speed in miles per hour");

    double milesPerHour = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], milesPerHour)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(milesPerHour) >= UNIT_MIN_ABS_INPUT && std::abs(milesPerHour) <= UNIT_MAX_ABS_INPUT,
                "Speed out of range");

    return CreateDoubleValue(env, MilesPerHourToKilometersPerHourValue(milesPerHour));
}

static double KnotsToKilometersPerHourValue(double knots)
{
    return knots * UNIT_KILOMETERS_PER_KNOT;
}

// Converts knots to kilometers per hour using the international knot.
napi_value KnotsToKilometersPerHour(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speed in knots");

    double knots = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], knots)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(knots) >= UNIT_MIN_ABS_INPUT && std::abs(knots) <= UNIT_MAX_ABS_INPUT,
                "Speed out of range");

    return CreateDoubleValue(env, KnotsToKilometersPerHourValue(knots));
}

static double KilometersPerHourToKnotsValue(double kilometersPerHour)
{
    return kilometersPerHour * UNIT_KNOTS_PER_KILOMETER;
}

// Converts kilometers per hour to knots using the international knot.
napi_value KilometersPerHourToKnots(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: speed in kilometers per hour");

    double kilometersPerHour = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilometersPerHour)) {
        return nullptr;
    }
    NAPI_ASSERT(env,
                std::abs(kilometersPerHour) >= UNIT_MIN_ABS_INPUT && std::abs(kilometersPerHour) <= UNIT_MAX_ABS_INPUT,
                "Speed out of range");

    return CreateDoubleValue(env, KilometersPerHourToKnotsValue(kilometersPerHour));
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("metersToFeet", MetersToFeet),
        DECLARE_NAPI_FUNCTION("feetToMeters", FeetToMeters),
        DECLARE_NAPI_FUNCTION("kilometersToMiles", KilometersToMiles),
        DECLARE_NAPI_FUNCTION("milesToKilometers", MilesToKilometers),
        DECLARE_NAPI_FUNCTION("centimetersToInches", CentimetersToInches),
        DECLARE_NAPI_FUNCTION("inchesToCentimeters", InchesToCentimeters),
        DECLARE_NAPI_FUNCTION("kilogramsToPounds", KilogramsToPounds),
        DECLARE_NAPI_FUNCTION("poundsToKilograms", PoundsToKilograms),
        DECLARE_NAPI_FUNCTION("gramsToOunces", GramsToOunces),
        DECLARE_NAPI_FUNCTION("ouncesToGrams", OuncesToGrams),
        DECLARE_NAPI_FUNCTION("litersToGallons", LitersToGallons),
        DECLARE_NAPI_FUNCTION("gallonsToLiters", GallonsToLiters),
        DECLARE_NAPI_FUNCTION("celsiusToFahrenheit", CelsiusToFahrenheit),
        DECLARE_NAPI_FUNCTION("fahrenheitToCelsius", FahrenheitToCelsius),
        DECLARE_NAPI_FUNCTION("celsiusToKelvin", CelsiusToKelvin),
        DECLARE_NAPI_FUNCTION("kelvinToCelsius", KelvinToCelsius),
        DECLARE_NAPI_FUNCTION("metersPerSecondToKmPerHour", MetersPerSecondToKmPerHour),
        DECLARE_NAPI_FUNCTION("kmPerHourToMetersPerSecond", KmPerHourToMetersPerSecond),
        DECLARE_NAPI_FUNCTION("kilometersPerHourToMilesPerHour", KilometersPerHourToMilesPerHour),
        DECLARE_NAPI_FUNCTION("milesPerHourToKilometersPerHour", MilesPerHourToKilometersPerHour),
        DECLARE_NAPI_FUNCTION("knotsToKilometersPerHour", KnotsToKilometersPerHour),
        DECLARE_NAPI_FUNCTION("kilometersPerHourToKnots", KilometersPerHourToKnots),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterUnitConvertOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module unitConvertSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "unitConvertSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void UnitConvertSuiteRegisterModule(void)
{
    napi_module_register(&unitConvertSuiteModule);
}
