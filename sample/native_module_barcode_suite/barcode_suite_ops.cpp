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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "barcode_suite_common.h"
#include "napi/native_node_api.h"

// Computes the MSI mod 10 check digit for a payload: digits are walked from the right and
// every second digit, starting with the second from the right, is doubled with carry folding.
static int MsiMod10CheckDigit(const std::string& payload)
{
    int sum = 0;
    bool doubleDigit = false;
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

// Computes the MSI mod 11 check digit for a payload: weights cycle from 2 to 7 starting at
// the rightmost digit; a result of 10 is reported as-is and cannot be encoded as one digit.
static int MsiMod11CheckDigit(const std::string& payload)
{
    int sum = 0;
    int weight = BARCODE_MSI_MOD11_WEIGHT_MIN;
    for (size_t i = 0; i < payload.size(); ++i) {
        char c = payload[payload.size() - BARCODE_LAST_INDEX_OFFSET - i];
        bool ok = false;
        int digit = CharDigitValue(c, ok);
        sum += digit * weight;
        if (weight == BARCODE_MSI_MOD11_WEIGHT_MAX) {
            weight = BARCODE_MSI_MOD11_WEIGHT_MIN;
        } else {
            weight = weight + BARCODE_EAN_WEIGHT_ONE;
        }
    }

    int remainder = sum % BARCODE_MODULUS_ELEVEN;
    return (BARCODE_MODULUS_ELEVEN - remainder) % BARCODE_MODULUS_ELEVEN;
}

// Computes the remainder of a digit string under the mod 97 scheme used by IBAN-style codes.
static int Mod97RemainderValue(const std::string& code)
{
    int remainder = 0;
    for (size_t i = 0; i < code.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(code[i], ok);
        remainder = (remainder * BARCODE_MOD97_RADIX + digit) % BARCODE_MODULUS_NINETY_SEVEN;
    }
    return remainder;
}

// Verifies a 12-digit UPC-A code against its trailing check digit.
static bool UpcAChecksumMatches(const std::string& code)
{
    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, BARCODE_UPCA_PAYLOAD_LENGTH);
    int check = ComputeGtinCheckDigit(payload, BARCODE_EAN_WEIGHT_THREE);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return check == last;
}

napi_value DigitsOnly(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }

    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (IsDigitChar(text[i])) {
            result.push_back(text[i]);
        }
    }
    return CreateStringValue(env, result);
}

napi_value CountDigits(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }

    int32_t count = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (IsDigitChar(text[i])) {
            ++count;
        }
    }
    return CreateInt32Value(env, count);
}

napi_value IsValidDigitString(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }

    std::string stripped = StripSeparators(text);
    if (stripped.size() < BARCODE_MIN_CODE_LENGTH || stripped.size() > BARCODE_MAX_CODE_LENGTH) {
        return CreateBoolValue(env, false);
    }
    for (size_t i = 0; i < stripped.size(); ++i) {
        if (!IsDigitChar(stripped[i])) {
            return CreateBoolValue(env, false);
        }
    }
    return CreateBoolValue(env, true);
}

napi_value StripSeparatorsFromCode(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }

    return CreateStringValue(env, StripSeparators(text));
}

napi_value GroupDigits(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: code, groupSizes");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    std::vector<int32_t> sizes;
    if (!ExtractInt32ArrayArg(env, argv[ARG_INDEX_ONE], sizes)) {
        return nullptr;
    }
    NAPI_ASSERT(env, sizes.size() > BARCODE_EMPTY_LENGTH, "Group sizes must not be empty");

    size_t total = 0;
    for (size_t i = 0; i < sizes.size(); ++i) {
        NAPI_ASSERT(env, sizes[i] >= BARCODE_GROUP_SIZE_MIN && sizes[i] <= BARCODE_GROUP_SIZE_MAX,
                    "Each group size must be between 1 and 13");
        total += static_cast<size_t>(sizes[i]);
    }
    NAPI_ASSERT(env, total == code.size(), "Group sizes must sum to the code length");

    std::string result;
    size_t position = 0;
    for (size_t i = 0; i < sizes.size(); ++i) {
        size_t size = static_cast<size_t>(sizes[i]);
        result += code.substr(position, size);
        if (i + BARCODE_LAST_INDEX_OFFSET < sizes.size()) {
            result.push_back(CHAR_HYPHEN);
        }
        position += size;
    }
    return CreateStringValue(env, result);
}

