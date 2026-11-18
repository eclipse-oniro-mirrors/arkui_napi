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

#include "barcode_suite_common.h"
#include "napi/native_node_api.h"

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
    if (length > BARCODE_MAX_INPUT_LENGTH) {
        napi_throw_error(env, nullptr, "Argument is too long");
        return false;
    }

    std::string buffer(length + BARCODE_STRING_BUFFER_PADDING, '\0');
    size_t copied = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, &buffer[0], buffer.size(), &copied), false);
    result.assign(buffer, 0, copied);
    return true;
}

bool ExtractInt32ArrayArg(napi_env env, napi_value value, std::vector<int32_t>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > BARCODE_MAX_ARRAY_LENGTH) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = 0; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        napi_valuetype type = napi_undefined;
        NAPI_CALL_BASE(env, napi_typeof(env, element, &type), false);
        if (type != napi_number) {
            napi_throw_error(env, nullptr, "Array element must be a number");
            return false;
        }

        double number = 0.0;
        NAPI_CALL_BASE(env, napi_get_value_double(env, element, &number), false);
        if (!std::isfinite(number) || std::floor(number) != number) {
            napi_throw_error(env, nullptr, "Array element must be a finite integer");
            return false;
        }
        result.push_back(static_cast<int32_t>(number));
    }
    return true;
}

bool ExtractCodeArg(napi_env env, napi_value value, std::string& result)
{
    std::string text;
    if (!ExtractStringArg(env, value, text)) {
        return false;
    }

    std::string stripped = StripSeparators(text);
    if (stripped.size() < BARCODE_MIN_CODE_LENGTH || stripped.size() > BARCODE_MAX_CODE_LENGTH) {
        napi_throw_error(env, nullptr, "Code must have 1 to 32 digits after separator removal");
        return false;
    }

    for (size_t i = 0; i < stripped.size(); ++i) {
        if (!IsDigitChar(stripped[i])) {
            napi_throw_error(env, nullptr, "Code must contain digits only");
            return false;
        }
    }

    result = stripped;
    return true;
}

bool ExtractCodeAllowXArg(napi_env env, napi_value value, std::string& result)
{
    std::string text;
    if (!ExtractStringArg(env, value, text)) {
        return false;
    }

    std::string stripped = StripSeparators(text);
    if (stripped.size() < BARCODE_MIN_CODE_LENGTH || stripped.size() > BARCODE_MAX_CODE_LENGTH) {
        napi_throw_error(env, nullptr, "Code must have 1 to 32 characters after separator removal");
        return false;
    }

    for (size_t i = 0; i < stripped.size(); ++i) {
        if (IsDigitChar(stripped[i])) {
            continue;
        }
        if (stripped[i] == BARCODE_CHAR_X && i + BARCODE_LAST_INDEX_OFFSET == stripped.size()) {
            continue;
        }
        napi_throw_error(env, nullptr, "Code must contain digits, with uppercase X allowed at the end only");
        return false;
    }

    result = stripped;
    return true;
}

napi_value CreateStringValue(napi_env env, const std::string& value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, value.c_str(), value.size(), &result));
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

