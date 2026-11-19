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

#ifndef SAMPLE_NATIVE_MODULE_CURRENCY_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_CURRENCY_SUITE_COMMON_H

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

constexpr int64_t CURRENCY_CENTS_SCALE = 100;
constexpr int64_t CURRENCY_MIN_CENTS = -1000000000000LL;
constexpr int64_t CURRENCY_MAX_CENTS = 1000000000000LL;
constexpr int64_t CURRENCY_ZERO_CENTS = 0;
constexpr int64_t CURRENCY_MIN_PEOPLE = 1;
constexpr int64_t CURRENCY_MAX_PEOPLE = 512;
constexpr int64_t CURRENCY_RADIX = 10;
constexpr int64_t CURRENCY_PARSE_MAJOR_LIMIT = CURRENCY_MAX_CENTS / CURRENCY_CENTS_SCALE;
constexpr int64_t CURRENCY_SAFE_CENTS_DIVISOR = 1;
constexpr int64_t CURRENCY_ROUND_STEP = 1;
constexpr int64_t CURRENCY_EVEN_RADIX = 2;

constexpr double CURRENCY_CENTS_SCALE_DOUBLE = 100.0;
constexpr double CURRENCY_PERCENT_BASE = 100.0;
constexpr double CURRENCY_MIN_PERCENT = 0.0;
constexpr double CURRENCY_MAX_PERCENT = 1000.0;
constexpr double CURRENCY_MAX_DISCOUNT_PERCENT = 100.0;
constexpr double CURRENCY_MIN_MAJOR_AMOUNT = -10000000000.0;
constexpr double CURRENCY_MAX_MAJOR_AMOUNT = 10000000000.0;
constexpr double CURRENCY_ROUND_OFFSET = 0.5;
constexpr double CURRENCY_BANKERS_TIE = 0.5;
constexpr double CURRENCY_ZERO_RATE = 0.0;
constexpr double CURRENCY_UNITY = 1.0;
constexpr double CURRENCY_HALVING_DIVISOR = 2.0;

constexpr size_t CURRENCY_MAX_ARRAY_LENGTH = 1024;
constexpr size_t CURRENCY_MIN_INTEGER_DIGITS = 1;
constexpr size_t CURRENCY_CENTS_DECIMALS = 2;
constexpr size_t CURRENCY_SYMBOL_MIN_LENGTH = 1;
constexpr size_t CURRENCY_SYMBOL_MAX_LENGTH = 3;
constexpr size_t CURRENCY_SINGLE_BYTE_LENGTH = 1;
constexpr size_t CURRENCY_TEXT_MAX_LENGTH = 64;
constexpr size_t CURRENCY_AMOUNT_TEXT_MAX_LENGTH = 32;
constexpr size_t CURRENCY_STRING_BUFFER_PADDING = 1;
constexpr size_t CURRENCY_EMPTY_LENGTH = 0;
constexpr size_t CURRENCY_GROUP_SIZE = 3;
constexpr size_t CURRENCY_SEPARATOR_LENGTH = 1;
constexpr size_t CURRENCY_LAST_INDEX_OFFSET = 1;
constexpr size_t CURRENCY_HALVING_INDEX_DIVISOR = 2;
constexpr size_t CURRENCY_DENOMINATION_COUNT = 14;
constexpr size_t CURRENCY_DENOMINATION_INDEX_ZERO = 0;

constexpr char CURRENCY_DOT_CHAR = '.';
constexpr char CURRENCY_MINUS_CHAR = '-';
constexpr char CURRENCY_DOLLAR_CHAR = '$';
constexpr char CURRENCY_GROUP_SEPARATOR_CHAR = ',';
constexpr char CURRENCY_PAD_CHAR = '0';
constexpr char CURRENCY_DIGIT_LOWER = '0';
constexpr char CURRENCY_DIGIT_UPPER = '9';

constexpr int CURRENCY_SYMBOL_LETTER_COUNT = 52;
const char* const CURRENCY_SYMBOL_LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

constexpr int CURRENCY_SIGN_SYMBOL_COUNT = 3;
// UTF-8 encodings of the pound sign, the yen sign and the euro sign.
const char* const CURRENCY_SIGN_SYMBOLS[CURRENCY_SIGN_SYMBOL_COUNT] = {
    "\xC2\xA3",
    "\xC2\xA5",
    "\xE2\x82\xAC",
};

