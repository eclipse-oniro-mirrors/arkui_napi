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
constexpr size_t K_UNICODE_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Unicode Op Configuration ---
constexpr size_t K_UNICODE_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_CLASSIFY = 0;
constexpr size_t K_OP_INDEX_TO_UPPER = 1;
constexpr size_t K_OP_INDEX_TO_LOWER = 2;

// --- ASCII Range Boundaries ---
constexpr uint32_t K_CP_ASCII_MIN = 0;
constexpr uint32_t K_CP_ASCII_MAX = 127;
constexpr uint32_t K_CP_CONTROL_MAX = 31;
constexpr uint32_t K_CP_PRINTABLE_MIN = 32;
constexpr uint32_t K_CP_PRINTABLE_MAX = 126;
constexpr uint32_t K_CP_DIGIT_LOW = 48;
constexpr uint32_t K_CP_DIGIT_HIGH = 57;
constexpr uint32_t K_CP_UPPER_LOW = 65;
constexpr uint32_t K_CP_UPPER_HIGH = 90;
constexpr uint32_t K_CP_LOWER_LOW = 97;
constexpr uint32_t K_CP_LOWER_HIGH = 122;
constexpr uint32_t K_CP_SPACE = 32;
constexpr uint32_t K_CP_DEL = 127;
constexpr uint32_t K_UNICODE_CASE_OFFSET = 32;
constexpr size_t K_SINGLE_CHAR_LEN = 1;

// --- Representative Codepoint Table ---
constexpr uint32_t K_CP_NUL = 0;
constexpr uint32_t K_CP_BEL = 7;
constexpr uint32_t K_CP_BS = 8;
constexpr uint32_t K_CP_LF = 10;
constexpr uint32_t K_CP_CR = 13;
constexpr uint32_t K_CP_ESC = 27;
constexpr uint32_t K_CP_EXCLAMATION = 33;
constexpr uint32_t K_CP_COMMA = 44;
constexpr uint32_t K_CP_COLON = 58;
constexpr uint32_t K_CP_AT = 64;
constexpr uint32_t K_CP_UNDERSCORE = 95;
constexpr uint32_t K_CP_TILDE = 126;
constexpr uint32_t K_CP_DIGIT_ZERO = 48;
constexpr uint32_t K_CP_DIGIT_FIVE = 53;
constexpr uint32_t K_CP_DIGIT_NINE = 57;
constexpr uint32_t K_CP_UPPER_A = 65;
constexpr uint32_t K_CP_UPPER_M = 77;
constexpr uint32_t K_CP_UPPER_Z = 90;
constexpr uint32_t K_CP_LOWER_A = 97;
constexpr uint32_t K_CP_LOWER_M = 109;
constexpr uint32_t K_CP_LOWER_Z = 122;
constexpr size_t K_UNICODE_TABLE_SIZE = 24;
constexpr uint32_t K_CP_TABLE[K_UNICODE_TABLE_SIZE] = {
    K_CP_NUL, K_CP_BEL, K_CP_BS, K_CP_LF, K_CP_CR, K_CP_ESC,
    K_CP_SPACE, K_CP_EXCLAMATION, K_CP_COMMA, K_CP_COLON, K_CP_AT, K_CP_UNDERSCORE,
    K_CP_TILDE, K_CP_DIGIT_ZERO, K_CP_DIGIT_FIVE, K_CP_DIGIT_NINE,
    K_CP_UPPER_A, K_CP_UPPER_M, K_CP_UPPER_Z,
    K_CP_LOWER_A, K_CP_LOWER_M, K_CP_LOWER_Z,
    K_CP_DEL, K_CP_DEL,
};

// --- Category Labels ---
const char* const K_CATEGORY_LETTER = "letter";
const char* const K_CATEGORY_DIGIT = "digit";
const char* const K_CATEGORY_SPACE = "space";
const char* const K_CATEGORY_PUNCT = "punct";
const char* const K_CATEGORY_CONTROL = "control";
const char* const K_CATEGORY_OTHER = "other";

// --- Non-Printable Placeholder ---
const char* const K_NON_PRINTABLE_PLACEHOLDER = "<non-printable>";

