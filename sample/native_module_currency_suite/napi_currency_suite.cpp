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
#include <string>
#include <vector>

#include "currency_suite_common.h"
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

bool ExtractInt64Arg(napi_env env, napi_value value, int64_t& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    double number = 0.0;
    NAPI_CALL_BASE(env, napi_get_value_double(env, value, &number), false);
    if (!std::isfinite(number) || std::floor(number) != number) {
        napi_throw_error(env, nullptr, "Argument must be a finite integer");
        return false;
    }
    if (number < static_cast<double>(CURRENCY_MIN_CENTS) || number > static_cast<double>(CURRENCY_MAX_CENTS)) {
        napi_throw_error(env, nullptr, "Argument is out of the supported integer range");
        return false;
    }

    result = static_cast<int64_t>(number);
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
    if (length > CURRENCY_TEXT_MAX_LENGTH) {
        napi_throw_error(env, nullptr, "String argument is too long");
        return false;
    }

    std::string buffer(length + CURRENCY_STRING_BUFFER_PADDING, '\0');
    size_t copied = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, &buffer[0], buffer.size(), &copied), false);
    result.assign(buffer, 0, copied);
    return true;
}

bool ExtractInt64Array(napi_env env, napi_value value, std::vector<int64_t>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > CURRENCY_MAX_ARRAY_LENGTH) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = 0; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        int64_t number = 0;
        if (!ExtractInt64Arg(env, element, number)) {
            return false;
        }
        result.push_back(number);
    }
    return true;
}

napi_value CreateInt64Value(napi_env env, int64_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int64(env, value, &result));
    return result;
}

napi_value CreateDoubleValue(napi_env env, double value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_double(env, value, &result));
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

napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateInt64Value(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

int64_t RoundHalfUp(double value)
{
    return static_cast<int64_t>(std::floor(value + CURRENCY_ROUND_OFFSET));
}

bool CentsValueWithinBounds(double value)
{
    return value >= static_cast<double>(CURRENCY_MIN_CENTS) && value <= static_cast<double>(CURRENCY_MAX_CENTS);
}

void SumCentsValues(const std::vector<int64_t>& values, int64_t& total)
{
    total = CURRENCY_ZERO_CENTS;
    for (size_t i = 0; i < values.size(); ++i) {
        total += values[i];
    }
}

static bool IsSymbolLetter(char candidate)
{
    for (int i = 0; i < CURRENCY_SYMBOL_LETTER_COUNT; ++i) {
        if (CURRENCY_SYMBOL_LETTERS[i] == candidate) {
            return true;
        }
    }
    return false;
}

static bool IsValidSymbolUnit(const std::string& text, size_t position, size_t& unitLength)
{
    if (IsSymbolLetter(text[position])) {
        unitLength = CURRENCY_SINGLE_BYTE_LENGTH;
        return true;
    }
    if (text[position] == CURRENCY_DOLLAR_CHAR) {
        unitLength = CURRENCY_SINGLE_BYTE_LENGTH;
        return true;
    }
    for (int i = 0; i < CURRENCY_SIGN_SYMBOL_COUNT; ++i) {
        std::string sign = CURRENCY_SIGN_SYMBOLS[i];
        if (position + sign.size() <= text.size() && text.compare(position, sign.size(), sign) == 0) {
            unitLength = sign.size();
            return true;
        }
    }
    return false;
}

static int64_t FloorDivideCents(int64_t totalCents, int64_t people)
{
    double safePeople = people > CURRENCY_ZERO_CENTS ? static_cast<double>(people) : CURRENCY_UNITY;
    return static_cast<int64_t>(std::floor(static_cast<double>(totalCents) / safePeople));
}

static bool CheckSymbolUnits(napi_env env, const std::string& symbol)
{
    size_t unitCount = 0;
    size_t position = 0;
    while (position < symbol.size()) {
        size_t unitLength = 0;
        if (!IsValidSymbolUnit(symbol, position, unitLength)) {
            napi_throw_error(env, nullptr, "Symbol contains invalid characters");
            return false;
        }
        position += unitLength;
        ++unitCount;
        if (unitCount > CURRENCY_SYMBOL_MAX_LENGTH) {
            napi_throw_error(env, nullptr, "Symbol must contain at most 3 characters");
            return false;
        }
    }
    return true;
}

static void GroupThousands(std::string& digits)
{
    if (digits.size() <= CURRENCY_GROUP_SIZE) {
        return;
    }
    size_t firstGroupLength = digits.size() % CURRENCY_GROUP_SIZE;
    if (firstGroupLength == CURRENCY_EMPTY_LENGTH) {
        firstGroupLength = CURRENCY_GROUP_SIZE;
    }
    size_t separatorCount = (digits.size() - firstGroupLength) / CURRENCY_GROUP_SIZE;
    size_t insertPosition = firstGroupLength;
    for (size_t i = 0; i < separatorCount; ++i) {
        digits.insert(insertPosition, CURRENCY_SEPARATOR_LENGTH, CURRENCY_GROUP_SEPARATOR_CHAR);
        insertPosition += CURRENCY_GROUP_SIZE + CURRENCY_SEPARATOR_LENGTH;
    }
}

napi_value FormatAmount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: cents, symbol");

    int64_t cents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], cents)) {
        return nullptr;
    }

    std::string symbol;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ONE], symbol)) {
        return nullptr;
    }
    NAPI_ASSERT(env, symbol.size() >= CURRENCY_SYMBOL_MIN_LENGTH, "Symbol must not be empty");
    if (!CheckSymbolUnits(env, symbol)) {
        return nullptr;
    }

    int64_t magnitude = cents;
    bool negative = cents < CURRENCY_ZERO_CENTS;
    if (negative) {
        magnitude = -cents;
    }
    int64_t majorPart = magnitude / CURRENCY_CENTS_SCALE;
    int64_t minorPart = magnitude % CURRENCY_CENTS_SCALE;

    std::string digits = std::to_string(majorPart);
    GroupThousands(digits);

    std::string minorText = std::to_string(minorPart);
    if (minorText.size() < CURRENCY_CENTS_DECIMALS) {
        minorText.insert(CURRENCY_EMPTY_LENGTH, CURRENCY_CENTS_DECIMALS - minorText.size(), CURRENCY_PAD_CHAR);
    }

    std::string formatted;
    if (negative) {
        formatted += CURRENCY_MINUS_CHAR;
    }
    formatted += symbol;
    formatted += digits;
    formatted += CURRENCY_DOT_CHAR;
    formatted += minorText;
    return CreateStringValue(env, formatted);
}

