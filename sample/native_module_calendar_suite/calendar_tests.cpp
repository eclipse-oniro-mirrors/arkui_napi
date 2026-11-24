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
constexpr size_t K_CAL_CASE_COUNT = 27;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Calendar Operation Cycle ---
constexpr size_t K_CAL_OP_CYCLE = 3;
constexpr size_t K_CAL_OP_INDEX_DAY_OF_WEEK = 0;
constexpr size_t K_CAL_OP_INDEX_IS_LEAP_YEAR = 1;
constexpr size_t K_CAL_OP_INDEX_DAY_OF_YEAR = 2;

// --- Date Derivation ---
constexpr int32_t K_CAL_BASE_YEAR = 2000;
constexpr size_t K_CAL_MONTHS_PER_YEAR = 12;
constexpr size_t K_CAL_DAYS_CYCLE = 28;
constexpr int32_t K_CAL_YEAR_DECREMENT = 1;
constexpr int32_t K_CAL_MONTH_BASE_ONE = 1;
constexpr int32_t K_CAL_DAY_BASE_ONE = 1;

// --- Zeller Congruence Coefficients ---
constexpr int K_CAL_ZELLER_MONTH_FACTOR = 13;
constexpr int K_CAL_ZELLER_MONTH_DIVISOR = 5;
constexpr int K_CAL_ZELLER_CENTURY_FACTOR = 5;
constexpr int K_CAL_QUADRENNIAL_DIV = 4;
constexpr int K_CAL_CENTURY_MOD = 100;
constexpr int K_CAL_QUADRICENTENNIAL_DIV = 400;
constexpr int K_CAL_WEEK_MOD = 7;
constexpr int K_CAL_MONTHS_IN_QUARTER = 3;

// --- Weekday Numbers (Zeller: 0=Sat..6=Fri) ---
constexpr int K_CAL_WEEKDAY_SAT = 0;
constexpr int K_CAL_WEEKDAY_SUN = 1;
constexpr int K_CAL_WEEKDAY_MON = 2;
constexpr int K_CAL_WEEKDAY_TUE = 3;
constexpr int K_CAL_WEEKDAY_WED = 4;
constexpr int K_CAL_WEEKDAY_THU = 5;
constexpr int K_CAL_WEEKDAY_FRI = 6;

// --- Weekday Names ---
const char* const K_CAL_WEEKDAY_NAME_SAT = "Saturday";
const char* const K_CAL_WEEKDAY_NAME_SUN = "Sunday";
const char* const K_CAL_WEEKDAY_NAME_MON = "Monday";
const char* const K_CAL_WEEKDAY_NAME_TUE = "Tuesday";
const char* const K_CAL_WEEKDAY_NAME_WED = "Wednesday";
const char* const K_CAL_WEEKDAY_NAME_THU = "Thursday";
const char* const K_CAL_WEEKDAY_NAME_FRI = "Friday";

// --- Month Indices (0-based table positions) ---
constexpr int K_CAL_MONTH_INDEX_JAN = 0;
constexpr int K_CAL_MONTH_INDEX_FEB = 1;
constexpr int K_CAL_MONTH_INDEX_MAR = 2;
constexpr int K_CAL_MONTH_INDEX_APR = 3;
constexpr int K_CAL_MONTH_INDEX_MAY = 4;
constexpr int K_CAL_MONTH_INDEX_JUN = 5;
constexpr int K_CAL_MONTH_INDEX_JUL = 6;
constexpr int K_CAL_MONTH_INDEX_AUG = 7;
constexpr int K_CAL_MONTH_INDEX_SEP = 8;
constexpr int K_CAL_MONTH_INDEX_OCT = 9;
constexpr int K_CAL_MONTH_INDEX_NOV = 10;
constexpr int K_CAL_MONTH_INDEX_DEC = 11;

