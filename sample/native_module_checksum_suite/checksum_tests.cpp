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
constexpr size_t K_CHECKSUM_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Algorithm Cycle ---
constexpr size_t K_CHECKSUM_ALGO_CYCLE = 3;
constexpr size_t K_CHECKSUM_ALGO_INDEX_LUHN = 0;
constexpr size_t K_CHECKSUM_ALGO_INDEX_ISBN = 1;
constexpr size_t K_CHECKSUM_ALGO_INDEX_DIGIT = 2;

// --- Digit String Generation ---
constexpr size_t K_LEN_CYCLE = 4;
constexpr size_t K_MIN_LEN = 8;
constexpr size_t K_DIGIT_OFFSET_BASE = 7;
constexpr size_t K_DIGIT_STEP_BASE = 5;
constexpr size_t K_DIGIT_SEED = 3;
constexpr size_t K_DIGIT_MOD = 10;

// --- Digit Character Bounds ---
constexpr char K_CHAR_ZERO = '0';
constexpr char K_CHAR_NINE = '9';

// --- Luhn Parameters ---
constexpr int32_t K_LUHN_DOUBLE_FACTOR = 2;
constexpr int32_t K_LUHN_DOUBLE_THRESHOLD = 9;
constexpr int32_t K_LUHN_DOUBLE_SUBTRACT = 9;
constexpr int32_t K_LUHN_SUM_MOD = 10;
constexpr size_t K_LAST_INDEX_OFFSET = 1;

// --- ISBN-13 Parameters ---
constexpr int32_t K_ISBN_PARITY_MOD = 2;
constexpr int32_t K_ISBN_WEIGHT_EVEN = 1;
constexpr int32_t K_ISBN_WEIGHT_ODD = 3;
constexpr int32_t K_ISBN_SUM_MOD = 10;

// --- Digit Sum Parameters ---
constexpr int32_t K_DIGIT_SUM_MOD = 10;

// --- Check Digit Sentinel ---
constexpr int32_t K_CHECK_DIGIT_INVALID = -1;

// --- Length Classification ---
constexpr size_t K_LEN_CLASS_SHORT_MAX = 9;
constexpr size_t K_LEN_CLASS_MEDIUM_MAX = 10;
const char* const K_LEN_CLASS_SHORT = "short";
const char* const K_LEN_CLASS_MEDIUM = "medium";
const char* const K_LEN_CLASS_LONG = "long";

// --- Algorithm Labels ---
const char* const K_ALGO_LABEL_LUHN = "luhn";
const char* const K_ALGO_LABEL_ISBN = "isbn13";
const char* const K_ALGO_LABEL_DIGIT = "digit_sum";

// --- Algorithm Descriptions ---
const char* const K_ALGO_DESC_LUHN = "validate using the Luhn checksum algorithm";
const char* const K_ALGO_DESC_ISBN = "validate using the ISBN-13 checksum algorithm";
const char* const K_ALGO_DESC_DIGIT = "validate by summing digits modulo ten";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class CheckAlgo {
    LUHN,
    ISBN13,
    DIGIT_SUM,
};

