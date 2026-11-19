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
constexpr size_t K_BIGINT_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Bigint Action Configuration ---
constexpr size_t K_BIGINT_ACTION_CYCLE = 3;
constexpr size_t K_BIGINT_PARITY_MOD = 2;
constexpr size_t K_BIGINT_SINGLE_WORD = 1;
constexpr size_t K_BIGINT_TWO_WORDS = 2;
constexpr size_t K_BIGINT_WORDS_MAX = 2;
constexpr size_t K_ACTION_INDEX_INT64 = 0;
constexpr size_t K_ACTION_INDEX_UINT64 = 1;
constexpr size_t K_ACTION_INDEX_WORDS = 2;
constexpr int K_BIGINT_SIGN_POSITIVE = 0;
constexpr int K_BIGINT_SIGN_NEGATIVE = 1;

// --- Bigint Test Values ---
constexpr int64_t K_BIGINT_INT64_SCALE = 1000000007LL;
constexpr uint64_t K_BIGINT_UINT64_SCALE = 1000000007ULL;
constexpr uint64_t K_BIGINT_WORD_BASE = 0xDEADBEEFULL;

// --- Action Labels ---
const char* const K_ACTION_LABEL_INT64 = "int64";
const char* const K_ACTION_LABEL_UINT64 = "uint64";
const char* const K_ACTION_LABEL_WORDS = "words";

// --- Action Descriptions ---
const char* const K_ACTION_DESC_INT64 = "create via napi_create_bigint_int64 and read back lossless";
const char* const K_ACTION_DESC_UINT64 = "create via napi_create_bigint_uint64 and read back lossless";
const char* const K_ACTION_DESC_WORDS = "create via napi_create_bigint_words and decompose back";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class BigintAction {
    CREATE_INT64,
    CREATE_UINT64,
    CREATE_WORDS,
};