// --- Days Per Month ---
constexpr int K_CAL_DAYS_JAN = 31;
constexpr int K_CAL_DAYS_FEB_COMMON = 28;
constexpr int K_CAL_DAYS_FEB_LEAP = 29;
constexpr int K_CAL_DAYS_MAR = 31;
constexpr int K_CAL_DAYS_APR = 30;
constexpr int K_CAL_DAYS_MAY = 31;
constexpr int K_CAL_DAYS_JUN = 30;
constexpr int K_CAL_DAYS_JUL = 31;
constexpr int K_CAL_DAYS_AUG = 31;
constexpr int K_CAL_DAYS_SEP = 30;
constexpr int K_CAL_DAYS_OCT = 31;
constexpr int K_CAL_DAYS_NOV = 30;
constexpr int K_CAL_DAYS_DEC = 31;

// --- Operation Labels ---
const char* const K_CAL_OP_LABEL_DAY_OF_WEEK = "day_of_week";
const char* const K_CAL_OP_LABEL_IS_LEAP_YEAR = "is_leap_year";
const char* const K_CAL_OP_LABEL_DAY_OF_YEAR = "day_of_year";

// --- Operation Descriptions ---
const char* const K_CAL_OP_DESC_DAY_OF_WEEK = "compute weekday via Zeller congruence (0=Sat..6=Fri)";
const char* const K_CAL_OP_DESC_IS_LEAP_YEAR = "check Gregorian leap year (div by 4, not 100 unless 400)";
const char* const K_CAL_OP_DESC_DAY_OF_YEAR = "sum days per month before the date plus the day";

// --- Season Names ---
const char* const K_CAL_SEASON_WINTER = "winter";
const char* const K_CAL_SEASON_SPRING = "spring";
const char* const K_CAL_SEASON_SUMMER = "summer";
const char* const K_CAL_SEASON_AUTUMN = "autumn";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class CalOp {
    DAY_OF_WEEK,
    IS_LEAP_YEAR,
    DAY_OF_YEAR,
};

