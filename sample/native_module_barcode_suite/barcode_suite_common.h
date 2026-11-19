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

#ifndef SAMPLE_NATIVE_MODULE_BARCODE_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_BARCODE_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"

// Number of JS arguments expected by each function and fixed argument indices.
constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

// Input limits. Code arguments are validated on the stripped form: '-' and ' ' separators are
// removed first, then the remaining characters must be 1 to 32 digits (plus 'X' where allowed).
constexpr size_t BARCODE_MIN_CODE_LENGTH = 1;
constexpr size_t BARCODE_MAX_CODE_LENGTH = 32;
constexpr size_t BARCODE_MAX_INPUT_LENGTH = 64;
constexpr size_t BARCODE_MAX_ARRAY_LENGTH = 32;
constexpr size_t BARCODE_EMPTY_LENGTH = 0;
constexpr size_t BARCODE_STRING_BUFFER_PADDING = 1;
constexpr size_t BARCODE_LAST_INDEX_OFFSET = 1;

// Character constants.
constexpr char CHAR_DIGIT_ZERO = '0';
constexpr char CHAR_DIGIT_NINE = '9';
constexpr char CHAR_HYPHEN = '-';
constexpr char CHAR_SPACE = ' ';
constexpr char CHAR_LOWER_A = 'a';
constexpr char CHAR_LOWER_Z = 'z';
constexpr char CHAR_CASE_DELTA = 32;
constexpr char BARCODE_CHAR_X = 'X';
constexpr char BARCODE_CHAR_LOWER_X = 'x';
constexpr char BARCODE_CODE39_STAR = '*';

// Moduli and algorithm weights.
constexpr int BARCODE_MODULUS_TEN = 10;
constexpr int BARCODE_MODULUS_ELEVEN = 11;
constexpr int BARCODE_MODULUS_NINETY_SEVEN = 97;
constexpr int BARCODE_LUHN_WEIGHT_2 = 2;
constexpr int BARCODE_LUHN_NINE_CARRY = 9;
constexpr int BARCODE_EAN_WEIGHT_ONE = 1;
constexpr int BARCODE_EAN_WEIGHT_THREE = 3;
constexpr int BARCODE_X_VALUE = 10;
constexpr int BARCODE_ISBN10_WEIGHT_START = 10;
constexpr int BARCODE_ISSN_WEIGHT_START = 8;
constexpr int BARCODE_EAN5_WEIGHT_BASE = 5;
constexpr int BARCODE_EAN5_WEIGHT_STEP = 3;
constexpr int BARCODE_MSI_MOD11_WEIGHT_MIN = 2;
constexpr int BARCODE_MSI_MOD11_WEIGHT_MAX = 7;
constexpr int BARCODE_MOD97_RADIX = 10;
constexpr int BARCODE_MOD97_VALID_REMAINDER = 1;
constexpr int BARCODE_CODE39_MODULUS = 43;
constexpr int BARCODE_GROUP_SIZE_MIN = 1;
constexpr int BARCODE_GROUP_SIZE_MAX = 13;

// Symbology lengths for fixed-position codes (checked on the stripped form).
constexpr size_t BARCODE_ISBN10_LENGTH = 10;
constexpr size_t BARCODE_ISBN10_PAYLOAD_LENGTH = 9;
constexpr size_t BARCODE_ISBN13_LENGTH = 13;
constexpr size_t BARCODE_ISBN13_PAYLOAD_LENGTH = 12;
constexpr size_t BARCODE_EAN13_LENGTH = 13;
constexpr size_t BARCODE_EAN13_PAYLOAD_LENGTH = 12;
constexpr size_t BARCODE_EAN8_LENGTH = 8;
constexpr size_t BARCODE_EAN8_PAYLOAD_LENGTH = 7;
constexpr size_t BARCODE_UPCA_LENGTH = 12;
constexpr size_t BARCODE_UPCA_PAYLOAD_LENGTH = 11;
constexpr size_t BARCODE_UPCE_LENGTH = 8;
constexpr size_t BARCODE_UPCE_PAYLOAD_LENGTH = 7;
constexpr size_t BARCODE_UPCE_COMPRESSED_DIGITS = 6;
constexpr size_t BARCODE_ITF14_LENGTH = 14;
constexpr size_t BARCODE_ITF14_PAYLOAD_LENGTH = 13;
constexpr size_t BARCODE_ISSN_LENGTH = 8;
constexpr size_t BARCODE_ISSN_PAYLOAD_LENGTH = 7;
constexpr size_t BARCODE_EAN5_LENGTH = 5;
constexpr size_t BARCODE_GTIN8_LENGTH = 8;
constexpr size_t BARCODE_GTIN12_LENGTH = 12;
constexpr size_t BARCODE_GTIN13_LENGTH = 13;
constexpr size_t BARCODE_GTIN14_LENGTH = 14;
constexpr size_t BARCODE_GS1_PREFIX_MIN_LENGTH = 13;
constexpr size_t BARCODE_ISBN_PREFIX_LENGTH = 3;
constexpr size_t BARCODE_MSI_MIN_LENGTH = 2;
constexpr size_t BARCODE_CODE39_MIN_LENGTH = 2;