napi_value CreateStringArrayValue(napi_env env, const std::vector<std::string>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateStringValue(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

std::string StripSeparators(const std::string& code)
{
    std::string result;
    result.reserve(code.size());
    for (size_t i = 0; i < code.size(); ++i) {
        if (code[i] == CHAR_HYPHEN || code[i] == CHAR_SPACE) {
            continue;
        }
        result.push_back(code[i]);
    }
    return result;
}

int CharDigitValue(char c, bool& ok)
{
    if (IsDigitChar(c)) {
        ok = true;
        return c - CHAR_DIGIT_ZERO;
    }
    if (c == BARCODE_CHAR_X) {
        ok = true;
        return BARCODE_X_VALUE;
    }
    ok = false;
    return ARG_INDEX_ZERO;
}

int DigitSumOfProduct(int product)
{
    if (product > BARCODE_LUHN_NINE_CARRY) {
        return product - BARCODE_LUHN_NINE_CARRY;
    }
    return product;
}

int ComputeGtinCheckDigit(const std::string& payload, int startWeight)
{
    int sum = 0;
    int weight = startWeight;
    for (size_t i = 0; i < payload.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(payload[i], ok);
        sum += digit * weight;
        if (weight == BARCODE_EAN_WEIGHT_THREE) {
            weight = BARCODE_EAN_WEIGHT_ONE;
        } else {
            weight = BARCODE_EAN_WEIGHT_THREE;
        }
    }

    int remainder = sum % BARCODE_MODULUS_TEN;
    return (BARCODE_MODULUS_TEN - remainder) % BARCODE_MODULUS_TEN;
}

bool HasIsbn13Prefix(const std::string& code)
{
    return code.compare(BARCODE_EMPTY_LENGTH, BARCODE_ISBN_PREFIX_LENGTH, BARCODE_ISBN_PREFIX_978) == ARG_INDEX_ZERO ||
           code.compare(BARCODE_EMPTY_LENGTH, BARCODE_ISBN_PREFIX_LENGTH, BARCODE_ISBN_PREFIX_979) == ARG_INDEX_ZERO;
}

int Code39CharValue(char c, bool& ok)
{
    for (int i = 0; i < BARCODE_CODE39_MODULUS; ++i) {
        if (BARCODE_CODE39_CHARSET[i] == c) {
            ok = true;
            return i;
        }
    }
    ok = false;
    return ARG_INDEX_ZERO;
}

// Computes the Luhn check digit for a payload: digits are walked from the right and every
// second digit, starting with the rightmost, is doubled with carry folding.
static int LuhnCheckDigitFor(const std::string& payload)
{
    int sum = 0;
    bool doubleDigit = true;
    for (size_t i = 0; i < payload.size(); ++i) {
        char c = payload[payload.size() - BARCODE_LAST_INDEX_OFFSET - i];
        bool ok = false;
        int digit = CharDigitValue(c, ok);
        if (doubleDigit) {
            digit = DigitSumOfProduct(digit * BARCODE_LUHN_WEIGHT_2);
        }
        sum += digit;
        doubleDigit = !doubleDigit;
    }

    int remainder = sum % BARCODE_MODULUS_TEN;
    return (BARCODE_MODULUS_TEN - remainder) % BARCODE_MODULUS_TEN;
}

// Verifies a fixed-length digit code with a trailing check character using weighted sums
// modulo 11; length and charset are checked by the caller.
static bool WeightedChecksumMatches(const std::string& code, size_t payloadLength, int weightStart)
{
    int sum = 0;
    int weight = weightStart;
    for (size_t i = 0; i < payloadLength; ++i) {
        bool ok = false;
        int digit = CharDigitValue(code[i], ok);
        sum += digit * weight;
        --weight;
    }

    bool ok = false;
    int checkValue = CharDigitValue(code[payloadLength], ok);
    return checkValue == (BARCODE_MODULUS_ELEVEN - sum % BARCODE_MODULUS_ELEVEN) % BARCODE_MODULUS_ELEVEN;
}

// Verifies a GTIN-style code whose payload uses alternating weights and whose check digit
// satisfies the modulo 10 sum rule.
static bool GtinChecksumMatches(const std::string& code, size_t payloadLength, int startWeight)
{
    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, payloadLength);
    int check = ComputeGtinCheckDigit(payload, startWeight);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return check == last;
}

// Verhoeff check digit: payload digits are folded from the right with the permutation index
// starting at 1, because position 0 is reserved for the appended check digit.
static int VerhoeffCheckDigitFor(const std::string& payload)
{
    int state = 0;
    for (size_t i = 0; i < payload.size(); ++i) {
        char c = payload[payload.size() - BARCODE_LAST_INDEX_OFFSET - i];
        bool ok = false;
        int digit = CharDigitValue(c, ok);
        int permutationIndex = static_cast<int>((i + BARCODE_LAST_INDEX_OFFSET) % BARCODE_VERHOF_PERMUTATION_COUNT);
        state = BARCODE_VERHOF_D_TABLE[state][BARCODE_VERHOF_P_TABLE[permutationIndex][digit]];
    }
    return BARCODE_VERHOF_INV_TABLE[state];
}

// Damm quasigroup walk: the interim digit is the state after processing every digit.
static int DammInterimValue(const std::string& code)
{
    int interim = 0;
    for (size_t i = 0; i < code.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(code[i], ok);
        interim = BARCODE_DAMM_TABLE[interim][digit];
    }
    return interim;
}

napi_value LuhnChecksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateInt32Value(env, LuhnCheckDigitFor(digits));
}

