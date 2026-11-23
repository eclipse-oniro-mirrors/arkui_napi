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

#ifndef SAMPLE_NATIVE_MODULE_UNIT_CONVERT_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_UNIT_CONVERT_SUITE_COMMON_H

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int ARG_INDEX_ZERO = 0;

constexpr double UNIT_MIN_ABS_INPUT = 1e-9;
constexpr double UNIT_MAX_ABS_INPUT = 1e12;

constexpr double UNIT_FEET_PER_METER = 3.280839895;
constexpr double UNIT_METERS_PER_FOOT = 0.3048;
constexpr double UNIT_MILES_PER_KILOMETER = 0.621371192;
constexpr double UNIT_KILOMETERS_PER_MILE = 1.609344;
constexpr double UNIT_INCHES_PER_CENTIMETER = 0.393700787;
constexpr double UNIT_CENTIMETERS_PER_INCH = 2.54;
constexpr double UNIT_POUNDS_PER_KILOGRAM = 2.204622622;
constexpr double UNIT_KILOGRAMS_PER_POUND = 0.45359237;
constexpr double UNIT_OUNCES_PER_GRAM = 0.035273962;
constexpr double UNIT_GRAMS_PER_OUNCE = 28.349523125;
constexpr double UNIT_GALLONS_PER_LITER = 0.264172052;
constexpr double UNIT_LITERS_PER_GALLON = 3.785411784;

constexpr double UNIT_FAHRENHEIT_SCALE = 9.0 / 5.0;
constexpr double UNIT_FAHRENHEIT_OFFSET = 32.0;
constexpr double UNIT_CELSIUS_SCALE = 5.0 / 9.0;
constexpr double UNIT_KELVIN_OFFSET = 273.15;
constexpr double UNIT_MIN_KELVIN = 0.0;

constexpr double UNIT_KMPH_PER_MPS = 3.6;
constexpr double UNIT_MPS_PER_KMPH = 1.0 / 3.6;
constexpr double UNIT_KILOMETERS_PER_KNOT = 1.852;
constexpr double UNIT_KNOTS_PER_KILOMETER = 1.0 / 1.852;

constexpr double UNIT_SQUARE_FEET_PER_SQUARE_METER = 10.7639104;
constexpr double UNIT_SQUARE_METERS_PER_SQUARE_FOOT = 0.09290304;
constexpr double UNIT_ACRES_PER_HECTARE = 2.471053815;
constexpr double UNIT_HECTARES_PER_ACRE = 0.404685642;

constexpr double UNIT_CUBIC_FEET_PER_CUBIC_METER = 35.3146667;
constexpr double UNIT_CUBIC_METERS_PER_CUBIC_FOOT = 0.0283168466;
constexpr double UNIT_FLUID_OUNCES_PER_MILLILITER = 0.0338140227;
constexpr double UNIT_MILLILITERS_PER_FLUID_OUNCE = 29.5735295625;

constexpr double UNIT_SECONDS_PER_MINUTE = 60.0;
constexpr double UNIT_MINUTES_PER_SECOND = 1.0 / 60.0;
constexpr double UNIT_MINUTES_PER_HOUR = 60.0;
constexpr double UNIT_HOURS_PER_MINUTE = 1.0 / 60.0;
constexpr double UNIT_HOURS_PER_DAY = 24.0;
constexpr double UNIT_DAYS_PER_HOUR = 1.0 / 24.0;

constexpr double UNIT_CALORIES_PER_JOULE = 0.239005736;
constexpr double UNIT_JOULES_PER_CALORIE = 4.184;
constexpr double UNIT_KILOWATT_HOURS_PER_JOULE = 2.777777778e-7;
constexpr double UNIT_JOULES_PER_KILOWATT_HOUR = 3.6e6;

constexpr double UNIT_MEGABYTES_PER_KILOBYTE = 1.0 / 1024.0;
constexpr double UNIT_KILOBYTES_PER_MEGABYTE = 1024.0;
constexpr double UNIT_TERABYTES_PER_GIGABYTE = 1.0 / 1024.0;
constexpr double UNIT_GIGABYTES_PER_TERABYTE = 1024.0;

constexpr double UNIT_POUNDS_PER_SQUARE_INCH_PER_PASCAL = 0.00014503773773;
constexpr double UNIT_PASCALS_PER_POUND_PER_SQUARE_INCH = 6894.757293;
constexpr double UNIT_PASCALS_PER_ATMOSPHERE = 101325.0;
constexpr double UNIT_ATMOSPHERES_PER_PASCAL = 1.0 / 101325.0;
constexpr double UNIT_PASCALS_PER_BAR = 100000.0;
constexpr double UNIT_BARS_PER_PASCAL = 1.0 / 100000.0;
constexpr double UNIT_PASCALS_PER_TORR = 101325.0 / 760.0;