struct BigintCaseSpec {
    std::string name;
    BigintAction action;
    size_t caseNumber;
    int64_t int64Value;
    uint64_t uint64Value;
    int signBit;
    size_t wordCount;
    uint64_t words[K_BIGINT_WORDS_MAX];
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

BigintCaseSpec GetBigintCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    BigintCaseSpec spec;
    spec.name = BuildIndexedName("bigintRoundtripCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.action = BigintAction::CREATE_INT64;
    spec.int64Value = 0;
    spec.uint64Value = 0;
    spec.signBit = K_BIGINT_SIGN_POSITIVE;
    spec.wordCount = 0;
    spec.words[0] = 0;
    spec.words[1] = 0;
    const size_t cycle = caseNumber % K_BIGINT_ACTION_CYCLE;
    if (cycle == K_ACTION_INDEX_INT64) {
        spec.action = BigintAction::CREATE_INT64;
        const int64_t sign = (caseNumber % K_BIGINT_PARITY_MOD == 0) ? 1 : -1;
        spec.int64Value = sign * static_cast<int64_t>(caseNumber) * K_BIGINT_INT64_SCALE;
        spec.signBit = (sign < 0) ? K_BIGINT_SIGN_NEGATIVE : K_BIGINT_SIGN_POSITIVE;
    } else if (cycle == K_ACTION_INDEX_UINT64) {
        spec.action = BigintAction::CREATE_UINT64;
        spec.uint64Value = static_cast<uint64_t>(caseNumber) * K_BIGINT_UINT64_SCALE;
    } else {
        spec.action = BigintAction::CREATE_WORDS;
        spec.wordCount = (caseNumber % K_BIGINT_PARITY_MOD == 0) ? K_BIGINT_SINGLE_WORD : K_BIGINT_TWO_WORDS;
        spec.words[0] = K_BIGINT_WORD_BASE * static_cast<uint64_t>(caseNumber);
        if (spec.wordCount == K_BIGINT_TWO_WORDS) {
            spec.words[1] = K_BIGINT_WORD_BASE;
        }
        spec.signBit = (caseNumber % K_BIGINT_PARITY_MOD == 0) ? K_BIGINT_SIGN_POSITIVE : K_BIGINT_SIGN_NEGATIVE;
    }
    return spec;
}

const char* ActionLabel(BigintAction action)
{
    if (action == BigintAction::CREATE_INT64) {
        return K_ACTION_LABEL_INT64;
    }
    if (action == BigintAction::CREATE_UINT64) {
        return K_ACTION_LABEL_UINT64;
    }
    return K_ACTION_LABEL_WORDS;
}

const char* ActionDescription(BigintAction action)
{
    if (action == BigintAction::CREATE_INT64) {
        return K_ACTION_DESC_INT64;
    }
    if (action == BigintAction::CREATE_UINT64) {
        return K_ACTION_DESC_UINT64;
    }
    return K_ACTION_DESC_WORDS;
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

bool SetNamedDouble(napi_env env, napi_value object, const char* name, double value)
{
    napi_value napiValue = nullptr;
    if (napi_create_double(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Helpers: Bigint Creation & Type Check ===

napi_value CreateBigintFromSpec(napi_env env, const BigintCaseSpec& spec)
{
    napi_value bigint = nullptr;
    if (spec.action == BigintAction::CREATE_INT64) {
        NAPI_CALL(env, napi_create_bigint_int64(env, spec.int64Value, &bigint));
    } else if (spec.action == BigintAction::CREATE_UINT64) {
        NAPI_CALL(env, napi_create_bigint_uint64(env, spec.uint64Value, &bigint));
    } else {
        NAPI_CALL(env, napi_create_bigint_words(env, spec.signBit, spec.wordCount, spec.words, &bigint));
    }
    return bigint;
}

bool VerifyBigintType(napi_env env, napi_value bigint)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, bigint, &type) != napi_ok) {
        return false;
    }
    return type == napi_bigint;
}

// === Helpers: Typed Round-Trip Extraction ===

bool ExtractInt64Roundtrip(napi_env env, napi_value bigint, int64_t expected, int* outSign, bool* outLossless)
{
    int64_t got = 0;
    if (napi_get_value_bigint_int64(env, bigint, &got, outLossless) != napi_ok) {
        return false;
    }
    *outSign = (got < 0) ? K_BIGINT_SIGN_NEGATIVE : K_BIGINT_SIGN_POSITIVE;
    return got == expected;
}

bool ExtractUint64Roundtrip(napi_env env, napi_value bigint, uint64_t expected, int* outSign, bool* outLossless)
{
    uint64_t got = 0;
    if (napi_get_value_bigint_uint64(env, bigint, &got, outLossless) != napi_ok) {
        return false;
    }
    *outSign = K_BIGINT_SIGN_POSITIVE;
    return got == expected;
}

bool ExtractWordsRoundtrip(napi_env env, napi_value bigint, const BigintCaseSpec& spec,
    int* outSign, size_t* outWordCount)
{
    size_t count = 0;
    if (napi_get_value_bigint_words(env, bigint, outSign, &count, nullptr) != napi_ok) {
        return false;
    }
    *outWordCount = count;
    if (count > K_BIGINT_WORDS_MAX || count != spec.wordCount) {
        return false;
    }
    uint64_t got[K_BIGINT_WORDS_MAX] = {0};
    if (napi_get_value_bigint_words(env, bigint, outSign, &count, got) != napi_ok) {
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        if (got[i] != spec.words[i]) {
            return false;
        }
    }
    return true;
}

bool ExtractBigintFromSpec(napi_env env, napi_value bigint, const BigintCaseSpec& spec,
    int* outSign, size_t* outWordCount, bool* outLossless)
{
    *outLossless = true;
    if (spec.action == BigintAction::CREATE_INT64) {
        *outWordCount = K_BIGINT_SINGLE_WORD;
        return ExtractInt64Roundtrip(env, bigint, spec.int64Value, outSign, outLossless);
    }
    if (spec.action == BigintAction::CREATE_UINT64) {
        *outWordCount = K_BIGINT_SINGLE_WORD;
        return ExtractUint64Roundtrip(env, bigint, spec.uint64Value, outSign, outLossless);
    }
    return ExtractWordsRoundtrip(env, bigint, spec, outSign, outWordCount);
}

// === Helpers: Word Decomposition For Reporting ===

bool ExtractWordsForReport(napi_env env, napi_value bigint, int* outSign, size_t* outWordCount,
    uint64_t* outWords)
{
    size_t actualCount = 0;
    if (napi_get_value_bigint_words(env, bigint, outSign, &actualCount, nullptr) != napi_ok) {
        return false;
    }
    size_t cap = (actualCount > K_BIGINT_WORDS_MAX) ? K_BIGINT_WORDS_MAX : actualCount;
    if (napi_get_value_bigint_words(env, bigint, outSign, &cap, outWords) != napi_ok) {
        return false;
    }
    *outWordCount = cap;
    return true;
}

napi_value BuildWordsArray(napi_env env, const uint64_t* words, size_t count)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &array));
    for (size_t i = 0; i < count; i++) {
        napi_value wordValue = nullptr;
        NAPI_CALL(env, napi_create_double(env, static_cast<double>(words[i]), &wordValue));
        NAPI_CALL(env, napi_set_element(env, array, static_cast<uint32_t>(i), wordValue));
    }
    return array;
}

