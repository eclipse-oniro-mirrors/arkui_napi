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
#include <vector>

#include "currency_suite_common.h"
#include "napi/native_node_api.h"

static void BuildChangeBreakdown(int64_t cents, std::vector<int64_t>& counts)
{
    counts.clear();
    counts.reserve(CURRENCY_DENOMINATION_COUNT);
    int64_t remaining = cents;
    for (size_t i = 0; i < CURRENCY_DENOMINATION_COUNT; ++i) {
        int64_t safeDenomination =
            CURRENCY_DENOMINATIONS[i] > CURRENCY_ZERO_CENTS ? CURRENCY_DENOMINATIONS[i] : CURRENCY_SAFE_CENTS_DIVISOR;
        counts.push_back(remaining / safeDenomination);
        remaining %= safeDenomination;
    }
}

napi_value InverseRate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: exchange rate");

    double rate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], rate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rate > CURRENCY_MIN_PERCENT && rate <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double safeRate = rate > CURRENCY_ZERO_RATE ? rate : CURRENCY_UNITY;
    return CreateDoubleValue(env, CURRENCY_UNITY / safeRate);
}

napi_value CrossRate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: rateA, rateB");

    double rateA = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], rateA)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rateA > CURRENCY_MIN_PERCENT && rateA <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double rateB = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], rateB)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rateB > CURRENCY_MIN_PERCENT && rateB <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double safeBase = rateA > CURRENCY_ZERO_RATE ? rateA : CURRENCY_UNITY;
    return CreateDoubleValue(env, rateB / safeBase);
}

napi_value RateSpread(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: rateA, rateB");

    double rateA = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], rateA)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rateA > CURRENCY_MIN_PERCENT && rateA <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double rateB = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], rateB)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rateB > CURRENCY_MIN_PERCENT && rateB <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double safeBase = rateA > CURRENCY_ZERO_RATE ? rateA : CURRENCY_UNITY;
    double spreadValue = (rateA - rateB) / safeBase * CURRENCY_PERCENT_BASE;
    return CreateDoubleValue(env, spreadValue);
}

napi_value ConvertWithSpread(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: amountCents, rate, spreadPercent");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double rate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], rate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rate > CURRENCY_MIN_PERCENT && rate <= CURRENCY_MAX_PERCENT, "Exchange rate out of range");

    double spreadPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], spreadPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, spreadPercent >= CURRENCY_MIN_PERCENT && spreadPercent <= CURRENCY_MAX_PERCENT,
                "Spread percent out of range");

    double effectiveRate = rate * (CURRENCY_UNITY - spreadPercent / CURRENCY_PERCENT_BASE);
    double convertedValue = static_cast<double>(amountCents) * effectiveRate;
    NAPI_ASSERT(env, CentsValueWithinBounds(convertedValue), "Converted amount out of range");
    return CreateInt64Value(env, RoundHalfUp(convertedValue));
}

napi_value BidAskMidpoint(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: bid, ask");

    double bid = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bid)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bid > CURRENCY_MIN_PERCENT && bid <= CURRENCY_MAX_PERCENT, "Bid rate out of range");

    double ask = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], ask)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ask > CURRENCY_MIN_PERCENT && ask <= CURRENCY_MAX_PERCENT, "Ask rate out of range");
    NAPI_ASSERT(env, bid <= ask, "Bid rate must not exceed ask rate");

    return CreateDoubleValue(env, (bid + ask) / CURRENCY_HALVING_DIVISOR);
}

napi_value BidAskSpreadPercent(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: bid, ask");

    double bid = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bid)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bid > CURRENCY_MIN_PERCENT && bid <= CURRENCY_MAX_PERCENT, "Bid rate out of range");

    double ask = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], ask)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ask > CURRENCY_MIN_PERCENT && ask <= CURRENCY_MAX_PERCENT, "Ask rate out of range");
    NAPI_ASSERT(env, bid <= ask, "Bid rate must not exceed ask rate");

    double safeAsk = ask > CURRENCY_ZERO_RATE ? ask : CURRENCY_UNITY;
    double spreadPercent = (ask - bid) / safeAsk * CURRENCY_PERCENT_BASE;
    return CreateDoubleValue(env, spreadPercent);
}

