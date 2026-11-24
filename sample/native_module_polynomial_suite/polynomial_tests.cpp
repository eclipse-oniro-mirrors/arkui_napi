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
constexpr size_t K_POLY_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Polynomial Operation Configuration ---
constexpr size_t K_POLY_OP_CYCLE = 3;
constexpr size_t K_POLY_OP_INDEX_EVAL = 0;
constexpr size_t K_POLY_OP_INDEX_DERIVATIVE = 1;
constexpr size_t K_POLY_OP_INDEX_ADD_FIXED = 2;

// --- Polynomial Coefficient Layout ---
constexpr size_t K_POLY_MAX_DEGREE = 4;
constexpr size_t K_POLY_COEFFS_CAPACITY = 4;
constexpr size_t K_POLY_FIXED_DEGREE = 2;
constexpr size_t K_POLY_FIXED_COEFFS_COUNT = 3;
constexpr size_t K_POLY_COEFF_COUNT_OFFSET = 1;
constexpr size_t K_POLY_DERIV_STEP = 1;

// --- Polynomial Test Values (Doubles) ---
constexpr double K_POLY_COEFF_SCALE = 1.5;
constexpr double K_POLY_X_SCALE = 0.5;
constexpr double K_POLY_HONNER_INIT = 0.0;
constexpr double K_POLY_COEFF_ZERO = 0.0;
constexpr double K_POLY_FIXED_COEFF_A = 2.0;
constexpr double K_POLY_FIXED_COEFF_B = 3.0;
constexpr double K_POLY_FIXED_COEFF_C = 4.0;

// --- Polynomial Naive Evaluation & Verification ---
constexpr double K_POLY_NAIVE_POW_INIT = 1.0;
constexpr double K_POLY_NAIVE_ACC_INIT = 0.0;
constexpr double K_POLY_COEFF_SUM_INIT = 0.0;
constexpr double K_POLY_LEADING_INIT = 0.0;
constexpr double K_POLY_VERIFY_TOLERANCE = 1e-9;

// --- Operation Labels ---
const char* const K_OP_LABEL_EVAL = "eval";
const char* const K_OP_LABEL_DERIVATIVE = "derivative";
const char* const K_OP_LABEL_ADD_FIXED = "add_fixed";

// --- Operation Descriptions ---
const char* const K_OP_DESC_EVAL = "evaluate polynomial at x using Horner's method";
const char* const K_OP_DESC_DERIVATIVE = "compute the first derivative coefficients";
const char* const K_OP_DESC_ADD_FIXED = "add a fixed second polynomial and report the sum";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class PolyOp {
    EVAL,
    DERIVATIVE,
    ADD_FIXED,
};

struct PolynomialCaseSpec {
    std::string name;
    PolyOp op;
    size_t caseNumber;
    size_t degree;
    double coeffs[K_POLY_COEFFS_CAPACITY];
    double x;
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

PolynomialCaseSpec GetPolynomialCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    PolynomialCaseSpec spec;
    spec.name = BuildIndexedName("polynomialCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.degree = caseNumber % K_POLY_MAX_DEGREE;
    spec.x = static_cast<double>(caseNumber) * K_POLY_X_SCALE;
    for (size_t i = 0; i < K_POLY_COEFFS_CAPACITY; i++) {
        spec.coeffs[i] = static_cast<double>(caseNumber + i) * K_POLY_COEFF_SCALE;
    }
    const size_t cycle = caseNumber % K_POLY_OP_CYCLE;
    if (cycle == K_POLY_OP_INDEX_EVAL) {
        spec.op = PolyOp::EVAL;
    } else if (cycle == K_POLY_OP_INDEX_DERIVATIVE) {
        spec.op = PolyOp::DERIVATIVE;
    } else {
        spec.op = PolyOp::ADD_FIXED;
    }
    return spec;
}

const char* PolyOpLabel(PolyOp op)
{
    if (op == PolyOp::EVAL) {
        return K_OP_LABEL_EVAL;
    }
    if (op == PolyOp::DERIVATIVE) {
        return K_OP_LABEL_DERIVATIVE;
    }
    return K_OP_LABEL_ADD_FIXED;
}

const char* PolyOpDescription(PolyOp op)
{
    if (op == PolyOp::EVAL) {
        return K_OP_DESC_EVAL;
    }
    if (op == PolyOp::DERIVATIVE) {
        return K_OP_DESC_DERIVATIVE;
    }
    return K_OP_DESC_ADD_FIXED;
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

// === Helpers: Polynomial Math ===

double PolyEval(const double* coeffs, size_t degree, double x)
{
    double acc = K_POLY_HONNER_INIT;
    const size_t count = degree + K_POLY_COEFF_COUNT_OFFSET;
    for (size_t i = count; i-- > 0;) {
        acc = acc * x + coeffs[i];
    }
    return acc;
}

size_t PolyDerivative(const double* coeffs, size_t degree, double* out)
{
    for (size_t i = 0; i < K_POLY_COEFFS_CAPACITY; i++) {
        out[i] = K_POLY_COEFF_ZERO;
    }
    if (degree == 0) {
        return 0;
    }
    const size_t newDegree = degree - K_POLY_DERIV_STEP;
    for (size_t i = 0; i <= newDegree; i++) {
        const size_t termIndex = i + K_POLY_DERIV_STEP;
        out[i] = coeffs[termIndex] * static_cast<double>(termIndex);
    }
    return newDegree;
}

size_t PolyAddFixed(const double* coeffs, size_t degree, double* out)
{
    const double fixed[K_POLY_FIXED_COEFFS_COUNT] = {
        K_POLY_FIXED_COEFF_A, K_POLY_FIXED_COEFF_B, K_POLY_FIXED_COEFF_C};
    const size_t outDegree = (degree > K_POLY_FIXED_DEGREE) ? degree : K_POLY_FIXED_DEGREE;
    for (size_t i = 0; i < K_POLY_COEFFS_CAPACITY; i++) {
        const double lhs = (i <= degree) ? coeffs[i] : K_POLY_COEFF_ZERO;
        const double rhs = (i < K_POLY_FIXED_COEFFS_COUNT) ? fixed[i] : K_POLY_COEFF_ZERO;
        out[i] = lhs + rhs;
    }
    return outDegree;
}

// === Helpers: Coefficient Array ===

napi_value BuildCoeffsArray(napi_env env, const double* coeffs, size_t degree)
{
    napi_value array = nullptr;
    const size_t count = degree + K_POLY_COEFF_COUNT_OFFSET;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &array));
    for (size_t i = 0; i < count; i++) {
        napi_value element = nullptr;
        NAPI_CALL(env, napi_create_double(env, coeffs[i], &element));
        NAPI_CALL(env, napi_set_element(env, array, static_cast<uint32_t>(i), element));
    }
    return array;
}

