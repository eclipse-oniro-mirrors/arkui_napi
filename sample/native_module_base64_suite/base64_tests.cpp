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
constexpr size_t K_BASE64_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Base64 Mode Cycle ---
constexpr size_t K_BASE64_MODE_CYCLE = 2;
constexpr size_t K_MODE_INDEX_ENCODE = 0;
constexpr size_t K_MODE_INDEX_DECODE = 1;

// --- Base64 Alphabet ---
const char* const K_BASE64_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char K_BASE64_PAD = '=';
constexpr char K_BASE64_UPPER_A = 'A';
constexpr char K_BASE64_UPPER_Z = 'Z';
constexpr char K_BASE64_LOWER_A = 'a';
constexpr char K_BASE64_LOWER_Z = 'z';
constexpr char K_BASE64_DIGIT_ZERO = '0';
constexpr char K_BASE64_DIGIT_NINE = '9';
constexpr char K_BASE64_PLUS = '+';
constexpr char K_BASE64_SLASH = '/';

// --- Base64 Decode Math ---
constexpr int K_BASE64_UPPER_BASE = 0;
constexpr int K_BASE64_LOWER_BASE = 26;
constexpr int K_BASE64_DIGIT_BASE = 52;
constexpr int K_BASE64_PLUS_VAL = 62;
constexpr int K_BASE64_SLASH_VAL = 63;
constexpr int K_BASE64_INVALID_VAL = -1;

// --- Base64 Block Geometry ---
constexpr size_t K_BASE64_BLOCK_IN = 3;
constexpr size_t K_BASE64_BLOCK_OUT = 4;
constexpr size_t K_BASE64_LEN_ONE = 1;
constexpr size_t K_BASE64_LEN_TWO = 2;
constexpr size_t K_BASE64_MIN_VALID_LEN = 2;
constexpr size_t K_BASE64_INDEX_THIRD = 2;
constexpr size_t K_BASE64_INDEX_FOURTH = 3;
constexpr uint8_t K_BASE64_MASK_SIX = 0x3F;
constexpr uint8_t K_BASE64_NIBBLE_MASK = 0x0F;
constexpr uint8_t K_BASE64_TWO_BIT_MASK = 0x03;
constexpr uint8_t K_BASE64_SHIFT_2 = 2;
constexpr uint8_t K_BASE64_SHIFT_4 = 4;
constexpr uint8_t K_BASE64_SHIFT_6 = 6;

// --- Mode Labels ---
const char* const K_MODE_LABEL_ENCODE = "encode";
const char* const K_MODE_LABEL_DECODE = "decode";

// --- Mode Descriptions ---
const char* const K_MODE_DESC_ENCODE = "encode plaintext to base64 and verify decode roundtrip";
const char* const K_MODE_DESC_DECODE = "decode base64 to plaintext and verify encode roundtrip";

// --- Plaintext Prefixes ---
const char* const K_ENCODE_PREFIX = "sampleData";
const char* const K_DECODE_PREFIX = "decodeData";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class Base64Mode {
    ENCODE,
    DECODE,
};