static size_t StripSymbolPrefix(const std::string& text, size_t start)
{
    size_t position = start;
    size_t symbolUnits = 0;
    while (position < text.size() && symbolUnits < CURRENCY_SYMBOL_MAX_LENGTH) {
        size_t unitLength = 0;
        if (!IsValidSymbolUnit(text, position, unitLength)) {
            break;
        }
        position += unitLength;
        ++symbolUnits;
    }
    return position;
}

static bool AccumulateAmountDigits(napi_env env,
                                   const std::string& text,
                                   size_t position,
                                   int64_t& majorPart,
                                   int64_t& minorPart,
                                   size_t& decimalDigits)
{
    size_t integerDigits = 0;
    bool dotSeen = false;
    while (position < text.size()) {
        char current = text[position];
        if (current == CURRENCY_DOT_CHAR) {
            if (dotSeen) {
                napi_throw_error(env, nullptr, "Amount text has more than one dot");
                return false;
            }
            dotSeen = true;
            ++position;
            continue;
        }
        if (current < CURRENCY_DIGIT_LOWER || current > CURRENCY_DIGIT_UPPER) {
            napi_throw_error(env, nullptr, "Amount text contains invalid characters");
            return false;
        }
        int64_t digit = static_cast<int64_t>(current - CURRENCY_DIGIT_LOWER);
        if (!dotSeen) {
            majorPart = majorPart * CURRENCY_RADIX + digit;
            if (majorPart > CURRENCY_PARSE_MAJOR_LIMIT) {
                napi_throw_error(env, nullptr, "Amount is out of range");
                return false;
            }
            ++integerDigits;
        } else {
            ++decimalDigits;
            if (decimalDigits > CURRENCY_CENTS_DECIMALS) {
                napi_throw_error(env, nullptr, "Amount text has more than two decimals");
                return false;
            }
            minorPart = minorPart * CURRENCY_RADIX + digit;
        }
        ++position;
    }
    if (integerDigits < CURRENCY_MIN_INTEGER_DIGITS) {
        napi_throw_error(env, nullptr, "Amount text must contain integer digits");
        return false;
    }
    return true;
}

