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

static double SquareMetersToSquareFeetValue(double squareMeters)
{
    return squareMeters * UNIT_SQUARE_FEET_PER_SQUARE_METER;
}

// Converts square meters to square feet.
napi_value SquareMetersToSquareFeet(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: area in square meters");

    double squareMeters = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], squareMeters)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(squareMeters) >= UNIT_MIN_ABS_INPUT && std::abs(squareMeters) <= UNIT_MAX_ABS_INPUT,
                "Area out of range");

    return CreateDoubleValue(env, SquareMetersToSquareFeetValue(squareMeters));
}

static double SquareFeetToSquareMetersValue(double squareFeet)
{
    return squareFeet * UNIT_SQUARE_METERS_PER_SQUARE_FOOT;
}

// Converts square feet to square meters.
napi_value SquareFeetToSquareMeters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: area in square feet");

    double squareFeet = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], squareFeet)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(squareFeet) >= UNIT_MIN_ABS_INPUT && std::abs(squareFeet) <= UNIT_MAX_ABS_INPUT,
                "Area out of range");

    return CreateDoubleValue(env, SquareFeetToSquareMetersValue(squareFeet));
}

static double HectaresToAcresValue(double hectares)
{
    return hectares * UNIT_ACRES_PER_HECTARE;
}

// Converts hectares to acres using the international acre definition.
napi_value HectaresToAcres(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: area in hectares");

    double hectares = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], hectares)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(hectares) >= UNIT_MIN_ABS_INPUT && std::abs(hectares) <= UNIT_MAX_ABS_INPUT,
                "Area out of range");

    return CreateDoubleValue(env, HectaresToAcresValue(hectares));
}

static double AcresToHectaresValue(double acres)
{
    return acres * UNIT_HECTARES_PER_ACRE;
}

// Converts acres to hectares using the international acre definition.
napi_value AcresToHectares(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: area in acres");

    double acres = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], acres)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(acres) >= UNIT_MIN_ABS_INPUT && std::abs(acres) <= UNIT_MAX_ABS_INPUT,
                "Area out of range");

    return CreateDoubleValue(env, AcresToHectaresValue(acres));
}

static double CubicMetersToCubicFeetValue(double cubicMeters)
{
    return cubicMeters * UNIT_CUBIC_FEET_PER_CUBIC_METER;
}

// Converts cubic meters to cubic feet.
napi_value CubicMetersToCubicFeet(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: volume in cubic meters");

    double cubicMeters = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], cubicMeters)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(cubicMeters) >= UNIT_MIN_ABS_INPUT && std::abs(cubicMeters) <= UNIT_MAX_ABS_INPUT,
                "Volume out of range");

    return CreateDoubleValue(env, CubicMetersToCubicFeetValue(cubicMeters));
}

static double CubicFeetToCubicMetersValue(double cubicFeet)
{
    return cubicFeet * UNIT_CUBIC_METERS_PER_CUBIC_FOOT;
}

// Converts cubic feet to cubic meters.
napi_value CubicFeetToCubicMeters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: volume in cubic feet");

    double cubicFeet = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], cubicFeet)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(cubicFeet) >= UNIT_MIN_ABS_INPUT && std::abs(cubicFeet) <= UNIT_MAX_ABS_INPUT,
                "Volume out of range");

    return CreateDoubleValue(env, CubicFeetToCubicMetersValue(cubicFeet));
}

static double MillilitersToFluidOuncesValue(double milliliters)
{
    return milliliters * UNIT_FLUID_OUNCES_PER_MILLILITER;
}

// Converts milliliters to US fluid ounces.
napi_value MillilitersToFluidOunces(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: volume in milliliters");

    double milliliters = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], milliliters)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(milliliters) >= UNIT_MIN_ABS_INPUT && std::abs(milliliters) <= UNIT_MAX_ABS_INPUT,
                "Volume out of range");

    return CreateDoubleValue(env, MillilitersToFluidOuncesValue(milliliters));
}

static double FluidOuncesToMillilitersValue(double fluidOunces)
{
    return fluidOunces * UNIT_MILLILITERS_PER_FLUID_OUNCE;
}

