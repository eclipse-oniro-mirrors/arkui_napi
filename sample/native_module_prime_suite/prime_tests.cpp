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
constexpr size_t K_PRIME_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Prime Op Cycle ---
constexpr size_t K_PRIME_OP_CYCLE = 3;
constexpr size_t K_PRIME_OP_INDEX_IS_PRIME = 0;
constexpr size_t K_PRIME_OP_INDEX_NTH_PRIME = 1;
constexpr size_t K_PRIME_OP_INDEX_PRIME_COUNT = 2;

// --- Op Labels ---
const char* const K_PRIME_OP_LABEL_IS_PRIME = "is_prime";
const char* const K_PRIME_OP_LABEL_NTH_PRIME = "nth_prime";
const char* const K_PRIME_OP_LABEL_PRIME_COUNT = "prime_count";

// --- Op Descriptions ---
const char* const K_PRIME_OP_DESC_IS_PRIME = "trial-division primality test on n";
const char* const K_PRIME_OP_DESC_NTH_PRIME = "k-th prime via sequential search, 2 is 1st";
const char* const K_PRIME_OP_DESC_PRIME_COUNT = "count of primes up to n via sieve of Eratosthenes";

// --- Primality Trial Division ---
constexpr int32_t K_PRIME_MIN_PRIME = 2;
constexpr int32_t K_PRIME_PARITY_MOD = 2;
constexpr int32_t K_PRIME_TRIAL_START = 3;
constexpr int32_t K_PRIME_TRIAL_STEP = 2;
constexpr int32_t K_PRIME_TRIAL_REMAINDER = 0;

// --- NthPrime Search ---
constexpr int32_t K_PRIME_NTH_MIN = 1;
constexpr int32_t K_PRIME_NTH_FOUND_INIT = 0;
constexpr int32_t K_PRIME_NTH_CANDIDATE_INIT = 1;
constexpr int32_t K_PRIME_NTH_INCREMENT = 1;
constexpr int32_t K_PRIME_INVALID = -1;

// --- PrimeCount Sieve ---
constexpr int32_t K_PRIME_COUNT_ZERO = 0;
constexpr int32_t K_PRIME_COUNT_INCREMENT = 1;
constexpr int32_t K_PRIME_TRIAL_INCREMENT = 1;
constexpr int32_t K_PRIME_SIEVE_INCLUSIVE = 1;

// --- Result Sentinels ---
constexpr int32_t K_PRIME_RESULT_INVALID = -1;
constexpr int32_t K_PRIME_BOOL_TRUE = 1;
constexpr int32_t K_PRIME_BOOL_FALSE = 0;

// --- Spec Validation ---
constexpr int32_t K_PRIME_N_MIN = 1;

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class PrimeOp {
    IS_PRIME,
    NTH_PRIME,
    PRIME_COUNT,
};

struct PrimeCaseSpec {
    std::string name;
    PrimeOp op;
    int32_t n;
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

// === Helpers: Primality ===

bool IsPrime(int32_t n)
{
    if (n < K_PRIME_MIN_PRIME) {
        return false;
    }
    if (n == K_PRIME_MIN_PRIME) {
        return true;
    }
    if (n % K_PRIME_PARITY_MOD == K_PRIME_TRIAL_REMAINDER) {
        return false;
    }
    for (int32_t d = K_PRIME_TRIAL_START; d * d <= n; d += K_PRIME_TRIAL_STEP) {
        if (n % d == K_PRIME_TRIAL_REMAINDER) {
            return false;
        }
    }
    return true;
}

int32_t NthPrime(int32_t k)
{
    if (k < K_PRIME_NTH_MIN) {
        return K_PRIME_INVALID;
    }
    int32_t found = K_PRIME_NTH_FOUND_INIT;
    int32_t candidate = K_PRIME_NTH_CANDIDATE_INIT;
    while (found < k) {
        candidate += K_PRIME_NTH_INCREMENT;
        if (IsPrime(candidate)) {
            found += K_PRIME_NTH_INCREMENT;
        }
    }
    return candidate;
}

int32_t CountPrimes(int32_t n)
{
    if (n < K_PRIME_MIN_PRIME) {
        return K_PRIME_COUNT_ZERO;
    }
    std::vector<bool> isComposite(n + K_PRIME_SIEVE_INCLUSIVE, false);
    for (int32_t p = K_PRIME_MIN_PRIME; p * p <= n; p += K_PRIME_TRIAL_INCREMENT) {
        if (isComposite[p]) {
            continue;
        }
        for (int32_t m = p * p; m <= n; m += p) {
            isComposite[m] = true;
        }
    }
    int32_t count = K_PRIME_COUNT_ZERO;
    for (int32_t p = K_PRIME_MIN_PRIME; p <= n; p += K_PRIME_TRIAL_INCREMENT) {
        if (!isComposite[p]) {
            count += K_PRIME_COUNT_INCREMENT;
        }
    }
    return count;
}

// === Helpers: Spec Construction ===

PrimeCaseSpec GetPrimeCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    PrimeCaseSpec spec;
    spec.name = BuildIndexedName("primeCase", caseNumber);
    spec.n = static_cast<int32_t>(caseNumber);
    const size_t opCycle = caseNumber % K_PRIME_OP_CYCLE;
    if (opCycle == K_PRIME_OP_INDEX_IS_PRIME) {
        spec.op = PrimeOp::IS_PRIME;
    } else if (opCycle == K_PRIME_OP_INDEX_NTH_PRIME) {
        spec.op = PrimeOp::NTH_PRIME;
    } else {
        spec.op = PrimeOp::PRIME_COUNT;
    }
    return spec;
}

