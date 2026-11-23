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
constexpr size_t K_IPV4_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- IPv4 Op Configuration ---
constexpr size_t K_IPV4_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_PARSE = 0;
constexpr size_t K_OP_INDEX_MASK = 1;
constexpr size_t K_OP_INDEX_NETWORK = 2;

// --- Octet Derivation ---
constexpr int K_IPV4_OCTET_RANGE = 256;
constexpr int K_IPV4_OCTET_UPPER_BOUND = 255;
constexpr int K_IPV4_O1_MUL = 1;
constexpr int K_IPV4_O2_MUL = 3;
constexpr int K_IPV4_O3_MUL = 7;
constexpr int K_IPV4_O4_MUL = 11;

// --- Mask Derivation ---
constexpr int K_IPV4_MASK_CYCLE = 5;
constexpr int K_IPV4_MASK_STEP = 8;
constexpr int K_IPV4_MASK_MIN = 0;
constexpr int K_IPV4_MASK_MAX = 32;
constexpr int K_IPV4_BITS_PER_OCTET = 8;

// --- IPv4 Fixed Values ---
constexpr size_t K_IPV4_OCTET_COUNT = 4;
constexpr size_t K_IPV4_OCTET_INDEX_0 = 0;
constexpr size_t K_IPV4_OCTET_INDEX_1 = 1;
constexpr size_t K_IPV4_OCTET_INDEX_2 = 2;
constexpr size_t K_IPV4_OCTET_INDEX_3 = 3;
constexpr uint32_t K_IPV4_OCTET_SHIFT_1 = 8;
constexpr uint32_t K_IPV4_OCTET_SHIFT_2 = 16;
constexpr uint32_t K_IPV4_OCTET_SHIFT_3 = 24;
constexpr uint32_t K_IPV4_OCTET_MASK = 0xFFu;
constexpr uint32_t K_IPV4_ALL_MASK = 0xFFFFFFFFu;
constexpr uint32_t K_IPV4_ZERO_MASK = 0u;

// --- Dotted String Formatting ---
const char* const K_IPV4_DOT_DELIMITER = ".";
constexpr size_t K_IPV4_DOT_DELIMITER_LEN = 1;

// --- Op Labels ---
const char* const K_OP_LABEL_PARSE = "parse";
const char* const K_OP_LABEL_MASK = "mask";
const char* const K_OP_LABEL_NETWORK = "network";

// --- Op Descriptions ---
const char* const K_OP_DESC_PARSE = "pack octets into uint32 and decompose back lossless";
const char* const K_OP_DESC_MASK = "derive prefix mask from maskBits and split into octets";
const char* const K_OP_DESC_NETWORK = "compute network address from ip and maskBits, split into octets";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class Ipv4Op {
    PARSE,
    MASK,
    NETWORK,
};

struct Ipv4CaseSpec {
    std::string name;
    Ipv4Op op;
    size_t caseNumber;
    int32_t octets[K_IPV4_OCTET_COUNT];
    int32_t maskBits;
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

Ipv4CaseSpec GetIpv4CaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    Ipv4CaseSpec spec;
    spec.name = BuildIndexedName("ipv4Case", caseNumber);
    spec.caseNumber = caseNumber;
    spec.op = Ipv4Op::PARSE;
    spec.octets[K_IPV4_OCTET_INDEX_0] = static_cast<int32_t>((caseNumber * K_IPV4_O1_MUL) % K_IPV4_OCTET_RANGE);
    spec.octets[K_IPV4_OCTET_INDEX_1] = static_cast<int32_t>((caseNumber * K_IPV4_O2_MUL) % K_IPV4_OCTET_RANGE);
    spec.octets[K_IPV4_OCTET_INDEX_2] = static_cast<int32_t>((caseNumber * K_IPV4_O3_MUL) % K_IPV4_OCTET_RANGE);
    spec.octets[K_IPV4_OCTET_INDEX_3] = static_cast<int32_t>((caseNumber * K_IPV4_O4_MUL) % K_IPV4_OCTET_RANGE);
    spec.maskBits = static_cast<int32_t>((caseNumber % K_IPV4_MASK_CYCLE) * K_IPV4_MASK_STEP);
    const size_t cycle = caseNumber % K_IPV4_OP_CYCLE;
    if (cycle == K_OP_INDEX_PARSE) {
        spec.op = Ipv4Op::PARSE;
    } else if (cycle == K_OP_INDEX_MASK) {
        spec.op = Ipv4Op::MASK;
    } else {
        spec.op = Ipv4Op::NETWORK;
    }
    return spec;
}