// --- Op Labels ---
const char* const K_OP_LABEL_CLASSIFY = "classify";
const char* const K_OP_LABEL_TO_UPPER = "to_upper";
const char* const K_OP_LABEL_TO_LOWER = "to_lower";

// --- Op Descriptions ---
const char* const K_OP_DESC_CLASSIFY = "classify codepoint into ASCII category letter/digit/space/punct/control";
const char* const K_OP_DESC_TO_UPPER = "map lowercase letter to uppercase, non-letters unchanged";
const char* const K_OP_DESC_TO_LOWER = "map uppercase letter to lowercase, non-letters unchanged";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class UnicodeOp {
    CLASSIFY,
    TO_UPPER,
    TO_LOWER,
};

struct UnicodeCaseSpec {
    std::string name;
    UnicodeOp op;
    size_t caseNumber;
    uint32_t codepoint;
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

UnicodeCaseSpec GetUnicodeCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    UnicodeCaseSpec spec;
    spec.name = BuildIndexedName("unicodeCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.op = UnicodeOp::CLASSIFY;
    spec.codepoint = (caseIndex < K_UNICODE_TABLE_SIZE) ? K_CP_TABLE[caseIndex] : K_CP_NUL;
    const size_t cycle = caseNumber % K_UNICODE_OP_CYCLE;
    if (cycle == K_OP_INDEX_CLASSIFY) {
        spec.op = UnicodeOp::CLASSIFY;
    } else if (cycle == K_OP_INDEX_TO_UPPER) {
        spec.op = UnicodeOp::TO_UPPER;
    } else {
        spec.op = UnicodeOp::TO_LOWER;
    }
    return spec;
}

const char* OpLabel(UnicodeOp op)
{
    if (op == UnicodeOp::CLASSIFY) {
        return K_OP_LABEL_CLASSIFY;
    }
    if (op == UnicodeOp::TO_UPPER) {
        return K_OP_LABEL_TO_UPPER;
    }
    return K_OP_LABEL_TO_LOWER;
}

const char* OpDescription(UnicodeOp op)
{
    if (op == UnicodeOp::CLASSIFY) {
        return K_OP_DESC_CLASSIFY;
    }
    if (op == UnicodeOp::TO_UPPER) {
        return K_OP_DESC_TO_UPPER;
    }
    return K_OP_DESC_TO_LOWER;
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

// === Helpers: Codepoint Classification ===

bool IsAscii(uint32_t cp)
{
    return cp >= K_CP_ASCII_MIN && cp <= K_CP_ASCII_MAX;
}

bool IsUpperLetter(uint32_t cp)
{
    return cp >= K_CP_UPPER_LOW && cp <= K_CP_UPPER_HIGH;
}

bool IsLowerLetter(uint32_t cp)
{
    return cp >= K_CP_LOWER_LOW && cp <= K_CP_LOWER_HIGH;
}

bool IsLetter(uint32_t cp)
{
    return IsUpperLetter(cp) || IsLowerLetter(cp);
}

bool IsDigit(uint32_t cp)
{
    return cp >= K_CP_DIGIT_LOW && cp <= K_CP_DIGIT_HIGH;
}

bool IsSpace(uint32_t cp)
{
    return cp == K_CP_SPACE;
}

bool IsControl(uint32_t cp)
{
    return cp <= K_CP_CONTROL_MAX || cp == K_CP_DEL;
}

bool IsPunct(uint32_t cp)
{
    if (!IsAscii(cp) || IsLetter(cp) || IsDigit(cp) || IsSpace(cp) || IsControl(cp)) {
        return false;
    }
    return true;
}

bool IsPrintable(uint32_t cp)
{
    return cp >= K_CP_PRINTABLE_MIN && cp <= K_CP_PRINTABLE_MAX;
}

bool IsAlphaNumeric(uint32_t cp)
{
    return IsLetter(cp) || IsDigit(cp);
}

const char* ClassifyCodepoint(uint32_t cp)
{
    if (IsLetter(cp)) {
        return K_CATEGORY_LETTER;
    }
    if (IsDigit(cp)) {
        return K_CATEGORY_DIGIT;
    }
    if (IsSpace(cp)) {
        return K_CATEGORY_SPACE;
    }
    if (IsControl(cp)) {
        return K_CATEGORY_CONTROL;
    }
    if (IsAscii(cp)) {
        return K_CATEGORY_PUNCT;
    }
    return K_CATEGORY_OTHER;
}

// === Helpers: Case Mapping ===

uint32_t ToUpperCodepoint(uint32_t cp)
{
    if (cp >= K_CP_LOWER_LOW && cp <= K_CP_LOWER_HIGH) {
        return cp - K_UNICODE_CASE_OFFSET;
    }
    return cp;
}

uint32_t ToLowerCodepoint(uint32_t cp)
{
    if (cp >= K_CP_UPPER_LOW && cp <= K_CP_UPPER_HIGH) {
        return cp + K_UNICODE_CASE_OFFSET;
    }
    return cp;
}

// === Helpers: Codepoint String Rendering ===

std::string CodepointToString(uint32_t cp)
{
    if (!IsPrintable(cp)) {
        return std::string(K_NON_PRINTABLE_PLACEHOLDER);
    }
    return std::string(K_SINGLE_CHAR_LEN, static_cast<char>(cp));
}

// === Result Building ===

napi_value BuildUnicodeResult(napi_env env, const UnicodeCaseSpec& spec, uint32_t mappedCodepoint,
    const char* category, bool isLetter, bool isDigit, bool isSpace, bool isControl, bool isPunct,
    bool isUpper, bool isLower, bool isAscii, bool isAlphaNumeric, bool isPrintable)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    SetNamedInt32(env, result, "codepoint", static_cast<int32_t>(spec.codepoint));
    SetNamedString(env, result, "category", category);
    SetNamedInt32(env, result, "mappedCodepoint", static_cast<int32_t>(mappedCodepoint));
    SetNamedBool(env, result, "isLetter", isLetter);
    SetNamedBool(env, result, "isDigit", isDigit);
    SetNamedBool(env, result, "isSpace", isSpace);
    SetNamedBool(env, result, "isControl", isControl);
    SetNamedBool(env, result, "isPunct", isPunct);
    SetNamedBool(env, result, "isUpper", isUpper);
    SetNamedBool(env, result, "isLower", isLower);
    SetNamedBool(env, result, "isAscii", isAscii);
    SetNamedBool(env, result, "isAlphaNumeric", isAlphaNumeric);
    SetNamedBool(env, result, "isPrintable", isPrintable);
    SetNamedString(env, result, "character", CodepointToString(spec.codepoint));
    SetNamedString(env, result, "mappedCharacter", CodepointToString(mappedCodepoint));
    return result;
}

// === Test Entry ===

static napi_value RunUnicodeCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_UNICODE_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid unicode case");
        return nullptr;
    }
    const auto spec = GetUnicodeCaseSpec(caseIndex);
    const uint32_t cp = spec.codepoint;
    uint32_t mappedCodepoint = cp;
    if (spec.op == UnicodeOp::TO_UPPER) {
        mappedCodepoint = ToUpperCodepoint(cp);
    } else if (spec.op == UnicodeOp::TO_LOWER) {
        mappedCodepoint = ToLowerCodepoint(cp);
    }
    const char* category = ClassifyCodepoint(cp);
    return BuildUnicodeResult(env, spec, mappedCodepoint, category, IsLetter(cp), IsDigit(cp),
        IsSpace(cp), IsControl(cp), IsPunct(cp), IsUpperLetter(cp), IsLowerLetter(cp), IsAscii(cp),
        IsAlphaNumeric(cp), IsPrintable(cp));
}

}  // namespace

static napi_value InitUnicodeSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_UNICODE_CASE_COUNT);
    exportNames.reserve(K_UNICODE_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_UNICODE_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testUnicodeCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunUnicodeCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_unicodeSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitUnicodeSuite,
    .nm_modname = "unicode_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterUnicodeSuiteModule(void)
{
    napi_module_register(&g_unicodeSuiteModule);
}