// Converts US fluid ounces to milliliters.
napi_value FluidOuncesToMilliliters(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: volume in fluid ounces");

    double fluidOunces = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], fluidOunces)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(fluidOunces) >= UNIT_MIN_ABS_INPUT && std::abs(fluidOunces) <= UNIT_MAX_ABS_INPUT,
                "Volume out of range");

    return CreateDoubleValue(env, FluidOuncesToMillilitersValue(fluidOunces));
}

static double SecondsToMinutesValue(double seconds)
{
    return seconds * UNIT_MINUTES_PER_SECOND;
}

// Converts seconds to minutes using sixty seconds per minute.
napi_value SecondsToMinutes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: duration in seconds");

    double seconds = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], seconds)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(seconds) >= UNIT_MIN_ABS_INPUT && std::abs(seconds) <= UNIT_MAX_ABS_INPUT,
                "Duration out of range");

    return CreateDoubleValue(env, SecondsToMinutesValue(seconds));
}

static double MinutesToSecondsValue(double minutes)
{
    return minutes * UNIT_SECONDS_PER_MINUTE;
}

// Converts minutes to seconds using sixty seconds per minute.
napi_value MinutesToSeconds(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: duration in minutes");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(minutes) >= UNIT_MIN_ABS_INPUT && std::abs(minutes) <= UNIT_MAX_ABS_INPUT,
                "Duration out of range");

    return CreateDoubleValue(env, MinutesToSecondsValue(minutes));
}

static double MinutesToHoursValue(double minutes)
{
    return minutes * UNIT_HOURS_PER_MINUTE;
}

// Converts minutes to hours using sixty minutes per hour.
napi_value MinutesToHours(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: duration in minutes");

    double minutes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], minutes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(minutes) >= UNIT_MIN_ABS_INPUT && std::abs(minutes) <= UNIT_MAX_ABS_INPUT,
                "Duration out of range");

    return CreateDoubleValue(env, MinutesToHoursValue(minutes));
}

static double HoursToMinutesValue(double hours)
{
    return hours * UNIT_MINUTES_PER_HOUR;
}

// Converts hours to minutes using sixty minutes per hour.
napi_value HoursToMinutes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: duration in hours");

    double hours = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], hours)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(hours) >= UNIT_MIN_ABS_INPUT && std::abs(hours) <= UNIT_MAX_ABS_INPUT,
                "Duration out of range");

    return CreateDoubleValue(env, HoursToMinutesValue(hours));
}

static double HoursToDaysValue(double hours)
{
    return hours * UNIT_DAYS_PER_HOUR;
}

// Converts hours to days using twenty-four hours per day.
napi_value HoursToDays(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: duration in hours");

    double hours = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], hours)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(hours) >= UNIT_MIN_ABS_INPUT && std::abs(hours) <= UNIT_MAX_ABS_INPUT,
                "Duration out of range");

    return CreateDoubleValue(env, HoursToDaysValue(hours));
}

static double DaysToHoursValue(double days)
{
    return days * UNIT_HOURS_PER_DAY;
}

// Converts days to hours using twenty-four hours per day.
napi_value DaysToHours(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: duration in days");

    double days = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], days)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(days) >= UNIT_MIN_ABS_INPUT && std::abs(days) <= UNIT_MAX_ABS_INPUT,
                "Duration out of range");

    return CreateDoubleValue(env, DaysToHoursValue(days));
}

static double JoulesToCaloriesValue(double joules)
{
    return joules * UNIT_CALORIES_PER_JOULE;
}

// Converts joules to thermochemical calories.
napi_value JoulesToCalories(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: energy in joules");

    double joules = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], joules)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(joules) >= UNIT_MIN_ABS_INPUT && std::abs(joules) <= UNIT_MAX_ABS_INPUT,
                "Energy out of range");

    return CreateDoubleValue(env, JoulesToCaloriesValue(joules));
}

static double CaloriesToJoulesValue(double calories)
{
    return calories * UNIT_JOULES_PER_CALORIE;
}