// === Result Building ===

napi_value BuildBigintResult(napi_env env, const BigintCaseSpec& spec, bool isBigint, bool lossless,
    bool roundtrip, int reportSign, size_t reportWordCount, const uint64_t* reportWords)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "action", ActionLabel(spec.action));
    SetNamedString(env, result, "description", ActionDescription(spec.action));
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    SetNamedInt32(env, result, "expectedSign", spec.signBit);
    SetNamedInt32(env, result, "expectedWordCount", static_cast<int32_t>(spec.wordCount));
    SetNamedBool(env, result, "isBigint", isBigint);
    SetNamedBool(env, result, "lossless", lossless);
    SetNamedBool(env, result, "roundtrip", roundtrip);
    SetNamedInt32(env, result, "reportSign", reportSign);
    SetNamedInt32(env, result, "reportWordCount", static_cast<int32_t>(reportWordCount));
    napi_value wordsArray = BuildWordsArray(env, reportWords, reportWordCount);
    NAPI_CALL(env, napi_set_named_property(env, result, "words", wordsArray));
    return result;
}

// === Test Entry ===

static napi_value RunBigintRoundtripCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_BIGINT_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid bigint roundtrip case");
        return nullptr;
    }
    const auto spec = GetBigintCaseSpec(caseIndex);
    napi_value bigint = CreateBigintFromSpec(env, spec);
    if (bigint == nullptr) {
        return nullptr;
    }
    const bool isBigint = VerifyBigintType(env, bigint);
    int typedSign = K_BIGINT_SIGN_POSITIVE;
    size_t typedWordCount = 0;
    bool lossless = false;
    const bool roundtrip = ExtractBigintFromSpec(env, bigint, spec, &typedSign, &typedWordCount, &lossless);
    int reportSign = K_BIGINT_SIGN_POSITIVE;
    size_t reportWordCount = 0;
    uint64_t reportWords[K_BIGINT_WORDS_MAX] = {0};
    ExtractWordsForReport(env, bigint, &reportSign, &reportWordCount, reportWords);
    return BuildBigintResult(env, spec, isBigint, lossless, roundtrip, reportSign, reportWordCount, reportWords);
}

}  // namespace

static napi_value InitBigintRoundtripSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_BIGINT_CASE_COUNT);
    exportNames.reserve(K_BIGINT_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_BIGINT_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testBigintRoundtripCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunBigintRoundtripCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_bigintRoundtripSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitBigintRoundtripSuite,
    .nm_modname = "bigint_roundtrip_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterBigintRoundtripSuiteModule(void)
{
    napi_module_register(&g_bigintRoundtripSuiteModule);
}