napi_value VatAmount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: netCents, vatPercent");

    int64_t netCentsValue = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], netCentsValue)) {
        return nullptr;
    }

    double vatPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], vatPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, vatPercent >= CURRENCY_MIN_PERCENT && vatPercent <= CURRENCY_MAX_PERCENT,
                "VAT percent out of range");

    double vatValue = static_cast<double>(netCentsValue) * vatPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(vatValue), "VAT amount out of range");
    return CreateInt64Value(env, RoundHalfUp(vatValue));
}

napi_value GrossFromVat(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: netCents, vatPercent");

    int64_t netCentsValue = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], netCentsValue)) {
        return nullptr;
    }

    double vatPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], vatPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, vatPercent >= CURRENCY_MIN_PERCENT && vatPercent <= CURRENCY_MAX_PERCENT,
                "VAT percent out of range");

    double vatValue = static_cast<double>(netCentsValue) * vatPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(vatValue), "VAT amount out of range");
    int64_t gross = netCentsValue + RoundHalfUp(vatValue);
    NAPI_ASSERT(env, gross >= CURRENCY_MIN_CENTS && gross <= CURRENCY_MAX_CENTS, "Gross amount out of range");
    return CreateInt64Value(env, gross);
}

napi_value VatFromGross(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: grossCents, vatPercent");

    int64_t grossCentsValue = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], grossCentsValue)) {
        return nullptr;
    }

    double vatPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], vatPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, vatPercent >= CURRENCY_MIN_PERCENT && vatPercent <= CURRENCY_MAX_PERCENT,
                "VAT percent out of range");

    double vatDenominator = CURRENCY_PERCENT_BASE + vatPercent;
    double safeDenominator = vatDenominator > CURRENCY_ZERO_RATE ? vatDenominator : CURRENCY_UNITY;
    double vatValue = static_cast<double>(grossCentsValue) * vatPercent / safeDenominator;
    NAPI_ASSERT(env, CentsValueWithinBounds(vatValue), "VAT amount out of range");
    int64_t vatPart = RoundHalfUp(vatValue);
    int64_t netPart = grossCentsValue - vatPart;

    std::vector<int64_t> parts;
    parts.push_back(netPart);
    parts.push_back(vatPart);
    return CreateInt64ArrayValue(env, parts);
}

napi_value SumWithTax(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: cents array, taxPercent");

    std::vector<int64_t> values;
    if (!ExtractInt64Array(env, argv[ARG_INDEX_ZERO], values)) {
        return nullptr;
    }

    double taxPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], taxPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, taxPercent >= CURRENCY_MIN_PERCENT && taxPercent <= CURRENCY_MAX_PERCENT,
                "Tax percent out of range");

    int64_t total = 0;
    SumCentsValues(values, total);
    NAPI_ASSERT(env, total >= CURRENCY_MIN_CENTS && total <= CURRENCY_MAX_CENTS, "Total out of range");

    double taxValue = static_cast<double>(total) * taxPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(taxValue), "Tax amount out of range");
    int64_t totalWithTax = total + RoundHalfUp(taxValue);
    NAPI_ASSERT(env, totalWithTax >= CURRENCY_MIN_CENTS && totalWithTax <= CURRENCY_MAX_CENTS,
                "Total with tax out of range");
    return CreateInt64Value(env, totalWithTax);
}

napi_value DiscountChain(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: amountCents, d1Percent, d2Percent");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double d1Percent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], d1Percent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, d1Percent >= CURRENCY_MIN_PERCENT && d1Percent <= CURRENCY_MAX_DISCOUNT_PERCENT,
                "Discount percent out of range");

    double d2Percent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_TWO], d2Percent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, d2Percent >= CURRENCY_MIN_PERCENT && d2Percent <= CURRENCY_MAX_DISCOUNT_PERCENT,
                "Discount percent out of range");

    double firstDiscountValue = static_cast<double>(amountCents) * d1Percent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(firstDiscountValue), "Discount amount out of range");
    int64_t firstPrice = amountCents - RoundHalfUp(firstDiscountValue);
    NAPI_ASSERT(env, firstPrice >= CURRENCY_MIN_CENTS && firstPrice <= CURRENCY_MAX_CENTS,
                "Discounted price out of range");

    double secondDiscountValue = static_cast<double>(firstPrice) * d2Percent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(secondDiscountValue), "Discount amount out of range");
    int64_t finalPrice = firstPrice - RoundHalfUp(secondDiscountValue);
    NAPI_ASSERT(env, finalPrice >= CURRENCY_MIN_CENTS && finalPrice <= CURRENCY_MAX_CENTS,
                "Discounted price out of range");
    return CreateInt64Value(env, finalPrice);
}