// UPC-E digit layout inside an 8-digit code: [0]=number system, [1..6]=d1..d6, [7]=check.
constexpr size_t BARCODE_UPCE_NS_INDEX = 0;
constexpr size_t BARCODE_UPCE_D1_INDEX = 1;
constexpr size_t BARCODE_UPCE_D2_INDEX = 2;
constexpr size_t BARCODE_UPCE_D3_INDEX = 3;
constexpr size_t BARCODE_UPCE_D4_INDEX = 4;
constexpr size_t BARCODE_UPCE_D5_INDEX = 5;
constexpr size_t BARCODE_UPCE_D6_INDEX = 6;
constexpr size_t BARCODE_UPCE_CHECK_INDEX = 7;

// UPC-A digit layout inside a 12-digit code: [0]=number system, [1..5]=M1..M5 (manufacturer),
// [6..10]=I1..I5 (item), [11]=check.
constexpr size_t BARCODE_UPCA_NS_INDEX = 0;
constexpr size_t BARCODE_UPCA_M1_INDEX = 1;
constexpr size_t BARCODE_UPCA_M2_INDEX = 2;
constexpr size_t BARCODE_UPCA_M3_INDEX = 3;
constexpr size_t BARCODE_UPCA_M4_INDEX = 4;
constexpr size_t BARCODE_UPCA_M5_INDEX = 5;
constexpr size_t BARCODE_UPCA_I1_INDEX = 6;
constexpr size_t BARCODE_UPCA_I2_INDEX = 7;
constexpr size_t BARCODE_UPCA_I3_INDEX = 8;
constexpr size_t BARCODE_UPCA_I4_INDEX = 9;
constexpr size_t BARCODE_UPCA_I5_INDEX = 10;
constexpr size_t BARCODE_UPCA_CHECK_INDEX = 11;

// UPC-E zero-suppression: the last compressed digit d6 selects the expansion case. Cases d6 <= 2
// and d6 >= 5 suppress four zeros; cases d6 == 3 and d6 == 4 suppress five zeros.
constexpr char BARCODE_UPCE_NUMBER_SYSTEM = '0';
constexpr char BARCODE_UPCE_CASE_TWO_LIMIT = '2';
constexpr char BARCODE_UPCE_CASE_THREE = '3';
constexpr char BARCODE_UPCE_CASE_FOUR = '4';
constexpr char BARCODE_UPCE_ITEM_MIN_FIVE = '5';
constexpr size_t BARCODE_UPCE_LOW_CASE_ZEROS = 4;
constexpr size_t BARCODE_UPCE_THREE_CASE_ZEROS = 5;
constexpr size_t BARCODE_UPCE_FOUR_CASE_ZEROS = 5;
constexpr size_t BARCODE_UPCE_HIGH_CASE_ZEROS = 4;

// ISBN-13 registration-group prefixes.
constexpr char BARCODE_ISBN_PREFIX_978[] = "978";
constexpr char BARCODE_ISBN_PREFIX_979[] = "979";

// Code 39 character set: the value of a character is its index in this table, modulus 43.
constexpr char BARCODE_CODE39_CHARSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";

// Lookup table dimensions.
constexpr size_t BARCODE_TABLE_DIM = 10;
constexpr size_t BARCODE_VERHOF_PERMUTATION_COUNT = 8;