struct Base64CaseSpec {
    std::string name;
    Base64Mode mode;
    std::string input;
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

const char* ModeLabel(Base64Mode mode)
{
    if (mode == Base64Mode::ENCODE) {
        return K_MODE_LABEL_ENCODE;
    }
    return K_MODE_LABEL_DECODE;
}

const char* ModeDescription(Base64Mode mode)
{
    if (mode == Base64Mode::ENCODE) {
        return K_MODE_DESC_ENCODE;
    }
    return K_MODE_DESC_DECODE;
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

// === Helpers: Base64 Encoding ===

void EncodeBlock(const uint8_t* block, size_t len, std::string& out)
{
    const char* alpha = K_BASE64_ALPHABET;
    out.push_back(alpha[(block[0] >> K_BASE64_SHIFT_2) & K_BASE64_MASK_SIX]);
    out.push_back(alpha[((block[0] << K_BASE64_SHIFT_4) | (block[1] >> K_BASE64_SHIFT_4)) & K_BASE64_MASK_SIX]);
    if (len == K_BASE64_LEN_ONE) {
        out.push_back(K_BASE64_PAD);
        out.push_back(K_BASE64_PAD);
        return;
    }
    out.push_back(alpha[((block[1] << K_BASE64_SHIFT_2) |
        (block[K_BASE64_INDEX_THIRD] >> K_BASE64_SHIFT_6)) & K_BASE64_MASK_SIX]);
    if (len == K_BASE64_LEN_TWO) {
        out.push_back(K_BASE64_PAD);
        return;
    }
    out.push_back(alpha[block[K_BASE64_INDEX_THIRD] & K_BASE64_MASK_SIX]);
}

void EncodeRemainder(const std::string& data, size_t remainder, size_t fullBlocks, std::string& out)
{
    if (remainder == 0) {
        return;
    }
    uint8_t block[K_BASE64_BLOCK_IN] = {0};
    const size_t base = fullBlocks * K_BASE64_BLOCK_IN;
    for (size_t i = 0; i < remainder; i++) {
        block[i] = static_cast<uint8_t>(data[base + i]);
    }
    EncodeBlock(block, remainder, out);
}

std::string Base64Encode(const std::string& data)
{
    std::string out;
    if (data.empty()) {
        return out;
    }
    const size_t fullBlocks = data.size() / K_BASE64_BLOCK_IN;
    const size_t remainder = data.size() % K_BASE64_BLOCK_IN;
    const size_t blockCount = fullBlocks + (remainder > 0 ? K_BASE64_LEN_ONE : K_ARGS_ZERO);
    out.reserve(blockCount * K_BASE64_BLOCK_OUT);
    for (size_t i = 0; i < fullBlocks; i++) {
        const size_t base = i * K_BASE64_BLOCK_IN;
        uint8_t block[K_BASE64_BLOCK_IN] = {
            static_cast<uint8_t>(data[base]),
            static_cast<uint8_t>(data[base + K_BASE64_LEN_ONE]),
            static_cast<uint8_t>(data[base + K_BASE64_LEN_TWO])
        };
        EncodeBlock(block, K_BASE64_BLOCK_IN, out);
    }
    EncodeRemainder(data, remainder, fullBlocks, out);
    return out;
}

// === Helpers: Base64 Decoding ===

int DecodeCharValue(char c)
{
    if (c >= K_BASE64_UPPER_A && c <= K_BASE64_UPPER_Z) {
        return static_cast<int>(c - K_BASE64_UPPER_A) + K_BASE64_UPPER_BASE;
    }
    if (c >= K_BASE64_LOWER_A && c <= K_BASE64_LOWER_Z) {
        return static_cast<int>(c - K_BASE64_LOWER_A) + K_BASE64_LOWER_BASE;
    }
    if (c >= K_BASE64_DIGIT_ZERO && c <= K_BASE64_DIGIT_NINE) {
        return static_cast<int>(c - K_BASE64_DIGIT_ZERO) + K_BASE64_DIGIT_BASE;
    }
    if (c == K_BASE64_PLUS) {
        return K_BASE64_PLUS_VAL;
    }
    if (c == K_BASE64_SLASH) {
        return K_BASE64_SLASH_VAL;
    }
    return K_BASE64_INVALID_VAL;
}

bool DecodeQuad(const char* quad, size_t quadLen, std::string& out)
{
    int vals[K_BASE64_BLOCK_OUT] = {0};
    size_t valid = 0;
    for (size_t i = 0; i < quadLen; i++) {
        if (quad[i] == K_BASE64_PAD) {
            break;
        }
        const int v = DecodeCharValue(quad[i]);
        if (v == K_BASE64_INVALID_VAL) {
            return false;
        }
        vals[i] = v;
        valid++;
    }
    if (valid < K_BASE64_MIN_VALID_LEN) {
        return false;
    }
    out.push_back(static_cast<char>((vals[0] << K_BASE64_SHIFT_2) | (vals[1] >> K_BASE64_SHIFT_4)));
    if (valid > K_BASE64_LEN_TWO) {
        out.push_back(static_cast<char>(((vals[1] & K_BASE64_NIBBLE_MASK) << K_BASE64_SHIFT_4) |
            (vals[K_BASE64_INDEX_THIRD] >> K_BASE64_SHIFT_2)));
    }
    if (valid > K_BASE64_BLOCK_IN) {
        out.push_back(static_cast<char>(
            ((vals[K_BASE64_INDEX_THIRD] & K_BASE64_TWO_BIT_MASK) << K_BASE64_SHIFT_6) |
            vals[K_BASE64_INDEX_FOURTH]));
    }
    return true;
}

std::string Base64Decode(const std::string& data)
{
    std::string out;
    if (data.empty()) {
        return out;
    }
    const size_t remainder = data.size() % K_BASE64_BLOCK_OUT;
    if (remainder != 0) {
        return std::string();
    }
    const size_t fullQuads = data.size() / K_BASE64_BLOCK_OUT;
    for (size_t q = 0; q < fullQuads; q++) {
        const size_t base = q * K_BASE64_BLOCK_OUT;
        if (!DecodeQuad(data.data() + base, K_BASE64_BLOCK_OUT, out)) {
            return std::string();
        }
    }
    return out;
}

// === Helpers: Input Validation ===

bool IsValidBase64Char(char c)
{
    if (c == K_BASE64_PAD) {
        return true;
    }
    return DecodeCharValue(c) != K_BASE64_INVALID_VAL;
}

bool ValidateBase64Input(const std::string& data)
{
    if (data.empty()) {
        return false;
    }
    if (data.size() % K_BASE64_BLOCK_OUT != 0) {
        return false;
    }
    for (size_t i = 0; i < data.size(); i++) {
        if (!IsValidBase64Char(data[i])) {
            return false;
        }
    }
    return true;
}

// === Helpers: CaseSpec Construction ===

Base64CaseSpec GetBase64CaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    Base64CaseSpec spec;
    spec.name = BuildIndexedName("base64Case", caseNumber);
    spec.caseNumber = caseNumber;
    spec.mode = Base64Mode::ENCODE;
    const size_t modeCycle = caseNumber % K_BASE64_MODE_CYCLE;
    if (modeCycle == K_MODE_INDEX_ENCODE) {
        spec.mode = Base64Mode::ENCODE;
        spec.input = std::string(K_ENCODE_PREFIX) + BuildIndexedName("", caseNumber);
    } else {
        spec.mode = Base64Mode::DECODE;
        const std::string plaintext = std::string(K_DECODE_PREFIX) + BuildIndexedName("", caseNumber);
        spec.input = Base64Encode(plaintext);
    }
    return spec;
}

// === Result Building ===

napi_value BuildBase64Result(napi_env env, const Base64CaseSpec& spec,
    const std::string& output, bool roundtrip, bool inputValid)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "mode", ModeLabel(spec.mode));
    SetNamedString(env, result, "description", ModeDescription(spec.mode));
    SetNamedString(env, result, "input", spec.input);
    SetNamedString(env, result, "output", output);
    SetNamedInt32(env, result, "inputLength", static_cast<int32_t>(spec.input.size()));
    SetNamedInt32(env, result, "outputLength", static_cast<int32_t>(output.size()));
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    SetNamedBool(env, result, "roundtrip", roundtrip);
    SetNamedBool(env, result, "inputValid", inputValid);
    return result;
}

// === Test Entry ===

static napi_value RunBase64Case(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_BASE64_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid base64 case");
        return nullptr;
    }
    const auto spec = GetBase64CaseSpec(caseIndex);
    std::string output;
    bool roundtrip = false;
    bool inputValid = false;
    if (spec.mode == Base64Mode::ENCODE) {
        output = Base64Encode(spec.input);
        roundtrip = (Base64Decode(output) == spec.input);
        inputValid = !spec.input.empty();
    } else {
        inputValid = ValidateBase64Input(spec.input);
        output = Base64Decode(spec.input);
        roundtrip = (Base64Encode(output) == spec.input);
    }
    return BuildBase64Result(env, spec, output, roundtrip, inputValid);
}

}  // namespace

static napi_value InitBase64Suite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_BASE64_CASE_COUNT);
    exportNames.reserve(K_BASE64_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_BASE64_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testBase64Case", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunBase64Case,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_base64SuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitBase64Suite,
    .nm_modname = "base64_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterBase64SuiteModule(void)
{
    napi_module_register(&g_base64SuiteModule);
}
