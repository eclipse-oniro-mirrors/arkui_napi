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

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// --- Case Count ---
constexpr size_t K_BIGINT_CASE_COUNT = 20;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Sign Bits ---
constexpr int K_SIGN_BIT_POSITIVE = 0;
constexpr int K_SIGN_BIT_NEGATIVE = 1;

// --- Word Counts ---
constexpr size_t K_WORD_COUNT_ZERO = 0;
constexpr size_t K_WORD_COUNT_ONE = 1;
constexpr size_t K_WORD_COUNT_TWO = 2;
constexpr size_t K_BIGINT_MAX_WORDS = 2;

// --- Test Values (Int64) ---
constexpr int64_t K_INT64_ZERO = 0LL;
constexpr int64_t K_INT64_ONE = 1LL;
constexpr int64_t K_INT64_NEG_ONE = -1LL;
constexpr int64_t K_INT64_LARGE = 9876543210LL;
constexpr int64_t K_INT64_NEG_LARGE = -9876543210LL;
constexpr int64_t K_INT64_MAX = 0x7FFFFFFFFFFFFFFFLL;
constexpr int64_t K_INT64_MIN = (-0x7FFFFFFFFFFFFFFFLL - 1);

// --- Test Values (Uint64) ---
constexpr uint64_t K_UINT64_ZERO = 0ULL;
constexpr uint64_t K_UINT64_ONE = 1ULL;
constexpr uint64_t K_UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL;
// Value strictly greater than INT64_MAX to exercise the lossy int64 path.
constexpr uint64_t K_UINT64_OVER_INT64 = 0x8000000000000000ULL;

// --- Test Values (BigInt Words) ---
constexpr uint64_t K_WORD_VALUE_SMALL = 12345ULL;
constexpr uint64_t K_WORD_VALUE_LOW = 0x123456789ABCDEF0ULL;
constexpr uint64_t K_WORD_VALUE_HIGH = 0xFEDCBA9876543210ULL;

// === Operation Enum ===
enum class BigintOp : int32_t {
    CREATE_INT64,
    CREATE_UINT64,
    GET_INT64,
    GET_INT64_LOSSY,
    GET_UINT64,
    GET_UINT64_LOSSY,
    CREATE_WORDS,
    GET_WORDS,
    GET_WORDS_COUNT,
};

// === Spec Structures ===
struct BigintCaseSpec {
    std::string name;
    BigintOp op;
    int64_t int64Value;
    uint64_t uint64Value;
    int signBit;
    size_t wordCount;
    uint64_t words[K_BIGINT_MAX_WORDS];
    bool expectedLossless;
};

struct BigintResult {
    bool passed;
    int64_t value;
    bool lossless;
    int32_t signBit;
    int32_t wordCount;
};

// === Helper Functions - Name Building & Case Index ===
std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data) { return static_cast<size_t>(reinterpret_cast<uintptr_t>(data)); }