napi_value LuhnValidate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() > BARCODE_MIN_CODE_LENGTH, "Luhn code must contain payload and check digits");

    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, code.size() - BARCODE_LAST_INDEX_OFFSET);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return CreateBoolValue(env, LuhnCheckDigitFor(payload) == last);
}

napi_value LuhnComplete(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: partial digits");

    std::string partial;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], partial)) {
        return nullptr;
    }

    int check = LuhnCheckDigitFor(partial);
    std::string result = partial;
    result.push_back(static_cast<char>(CHAR_DIGIT_ZERO + check));
    return CreateStringValue(env, result);
}

napi_value Isbn10Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: isbn10");

    std::string code;
    if (!ExtractCodeAllowXArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_ISBN10_LENGTH, "ISBN-10 must have exactly 10 characters");

    return CreateBoolValue(env,
                           WeightedChecksumMatches(code, BARCODE_ISBN10_PAYLOAD_LENGTH, BARCODE_ISBN10_WEIGHT_START));
}

napi_value Isbn10ChecksumCharacter(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: isbn10 payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_ISBN10_PAYLOAD_LENGTH, "ISBN-10 payload must have 9 digits");

    int sum = 0;
    int weight = BARCODE_ISBN10_WEIGHT_START;
    for (size_t i = 0; i < payload.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(payload[i], ok);
        sum += digit * weight;
        --weight;
    }

    int remainder = sum % BARCODE_MODULUS_ELEVEN;
    int check = (BARCODE_MODULUS_ELEVEN - remainder) % BARCODE_MODULUS_ELEVEN;
    std::string character;
    if (check == BARCODE_X_VALUE) {
        character.push_back(BARCODE_CHAR_X);
    } else {
        character.push_back(static_cast<char>(CHAR_DIGIT_ZERO + check));
    }
    return CreateStringValue(env, character);
}

napi_value Isbn13Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: isbn13");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_ISBN13_LENGTH, "ISBN-13 must have exactly 13 digits");

    return CreateBoolValue(env, GtinChecksumMatches(code, BARCODE_ISBN13_PAYLOAD_LENGTH, BARCODE_EAN_WEIGHT_ONE));
}

napi_value Isbn13ChecksumDigit(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: isbn13 payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_ISBN13_PAYLOAD_LENGTH, "ISBN-13 payload must have 12 digits");

    return CreateInt32Value(env, ComputeGtinCheckDigit(payload, BARCODE_EAN_WEIGHT_ONE));
}

napi_value Isbn10To13(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: isbn10");

    std::string code;
    if (!ExtractCodeAllowXArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_ISBN10_LENGTH, "ISBN-10 must have exactly 10 characters");
    NAPI_ASSERT(env, WeightedChecksumMatches(code, BARCODE_ISBN10_PAYLOAD_LENGTH, BARCODE_ISBN10_WEIGHT_START),
                "ISBN-10 checksum is invalid");

    std::string result = BARCODE_ISBN_PREFIX_978;
    result += code.substr(BARCODE_EMPTY_LENGTH, BARCODE_ISBN10_PAYLOAD_LENGTH);
    int check = ComputeGtinCheckDigit(result, BARCODE_EAN_WEIGHT_ONE);
    result.push_back(static_cast<char>(CHAR_DIGIT_ZERO + check));
    return CreateStringValue(env, result);
}

napi_value Ean13Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: ean13");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_EAN13_LENGTH, "EAN-13 must have exactly 13 digits");

    return CreateBoolValue(env, GtinChecksumMatches(code, BARCODE_EAN13_PAYLOAD_LENGTH, BARCODE_EAN_WEIGHT_ONE));
}

napi_value Ean13ChecksumDigit(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: ean13 payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_EAN13_PAYLOAD_LENGTH, "EAN-13 payload must have 12 digits");

    return CreateInt32Value(env, ComputeGtinCheckDigit(payload, BARCODE_EAN_WEIGHT_ONE));
}