napi_value ReverseDigits(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    std::string result;
    result.reserve(code.size());
    for (size_t i = code.size(); i > BARCODE_EMPTY_LENGTH; --i) {
        result.push_back(code[i - BARCODE_LAST_INDEX_OFFSET]);
    }
    return CreateStringValue(env, result);
}

napi_value DigitSum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    int sum = 0;
    for (size_t i = 0; i < code.size(); ++i) {
        bool ok = false;
        sum += CharDigitValue(code[i], ok);
    }
    return CreateInt32Value(env, sum);
}

napi_value AlternatingSum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    int sum = 0;
    int sign = BARCODE_EAN_WEIGHT_ONE;
    for (size_t i = 0; i < code.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(code[i], ok);
        sum += sign * digit;
        sign = -sign;
    }
    return CreateInt32Value(env, sum);
}

napi_value WeightedSum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: code, weights");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    std::vector<int32_t> weights;
    if (!ExtractInt32ArrayArg(env, argv[ARG_INDEX_ONE], weights)) {
        return nullptr;
    }
    NAPI_ASSERT(env, weights.size() == code.size(), "Weights length must match the code length");

    int sum = 0;
    for (size_t i = 0; i < code.size(); ++i) {
        bool ok = false;
        int digit = CharDigitValue(code[i], ok);
        sum += digit * weights[i];
    }
    return CreateInt32Value(env, sum);
}

napi_value Code39Encode(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }
    NAPI_ASSERT(env, text.size() >= BARCODE_MIN_CODE_LENGTH, "Code 39 payload must not be empty");

    int sum = 0;
    std::string normalized;
    normalized.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c >= CHAR_LOWER_A && c <= CHAR_LOWER_Z) {
            c = static_cast<char>(c - CHAR_CASE_DELTA);
        }
        bool ok = false;
        int value = Code39CharValue(c, ok);
        NAPI_ASSERT(env, ok, "Code 39 contains characters outside the charset");
        sum += value;
        normalized.push_back(c);
    }

    char check = BARCODE_CODE39_CHARSET[sum % BARCODE_CODE39_MODULUS];
    std::string result;
    result.push_back(BARCODE_CODE39_STAR);
    result += normalized;
    result.push_back(check);
    result.push_back(BARCODE_CODE39_STAR);
    return CreateStringValue(env, result);
}

napi_value MsiMod10Checksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateInt32Value(env, MsiMod10CheckDigit(digits));
}

napi_value MsiMod10Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() >= BARCODE_MSI_MIN_LENGTH, "MSI code must contain payload and check digits");

    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, code.size() - BARCODE_LAST_INDEX_OFFSET);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return CreateBoolValue(env, MsiMod10CheckDigit(payload) == last);
}

napi_value MsiMod11Checksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateInt32Value(env, MsiMod11CheckDigit(digits));
}

napi_value MsiMod11Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() >= BARCODE_MSI_MIN_LENGTH, "MSI code must contain payload and check digits");

    std::string payload = code.substr(BARCODE_EMPTY_LENGTH, code.size() - BARCODE_LAST_INDEX_OFFSET);
    bool ok = false;
    int last = CharDigitValue(code[code.size() - BARCODE_LAST_INDEX_OFFSET], ok);
    return CreateBoolValue(env, MsiMod11CheckDigit(payload) == last);
}

napi_value Ean5Checksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: five digit code");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_EAN5_LENGTH, "EAN-5 add-on must have exactly 5 digits");

    // Weighted sum with factors BASE + offset * STEP counted from the rightmost digit.
    int sum = 0;
    for (size_t i = 0; i < code.size(); ++i) {
        char c = code[code.size() - BARCODE_LAST_INDEX_OFFSET - i];
        bool ok = false;
        int digit = CharDigitValue(c, ok);
        int weight = BARCODE_EAN5_WEIGHT_BASE + static_cast<int>(i) * BARCODE_EAN5_WEIGHT_STEP;
        sum += digit * weight;
    }
    return CreateInt32Value(env, sum % BARCODE_MODULUS_TEN);
}

// Appends the UPC-A digits for UPC-E case d6 <= 2: manufacturer ends with d6 0 0 and the
// item starts with two zeros.
static void AppendUpcELowCase(const std::string& code, std::string& upca)
{
    upca.push_back(code[BARCODE_UPCE_D1_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D2_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D6_INDEX]);
    for (size_t z = 0; z < BARCODE_UPCE_LOW_CASE_ZEROS; ++z) {
        upca.push_back(CHAR_DIGIT_ZERO);
    }
    upca.push_back(code[BARCODE_UPCE_D3_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D4_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D5_INDEX]);
}