// === Helper Functions - Case Specifications ===
const BigintCaseSpec& GetBigintCaseSpec(size_t caseIndex)
{
    static const BigintCaseSpec specs[K_BIGINT_CASE_COUNT] = {
        { "bigintCase01", BigintOp::CREATE_INT64, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase02", BigintOp::CREATE_INT64, K_INT64_LARGE, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase03", BigintOp::CREATE_INT64, K_INT64_NEG_LARGE, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase04", BigintOp::CREATE_INT64, K_INT64_MAX, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase05", BigintOp::CREATE_INT64, K_INT64_MIN, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase06", BigintOp::CREATE_UINT64, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase07", BigintOp::CREATE_UINT64, K_INT64_ZERO, K_UINT64_MAX,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase08", BigintOp::CREATE_UINT64, K_INT64_ZERO, K_UINT64_OVER_INT64,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase09", BigintOp::GET_INT64, K_INT64_ONE, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase10", BigintOp::GET_INT64, K_INT64_MAX, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase11", BigintOp::GET_INT64_LOSSY, K_INT64_ZERO, K_UINT64_OVER_INT64,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, false },
        { "bigintCase12", BigintOp::GET_UINT64, K_INT64_ZERO, K_UINT64_ONE,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase13", BigintOp::GET_UINT64, K_INT64_ZERO, K_UINT64_MAX,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, true },
        { "bigintCase14", BigintOp::GET_UINT64_LOSSY, K_INT64_NEG_ONE, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO, { K_UINT64_ZERO, K_UINT64_ZERO }, false },
        { "bigintCase15", BigintOp::CREATE_WORDS, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ONE, { K_WORD_VALUE_SMALL, K_UINT64_ZERO }, true },
        { "bigintCase16", BigintOp::CREATE_WORDS, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_NEGATIVE, K_WORD_COUNT_ONE, { K_WORD_VALUE_SMALL, K_UINT64_ZERO }, true },
        { "bigintCase17", BigintOp::CREATE_WORDS, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_TWO, { K_WORD_VALUE_LOW, K_WORD_VALUE_HIGH }, true },
        { "bigintCase18", BigintOp::GET_WORDS, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ONE, { K_WORD_VALUE_SMALL, K_UINT64_ZERO }, true },
        { "bigintCase19", BigintOp::GET_WORDS, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_TWO, { K_WORD_VALUE_LOW, K_WORD_VALUE_HIGH }, true },
        { "bigintCase20", BigintOp::GET_WORDS_COUNT, K_INT64_ZERO, K_UINT64_ZERO,
            K_SIGN_BIT_POSITIVE, K_WORD_COUNT_TWO, { K_WORD_VALUE_LOW, K_WORD_VALUE_HIGH }, true },
    };
    return specs[caseIndex];
}

// === Helper Functions - Object Property ===
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

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedInt64(napi_env env, napi_value object, const char* name, int64_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_bigint_int64(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Helper Functions - Words Comparison ===
bool WordsMatch(const uint64_t* expected, const uint64_t* actual, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (expected[i] != actual[i]) {
            return false;
        }
    }
    return true;
}

// === Helper Functions - Result Object ===
napi_value MakeBigintResult(napi_env env, const BigintCaseSpec& spec, const BigintResult& res)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedBool(env, result, "passed", res.passed);
    SetNamedInt64(env, result, "value", res.value);
    SetNamedBool(env, result, "lossless", res.lossless);
    SetNamedInt32(env, result, "signBit", res.signBit);
    SetNamedInt32(env, result, "wordCount", res.wordCount);
    return result;
}

// === Test Functions ===
napi_value RunCreateInt64(napi_env env, const BigintCaseSpec& spec)
{
    napi_value bigintValue = nullptr;
    NAPI_CALL(env, napi_create_bigint_int64(env, spec.int64Value, &bigintValue));
    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, bigintValue, &type));
    const bool passed = (type == napi_bigint);
    return MakeBigintResult(env, spec, { passed, spec.int64Value, true, K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO });
}

napi_value RunCreateUint64(napi_env env, const BigintCaseSpec& spec)
{
    napi_value bigintValue = nullptr;
    NAPI_CALL(env, napi_create_bigint_uint64(env, spec.uint64Value, &bigintValue));
    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, bigintValue, &type));
    const bool passed = (type == napi_bigint);
    return MakeBigintResult(env, spec, { passed, K_INT64_ZERO, true, K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO });
}

napi_value RunGetInt64(napi_env env, const BigintCaseSpec& spec, bool fromUint64)
{
    napi_value bigintValue = nullptr;
    if (fromUint64) {
        NAPI_CALL(env, napi_create_bigint_uint64(env, spec.uint64Value, &bigintValue));
    } else {
        NAPI_CALL(env, napi_create_bigint_int64(env, spec.int64Value, &bigintValue));
    }
    int64_t actual = K_INT64_ZERO;
    bool lossless = false;
    NAPI_CALL(env, napi_get_value_bigint_int64(env, bigintValue, &actual, &lossless));
    const bool passed = (lossless == spec.expectedLossless);
    return MakeBigintResult(env, spec, { passed, actual, lossless, K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO });
}