napi_value ParseAmount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount text");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }
    NAPI_ASSERT(env, text.size() >= CURRENCY_SYMBOL_MIN_LENGTH, "Amount text must not be empty");
    NAPI_ASSERT(env, text.size() <= CURRENCY_AMOUNT_TEXT_MAX_LENGTH, "Amount text is too long");

    size_t position = 0;
    bool negative = false;
    if (text[position] == CURRENCY_MINUS_CHAR) {
        negative = true;
        ++position;
    }
    position = StripSymbolPrefix(text, position);
    NAPI_ASSERT(env, position < text.size(), "Amount text contains no digits");

    if (!negative && text[position] == CURRENCY_MINUS_CHAR) {
        negative = true;
        ++position;
    }
    NAPI_ASSERT(env, position < text.size(), "Amount text contains no digits");

    int64_t majorPart = 0;
    int64_t minorPart = 0;
    size_t decimalDigits = 0;
    if (!AccumulateAmountDigits(env, text, position, majorPart, minorPart, decimalDigits)) {
        return nullptr;
    }

    for (size_t i = decimalDigits; i < CURRENCY_CENTS_DECIMALS; ++i) {
        minorPart *= CURRENCY_RADIX;
    }

    int64_t magnitude = majorPart * CURRENCY_CENTS_SCALE + minorPart;
    NAPI_ASSERT(env, magnitude <= CURRENCY_MAX_CENTS, "Amount is out of range");
    int64_t cents = negative ? -magnitude : magnitude;
    NAPI_ASSERT(env, cents >= CURRENCY_MIN_CENTS, "Amount is out of range");
    return CreateInt64Value(env, cents);
}

napi_value MajorToCents(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount in major units");

    double amount = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], amount)) {
        return nullptr;
    }
    NAPI_ASSERT(env, amount >= CURRENCY_MIN_MAJOR_AMOUNT && amount <= CURRENCY_MAX_MAJOR_AMOUNT, "Amount out of range");

    double centsValue = amount * CURRENCY_CENTS_SCALE_DOUBLE;
    NAPI_ASSERT(env, CentsValueWithinBounds(centsValue), "Converted cents out of range");
    return CreateInt64Value(env, RoundHalfUp(centsValue));
}

napi_value CentsToMajor(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount in cents");

    int64_t cents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], cents)) {
        return nullptr;
    }

    double major = static_cast<double>(cents) / CURRENCY_CENTS_SCALE_DOUBLE;
    return CreateDoubleValue(env, major);
}

napi_value ApplyExchangeRate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: amountCents, rate");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double rate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], rate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rate > CURRENCY_MIN_PERCENT && rate <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double convertedValue = static_cast<double>(amountCents) * rate;
    NAPI_ASSERT(env, CentsValueWithinBounds(convertedValue), "Converted amount out of range");
    return CreateInt64Value(env, RoundHalfUp(convertedValue));
}

napi_value ApplyFee(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: amountCents, feePercent");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double feePercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], feePercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, feePercent >= CURRENCY_MIN_PERCENT && feePercent <= CURRENCY_MAX_PERCENT,
                "Fee percent out of range");

    double feeValue = static_cast<double>(amountCents) * feePercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(feeValue), "Fee amount out of range");
    return CreateInt64Value(env, RoundHalfUp(feeValue));
}

napi_value SplitBillEvenly(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: totalCents, people");

    int64_t totalCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], totalCents)) {
        return nullptr;
    }

    int64_t people = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], people)) {
        return nullptr;
    }
    NAPI_ASSERT(env, people >= CURRENCY_MIN_PEOPLE && people <= CURRENCY_MAX_PEOPLE, "People count out of range");

    return CreateInt64Value(env, FloorDivideCents(totalCents, people));
}

napi_value SplitBillRemainder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: totalCents, people");

    int64_t totalCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], totalCents)) {
        return nullptr;
    }

    int64_t people = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], people)) {
        return nullptr;
    }
    NAPI_ASSERT(env, people >= CURRENCY_MIN_PEOPLE && people <= CURRENCY_MAX_PEOPLE, "People count out of range");

    int64_t perPerson = FloorDivideCents(totalCents, people);
    int64_t remainder = totalCents - perPerson * people;
    return CreateInt64Value(env, remainder);
}