napi_value Ean8Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: ean8");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_EAN8_LENGTH, "EAN-8 must have exactly 8 digits");

    return CreateBoolValue(env, GtinChecksumMatches(code, BARCODE_EAN8_PAYLOAD_LENGTH, BARCODE_EAN_WEIGHT_THREE));
}

napi_value Ean8ChecksumDigit(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: ean8 payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_EAN8_PAYLOAD_LENGTH, "EAN-8 payload must have 7 digits");

    return CreateInt32Value(env, ComputeGtinCheckDigit(payload, BARCODE_EAN_WEIGHT_THREE));
}

napi_value UpcAValidate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: upcA");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_UPCA_LENGTH, "UPC-A must have exactly 12 digits");

    return CreateBoolValue(env, GtinChecksumMatches(code, BARCODE_UPCA_PAYLOAD_LENGTH, BARCODE_EAN_WEIGHT_THREE));
}

napi_value UpcAChecksumDigit(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: upcA payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_UPCA_PAYLOAD_LENGTH, "UPC-A payload must have 11 digits");

    return CreateInt32Value(env, ComputeGtinCheckDigit(payload, BARCODE_EAN_WEIGHT_THREE));
}

napi_value Itf14Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: itf14");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_ITF14_LENGTH, "ITF-14 must have exactly 14 digits");

    return CreateBoolValue(env, GtinChecksumMatches(code, BARCODE_ITF14_PAYLOAD_LENGTH, BARCODE_EAN_WEIGHT_THREE));
}

napi_value Itf14ChecksumDigit(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: itf14 payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_ITF14_PAYLOAD_LENGTH, "ITF-14 payload must have 13 digits");

    return CreateInt32Value(env, ComputeGtinCheckDigit(payload, BARCODE_EAN_WEIGHT_THREE));
}

napi_value Code39Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code39");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }
    NAPI_ASSERT(env, text.size() >= BARCODE_CODE39_MIN_LENGTH, "Code 39 must contain payload and check characters");

    int sum = 0;
    bool lastOk = false;
    int lastValue = ARG_INDEX_ZERO;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c >= CHAR_LOWER_A && c <= CHAR_LOWER_Z) {
            c = static_cast<char>(c - CHAR_CASE_DELTA);
        }
        bool ok = false;
        int value = Code39CharValue(c, ok);
        NAPI_ASSERT(env, ok, "Code 39 contains characters outside the charset");
        if (i + BARCODE_LAST_INDEX_OFFSET == text.size()) {
            lastOk = true;
            lastValue = value;
        } else {
            sum += value;
        }
    }
    NAPI_ASSERT(env, lastOk, "Code 39 must contain a check character");

    return CreateBoolValue(env, sum % BARCODE_CODE39_MODULUS == lastValue);
}

napi_value IssnValidate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: issn");

    std::string code;
    if (!ExtractCodeAllowXArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_ISSN_LENGTH, "ISSN must have exactly 8 characters");

    return CreateBoolValue(env, WeightedChecksumMatches(code, BARCODE_ISSN_PAYLOAD_LENGTH, BARCODE_ISSN_WEIGHT_START));
}

napi_value IssnChecksumCharacter(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: issn payload");

    std::string payload;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], payload)) {
        return nullptr;
    }
    NAPI_ASSERT(env, payload.size() == BARCODE_ISSN_PAYLOAD_LENGTH, "ISSN payload must have 7 digits");

    int sum = 0;
    int weight = BARCODE_ISSN_WEIGHT_START;
    for (size_t i = 0; i < payload.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(payload[i], ok);
        sum += digit * weight;
        --weight;
    }

    int remainder = sum % BARCODE_MODULUS_ELEVEN;
    int check = (BARCODE_MODULUS_ELEVEN - remainder) % BARCODE_MODULUS_ELEVEN;
    std::string character;
    if (check == BARCODE_X_VALUE) {
        character.push_back(BARCODE_CHAR_X);
    } else {
        character.push_back(static_cast<char>(CHAR_DIGIT_ZERO + check));
    }
    return CreateStringValue(env, character);
}

napi_value VerhoeffChecksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateInt32Value(env, VerhoeffCheckDigitFor(digits));
}