napi_value EffectiveDiscount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: d1Percent, d2Percent");

    double d1Percent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], d1Percent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, d1Percent >= CURRENCY_MIN_PERCENT && d1Percent <= CURRENCY_MAX_DISCOUNT_PERCENT,
                "Discount percent out of range");

    double d2Percent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], d2Percent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, d2Percent >= CURRENCY_MIN_PERCENT && d2Percent <= CURRENCY_MAX_DISCOUNT_PERCENT,
                "Discount percent out of range");

    double percentDenominator = CURRENCY_PERCENT_BASE;
    double safeDenominator = percentDenominator > CURRENCY_ZERO_RATE ? percentDenominator : CURRENCY_UNITY;
    double combined = d1Percent + d2Percent - d1Percent * d2Percent / safeDenominator;
    return CreateDoubleValue(env, combined);
}

napi_value RebateAmount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: purchaseCents, rebatePercent");

    int64_t purchaseCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], purchaseCents)) {
        return nullptr;
    }

    double rebatePercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], rebatePercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, rebatePercent >= CURRENCY_MIN_PERCENT && rebatePercent <= CURRENCY_MAX_PERCENT,
                "Rebate percent out of range");

    double rebateValue = static_cast<double>(purchaseCents) * rebatePercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(rebateValue), "Rebate amount out of range");
    return CreateInt64Value(env, RoundHalfUp(rebateValue));
}

napi_value CashbackAmount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: purchaseCents, cashbackPercent");

    int64_t purchaseCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], purchaseCents)) {
        return nullptr;
    }

    double cashbackPercent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], cashbackPercent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, cashbackPercent >= CURRENCY_MIN_PERCENT && cashbackPercent <= CURRENCY_MAX_PERCENT,
                "Cashback percent out of range");

    double cashbackValue = static_cast<double>(purchaseCents) * cashbackPercent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(cashbackValue), "Cashback amount out of range");
    return CreateInt64Value(env, RoundHalfUp(cashbackValue));
}

napi_value BankersRounding(napi_env env, napi_callback_info info)
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
    double floorValue = std::floor(centsValue);
    double difference = centsValue - floorValue;
    int64_t floorCents = static_cast<int64_t>(floorValue);
    int64_t rounded = floorCents;
    if (difference > CURRENCY_BANKERS_TIE) {
        rounded = floorCents + CURRENCY_ROUND_STEP;
    } else if (difference < CURRENCY_BANKERS_TIE) {
        rounded = floorCents;
    } else if (floorCents % CURRENCY_EVEN_RADIX == CURRENCY_ZERO_CENTS) {
        rounded = floorCents;
    } else {
        rounded = floorCents + CURRENCY_ROUND_STEP;
    }
    return CreateInt64Value(env, rounded);
}

napi_value CentsRoundUp(napi_env env, napi_callback_info info)
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
    return CreateInt64Value(env, static_cast<int64_t>(std::ceil(centsValue)));
}

napi_value CentsRoundDown(napi_env env, napi_callback_info info)
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
    return CreateInt64Value(env, static_cast<int64_t>(std::floor(centsValue)));
}

napi_value ChangeBreakdown(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount in cents");

    int64_t cents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], cents)) {
        return nullptr;
    }
    NAPI_ASSERT(env, cents >= CURRENCY_ZERO_CENTS, "Amount must not be negative");

    std::vector<int64_t> counts;
    BuildChangeBreakdown(cents, counts);
    return CreateInt64ArrayValue(env, counts);
}

napi_value DenominationsUsed(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount in cents");

    int64_t cents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], cents)) {
        return nullptr;
    }
    NAPI_ASSERT(env, cents >= CURRENCY_ZERO_CENTS, "Amount must not be negative");

    std::vector<int64_t> counts;
    BuildChangeBreakdown(cents, counts);
    int64_t usedCount = 0;
    for (size_t i = 0; i < counts.size(); ++i) {
        usedCount += counts[i];
    }
    return CreateInt64Value(env, usedCount);
}