napi_value TipTotal(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: amountCents, tipPercent");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double tipPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], tipPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, tipPercent >= CURRENCY_MIN_PERCENT && tipPercent <= CURRENCY_MAX_PERCENT,
                "Tip percent out of range");

    double tipValue = static_cast<double>(amountCents) * tipPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(tipValue), "Tip amount out of range");
    int64_t total = amountCents + RoundHalfUp(tipValue);
    NAPI_ASSERT(env, total >= CURRENCY_MIN_CENTS && total <= CURRENCY_MAX_CENTS, "Total amount out of range");
    return CreateInt64Value(env, total);
}

napi_value DiscountPrice(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: amountCents, discountPercent");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double discountPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], discountPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, discountPercent >= CURRENCY_MIN_PERCENT && discountPercent <= CURRENCY_MAX_DISCOUNT_PERCENT,
                "Discount percent out of range");

    double discountValue = static_cast<double>(amountCents) * discountPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(discountValue), "Discount amount out of range");
    int64_t price = amountCents - RoundHalfUp(discountValue);
    NAPI_ASSERT(env, price >= CURRENCY_MIN_CENTS && price <= CURRENCY_MAX_CENTS, "Discounted price out of range");
    return CreateInt64Value(env, price);
}

napi_value MarkupPrice(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: costCents, markupPercent");

    int64_t costCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], costCents)) {
        return nullptr;
    }

    double markupPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], markupPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, markupPercent >= CURRENCY_MIN_PERCENT && markupPercent <= CURRENCY_MAX_PERCENT,
                "Markup percent out of range");

    double markupValue = static_cast<double>(costCents) * markupPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(markupValue), "Markup amount out of range");
    int64_t price = costCents + RoundHalfUp(markupValue);
    NAPI_ASSERT(env, price >= CURRENCY_MIN_CENTS && price <= CURRENCY_MAX_CENTS, "Marked up price out of range");
    return CreateInt64Value(env, price);
}

napi_value RoundToCents(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount in cents");

    double amount = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], amount)) {
        return nullptr;
    }
    NAPI_ASSERT(env, CentsValueWithinBounds(amount), "Amount out of range");

    return CreateInt64Value(env, RoundHalfUp(amount));
}

napi_value TotalCents(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: cents array");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t total = 0;
    SumCentsValues(values, total);
    NAPI_ASSERT(env, total >= CURRENCY_MIN_CENTS && total <= CURRENCY_MAX_CENTS, "Total out of range");
    return CreateInt64Value(env, total);
}

napi_value AverageCents(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: cents array");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !values.empty(), "Values must not be empty");

    int64_t total = 0;
    SumCentsValues(values, total);
    double safeSize = values.size() > CURRENCY_EMPTY_LENGTH ? static_cast<double>(values.size()) : CURRENCY_UNITY;
    double meanValue = static_cast<double>(total) / safeSize;
    NAPI_ASSERT(env, CentsValueWithinBounds(meanValue), "Average out of range");
    return CreateInt64Value(env, RoundHalfUp(meanValue));
}

napi_value RunningBalance(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: startCents, deltas array");

    int64_t startCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], startCents)) {
        return nullptr;
    }

    std::vector<int64_t> deltas;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ONE], deltas)) {
        return nullptr;
    }

    std::vector<int64_t> balances;
    balances.reserve(deltas.size());
    int64_t balance = startCents;
    for (size_t i = 0; i < deltas.size(); ++i) {
        balance += deltas[i];
        NAPI_ASSERT(env, balance >= CURRENCY_MIN_CENTS && balance <= CURRENCY_MAX_CENTS, "Balance out of range");
        balances.push_back(balance);
    }
    return CreateInt64ArrayValue(env, balances);
}

napi_value LargestExpense(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: cents array");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t largestExpenseValue = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < largestExpenseValue) {
            largestExpenseValue = values[i];
        }
    }
    return CreateInt64Value(env, largestExpenseValue);
}