napi_value VerhoeffValidate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() > BARCODE_MIN_CODE_LENGTH, "Code must contain payload and check digits");

    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, code.size() - BARCODE_LAST_INDEX_OFFSET);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return CreateBoolValue(env, VerhoeffCheckDigitFor(payload) == last);
}

napi_value DammChecksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateInt32Value(env, DammInterimValue(digits));
}

napi_value DammValidate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() > BARCODE_MIN_CODE_LENGTH, "Code must contain payload and check digits");

    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, code.size() - BARCODE_LAST_INDEX_OFFSET);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return CreateBoolValue(env, DammInterimValue(payload) == last);
}

napi_value DetectSymbology(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    if (code.size() == BARCODE_UPCA_LENGTH) {
        return CreateStringValue(env, "UPC-A");
    }
    if (code.size() == BARCODE_EAN8_LENGTH) {
        return CreateStringValue(env, "EAN-8");
    }
    if (code.size() == BARCODE_ISBN13_LENGTH) {
        if (HasIsbn13Prefix(code)) {
            return CreateStringValue(env, "ISBN-13");
        }
        return CreateStringValue(env, "EAN-13");
    }
    if (code.size() == BARCODE_GTIN14_LENGTH) {
        return CreateStringValue(env, "GTIN-14");
    }
    return CreateStringValue(env, "unknown");
}

napi_value Gs1Prefix(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() >= BARCODE_GS1_PREFIX_MIN_LENGTH, "Code must have at least 13 digits");

    return CreateStringValue(env, code.substr(BARCODE_EMPTY_LENGTH, BARCODE_ISBN_PREFIX_LENGTH));
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("luhnChecksum", LuhnChecksum),
        DECLARE_NAPI_FUNCTION("luhnValidate", LuhnValidate),
        DECLARE_NAPI_FUNCTION("luhnComplete", LuhnComplete),
        DECLARE_NAPI_FUNCTION("isbn10Validate", Isbn10Validate),
        DECLARE_NAPI_FUNCTION("isbn10ChecksumCharacter", Isbn10ChecksumCharacter),
        DECLARE_NAPI_FUNCTION("isbn13Validate", Isbn13Validate),
        DECLARE_NAPI_FUNCTION("isbn13ChecksumDigit", Isbn13ChecksumDigit),
        DECLARE_NAPI_FUNCTION("isbn10To13", Isbn10To13),
        DECLARE_NAPI_FUNCTION("ean13Validate", Ean13Validate),
        DECLARE_NAPI_FUNCTION("ean13ChecksumDigit", Ean13ChecksumDigit),
        DECLARE_NAPI_FUNCTION("ean8Validate", Ean8Validate),
        DECLARE_NAPI_FUNCTION("ean8ChecksumDigit", Ean8ChecksumDigit),
        DECLARE_NAPI_FUNCTION("upcAValidate", UpcAValidate),
        DECLARE_NAPI_FUNCTION("upcAChecksumDigit", UpcAChecksumDigit),
        DECLARE_NAPI_FUNCTION("itf14Validate", Itf14Validate),
        DECLARE_NAPI_FUNCTION("itf14ChecksumDigit", Itf14ChecksumDigit),
        DECLARE_NAPI_FUNCTION("code39Validate", Code39Validate),
        DECLARE_NAPI_FUNCTION("issnValidate", IssnValidate),
        DECLARE_NAPI_FUNCTION("issnChecksumCharacter", IssnChecksumCharacter),
        DECLARE_NAPI_FUNCTION("verhoeffChecksum", VerhoeffChecksum),
        DECLARE_NAPI_FUNCTION("verhoeffValidate", VerhoeffValidate),
        DECLARE_NAPI_FUNCTION("dammChecksum", DammChecksum),
        DECLARE_NAPI_FUNCTION("dammValidate", DammValidate),
        DECLARE_NAPI_FUNCTION("detectSymbology", DetectSymbology),
        DECLARE_NAPI_FUNCTION("gs1Prefix", Gs1Prefix),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterBarcodeOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module barcodeSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "barcodeSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void BarcodeSuiteRegisterModule(void)
{
    napi_module_register(&barcodeSuiteModule);
}