// Appends the UPC-A digits for UPC-E case d6 == 3: manufacturer ends with two zeros and the
// item starts with three zeros.
static void AppendUpcEThreeCase(const std::string& code, std::string& upca)
{
    upca.push_back(code[BARCODE_UPCE_D1_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D2_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D3_INDEX]);
    for (size_t z = 0; z < BARCODE_UPCE_THREE_CASE_ZEROS; ++z) {
        upca.push_back(CHAR_DIGIT_ZERO);
    }
    upca.push_back(code[BARCODE_UPCE_D4_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D5_INDEX]);
}

// Appends the UPC-A digits for UPC-E case d6 == 4: manufacturer ends with one zero and the
// item starts with four zeros.
static void AppendUpcEFourCase(const std::string& code, std::string& upca)
{
    upca.push_back(code[BARCODE_UPCE_D1_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D2_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D3_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D4_INDEX]);
    for (size_t z = 0; z < BARCODE_UPCE_FOUR_CASE_ZEROS; ++z) {
        upca.push_back(CHAR_DIGIT_ZERO);
    }
    upca.push_back(code[BARCODE_UPCE_D5_INDEX]);
}

// Appends the UPC-A digits for UPC-E case d6 >= 5: the item starts with four zeros and d6 is
// the last item digit.
static void AppendUpcEHighCase(const std::string& code, std::string& upca)
{
    upca.push_back(code[BARCODE_UPCE_D1_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D2_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D3_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D4_INDEX]);
    upca.push_back(code[BARCODE_UPCE_D5_INDEX]);
    for (size_t z = 0; z < BARCODE_UPCE_HIGH_CASE_ZEROS; ++z) {
        upca.push_back(CHAR_DIGIT_ZERO);
    }
    upca.push_back(code[BARCODE_UPCE_D6_INDEX]);
}

napi_value UpcEExpand(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: upcE");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() == BARCODE_UPCE_LENGTH, "UPC-E must have exactly 8 digits");
    NAPI_ASSERT(env, code[BARCODE_UPCE_NS_INDEX] == BARCODE_UPCE_NUMBER_SYSTEM, "UPC-E number system must be 0");

    char d6 = code[BARCODE_UPCE_D6_INDEX];
    std::string upca;
    upca.push_back(code[BARCODE_UPCE_NS_INDEX]);
    if (d6 <= BARCODE_UPCE_CASE_TWO_LIMIT) {
        AppendUpcELowCase(code, upca);
    } else if (d6 == BARCODE_UPCE_CASE_THREE) {
        AppendUpcEThreeCase(code, upca);
    } else if (d6 == BARCODE_UPCE_CASE_FOUR) {
        AppendUpcEFourCase(code, upca);
    } else {
        AppendUpcEHighCase(code, upca);
    }
    upca.push_back(code[BARCODE_UPCE_CHECK_INDEX]);

    NAPI_ASSERT(env, UpcAChecksumMatches(upca), "UPC-E check digit does not match the expanded UPC-A");
    return CreateStringValue(env, upca);
}

// Appends the compressed UPC-E digits when the manufacturer ends with d6 0 0 and the item
// starts with two zeros; returns false when the pattern does not apply.
static bool TryAppendUpcELowCase(const std::string& code, std::string& result)
{
    char m3 = code[BARCODE_UPCA_M3_INDEX];
    char m4 = code[BARCODE_UPCA_M4_INDEX];
    char m5 = code[BARCODE_UPCA_M5_INDEX];
    if (m4 != CHAR_DIGIT_ZERO || m5 != CHAR_DIGIT_ZERO || m3 > BARCODE_UPCE_CASE_TWO_LIMIT) {
        return false;
    }
    if (code[BARCODE_UPCA_I1_INDEX] != CHAR_DIGIT_ZERO || code[BARCODE_UPCA_I2_INDEX] != CHAR_DIGIT_ZERO) {
        return false;
    }
    result.push_back(code[BARCODE_UPCA_M1_INDEX]);
    result.push_back(code[BARCODE_UPCA_M2_INDEX]);
    result.push_back(code[BARCODE_UPCA_I3_INDEX]);
    result.push_back(code[BARCODE_UPCA_I4_INDEX]);
    result.push_back(code[BARCODE_UPCA_I5_INDEX]);
    result.push_back(m3);
    return true;
}