// Damm quasigroup operation table (weakly totally anti-symmetric, main diagonal is zero).
// Digits are processed left to right: interim = table[interim][digit].
constexpr int BARCODE_DAMM_TABLE[BARCODE_TABLE_DIM][BARCODE_TABLE_DIM] = {
    { 0, 3, 1, 7, 5, 9, 8, 6, 4, 2 }, { 7, 0, 9, 2, 1, 5, 4, 8, 6, 3 }, { 4, 2, 0, 6, 8, 7, 1, 3, 5, 9 },
    { 1, 7, 5, 0, 9, 8, 3, 4, 2, 6 }, { 6, 1, 2, 3, 0, 4, 5, 9, 7, 8 }, { 3, 6, 7, 4, 2, 0, 9, 5, 8, 1 },
    { 5, 8, 6, 9, 7, 2, 0, 1, 3, 4 }, { 8, 9, 4, 5, 3, 6, 2, 0, 1, 7 }, { 9, 4, 3, 8, 6, 1, 7, 2, 0, 5 },
    { 2, 5, 8, 1, 4, 3, 6, 7, 9, 0 },
};

// Verhoeff multiplication table of the dihedral group D5 (note: not commutative).
constexpr int BARCODE_VERHOF_D_TABLE[BARCODE_TABLE_DIM][BARCODE_TABLE_DIM] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, { 1, 2, 3, 4, 0, 6, 7, 8, 9, 5 }, { 2, 3, 4, 0, 1, 7, 8, 9, 5, 6 },
    { 3, 4, 0, 1, 2, 8, 9, 5, 6, 7 }, { 4, 0, 1, 2, 3, 9, 5, 6, 7, 8 }, { 5, 9, 8, 7, 6, 0, 4, 3, 2, 1 },
    { 6, 5, 9, 8, 7, 1, 0, 4, 3, 2 }, { 7, 6, 5, 9, 8, 2, 1, 0, 4, 3 }, { 8, 7, 6, 5, 9, 3, 2, 1, 0, 4 },
    { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
};

// Verhoeff permutation table, selected by the digit position counted from the right modulo 8.
constexpr int BARCODE_VERHOF_P_TABLE[BARCODE_VERHOF_PERMUTATION_COUNT][BARCODE_TABLE_DIM] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, { 1, 5, 7, 6, 2, 8, 3, 0, 9, 4 }, { 5, 8, 0, 3, 7, 9, 6, 1, 4, 2 },
    { 8, 9, 1, 6, 0, 4, 3, 5, 2, 7 }, { 9, 4, 5, 3, 1, 2, 6, 8, 7, 0 }, { 4, 2, 8, 6, 5, 7, 3, 9, 0, 1 },
    { 2, 7, 9, 3, 8, 0, 6, 4, 1, 5 }, { 7, 0, 4, 6, 9, 1, 3, 2, 5, 8 },
};

// Verhoeff inverse table: inv[j] satisfies d(j, inv[j]) == 0.
constexpr int BARCODE_VERHOF_INV_TABLE[BARCODE_TABLE_DIM] = { 0, 4, 3, 2, 1, 5, 6, 7, 8, 9 };

inline bool IsDigitChar(char c)
{
    return c >= CHAR_DIGIT_ZERO && c <= CHAR_DIGIT_NINE;
}

// Extracts a string argument of at most BARCODE_MAX_INPUT_LENGTH characters.
bool ExtractStringArg(napi_env env, napi_value value, std::string& result);

// Extracts an array of int32 numbers of at most BARCODE_MAX_ARRAY_LENGTH elements.
bool ExtractInt32ArrayArg(napi_env env, napi_value value, std::vector<int32_t>& result);

// Extracts a code argument: strips '-' and ' ' separators, then requires 1 to 32 digits.
// All functions are strict and validate the stripped form.
bool ExtractCodeArg(napi_env env, napi_value value, std::string& result);

// Variant of ExtractCodeArg that additionally accepts 'X' at the last position only (ISBN-10 and
// ISSN check characters); lowercase 'x' is rejected.
bool ExtractCodeAllowXArg(napi_env env, napi_value value, std::string& result);