napi_value RunGetUint64(napi_env env, const BigintCaseSpec& spec, bool fromInt64)
{
    napi_value bigintValue = nullptr;
    if (fromInt64) {
        NAPI_CALL(env, napi_create_bigint_int64(env, spec.int64Value, &bigintValue));
    } else {
        NAPI_CALL(env, napi_create_bigint_uint64(env, spec.uint64Value, &bigintValue));
    }
    uint64_t actual = K_UINT64_ZERO;
    bool lossless = false;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, bigintValue, &actual, &lossless));
    const bool passed = (lossless == spec.expectedLossless);
    return MakeBigintResult(env, spec, { passed, static_cast<int64_t>(actual), lossless,
        K_SIGN_BIT_POSITIVE, K_WORD_COUNT_ZERO });
}

napi_value RunCreateWords(napi_env env, const BigintCaseSpec& spec)
{
    napi_value bigintValue = nullptr;
    NAPI_CALL(env, napi_create_bigint_words(env, spec.signBit, spec.wordCount, spec.words, &bigintValue));
    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, bigintValue, &type));
    const bool passed = (type == napi_bigint);
    return MakeBigintResult(env, spec,
        { passed, K_INT64_ZERO, true, spec.signBit, static_cast<int32_t>(spec.wordCount) });
}

napi_value RunGetWords(napi_env env, const BigintCaseSpec& spec)
{
    napi_value bigintValue = nullptr;
    NAPI_CALL(env, napi_create_bigint_words(env, spec.signBit, spec.wordCount, spec.words, &bigintValue));
    int actualSign = K_SIGN_BIT_POSITIVE;
    size_t capacity = K_BIGINT_MAX_WORDS;
    uint64_t actualWords[K_BIGINT_MAX_WORDS] = { K_UINT64_ZERO, K_UINT64_ZERO };
    NAPI_CALL(env, napi_get_value_bigint_words(env, bigintValue, &actualSign, &capacity, actualWords));
    const bool signMatch = (actualSign == spec.signBit);
    const bool passed = signMatch && WordsMatch(spec.words, actualWords, spec.wordCount);
    return MakeBigintResult(env, spec,
        { passed, K_INT64_ZERO, true, actualSign, static_cast<int32_t>(spec.wordCount) });
}

napi_value RunGetWordsCount(napi_env env, const BigintCaseSpec& spec)
{
    napi_value bigintValue = nullptr;
    NAPI_CALL(env, napi_create_bigint_words(env, spec.signBit, spec.wordCount, spec.words, &bigintValue));
    size_t actualCount = K_WORD_COUNT_ZERO;
    NAPI_CALL(env, napi_get_value_bigint_words(env, bigintValue, nullptr, &actualCount, nullptr));
    const bool passed = (actualCount == spec.wordCount);
    return MakeBigintResult(env, spec,
        { passed, K_INT64_ZERO, true, spec.signBit, static_cast<int32_t>(actualCount) });
}

static napi_value RunBigintCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_BIGINT_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid bigint case");
        return nullptr;
    }
    const BigintCaseSpec& spec = GetBigintCaseSpec(caseIndex);
    switch (spec.op) {
        case BigintOp::CREATE_INT64: return RunCreateInt64(env, spec);
        case BigintOp::CREATE_UINT64: return RunCreateUint64(env, spec);
        case BigintOp::GET_INT64: return RunGetInt64(env, spec, false);
        case BigintOp::GET_INT64_LOSSY: return RunGetInt64(env, spec, true);
        case BigintOp::GET_UINT64: return RunGetUint64(env, spec, false);
        case BigintOp::GET_UINT64_LOSSY: return RunGetUint64(env, spec, true);
        case BigintOp::CREATE_WORDS: return RunCreateWords(env, spec);
        case BigintOp::GET_WORDS: return RunGetWords(env, spec);
        case BigintOp::GET_WORDS_COUNT: return RunGetWordsCount(env, spec);
        default:
            napi_throw_error(env, nullptr, "unknown bigint op");
            return nullptr;
    }
}

}  // namespace

// === Module Registration ===
static napi_value InitBigintSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_BIGINT_CASE_COUNT);
    exportNames.reserve(K_BIGINT_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_BIGINT_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testBigintCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunBigintCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_bigintSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitBigintSuite,
    .nm_modname = "bigint_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterBigintSuiteModule(void)
{
    napi_module_register(&g_bigintSuiteModule);
}