const char* PrimeOpLabel(PrimeOp op)
{
    if (op == PrimeOp::IS_PRIME) {
        return K_PRIME_OP_LABEL_IS_PRIME;
    }
    if (op == PrimeOp::NTH_PRIME) {
        return K_PRIME_OP_LABEL_NTH_PRIME;
    }
    return K_PRIME_OP_LABEL_PRIME_COUNT;
}

const char* PrimeOpDescription(PrimeOp op)
{
    if (op == PrimeOp::IS_PRIME) {
        return K_PRIME_OP_DESC_IS_PRIME;
    }
    if (op == PrimeOp::NTH_PRIME) {
        return K_PRIME_OP_DESC_NTH_PRIME;
    }
    return K_PRIME_OP_DESC_PRIME_COUNT;
}

bool VerifyPrimeSpec(const PrimeCaseSpec& spec)
{
    if (spec.name.empty()) {
        return false;
    }
    if (spec.n < K_PRIME_N_MIN) {
        return false;
    }
    return true;
}

// === Helpers: Per-Op Computation ===

int32_t ComputeIsPrimeResult(int32_t n, bool* outIsPrime)
{
    *outIsPrime = IsPrime(n);
    return (*outIsPrime) ? K_PRIME_BOOL_TRUE : K_PRIME_BOOL_FALSE;
}

int32_t ComputeNthPrimeResult(int32_t k, bool* outIsPrime)
{
    const int32_t prime = NthPrime(k);
    *outIsPrime = IsPrime(prime);
    return prime;
}

int32_t ComputePrimeCountResult(int32_t n, bool* outIsPrime)
{
    *outIsPrime = IsPrime(n);
    return CountPrimes(n);
}

void ComputePrimeResult(const PrimeCaseSpec& spec, int32_t* outResult, bool* outIsPrime)
{
    if (spec.op == PrimeOp::IS_PRIME) {
        *outResult = ComputeIsPrimeResult(spec.n, outIsPrime);
    } else if (spec.op == PrimeOp::NTH_PRIME) {
        *outResult = ComputeNthPrimeResult(spec.n, outIsPrime);
    } else {
        *outResult = ComputePrimeCountResult(spec.n, outIsPrime);
    }
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

napi_value BuildPrimeResult(napi_env env, const PrimeCaseSpec& spec, int32_t result, bool isPrime)
{
    napi_value resultObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &resultObj));
    SetNamedString(env, resultObj, "name", spec.name);
    SetNamedString(env, resultObj, "op", PrimeOpLabel(spec.op));
    SetNamedInt32(env, resultObj, "n", spec.n);
    SetNamedInt32(env, resultObj, "result", result);
    SetNamedBool(env, resultObj, "isPrime", isPrime);
    SetNamedString(env, resultObj, "description", PrimeOpDescription(spec.op));
    return resultObj;
}

// === Test Entry ===

static napi_value RunPrimeCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_PRIME_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid prime case");
        return nullptr;
    }
    const auto spec = GetPrimeCaseSpec(caseIndex);
    if (!VerifyPrimeSpec(spec)) {
        napi_throw_error(env, nullptr, "prime spec inconsistent");
        return nullptr;
    }
    int32_t result = K_PRIME_RESULT_INVALID;
    bool isPrime = false;
    ComputePrimeResult(spec, &result, &isPrime);
    return BuildPrimeResult(env, spec, result, isPrime);
}

}  // namespace

static napi_value InitPrimeSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_PRIME_CASE_COUNT);
    exportNames.reserve(K_PRIME_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_PRIME_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testPrimeCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunPrimeCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_primeSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitPrimeSuite,
    .nm_modname = "prime_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterPrimeSuiteModule(void)
{
    napi_module_register(&g_primeSuiteModule);
}