napi_value CreateStringValue(napi_env env, const std::string& value);
napi_value CreateInt32Value(napi_env env, int32_t value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateStringArrayValue(napi_env env, const std::vector<std::string>& values);

// Removes '-' and ' ' separators from the given code.
std::string StripSeparators(const std::string& code);

// Converts '0'-'9' to 0-9 and 'X' to 10; sets ok to false for any other character.
int CharDigitValue(char c, bool& ok);

// Returns the sum of the decimal digits of a doubled digit product (doubled digit is at most 18).
int DigitSumOfProduct(int product);

// Computes the GTIN-style check digit for a payload using alternating 1/3 weights that start with
// startWeight at the leftmost digit; the check digit satisfies total sum mod 10 == 0.
int ComputeGtinCheckDigit(const std::string& payload, int startWeight);

// Returns true when a 13-digit code starts with the 978 or 979 ISBN prefix.
bool HasIsbn13Prefix(const std::string& code);

// Returns the Code 39 value (0-42) of a character; ok is false outside the Code 39 charset.
int Code39CharValue(char c, bool& ok);

// Check-digit algorithms (Group 1, registered by napi_barcode_suite.cpp).
napi_value LuhnChecksum(napi_env env, napi_callback_info info);
napi_value LuhnValidate(napi_env env, napi_callback_info info);
napi_value LuhnComplete(napi_env env, napi_callback_info info);
napi_value Isbn10Validate(napi_env env, napi_callback_info info);
napi_value Isbn10ChecksumCharacter(napi_env env, napi_callback_info info);
napi_value Isbn13Validate(napi_env env, napi_callback_info info);
napi_value Isbn13ChecksumDigit(napi_env env, napi_callback_info info);
napi_value Isbn10To13(napi_env env, napi_callback_info info);
napi_value Ean13Validate(napi_env env, napi_callback_info info);
napi_value Ean13ChecksumDigit(napi_env env, napi_callback_info info);
napi_value Ean8Validate(napi_env env, napi_callback_info info);
napi_value Ean8ChecksumDigit(napi_env env, napi_callback_info info);
napi_value UpcAValidate(napi_env env, napi_callback_info info);
napi_value UpcAChecksumDigit(napi_env env, napi_callback_info info);
napi_value Itf14Validate(napi_env env, napi_callback_info info);
napi_value Itf14ChecksumDigit(napi_env env, napi_callback_info info);
napi_value Code39Validate(napi_env env, napi_callback_info info);
napi_value IssnValidate(napi_env env, napi_callback_info info);
napi_value IssnChecksumCharacter(napi_env env, napi_callback_info info);
napi_value VerhoeffChecksum(napi_env env, napi_callback_info info);
napi_value VerhoeffValidate(napi_env env, napi_callback_info info);
napi_value DammChecksum(napi_env env, napi_callback_info info);
napi_value DammValidate(napi_env env, napi_callback_info info);
napi_value DetectSymbology(napi_env env, napi_callback_info info);
napi_value Gs1Prefix(napi_env env, napi_callback_info info);

// Code manipulation and extra checksums (Group 2, registered by barcode_suite_ops.cpp).
napi_value DigitsOnly(napi_env env, napi_callback_info info);
napi_value CountDigits(napi_env env, napi_callback_info info);
napi_value IsValidDigitString(napi_env env, napi_callback_info info);
napi_value StripSeparatorsFromCode(napi_env env, napi_callback_info info);
napi_value GroupDigits(napi_env env, napi_callback_info info);
napi_value ReverseDigits(napi_env env, napi_callback_info info);
napi_value DigitSum(napi_env env, napi_callback_info info);
napi_value AlternatingSum(napi_env env, napi_callback_info info);
napi_value WeightedSum(napi_env env, napi_callback_info info);
napi_value Code39Encode(napi_env env, napi_callback_info info);
napi_value MsiMod10Checksum(napi_env env, napi_callback_info info);
napi_value MsiMod10Validate(napi_env env, napi_callback_info info);
napi_value MsiMod11Checksum(napi_env env, napi_callback_info info);
napi_value MsiMod11Validate(napi_env env, napi_callback_info info);
napi_value Ean5Checksum(napi_env env, napi_callback_info info);
napi_value UpcEExpand(napi_env env, napi_callback_info info);
napi_value UpcECompress(napi_env env, napi_callback_info info);
napi_value GtinFormatName(napi_env env, napi_callback_info info);
napi_value GtinNormalizeTo14(napi_env env, napi_callback_info info);
napi_value Isbn13PrefixValidate(napi_env env, napi_callback_info info);
napi_value Mod97Checksum(napi_env env, napi_callback_info info);
napi_value Mod97Validate(napi_env env, napi_callback_info info);
napi_value Isbn10NormalizedUpper(napi_env env, napi_callback_info info);
napi_value CountInvalidChars(napi_env env, napi_callback_info info);

napi_status RegisterBarcodeOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_BARCODE_SUITE_COMMON_H