// Converts thermochemical calories to joules.
napi_value CaloriesToJoules(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: energy in calories");

    double calories = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], calories)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(calories) >= UNIT_MIN_ABS_INPUT && std::abs(calories) <= UNIT_MAX_ABS_INPUT,
                "Energy out of range");

    return CreateDoubleValue(env, CaloriesToJoulesValue(calories));
}

static double JoulesToKilowattHoursValue(double joules)
{
    return joules * UNIT_KILOWATT_HOURS_PER_JOULE;
}

// Converts joules to kilowatt hours.
napi_value JoulesToKilowattHours(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: energy in joules");

    double joules = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], joules)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(joules) >= UNIT_MIN_ABS_INPUT && std::abs(joules) <= UNIT_MAX_ABS_INPUT,
                "Energy out of range");

    return CreateDoubleValue(env, JoulesToKilowattHoursValue(joules));
}

static double KilowattHoursToJoulesValue(double kilowattHours)
{
    return kilowattHours * UNIT_JOULES_PER_KILOWATT_HOUR;
}

// Converts kilowatt hours to joules.
napi_value KilowattHoursToJoules(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: energy in kilowatt hours");

    double kilowattHours = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilowattHours)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(kilowattHours) >= UNIT_MIN_ABS_INPUT && std::abs(kilowattHours) <= UNIT_MAX_ABS_INPUT,
                "Energy out of range");

    return CreateDoubleValue(env, KilowattHoursToJoulesValue(kilowattHours));
}

static double KilobytesToMegabytesValue(double kilobytes)
{
    return kilobytes * UNIT_MEGABYTES_PER_KILOBYTE;
}

// Converts kilobytes to megabytes using the binary scale.
napi_value KilobytesToMegabytes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: data size in kilobytes");

    double kilobytes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], kilobytes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(kilobytes) >= UNIT_MIN_ABS_INPUT && std::abs(kilobytes) <= UNIT_MAX_ABS_INPUT,
                "Data size out of range");

    return CreateDoubleValue(env, KilobytesToMegabytesValue(kilobytes));
}

static double MegabytesToKilobytesValue(double megabytes)
{
    return megabytes * UNIT_KILOBYTES_PER_MEGABYTE;
}

// Converts megabytes to kilobytes using the binary scale.
napi_value MegabytesToKilobytes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: data size in megabytes");

    double megabytes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], megabytes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(megabytes) >= UNIT_MIN_ABS_INPUT && std::abs(megabytes) <= UNIT_MAX_ABS_INPUT,
                "Data size out of range");

    return CreateDoubleValue(env, MegabytesToKilobytesValue(megabytes));
}

static double GigabytesToTerabytesValue(double gigabytes)
{
    return gigabytes * UNIT_TERABYTES_PER_GIGABYTE;
}

// Converts gigabytes to terabytes using the binary scale.
napi_value GigabytesToTerabytes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: data size in gigabytes");

    double gigabytes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], gigabytes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(gigabytes) >= UNIT_MIN_ABS_INPUT && std::abs(gigabytes) <= UNIT_MAX_ABS_INPUT,
                "Data size out of range");

    return CreateDoubleValue(env, GigabytesToTerabytesValue(gigabytes));
}

static double TerabytesToGigabytesValue(double terabytes)
{
    return terabytes * UNIT_GIGABYTES_PER_TERABYTE;
}

// Converts terabytes to gigabytes using the binary scale.
napi_value TerabytesToGigabytes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: data size in terabytes");

    double terabytes = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], terabytes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(terabytes) >= UNIT_MIN_ABS_INPUT && std::abs(terabytes) <= UNIT_MAX_ABS_INPUT,
                "Data size out of range");

    return CreateDoubleValue(env, TerabytesToGigabytesValue(terabytes));
}

static double PascalsToPoundsPerSquareInchValue(double pascals)
{
    return pascals * UNIT_POUNDS_PER_SQUARE_INCH_PER_PASCAL;
}

// Converts pascals to pounds per square inch.
napi_value PascalsToPoundsPerSquareInch(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in pascals");

    double pascals = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], pascals)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(pascals) >= UNIT_MIN_ABS_INPUT && std::abs(pascals) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, PascalsToPoundsPerSquareInchValue(pascals));
}