constexpr double UNIT_HORSEPOWER_PER_WATT = 1.0 / 745.699872;
constexpr double UNIT_WATTS_PER_HORSEPOWER = 745.699872;

constexpr double UNIT_JOULES_PER_BTU = 1055.05585262;
constexpr double UNIT_BTU_PER_JOULE = 1.0 / 1055.05585262;

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
napi_value CreateDoubleValue(napi_env env, double value);

napi_value MetersToFeet(napi_env env, napi_callback_info info);
napi_value FeetToMeters(napi_env env, napi_callback_info info);
napi_value KilometersToMiles(napi_env env, napi_callback_info info);
napi_value MilesToKilometers(napi_env env, napi_callback_info info);
napi_value CentimetersToInches(napi_env env, napi_callback_info info);
napi_value InchesToCentimeters(napi_env env, napi_callback_info info);
napi_value KilogramsToPounds(napi_env env, napi_callback_info info);
napi_value PoundsToKilograms(napi_env env, napi_callback_info info);
napi_value GramsToOunces(napi_env env, napi_callback_info info);
napi_value OuncesToGrams(napi_env env, napi_callback_info info);
napi_value LitersToGallons(napi_env env, napi_callback_info info);
napi_value GallonsToLiters(napi_env env, napi_callback_info info);
napi_value CelsiusToFahrenheit(napi_env env, napi_callback_info info);
napi_value FahrenheitToCelsius(napi_env env, napi_callback_info info);
napi_value CelsiusToKelvin(napi_env env, napi_callback_info info);
napi_value KelvinToCelsius(napi_env env, napi_callback_info info);
napi_value MetersPerSecondToKmPerHour(napi_env env, napi_callback_info info);
napi_value KmPerHourToMetersPerSecond(napi_env env, napi_callback_info info);
napi_value KilometersPerHourToMilesPerHour(napi_env env, napi_callback_info info);
napi_value MilesPerHourToKilometersPerHour(napi_env env, napi_callback_info info);
napi_value KnotsToKilometersPerHour(napi_env env, napi_callback_info info);
napi_value KilometersPerHourToKnots(napi_env env, napi_callback_info info);

napi_value SquareMetersToSquareFeet(napi_env env, napi_callback_info info);
napi_value SquareFeetToSquareMeters(napi_env env, napi_callback_info info);
napi_value HectaresToAcres(napi_env env, napi_callback_info info);
napi_value AcresToHectares(napi_env env, napi_callback_info info);
napi_value CubicMetersToCubicFeet(napi_env env, napi_callback_info info);
napi_value CubicFeetToCubicMeters(napi_env env, napi_callback_info info);
napi_value MillilitersToFluidOunces(napi_env env, napi_callback_info info);
napi_value FluidOuncesToMilliliters(napi_env env, napi_callback_info info);
napi_value SecondsToMinutes(napi_env env, napi_callback_info info);
napi_value MinutesToSeconds(napi_env env, napi_callback_info info);
napi_value MinutesToHours(napi_env env, napi_callback_info info);
napi_value HoursToMinutes(napi_env env, napi_callback_info info);
napi_value HoursToDays(napi_env env, napi_callback_info info);
napi_value DaysToHours(napi_env env, napi_callback_info info);
napi_value JoulesToCalories(napi_env env, napi_callback_info info);
napi_value CaloriesToJoules(napi_env env, napi_callback_info info);
napi_value JoulesToKilowattHours(napi_env env, napi_callback_info info);
napi_value KilowattHoursToJoules(napi_env env, napi_callback_info info);
napi_value KilobytesToMegabytes(napi_env env, napi_callback_info info);
napi_value MegabytesToKilobytes(napi_env env, napi_callback_info info);
napi_value GigabytesToTerabytes(napi_env env, napi_callback_info info);
napi_value TerabytesToGigabytes(napi_env env, napi_callback_info info);
napi_value PascalsToPoundsPerSquareInch(napi_env env, napi_callback_info info);
napi_value PoundsPerSquareInchToPascals(napi_env env, napi_callback_info info);

napi_value AtmospheresToPascals(napi_env env, napi_callback_info info);
napi_value PascalsToAtmospheres(napi_env env, napi_callback_info info);
napi_value BarsToPascals(napi_env env, napi_callback_info info);
napi_value PascalsToBars(napi_env env, napi_callback_info info);
napi_value TorrToPascals(napi_env env, napi_callback_info info);
napi_value WattsToHorsepower(napi_env env, napi_callback_info info);
napi_value HorsepowerToWatts(napi_env env, napi_callback_info info);
napi_value BtuToJoules(napi_env env, napi_callback_info info);
napi_value JoulesToBtu(napi_env env, napi_callback_info info);

napi_status RegisterUnitConvertOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_UNIT_CONVERT_SUITE_COMMON_H