constexpr int64_t CURRENCY_DENOMINATIONS[CURRENCY_DENOMINATION_COUNT] = {
    100000, 50000, 20000, 10000, 5000, 1000, 500, 200, 100, 50, 20, 10, 5, 1,
};

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
bool ExtractInt64Arg(napi_env env, napi_value value, int64_t& result);
bool ExtractStringArg(napi_env env, napi_value value, std::string& result);
bool ExtractInt64Array(napi_env env, napi_value value, std::vector<int64_t>& result);
napi_value CreateInt64Value(napi_env env, int64_t value);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateStringValue(napi_env env, const std::string& text);
napi_value CreateInt64ArrayValue(napi_env env, const std::vector<int64_t>& values);
int64_t RoundHalfUp(double value);
bool CentsValueWithinBounds(double value);
void SumCentsValues(const std::vector<int64_t>& values, int64_t& total);

napi_value FormatAmount(napi_env env, napi_callback_info info);
napi_value ParseAmount(napi_env env, napi_callback_info info);
napi_value MajorToCents(napi_env env, napi_callback_info info);
napi_value CentsToMajor(napi_env env, napi_callback_info info);
napi_value ApplyExchangeRate(napi_env env, napi_callback_info info);
napi_value ApplyFee(napi_env env, napi_callback_info info);
napi_value SplitBillEvenly(napi_env env, napi_callback_info info);
napi_value SplitBillRemainder(napi_env env, napi_callback_info info);
napi_value TipTotal(napi_env env, napi_callback_info info);
napi_value DiscountPrice(napi_env env, napi_callback_info info);
napi_value MarkupPrice(napi_env env, napi_callback_info info);
napi_value RoundToCents(napi_env env, napi_callback_info info);
napi_value TotalCents(napi_env env, napi_callback_info info);
napi_value AverageCents(napi_env env, napi_callback_info info);
napi_value RunningBalance(napi_env env, napi_callback_info info);
napi_value LargestExpense(napi_env env, napi_callback_info info);
napi_value SmallestIncome(napi_env env, napi_callback_info info);
napi_value IsBalanced(napi_env env, napi_callback_info info);
napi_value NetCents(napi_env env, napi_callback_info info);
napi_value BudgetRemaining(napi_env env, napi_callback_info info);
napi_value MedianExpense(napi_env env, napi_callback_info info);
napi_value ExpenseCountAbove(napi_env env, napi_callback_info info);

napi_value InverseRate(napi_env env, napi_callback_info info);
napi_value CrossRate(napi_env env, napi_callback_info info);
napi_value RateSpread(napi_env env, napi_callback_info info);
napi_value ConvertWithSpread(napi_env env, napi_callback_info info);
napi_value BidAskMidpoint(napi_env env, napi_callback_info info);
napi_value BidAskSpreadPercent(napi_env env, napi_callback_info info);
napi_value VatAmount(napi_env env, napi_callback_info info);
napi_value GrossFromVat(napi_env env, napi_callback_info info);
napi_value VatFromGross(napi_env env, napi_callback_info info);
napi_value SumWithTax(napi_env env, napi_callback_info info);
napi_value DiscountChain(napi_env env, napi_callback_info info);
napi_value EffectiveDiscount(napi_env env, napi_callback_info info);
napi_value RebateAmount(napi_env env, napi_callback_info info);
napi_value CashbackAmount(napi_env env, napi_callback_info info);
napi_value BankersRounding(napi_env env, napi_callback_info info);
napi_value CentsRoundUp(napi_env env, napi_callback_info info);
napi_value CentsRoundDown(napi_env env, napi_callback_info info);
napi_value ChangeBreakdown(napi_env env, napi_callback_info info);
napi_value DenominationsUsed(napi_env env, napi_callback_info info);
napi_value SmallestDenominationCount(napi_env env, napi_callback_info info);
napi_value PercentOf(napi_env env, napi_callback_info info);
napi_value PercentChange(napi_env env, napi_callback_info info);

napi_status RegisterCurrencyOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_CURRENCY_SUITE_COMMON_H