static double PoundsPerSquareInchToPascalsValue(double poundsPerSquareInch)
{
    return poundsPerSquareInch * UNIT_PASCALS_PER_POUND_PER_SQUARE_INCH;
}

// Converts pounds per square inch to pascals.
napi_value PoundsPerSquareInchToPascals(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in pounds per square inch");

    double poundsPerSquareInch = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], poundsPerSquareInch)) {
        return nullptr;
    }
    NAPI_ASSERT(env,
                std::abs(poundsPerSquareInch) >= UNIT_MIN_ABS_INPUT &&
                std::abs(poundsPerSquareInch) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, PoundsPerSquareInchToPascalsValue(poundsPerSquareInch));
}

static double AtmospheresToPascalsValue(double atmospheres)
{
    return atmospheres * UNIT_PASCALS_PER_ATMOSPHERE;
}

// Converts standard atmospheres to pascals.
napi_value AtmospheresToPascals(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in atmospheres");

    double atmospheres = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], atmospheres)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(atmospheres) >= UNIT_MIN_ABS_INPUT && std::abs(atmospheres) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, AtmospheresToPascalsValue(atmospheres));
}

static double PascalsToAtmospheresValue(double pascals)
{
    return pascals * UNIT_ATMOSPHERES_PER_PASCAL;
}

// Converts pascals to standard atmospheres.
napi_value PascalsToAtmospheres(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in pascals");

    double pascals = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], pascals)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(pascals) >= UNIT_MIN_ABS_INPUT && std::abs(pascals) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, PascalsToAtmospheresValue(pascals));
}

static double BarsToPascalsValue(double bars)
{
    return bars * UNIT_PASCALS_PER_BAR;
}

// Converts bars to pascals.
napi_value BarsToPascals(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in bars");

    double bars = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bars)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(bars) >= UNIT_MIN_ABS_INPUT && std::abs(bars) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, BarsToPascalsValue(bars));
}

static double PascalsToBarsValue(double pascals)
{
    return pascals * UNIT_BARS_PER_PASCAL;
}

// Converts pascals to bars.
napi_value PascalsToBars(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in pascals");

    double pascals = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], pascals)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(pascals) >= UNIT_MIN_ABS_INPUT && std::abs(pascals) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, PascalsToBarsValue(pascals));
}

static double TorrToPascalsValue(double torr)
{
    return torr * UNIT_PASCALS_PER_TORR;
}

// Converts torr to pascals.
napi_value TorrToPascals(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pressure in torr");

    double torr = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], torr)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(torr) >= UNIT_MIN_ABS_INPUT && std::abs(torr) <= UNIT_MAX_ABS_INPUT,
                "Pressure out of range");

    return CreateDoubleValue(env, TorrToPascalsValue(torr));
}

static double WattsToHorsepowerValue(double watts)
{
    return watts * UNIT_HORSEPOWER_PER_WATT;
}

// Converts watts to mechanical horsepower.
napi_value WattsToHorsepower(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: power in watts");

    double watts = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], watts)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(watts) >= UNIT_MIN_ABS_INPUT && std::abs(watts) <= UNIT_MAX_ABS_INPUT,
                "Power out of range");

    return CreateDoubleValue(env, WattsToHorsepowerValue(watts));
}

static double HorsepowerToWattsValue(double horsepower)
{
    return horsepower * UNIT_WATTS_PER_HORSEPOWER;
}

// Converts mechanical horsepower to watts.
napi_value HorsepowerToWatts(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: power in horsepower");

    double horsepower = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], horsepower)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(horsepower) >= UNIT_MIN_ABS_INPUT && std::abs(horsepower) <= UNIT_MAX_ABS_INPUT,
                "Power out of range");

    return CreateDoubleValue(env, HorsepowerToWattsValue(horsepower));
}

static double BtuToJoulesValue(double btu)
{
    return btu * UNIT_JOULES_PER_BTU;
}

// Converts British thermal units to joules.
napi_value BtuToJoules(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: energy in btu");

    double btu = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], btu)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(btu) >= UNIT_MIN_ABS_INPUT && std::abs(btu) <= UNIT_MAX_ABS_INPUT, "Energy out of range");

    return CreateDoubleValue(env, BtuToJoulesValue(btu));
}