struct ChecksumCaseSpec {
    std::string name;
    CheckAlgo algorithm;
    std::string input;
    bool expectedValid;
    int32_t checkDigit;
    size_t caseNumber;
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

// === Helpers: Digit Character Utilities ===

bool IsDigitChar(char c)
{
    return c >= K_CHAR_ZERO && c <= K_CHAR_NINE;
}

int32_t DigitValue(char c)
{
    return static_cast<int32_t>(c - K_CHAR_ZERO);
}

int32_t LastDigitValue(const std::string& digits)
{
    if (digits.empty()) {
        return K_CHECK_DIGIT_INVALID;
    }
    const char last = digits.back();
    if (!IsDigitChar(last)) {
        return K_CHECK_DIGIT_INVALID;
    }
    return DigitValue(last);
}

bool AllDigits(const std::string& digits)
{
    const size_t n = digits.size();
    for (size_t i = 0; i < n; i++) {
        if (!IsDigitChar(digits[i])) {
            return false;
        }
    }
    return true;
}

const char* ClassifyLength(size_t length)
{
    if (length <= K_LEN_CLASS_SHORT_MAX) {
        return K_LEN_CLASS_SHORT;
    }
    if (length <= K_LEN_CLASS_MEDIUM_MAX) {
        return K_LEN_CLASS_MEDIUM;
    }
    return K_LEN_CLASS_LONG;
}

// === Helpers: Checksum Algorithms ===

int32_t LuhnDoubled(int32_t digit)
{
    int32_t value = digit * K_LUHN_DOUBLE_FACTOR;
    if (value > K_LUHN_DOUBLE_THRESHOLD) {
        value -= K_LUHN_DOUBLE_SUBTRACT;
    }
    return value;
}

bool LuhnCheck(const std::string& digits)
{
    int32_t sum = 0;
    const size_t n = digits.size();
    bool doubleIt = false;
    for (size_t i = 0; i < n; i++) {
        const size_t pos = n - K_LAST_INDEX_OFFSET - i;
        if (!IsDigitChar(digits[pos])) {
            return false;
        }
        int32_t d = DigitValue(digits[pos]);
        if (doubleIt) {
            d = LuhnDoubled(d);
        }
        sum += d;
        doubleIt = !doubleIt;
    }
    return (sum % K_LUHN_SUM_MOD) == 0;
}

bool Isbn13Check(const std::string& digits)
{
    int32_t sum = 0;
    const size_t n = digits.size();
    for (size_t i = 0; i < n; i++) {
        if (!IsDigitChar(digits[i])) {
            return false;
        }
        const int32_t d = DigitValue(digits[i]);
        const int32_t weight = (i % K_ISBN_PARITY_MOD == 0) ? K_ISBN_WEIGHT_EVEN : K_ISBN_WEIGHT_ODD;
        sum += d * weight;
    }
    return (sum % K_ISBN_SUM_MOD) == 0;
}

bool DigitSumCheck(const std::string& digits)
{
    int32_t sum = 0;
    const size_t n = digits.size();
    for (size_t i = 0; i < n; i++) {
        if (!IsDigitChar(digits[i])) {
            return false;
        }
        sum += DigitValue(digits[i]);
    }
    return (sum % K_DIGIT_SUM_MOD) == 0;
}

// === Helpers: Digit String Generation & CaseSpec ===

std::string BuildDigitString(size_t caseNumber)
{
    const size_t length = caseNumber % K_LEN_CYCLE + K_MIN_LEN;
    std::string digits;
    digits.reserve(length);
    for (size_t i = 0; i < length; i++) {
        const size_t raw = caseNumber * K_DIGIT_OFFSET_BASE + i * K_DIGIT_STEP_BASE + K_DIGIT_SEED;
        const char digit = static_cast<char>(K_CHAR_ZERO + (raw % K_DIGIT_MOD));
        digits.push_back(digit);
    }
    return digits;
}

ChecksumCaseSpec GetChecksumCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    ChecksumCaseSpec spec;
    spec.name = BuildIndexedName("checksumCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.input = BuildDigitString(caseNumber);
    spec.checkDigit = LastDigitValue(spec.input);
    spec.expectedValid = false;
    const size_t cycle = caseNumber % K_CHECKSUM_ALGO_CYCLE;
    if (cycle == K_CHECKSUM_ALGO_INDEX_LUHN) {
        spec.algorithm = CheckAlgo::LUHN;
        spec.expectedValid = LuhnCheck(spec.input);
    } else if (cycle == K_CHECKSUM_ALGO_INDEX_ISBN) {
        spec.algorithm = CheckAlgo::ISBN13;
        spec.expectedValid = Isbn13Check(spec.input);
    } else {
        spec.algorithm = CheckAlgo::DIGIT_SUM;
        spec.expectedValid = DigitSumCheck(spec.input);
    }
    return spec;
}

const char* AlgoLabel(CheckAlgo algo)
{
    if (algo == CheckAlgo::LUHN) {
        return K_ALGO_LABEL_LUHN;
    }
    if (algo == CheckAlgo::ISBN13) {
        return K_ALGO_LABEL_ISBN;
    }
    return K_ALGO_LABEL_DIGIT;
}

const char* AlgoDescription(CheckAlgo algo)
{
    if (algo == CheckAlgo::LUHN) {
        return K_ALGO_DESC_LUHN;
    }
    if (algo == CheckAlgo::ISBN13) {
        return K_ALGO_DESC_ISBN;
    }
    return K_ALGO_DESC_DIGIT;
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

napi_value BuildChecksumResult(napi_env env, const ChecksumCaseSpec& spec, bool valid, bool matches)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "algorithm", AlgoLabel(spec.algorithm));
    SetNamedString(env, result, "description", AlgoDescription(spec.algorithm));
    SetNamedString(env, result, "input", spec.input);
    SetNamedInt32(env, result, "length", static_cast<int32_t>(spec.input.size()));
    SetNamedString(env, result, "lengthClass", ClassifyLength(spec.input.size()));
    SetNamedBool(env, result, "hasValidInput", AllDigits(spec.input));
    SetNamedBool(env, result, "valid", valid);
    SetNamedBool(env, result, "expectedValid", spec.expectedValid);
    SetNamedBool(env, result, "matches", matches);
    SetNamedInt32(env, result, "checkDigit", spec.checkDigit);
    SetNamedInt32(env, result, "caseNumber", static_cast<int32_t>(spec.caseNumber));
    return result;
}

// === Test Entry ===

static napi_value RunChecksumCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_CHECKSUM_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid checksum case");
        return nullptr;
    }
    const auto spec = GetChecksumCaseSpec(caseIndex);
    bool valid = false;
    if (spec.algorithm == CheckAlgo::LUHN) {
        valid = LuhnCheck(spec.input);
    } else if (spec.algorithm == CheckAlgo::ISBN13) {
        valid = Isbn13Check(spec.input);
    } else {
        valid = DigitSumCheck(spec.input);
    }
    const bool matches = (valid == spec.expectedValid);
    return BuildChecksumResult(env, spec, valid, matches);
}

}  // namespace

static napi_value InitChecksumSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_CHECKSUM_CASE_COUNT);
    exportNames.reserve(K_CHECKSUM_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_CHECKSUM_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testChecksumCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunChecksumCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_checksumSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitChecksumSuite,
    .nm_modname = "checksum_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterChecksumSuiteModule(void)
{
    napi_module_register(&g_checksumSuiteModule);
}