napi_value SmallestIncome(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: cents array");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t smallestIncomeValue = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        bool isIncome = values[i] > CURRENCY_ZERO_CENTS;
        bool notFoundYet = smallestIncomeValue == CURRENCY_ZERO_CENTS;
        if (isIncome && (notFoundYet || values[i] < smallestIncomeValue)) {
            smallestIncomeValue = values[i];
        }
    }
    return CreateInt64Value(env, smallestIncomeValue);
}

napi_value IsBalanced(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: debits array, credits array");

    std::vector<int64_t> debits;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], debits)) {
        return nullptr;
    }

    std::vector<int64_t> credits;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ONE], credits)) {
        return nullptr;
    }

    int64_t debitTotal = 0;
    SumCentsValues(debits, debitTotal);
    int64_t creditTotal = 0;
    SumCentsValues(credits, creditTotal);
    return CreateBoolValue(env, debitTotal == creditTotal);
}

napi_value NetCents(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: deltas array");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t total = 0;
    SumCentsValues(values, total);
    NAPI_ASSERT(env, total >= CURRENCY_MIN_CENTS && total <= CURRENCY_MAX_CENTS, "Net amount out of range");
    return CreateInt64Value(env, total);
}

napi_value BudgetRemaining(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: budgetCents, expenses array");

    int64_t budgetCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], budgetCents)) {
        return nullptr;
    }

    std::vector<int64_t> expenses;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ONE], expenses)) {
        return nullptr;
    }

    int64_t expenseTotal = 0;
    SumCentsValues(expenses, expenseTotal);
    int64_t remaining = budgetCents - expenseTotal;
    NAPI_ASSERT(env, remaining >= CURRENCY_MIN_CENTS && remaining <= CURRENCY_MAX_CENTS,
                "Remaining budget out of range");
    return CreateInt64Value(env, remaining);
}

napi_value MedianExpense(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: cents array");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !values.empty(), "Values must not be empty");

    std::vector<int64_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    size_t middleIndex = (sorted.size() - CURRENCY_LAST_INDEX_OFFSET) / CURRENCY_HALVING_INDEX_DIVISOR;
    return CreateInt64Value(env, sorted[middleIndex]);
}

napi_value ExpenseCountAbove(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: cents array, thresholdCents");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    int64_t thresholdCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], thresholdCents)) {
        return nullptr;
    }

    int64_t count = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] > thresholdCents) {
            ++count;
        }
    }
    return CreateInt64Value(env, count);
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("formatAmount", FormatAmount),
        DECLARE_NAPI_FUNCTION("parseAmount", ParseAmount),
        DECLARE_NAPI_FUNCTION("majorToCents", MajorToCents),
        DECLARE_NAPI_FUNCTION("centsToMajor", CentsToMajor),
        DECLARE_NAPI_FUNCTION("applyExchangeRate", ApplyExchangeRate),
        DECLARE_NAPI_FUNCTION("applyFee", ApplyFee),
        DECLARE_NAPI_FUNCTION("splitBillEvenly", SplitBillEvenly),
        DECLARE_NAPI_FUNCTION("splitBillRemainder", SplitBillRemainder),
        DECLARE_NAPI_FUNCTION("tipTotal", TipTotal),
        DECLARE_NAPI_FUNCTION("discountPrice", DiscountPrice),
        DECLARE_NAPI_FUNCTION("markupPrice", MarkupPrice),
        DECLARE_NAPI_FUNCTION("roundToCents", RoundToCents),
        DECLARE_NAPI_FUNCTION("totalCents", TotalCents),
        DECLARE_NAPI_FUNCTION("averageCents", AverageCents),
        DECLARE_NAPI_FUNCTION("runningBalance", RunningBalance),
        DECLARE_NAPI_FUNCTION("largestExpense", LargestExpense),
        DECLARE_NAPI_FUNCTION("smallestIncome", SmallestIncome),
        DECLARE_NAPI_FUNCTION("isBalanced", IsBalanced),
        DECLARE_NAPI_FUNCTION("netCents", NetCents),
        DECLARE_NAPI_FUNCTION("budgetRemaining", BudgetRemaining),
        DECLARE_NAPI_FUNCTION("medianExpense", MedianExpense),
        DECLARE_NAPI_FUNCTION("expenseCountAbove", ExpenseCountAbove),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterCurrencyOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module currencySuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "currencySuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void CurrencySuiteRegisterModule(void)
{
    napi_module_register(&currencySuiteModule);
}