// Appends the compressed UPC-E digits when the manufacturer ends with two zeros and the item
// starts with three zeros; returns false when the pattern does not apply.
static bool TryAppendUpcEThreeCase(const std::string& code, std::string& result)
{
    char m4 = code[BARCODE_UPCA_M4_INDEX];
    char m5 = code[BARCODE_UPCA_M5_INDEX];
    if (m4 != CHAR_DIGIT_ZERO || m5 != CHAR_DIGIT_ZERO) {
        return false;
    }
    if (code[BARCODE_UPCA_I1_INDEX] != CHAR_DIGIT_ZERO || code[BARCODE_UPCA_I2_INDEX] != CHAR_DIGIT_ZERO ||
        code[BARCODE_UPCA_I3_INDEX] != CHAR_DIGIT_ZERO) {
        return false;
    }
    result.push_back(code[BARCODE_UPCA_M1_INDEX]);
    result.push_back(code[BARCODE_UPCA_M2_INDEX]);
    result.push_back(code[BARCODE_UPCA_M3_INDEX]);
    result.push_back(code[BARCODE_UPCA_I4_INDEX]);
    result.push_back(code[BARCODE_UPCA_I5_INDEX]);
    result.push_back(BARCODE_UPCE_CASE_THREE);
    return true;
}

// Appends the compressed UPC-E digits when the manufacturer ends with one zero and the item
// starts with four zeros; returns false when the pattern does not apply.
static bool TryAppendUpcEFourCase(const std::string& code, std::string& result)
{
    if (code[BARCODE_UPCA_M5_INDEX] != CHAR_DIGIT_ZERO) {
        return false;
    }
    if (code[BARCODE_UPCA_I1_INDEX] != CHAR_DIGIT_ZERO || code[BARCODE_UPCA_I2_INDEX] != CHAR_DIGIT_ZERO ||
        code[BARCODE_UPCA_I3_INDEX] != CHAR_DIGIT_ZERO || code[BARCODE_UPCA_I4_INDEX] != CHAR_DIGIT_ZERO) {
        return false;
    }
    result.push_back(code[BARCODE_UPCA_M1_INDEX]);
    result.push_back(code[BARCODE_UPCA_M2_INDEX]);
    result.push_back(code[BARCODE_UPCA_M3_INDEX]);
    result.push_back(code[BARCODE_UPCA_M4_INDEX]);
    result.push_back(code[BARCODE_UPCA_I5_INDEX]);
    result.push_back(BARCODE_UPCE_CASE_FOUR);
    return true;
}

// Appends the compressed UPC-E digits when the item starts with four zeros and the last item
// digit selects the case; returns false when the pattern does not apply.
static bool TryAppendUpcEHighCase(const std::string& code, std::string& result)
{
    if (code[BARCODE_UPCA_I1_INDEX] != CHAR_DIGIT_ZERO || code[BARCODE_UPCA_I2_INDEX] != CHAR_DIGIT_ZERO ||
        code[BARCODE_UPCA_I3_INDEX] != CHAR_DIGIT_ZERO || code[BARCODE_UPCA_I4_INDEX] != CHAR_DIGIT_ZERO) {
        return false;
    }
    result.push_back(code[BARCODE_UPCA_M1_INDEX]);
    result.push_back(code[BARCODE_UPCA_M2_INDEX]);
    result.push_back(code[BARCODE_UPCA_M3_INDEX]);
    result.push_back(code[BARCODE_UPCA_M4_INDEX]);
    result.push_back(code[BARCODE_UPCA_M5_INDEX]);
    result.push_back(code[BARCODE_UPCA_I5_INDEX]);
    return true;
}

napi_value UpcECompress(napi_env env, napi_callback_info info)
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
    NAPI_ASSERT(env, code[BARCODE_UPCA_NS_INDEX] == BARCODE_UPCE_NUMBER_SYSTEM, "UPC-A number system must be 0");
    NAPI_ASSERT(env, UpcAChecksumMatches(code), "UPC-A checksum is invalid");

    std::string result;
    result.push_back(code[BARCODE_UPCA_NS_INDEX]);
    bool compressed = TryAppendUpcELowCase(code, result);
    if (!compressed) {
        compressed = TryAppendUpcEThreeCase(code, result);
    }
    if (!compressed) {
        compressed = TryAppendUpcEFourCase(code, result);
    }
    if (!compressed) {
        compressed = TryAppendUpcEHighCase(code, result);
    }
    NAPI_ASSERT(env, compressed, "UPC-A is not compressible to UPC-E");

    result.push_back(code[BARCODE_UPCA_CHECK_INDEX]);
    return CreateStringValue(env, result);
}