const char* OpLabel(Ipv4Op op)
{
    if (op == Ipv4Op::PARSE) {
        return K_OP_LABEL_PARSE;
    }
    if (op == Ipv4Op::MASK) {
        return K_OP_LABEL_MASK;
    }
    return K_OP_LABEL_NETWORK;
}

const char* OpDescription(Ipv4Op op)
{
    if (op == Ipv4Op::PARSE) {
        return K_OP_DESC_PARSE;
    }
    if (op == Ipv4Op::MASK) {
        return K_OP_DESC_MASK;
    }
    return K_OP_DESC_NETWORK;
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

// === Helpers: Octet Validation & Conversion ===

bool ValidateOctets(const int32_t* octets)
{
    for (size_t i = 0; i < K_IPV4_OCTET_COUNT; i++) {
        if (octets[i] < 0 || octets[i] > K_IPV4_OCTET_UPPER_BOUND) {
            return false;
        }
    }
    return true;
}

uint32_t OctetsToUint32(const int32_t* octets)
{
    uint32_t value = K_IPV4_ZERO_MASK;
    value |= static_cast<uint32_t>(octets[K_IPV4_OCTET_INDEX_0]) << K_IPV4_OCTET_SHIFT_3;
    value |= static_cast<uint32_t>(octets[K_IPV4_OCTET_INDEX_1]) << K_IPV4_OCTET_SHIFT_2;
    value |= static_cast<uint32_t>(octets[K_IPV4_OCTET_INDEX_2]) << K_IPV4_OCTET_SHIFT_1;
    value |= static_cast<uint32_t>(octets[K_IPV4_OCTET_INDEX_3]);
    return value;
}

void Uint32ToOctets(uint32_t value, int32_t* out)
{
    out[K_IPV4_OCTET_INDEX_0] = static_cast<int32_t>((value >> K_IPV4_OCTET_SHIFT_3) & K_IPV4_OCTET_MASK);
    out[K_IPV4_OCTET_INDEX_1] = static_cast<int32_t>((value >> K_IPV4_OCTET_SHIFT_2) & K_IPV4_OCTET_MASK);
    out[K_IPV4_OCTET_INDEX_2] = static_cast<int32_t>((value >> K_IPV4_OCTET_SHIFT_1) & K_IPV4_OCTET_MASK);
    out[K_IPV4_OCTET_INDEX_3] = static_cast<int32_t>(value & K_IPV4_OCTET_MASK);
}

// === Helpers: Mask & Address Arithmetic ===

uint32_t PrefixMask(int32_t maskBits)
{
    if (maskBits <= K_IPV4_MASK_MIN) {
        return K_IPV4_ZERO_MASK;
    }
    if (maskBits >= K_IPV4_MASK_MAX) {
        return K_IPV4_ALL_MASK;
    }
    const uint32_t shift = static_cast<uint32_t>(K_IPV4_MASK_MAX - maskBits);
    return K_IPV4_ALL_MASK << shift;
}

uint32_t NetworkAddress(uint32_t ipUint32, int32_t maskBits)
{
    const uint32_t mask = PrefixMask(maskBits);
    return ipUint32 & mask;
}

uint32_t BroadcastAddress(uint32_t ipUint32, int32_t maskBits)
{
    const uint32_t mask = PrefixMask(maskBits);
    return ipUint32 | (~mask);
}

int32_t HostBitsFromMask(int32_t maskBits)
{
    if (maskBits <= K_IPV4_MASK_MIN) {
        return K_IPV4_MASK_MAX;
    }
    if (maskBits >= K_IPV4_MASK_MAX) {
        return K_IPV4_MASK_MIN;
    }
    return K_IPV4_MASK_MAX - maskBits;
}

// === Helpers: Dotted String Formatting ===

std::string OctetsToDottedString(const int32_t* octets)
{
    std::string result = std::to_string(octets[K_IPV4_OCTET_INDEX_0]);
    result += K_IPV4_DOT_DELIMITER;
    result += std::to_string(octets[K_IPV4_OCTET_INDEX_1]);
    result += K_IPV4_DOT_DELIMITER;
    result += std::to_string(octets[K_IPV4_OCTET_INDEX_2]);
    result += K_IPV4_DOT_DELIMITER;
    result += std::to_string(octets[K_IPV4_OCTET_INDEX_3]);
    return result;
}

// === Helpers: Result Array Building ===

napi_value BuildOctetsArray(napi_env env, const int32_t* octets)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_IPV4_OCTET_COUNT, &array));
    for (size_t i = 0; i < K_IPV4_OCTET_COUNT; i++) {
        napi_value octetValue = nullptr;
        NAPI_CALL(env, napi_create_int32(env, octets[i], &octetValue));
        NAPI_CALL(env, napi_set_element(env, array, static_cast<uint32_t>(i), octetValue));
    }
    return array;
}