napi_value SmallestDenominationCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amount in cents");

    int64_t cents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], cents)) {
        return nullptr;
    }
    NAPI_ASSERT(env, cents >= CURRENCY_ZERO_CENTS, "Amount must not be negative");

    double largestDenomination = static_cast<double>(CURRENCY_DENOMINATIONS[CURRENCY_DENOMINATION_INDEX_ZERO]);
    double safeLargest = largestDenomination > CURRENCY_ZERO_RATE ? largestDenomination : CURRENCY_UNITY;
    int64_t count = static_cast<int64_t>(std::ceil(static_cast<double>(cents) / safeLargest));
    return CreateInt64Value(env, count);
}

napi_value PercentOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: amountCents, percent");

    int64_t amountCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], amountCents)) {
        return nullptr;
    }

    double percent = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], percent)) {
        return nullptr;
    }
    NAPI_ASSERT(env, percent >= CURRENCY_MIN_PERCENT && percent <= CURRENCY_MAX_PERCENT, "Percent out of range");

    double partValue = static_cast<double>(amountCents) * percent / CURRENCY_PERCENT_BASE;
    NAPI_ASSERT(env, CentsValueWithinBounds(partValue), "Computed amount out of range");
    return CreateInt64Value(env, RoundHalfUp(partValue));
}

napi_value PercentChange(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: fromCents, toCents");

    int64_t fromCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ZERO], fromCents)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fromCents != CURRENCY_ZERO_CENTS, "Baseline amount must not be zero");

    int64_t toCents = 0;
    if (!ExtractInt64Arg(env, argv[ARG_INDEX_ONE], toCents)) {
        return nullptr;
    }

    int64_t magnitude = fromCents < CURRENCY_ZERO_CENTS ? -fromCents : fromCents;
    double safeMagnitude =
        static_cast<double>(magnitude) > CURRENCY_ZERO_RATE ? static_cast<double>(magnitude) : CURRENCY_UNITY;
    double changePercent =
        (static_cast<double>(toCents) - static_cast<double>(fromCents)) / safeMagnitude * CURRENCY_PERCENT_BASE;
    return CreateDoubleValue(env, changePercent);
}

napi_status RegisterCurrencyOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("inverseRate", InverseRate),
        DECLARE_NAPI_FUNCTION("crossRate", CrossRate),
        DECLARE_NAPI_FUNCTION("rateSpread", RateSpread),
        DECLARE_NAPI_FUNCTION("convertWithSpread", ConvertWithSpread),
        DECLARE_NAPI_FUNCTION("bidAskMidpoint", BidAskMidpoint),
        DECLARE_NAPI_FUNCTION("bidAskSpreadPercent", BidAskSpreadPercent),
        DECLARE_NAPI_FUNCTION("vatAmount", VatAmount),
        DECLARE_NAPI_FUNCTION("grossFromVat", GrossFromVat),
        DECLARE_NAPI_FUNCTION("vatFromGross", VatFromGross),
        DECLARE_NAPI_FUNCTION("sumWithTax", SumWithTax),
        DECLARE_NAPI_FUNCTION("discountChain", DiscountChain),
        DECLARE_NAPI_FUNCTION("effectiveDiscount", EffectiveDiscount),
        DECLARE_NAPI_FUNCTION("rebateAmount", RebateAmount),
        DECLARE_NAPI_FUNCTION("cashbackAmount", CashbackAmount),
        DECLARE_NAPI_FUNCTION("bankersRounding", BankersRounding),
        DECLARE_NAPI_FUNCTION("centsRoundUp", CentsRoundUp),
        DECLARE_NAPI_FUNCTION("centsRoundDown", CentsRoundDown),
        DECLARE_NAPI_FUNCTION("changeBreakdown", ChangeBreakdown),
        DECLARE_NAPI_FUNCTION("denominationsUsed", DenominationsUsed),
        DECLARE_NAPI_FUNCTION("smallestDenominationCount", SmallestDenominationCount),
        DECLARE_NAPI_FUNCTION("percentOf", PercentOf),
        DECLARE_NAPI_FUNCTION("percentChange", PercentChange),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