napi_value GtinFormatName(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }

    if (code.size() == BARCODE_GTIN8_LENGTH) {
        return CreateStringValue(env, "GTIN-8");
    }
    if (code.size() == BARCODE_GTIN12_LENGTH) {
        return CreateStringValue(env, "GTIN-12");
    }
    if (code.size() == BARCODE_GTIN13_LENGTH) {
        return CreateStringValue(env, "GTIN-13");
    }
    if (code.size() == BARCODE_GTIN14_LENGTH) {
        return CreateStringValue(env, "GTIN-14");
    }
    return CreateStringValue(env, "unknown");
}

napi_value GtinNormalizeTo14(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], code)) {
        return nullptr;
    }
    NAPI_ASSERT(env, code.size() <= BARCODE_GTIN14_LENGTH, "Code exceeds the GTIN-14 length");

    std::string padding(BARCODE_GTIN14_LENGTH - code.size(), CHAR_DIGIT_ZERO);
    return CreateStringValue(env, padding + code);
}

napi_value Isbn13PrefixValidate(napi_env env, napi_callback_info info)
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

    return CreateBoolValue(env, HasIsbn13Prefix(code));
}

napi_value Mod97Checksum(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateInt32Value(env, Mod97RemainderValue(digits));
}

napi_value Mod97Validate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: digits");

    std::string digits;
    if (!ExtractCodeArg(env, argv[ARG_INDEX_ZERO], digits)) {
        return nullptr;
    }

    return CreateBoolValue(env, Mod97RemainderValue(digits) == BARCODE_MOD97_VALID_REMAINDER);
}

napi_value Isbn10NormalizedUpper(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: isbn10");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }

    std::string stripped = StripSeparators(text);
    std::string result;
    result.reserve(stripped.size());
    for (size_t i = 0; i < stripped.size(); ++i) {
        char c = stripped[i];
        if (c >= CHAR_LOWER_A && c <= CHAR_LOWER_Z) {
            c = static_cast<char>(c - CHAR_CASE_DELTA);
        }
        result.push_back(c);
    }
    return CreateStringValue(env, result);
}

napi_value CountInvalidChars(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string text;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], text)) {
        return nullptr;
    }

    int32_t count = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        bool valid =
            IsDigitChar(c) || c == CHAR_HYPHEN || c == CHAR_SPACE || c == BARCODE_CHAR_X || c == BARCODE_CHAR_LOWER_X;
        if (!valid) {
            ++count;
        }
    }
    return CreateInt32Value(env, count);
}

napi_status RegisterBarcodeOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("digitsOnly", DigitsOnly),
        DECLARE_NAPI_FUNCTION("countDigits", CountDigits),
        DECLARE_NAPI_FUNCTION("isValidDigitString", IsValidDigitString),
        DECLARE_NAPI_FUNCTION("stripSeparators", StripSeparatorsFromCode),
        DECLARE_NAPI_FUNCTION("groupDigits", GroupDigits),
        DECLARE_NAPI_FUNCTION("reverseDigits", ReverseDigits),
        DECLARE_NAPI_FUNCTION("sumDigits", DigitSum),
        DECLARE_NAPI_FUNCTION("alternatingDigitSum", AlternatingSum),
        DECLARE_NAPI_FUNCTION("weightedSum", WeightedSum),
        DECLARE_NAPI_FUNCTION("code39Encode", Code39Encode),
        DECLARE_NAPI_FUNCTION("msiMod10Checksum", MsiMod10Checksum),
        DECLARE_NAPI_FUNCTION("msiMod10Validate", MsiMod10Validate),
        DECLARE_NAPI_FUNCTION("msiMod11Checksum", MsiMod11Checksum),
        DECLARE_NAPI_FUNCTION("msiMod11Validate", MsiMod11Validate),
        DECLARE_NAPI_FUNCTION("ean5Checksum", Ean5Checksum),
        DECLARE_NAPI_FUNCTION("upcEExpand", UpcEExpand),
        DECLARE_NAPI_FUNCTION("upcECompress", UpcECompress),
        DECLARE_NAPI_FUNCTION("gtinFormatName", GtinFormatName),
        DECLARE_NAPI_FUNCTION("gtinNormalizeTo14", GtinNormalizeTo14),
        DECLARE_NAPI_FUNCTION("isbn13PrefixValidate", Isbn13PrefixValidate),
        DECLARE_NAPI_FUNCTION("mod97Checksum", Mod97Checksum),
        DECLARE_NAPI_FUNCTION("mod97Validate", Mod97Validate),
        DECLARE_NAPI_FUNCTION("isbn10NormalizedUpper", Isbn10NormalizedUpper),
        DECLARE_NAPI_FUNCTION("countInvalidChars", CountInvalidChars),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