// === Result Building ===

napi_value BuildIpv4Result(napi_env env, const Ipv4CaseSpec& spec, uint32_t ipUint32, uint32_t maskUint32,
    uint32_t networkUint32, uint32_t broadcastUint32, const int32_t* maskOctets,
    const int32_t* networkOctets, const int32_t* broadcastOctets, bool octetsValid, int32_t hostBits)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    SetNamedInt32(env, result, "maskBits", spec.maskBits);
    SetNamedInt32(env, result, "hostBits", hostBits);
    SetNamedBool(env, result, "octetsValid", octetsValid);
    napi_value octetsArray = BuildOctetsArray(env, spec.octets);
    NAPI_CALL(env, napi_set_named_property(env, result, "octets", octetsArray));
    napi_value maskOctetsArray = BuildOctetsArray(env, maskOctets);
    NAPI_CALL(env, napi_set_named_property(env, result, "maskOctets", maskOctetsArray));
    napi_value networkOctetsArray = BuildOctetsArray(env, networkOctets);
    NAPI_CALL(env, napi_set_named_property(env, result, "networkOctets", networkOctetsArray));
    napi_value broadcastOctetsArray = BuildOctetsArray(env, broadcastOctets);
    NAPI_CALL(env, napi_set_named_property(env, result, "broadcastOctets", broadcastOctetsArray));
    SetNamedDouble(env, result, "ipUint32", static_cast<double>(ipUint32));
    SetNamedDouble(env, result, "maskUint32", static_cast<double>(maskUint32));
    SetNamedDouble(env, result, "networkUint32", static_cast<double>(networkUint32));
    SetNamedDouble(env, result, "broadcastUint32", static_cast<double>(broadcastUint32));
    SetNamedString(env, result, "ipString", OctetsToDottedString(spec.octets));
    SetNamedString(env, result, "networkString", OctetsToDottedString(networkOctets));
    return result;
}

// === Test Entry ===

static napi_value RunIpv4Case(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_IPV4_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid ipv4 case");
        return nullptr;
    }
    const auto spec = GetIpv4CaseSpec(caseIndex);
    const bool octetsValid = ValidateOctets(spec.octets);
    const uint32_t ipUint32 = OctetsToUint32(spec.octets);
    const uint32_t maskUint32 = PrefixMask(spec.maskBits);
    const uint32_t networkUint32 = NetworkAddress(ipUint32, spec.maskBits);
    const uint32_t broadcastUint32 = BroadcastAddress(ipUint32, spec.maskBits);
    int32_t maskOctets[K_IPV4_OCTET_COUNT] = {0};
    int32_t networkOctets[K_IPV4_OCTET_COUNT] = {0};
    int32_t broadcastOctets[K_IPV4_OCTET_COUNT] = {0};
    Uint32ToOctets(maskUint32, maskOctets);
    Uint32ToOctets(networkUint32, networkOctets);
    Uint32ToOctets(broadcastUint32, broadcastOctets);
    const int32_t hostBits = HostBitsFromMask(spec.maskBits);
    return BuildIpv4Result(env, spec, ipUint32, maskUint32, networkUint32, broadcastUint32,
        maskOctets, networkOctets, broadcastOctets, octetsValid, hostBits);
}

}  // namespace

static napi_value InitIpv4Suite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_IPV4_CASE_COUNT);
    exportNames.reserve(K_IPV4_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_IPV4_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testIpv4Case", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunIpv4Case,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_ipv4SuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitIpv4Suite,
    .nm_modname = "ipv4_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterIpv4SuiteModule(void)
{
    napi_module_register(&g_ipv4SuiteModule);
}