// === Helpers: Polynomial Verification ===

double PolyEvalNaive(const double* coeffs, size_t degree, double x)
{
    double acc = K_POLY_NAIVE_ACC_INIT;
    double power = K_POLY_NAIVE_POW_INIT;
    for (size_t i = 0; i <= degree; i++) {
        acc += coeffs[i] * power;
        power *= x;
    }
    return acc;
}

bool PolyHornerMatchesNaive(const double* coeffs, size_t degree, double x)
{
    const double horner = PolyEval(coeffs, degree, x);
    const double naive = PolyEvalNaive(coeffs, degree, x);
    const double diff = horner - naive;
    if (diff < K_POLY_COEFF_ZERO) {
        return -diff < K_POLY_VERIFY_TOLERANCE;
    }
    return diff < K_POLY_VERIFY_TOLERANCE;
}

double PolyCoeffSum(const double* coeffs, size_t degree)
{
    double sum = K_POLY_COEFF_SUM_INIT;
    for (size_t i = 0; i <= degree; i++) {
        sum += coeffs[i];
    }
    return sum;
}

double PolyLeadingCoeff(const double* coeffs, size_t degree)
{
    if (degree == 0) {
        return coeffs[0];
    }
    return coeffs[degree];
}

// === Result Building ===

napi_value BuildPolynomialResult(napi_env env, const PolynomialCaseSpec& spec,
    size_t outDegree, const double* outCoeffs)
{
    const double value = PolyEval(outCoeffs, outDegree, spec.x);
    const double naiveValue = PolyEvalNaive(spec.coeffs, spec.degree, spec.x);
    const double coeffSum = PolyCoeffSum(spec.coeffs, spec.degree);
    const double leadingCoeff = PolyLeadingCoeff(spec.coeffs, spec.degree);
    const bool isConstant = (spec.degree == 0);
    const bool hornerVerified = PolyHornerMatchesNaive(spec.coeffs, spec.degree, spec.x);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", PolyOpLabel(spec.op));
    SetNamedString(env, result, "description", PolyOpDescription(spec.op));
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    SetNamedInt32(env, result, "degree", static_cast<int32_t>(spec.degree));
    SetNamedInt32(env, result, "derivativeDegree", static_cast<int32_t>(outDegree));
    SetNamedDouble(env, result, "x", spec.x);
    SetNamedDouble(env, result, "value", value);
    SetNamedDouble(env, result, "naiveValue", naiveValue);
    SetNamedDouble(env, result, "coeffSum", coeffSum);
    SetNamedDouble(env, result, "leadingCoeff", leadingCoeff);
    SetNamedBool(env, result, "isConstant", isConstant);
    SetNamedBool(env, result, "hornerVerified", hornerVerified);
    napi_value coeffsArray = BuildCoeffsArray(env, outCoeffs, outDegree);
    NAPI_CALL(env, napi_set_named_property(env, result, "coeffs", coeffsArray));
    return result;
}

// === Test Entry ===

static napi_value RunPolynomialCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_POLY_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid polynomial case");
        return nullptr;
    }
    const auto spec = GetPolynomialCaseSpec(caseIndex);
    double outCoeffs[K_POLY_COEFFS_CAPACITY] = {K_POLY_COEFF_ZERO, K_POLY_COEFF_ZERO,
        K_POLY_COEFF_ZERO, K_POLY_COEFF_ZERO};
    size_t outDegree = 0;
    if (spec.op == PolyOp::EVAL) {
        for (size_t i = 0; i < K_POLY_COEFFS_CAPACITY; i++) {
            outCoeffs[i] = spec.coeffs[i];
        }
        outDegree = spec.degree;
    } else if (spec.op == PolyOp::DERIVATIVE) {
        outDegree = PolyDerivative(spec.coeffs, spec.degree, outCoeffs);
    } else {
        outDegree = PolyAddFixed(spec.coeffs, spec.degree, outCoeffs);
    }
    return BuildPolynomialResult(env, spec, outDegree, outCoeffs);
}

}  // namespace

static napi_value InitPolynomialSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_POLY_CASE_COUNT);
    exportNames.reserve(K_POLY_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_POLY_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testPolynomialCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunPolynomialCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_polynomialSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitPolynomialSuite,
    .nm_modname = "polynomial_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterPolynomialSuiteModule(void)
{
    napi_module_register(&g_polynomialSuiteModule);
}