struct CalendarCaseSpec {
    std::string name;
    CalOp op;
    size_t caseNumber;
    int32_t year;
    int32_t month;
    int32_t day;
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

CalendarCaseSpec GetCalendarCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    CalendarCaseSpec spec;
    spec.name = BuildIndexedName("calendarCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.year = K_CAL_BASE_YEAR + static_cast<int32_t>(caseNumber);
    spec.month = static_cast<int32_t>(caseNumber % K_CAL_MONTHS_PER_YEAR) + K_CAL_MONTH_BASE_ONE;
    spec.day = static_cast<int32_t>(caseNumber % K_CAL_DAYS_CYCLE) + K_CAL_DAY_BASE_ONE;
    const size_t cycle = caseNumber % K_CAL_OP_CYCLE;
    if (cycle == K_CAL_OP_INDEX_DAY_OF_WEEK) {
        spec.op = CalOp::DAY_OF_WEEK;
    } else if (cycle == K_CAL_OP_INDEX_IS_LEAP_YEAR) {
        spec.op = CalOp::IS_LEAP_YEAR;
    } else {
        spec.op = CalOp::DAY_OF_YEAR;
    }
    return spec;
}

const char* CalOpLabel(CalOp op)
{
    if (op == CalOp::DAY_OF_WEEK) {
        return K_CAL_OP_LABEL_DAY_OF_WEEK;
    }
    if (op == CalOp::IS_LEAP_YEAR) {
        return K_CAL_OP_LABEL_IS_LEAP_YEAR;
    }
    return K_CAL_OP_LABEL_DAY_OF_YEAR;
}

const char* CalOpDescription(CalOp op)
{
    if (op == CalOp::DAY_OF_WEEK) {
        return K_CAL_OP_DESC_DAY_OF_WEEK;
    }
    if (op == CalOp::IS_LEAP_YEAR) {
        return K_CAL_OP_DESC_IS_LEAP_YEAR;
    }
    return K_CAL_OP_DESC_DAY_OF_YEAR;
}

// === Helpers: Calendar Arithmetic ===

bool IsLeapYear(int year)
{
    const bool divByFour = (year % K_CAL_QUADRENNIAL_DIV == 0);
    const bool divByCentury = (year % K_CAL_CENTURY_MOD == 0);
    const bool divByQuadricentennial = (year % K_CAL_QUADRICENTENNIAL_DIV == 0);
    return divByFour && (!divByCentury || divByQuadricentennial);
}

void PopulateCommonDays(int* days)
{
    days[K_CAL_MONTH_INDEX_JAN] = K_CAL_DAYS_JAN;
    days[K_CAL_MONTH_INDEX_FEB] = K_CAL_DAYS_FEB_COMMON;
    days[K_CAL_MONTH_INDEX_MAR] = K_CAL_DAYS_MAR;
    days[K_CAL_MONTH_INDEX_APR] = K_CAL_DAYS_APR;
    days[K_CAL_MONTH_INDEX_MAY] = K_CAL_DAYS_MAY;
    days[K_CAL_MONTH_INDEX_JUN] = K_CAL_DAYS_JUN;
    days[K_CAL_MONTH_INDEX_JUL] = K_CAL_DAYS_JUL;
    days[K_CAL_MONTH_INDEX_AUG] = K_CAL_DAYS_AUG;
    days[K_CAL_MONTH_INDEX_SEP] = K_CAL_DAYS_SEP;
    days[K_CAL_MONTH_INDEX_OCT] = K_CAL_DAYS_OCT;
    days[K_CAL_MONTH_INDEX_NOV] = K_CAL_DAYS_NOV;
    days[K_CAL_MONTH_INDEX_DEC] = K_CAL_DAYS_DEC;
}

int DaysInMonth(int year, int monthIndex)
{
    static int commonDays[K_CAL_MONTHS_PER_YEAR];
    static bool commonDaysReady = false;
    if (!commonDaysReady) {
        PopulateCommonDays(commonDays);
        commonDaysReady = true;
    }
    if (monthIndex == K_CAL_MONTH_INDEX_FEB && IsLeapYear(year)) {
        return K_CAL_DAYS_FEB_LEAP;
    }
    return commonDays[monthIndex];
}

int DayOfYear(int year, int month, int day)
{
    int total = 0;
    for (int prevMonth = K_CAL_MONTH_BASE_ONE; prevMonth < month; prevMonth++) {
        total += DaysInMonth(year, prevMonth - K_CAL_MONTH_BASE_ONE);
    }
    total += day;
    return total;
}

int DayOfWeekZeller(int year, int month, int day)
{
    int adjustedYear = year;
    int adjustedMonth = month;
    if (adjustedMonth < K_CAL_MONTHS_IN_QUARTER) {
        adjustedMonth += static_cast<int>(K_CAL_MONTHS_PER_YEAR);
        adjustedYear -= K_CAL_YEAR_DECREMENT;
    }
    const int centuryPart = adjustedYear % K_CAL_CENTURY_MOD;
    const int centuryDiv = adjustedYear / K_CAL_CENTURY_MOD;
    const int monthTerm = (K_CAL_ZELLER_MONTH_FACTOR * (adjustedMonth + K_CAL_MONTH_BASE_ONE)) /
        K_CAL_ZELLER_MONTH_DIVISOR;
    const int sum = day + monthTerm + centuryPart + centuryPart / K_CAL_QUADRENNIAL_DIV +
        centuryDiv / K_CAL_QUADRENNIAL_DIV + K_CAL_ZELLER_CENTURY_FACTOR * centuryDiv;
    const int raw = sum % K_CAL_WEEK_MOD;
    return raw;
}

const char* WeekdayName(int weekday)
{
    if (weekday == K_CAL_WEEKDAY_SAT) {
        return K_CAL_WEEKDAY_NAME_SAT;
    }
    if (weekday == K_CAL_WEEKDAY_SUN) {
        return K_CAL_WEEKDAY_NAME_SUN;
    }
    if (weekday == K_CAL_WEEKDAY_MON) {
        return K_CAL_WEEKDAY_NAME_MON;
    }
    if (weekday == K_CAL_WEEKDAY_TUE) {
        return K_CAL_WEEKDAY_NAME_TUE;
    }
    if (weekday == K_CAL_WEEKDAY_WED) {
        return K_CAL_WEEKDAY_NAME_WED;
    }
    if (weekday == K_CAL_WEEKDAY_THU) {
        return K_CAL_WEEKDAY_NAME_THU;
    }
    return K_CAL_WEEKDAY_NAME_FRI;
}

// === Helpers: Extended Calendar Arithmetic ===

bool IsValidDate(int year, int month, int day)
{
    if (month < K_CAL_MONTH_BASE_ONE || month > static_cast<int>(K_CAL_MONTHS_PER_YEAR)) {
        return false;
    }
    const int monthIndex = month - K_CAL_MONTH_BASE_ONE;
    return day >= K_CAL_DAY_BASE_ONE && day <= DaysInMonth(year, monthIndex);
}

int NextLeapYear(int year)
{
    int candidate = year + K_CAL_MONTH_BASE_ONE;
    while (!IsLeapYear(candidate)) {
        candidate += K_CAL_MONTH_BASE_ONE;
    }
    return candidate;
}

const char* SeasonFromMonth(int month)
{
    static const char* const seasonByBucket[K_CAL_MONTHS_PER_YEAR / K_CAL_MONTHS_IN_QUARTER] = {
        K_CAL_SEASON_WINTER, K_CAL_SEASON_SPRING, K_CAL_SEASON_SUMMER, K_CAL_SEASON_AUTUMN
    };
    const int cyclicIndex = month % static_cast<int>(K_CAL_MONTHS_PER_YEAR);
    return seasonByBucket[cyclicIndex / K_CAL_MONTHS_IN_QUARTER];
}

bool IsWeekend(int weekday)
{
    return weekday == K_CAL_WEEKDAY_SAT || weekday == K_CAL_WEEKDAY_SUN;
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

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Result Building ===

napi_value BuildCalendarResult(napi_env env, const CalendarCaseSpec& spec, int weekday,
    bool isLeap, int dayOfYear, bool isValidDate, int nextLeapYear, int century,
    const char* season, bool isWeekend, int quarter)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", CalOpLabel(spec.op));
    SetNamedString(env, result, "description", CalOpDescription(spec.op));
    SetNamedInt32(env, result, "year", spec.year);
    SetNamedInt32(env, result, "month", spec.month);
    SetNamedInt32(env, result, "day", spec.day);
    SetNamedString(env, result, "weekday", WeekdayName(weekday));
    SetNamedInt32(env, result, "weekdayNum", weekday);
    SetNamedBool(env, result, "isLeapYear", isLeap);
    SetNamedInt32(env, result, "dayOfYear", dayOfYear);
    SetNamedBool(env, result, "isValidDate", isValidDate);
    SetNamedInt32(env, result, "nextLeapYear", nextLeapYear);
    SetNamedInt32(env, result, "century", century);
    SetNamedString(env, result, "season", season);
    SetNamedBool(env, result, "isWeekend", isWeekend);
    SetNamedInt32(env, result, "quarter", quarter);
    return result;
}

// === Test Entry ===

static napi_value RunCalendarCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_CAL_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid calendar case");
        return nullptr;
    }
    const auto spec = GetCalendarCaseSpec(caseIndex);
    const int weekday = DayOfWeekZeller(spec.year, spec.month, spec.day);
    const bool isLeap = IsLeapYear(spec.year);
    const int dayOfYear = DayOfYear(spec.year, spec.month, spec.day);
    const bool isValidDate = IsValidDate(spec.year, spec.month, spec.day);
    const int nextLeapYear = NextLeapYear(spec.year);
    const int century = (spec.year - K_CAL_MONTH_BASE_ONE) / K_CAL_CENTURY_MOD + K_CAL_MONTH_BASE_ONE;
    const char* season = SeasonFromMonth(spec.month);
    const bool isWeekend = IsWeekend(weekday);
    const int quarter = (spec.month - K_CAL_MONTH_BASE_ONE) / K_CAL_MONTHS_IN_QUARTER +
        K_CAL_MONTH_BASE_ONE;
    return BuildCalendarResult(env, spec, weekday, isLeap, dayOfYear, isValidDate, nextLeapYear,
        century, season, isWeekend, quarter);
}

}  // namespace

static napi_value InitCalendarSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_CAL_CASE_COUNT);
    exportNames.reserve(K_CAL_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_CAL_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testCalendarCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunCalendarCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_calendarSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitCalendarSuite,
    .nm_modname = "calendar_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterCalendarSuiteModule(void)
{
    napi_module_register(&g_calendarSuiteModule);
}