static double JoulesToBtuValue(double joules)
{
    return joules * UNIT_BTU_PER_JOULE;
}

// Converts joules to British thermal units.
napi_value JoulesToBtu(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: energy in joules");

    double joules = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], joules)) {
        return nullptr;
    }
    NAPI_ASSERT(env, std::abs(joules) >= UNIT_MIN_ABS_INPUT && std::abs(joules) <= UNIT_MAX_ABS_INPUT,
                "Energy out of range");

    return CreateDoubleValue(env, JoulesToBtuValue(joules));
}

napi_status RegisterUnitConvertOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("squareMetersToSquareFeet", SquareMetersToSquareFeet),
        DECLARE_NAPI_FUNCTION("squareFeetToSquareMeters", SquareFeetToSquareMeters),
        DECLARE_NAPI_FUNCTION("hectaresToAcres", HectaresToAcres),
        DECLARE_NAPI_FUNCTION("acresToHectares", AcresToHectares),
        DECLARE_NAPI_FUNCTION("cubicMetersToCubicFeet", CubicMetersToCubicFeet),
        DECLARE_NAPI_FUNCTION("cubicFeetToCubicMeters", CubicFeetToCubicMeters),
        DECLARE_NAPI_FUNCTION("millilitersToFluidOunces", MillilitersToFluidOunces),
        DECLARE_NAPI_FUNCTION("fluidOuncesToMilliliters", FluidOuncesToMilliliters),
        DECLARE_NAPI_FUNCTION("secondsToMinutes", SecondsToMinutes),
        DECLARE_NAPI_FUNCTION("minutesToSeconds", MinutesToSeconds),
        DECLARE_NAPI_FUNCTION("minutesToHours", MinutesToHours),
        DECLARE_NAPI_FUNCTION("hoursToMinutes", HoursToMinutes),
        DECLARE_NAPI_FUNCTION("hoursToDays", HoursToDays),
        DECLARE_NAPI_FUNCTION("daysToHours", DaysToHours),
        DECLARE_NAPI_FUNCTION("joulesToCalories", JoulesToCalories),
        DECLARE_NAPI_FUNCTION("caloriesToJoules", CaloriesToJoules),
        DECLARE_NAPI_FUNCTION("joulesToKilowattHours", JoulesToKilowattHours),
        DECLARE_NAPI_FUNCTION("kilowattHoursToJoules", KilowattHoursToJoules),
        DECLARE_NAPI_FUNCTION("kilobytesToMegabytes", KilobytesToMegabytes),
        DECLARE_NAPI_FUNCTION("megabytesToKilobytes", MegabytesToKilobytes),
        DECLARE_NAPI_FUNCTION("gigabytesToTerabytes", GigabytesToTerabytes),
        DECLARE_NAPI_FUNCTION("terabytesToGigabytes", TerabytesToGigabytes),
        DECLARE_NAPI_FUNCTION("pascalsToPoundsPerSquareInch", PascalsToPoundsPerSquareInch),
        DECLARE_NAPI_FUNCTION("poundsPerSquareInchToPascals", PoundsPerSquareInchToPascals),
        DECLARE_NAPI_FUNCTION("atmospheresToPascals", AtmospheresToPascals),
        DECLARE_NAPI_FUNCTION("pascalsToAtmospheres", PascalsToAtmospheres),
        DECLARE_NAPI_FUNCTION("barsToPascals", BarsToPascals),
        DECLARE_NAPI_FUNCTION("pascalsToBars", PascalsToBars),
        DECLARE_NAPI_FUNCTION("torrToPascals", TorrToPascals),
        DECLARE_NAPI_FUNCTION("wattsToHorsepower", WattsToHorsepower),
        DECLARE_NAPI_FUNCTION("horsepowerToWatts", HorsepowerToWatts),
        DECLARE_NAPI_FUNCTION("btuToJoules", BtuToJoules),
        DECLARE_NAPI_FUNCTION("joulesToBtu", JoulesToBtu),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
