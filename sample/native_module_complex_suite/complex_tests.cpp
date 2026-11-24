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

// This suite exercises complex-number arithmetic through three cyclic
// operations derived from the case number: addition, multiplication, and
// magnitude. Each case derives its four real/imaginary coefficients from
// the case number using named scale constants, then dispatches to the
// matching helper. Magnitude is computed with Newton's method, where each
// iteration divides by a safe copy of the running guess so the divisor is
// never zero. Results report the input coefficients, the produced real and
// imaginary parts, and the input/output magnitudes.

namespace {

// === Constants ===

// --- Argument Counts ---
constexpr size_t K_ARGS_ZERO = 0;

// --- Case Count ---
constexpr size_t K_COMPLEX_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_COMPLEX_MAX_CASE_INDEX = 23;

// --- Complex Op Cycle ---
// The operation cycles every three cases across the full case set, so each
// of ADD, MULTIPLY and MAGNITUDE is exercised by exactly eight cases.
constexpr size_t K_COMPLEX_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_ADD = 0;
constexpr size_t K_OP_INDEX_MULTIPLY = 1;
constexpr size_t K_OP_INDEX_MAGNITUDE = 2;

// --- Complex Parity ---
constexpr size_t K_COMPLEX_PARITY_MOD = 2;

// --- Newton Iteration (Magnitude Square Root) ---
// Magnitude is sqrt(real*real + imag*imag). The square root is obtained with
// Newton's method; each step divides by a safe copy of the running guess so
// the divisor can never be zero (see the safe-variable ternary in the helper).
constexpr size_t K_NEWTON_MAX_ITER = 8;
constexpr double K_DOUBLE_HALF = 0.5;

// --- Test Values (Doubles) ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;
constexpr double K_DOUBLE_TWO = 2.0;

// --- Complex Coefficient Scales (Base) ---
constexpr double K_COMPLEX_REAL_A_SCALE = 1.5;
constexpr double K_COMPLEX_IMAG_A_SCALE = 0.5;
constexpr double K_COMPLEX_REAL_B_SCALE = 0.75;
constexpr double K_COMPLEX_IMAG_B_SCALE = 1.25;

// --- Complex Coefficient Scales (Parity Offset) ---
constexpr double K_COMPLEX_REAL_A_PARITY_SCALE = 0.25;
constexpr double K_COMPLEX_IMAG_A_PARITY_SCALE = 0.75;
constexpr double K_COMPLEX_REAL_B_PARITY_SCALE = 1.0;
constexpr double K_COMPLEX_IMAG_B_PARITY_SCALE = 0.5;

// --- Op Labels ---
const char* const K_OP_LABEL_ADD = "add";
const char* const K_OP_LABEL_MULTIPLY = "multiply";
const char* const K_OP_LABEL_MAGNITUDE = "magnitude";

// --- Op Descriptions ---
const char* const K_OP_DESC_ADD = "complex addition (realA+realB, imagA+imagB)";
const char* const K_OP_DESC_MULTIPLY = "complex multiplication (ac-bd, ad+bc)";
const char* const K_OP_DESC_MAGNITUDE = "complex magnitude via Newton sqrt of sum of squares";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// --- Argument (Angle) Series ---
// The complex argument atan2(imag, real) is approximated by the Taylor
// series for atan, evaluated on a ratio whose magnitude is clamped to one
// so the series converges quickly. Each division by a running denominator
// uses a safe-variable ternary so the divisor can never be zero.
constexpr double K_DOUBLE_HALF_PI = 1.57079632679490;
constexpr size_t K_ATAN_SERIES_TERMS = 5;
constexpr size_t K_ATAN_DENOM_BASE = 1;
constexpr size_t K_ATAN_DENOM_STEP = 2;

// --- Complex Derived Diagnostics ---
constexpr double K_COMPLEX_SCALE_FACTOR = 2.5;
constexpr double K_COMPLEX_TOLERANCE = 1.0e-6;

// === CaseSpec ===

enum class ComplexOp {
    ADD,
    MULTIPLY,
    MAGNITUDE,
};

struct ComplexCaseSpec {
    std::string name;
    ComplexOp op;
    size_t caseNumber;
    double realA;
    double imagA;
    double realB;
    double imagB;
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

bool ValidateComplexCaseIndex(size_t caseIndex)
{
    return caseIndex <= K_COMPLEX_MAX_CASE_INDEX;
}

void DeriveComplexCoefficients(ComplexCaseSpec& spec, size_t caseNumber)
{
    const double caseValue = static_cast<double>(caseNumber);
    const double parity = static_cast<double>(caseNumber % K_COMPLEX_PARITY_MOD);
    spec.realA = caseValue * K_COMPLEX_REAL_A_SCALE + parity * K_COMPLEX_REAL_A_PARITY_SCALE;
    spec.imagA = caseValue * K_COMPLEX_IMAG_A_SCALE + parity * K_COMPLEX_IMAG_A_PARITY_SCALE;
    spec.realB = caseValue * K_COMPLEX_REAL_B_SCALE + parity * K_COMPLEX_REAL_B_PARITY_SCALE;
    spec.imagB = caseValue * K_COMPLEX_IMAG_B_SCALE + parity * K_COMPLEX_IMAG_B_PARITY_SCALE;
}

ComplexOp DetermineComplexOp(size_t caseNumber)
{
    const size_t cycle = caseNumber % K_COMPLEX_OP_CYCLE;
    if (cycle == K_OP_INDEX_ADD) {
        return ComplexOp::ADD;
    }
    if (cycle == K_OP_INDEX_MULTIPLY) {
        return ComplexOp::MULTIPLY;
    }
    return ComplexOp::MAGNITUDE;
}

ComplexCaseSpec GetComplexCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    ComplexCaseSpec spec;
    spec.name = BuildIndexedName("complexCase", caseNumber);
    spec.caseNumber = caseNumber;
    DeriveComplexCoefficients(spec, caseNumber);
    spec.op = DetermineComplexOp(caseNumber);
    return spec;
}

const char* OpLabel(ComplexOp op)
{
    if (op == ComplexOp::ADD) {
        return K_OP_LABEL_ADD;
    }
    if (op == ComplexOp::MULTIPLY) {
        return K_OP_LABEL_MULTIPLY;
    }
    return K_OP_LABEL_MAGNITUDE;
}

const char* OpDescription(ComplexOp op)
{
    if (op == ComplexOp::ADD) {
        return K_OP_DESC_ADD;
    }
    if (op == ComplexOp::MULTIPLY) {
        return K_OP_DESC_MULTIPLY;
    }
    return K_OP_DESC_MAGNITUDE;
}

// === Helpers: Object Property Setters ===

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
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

// === Helpers: Complex Arithmetic ===

void ComplexAdd(double realA, double imagA, double realB, double imagB,
    double* outReal, double* outImag)
{
    *outReal = realA + realB;
    *outImag = imagA + imagB;
}

void ComplexMultiply(double realA, double imagA, double realB, double imagB,
    double* outReal, double* outImag)
{
    *outReal = realA * realB - imagA * imagB;
    *outImag = realA * imagB + imagA * realB;
}

double ComplexMagnitude(double real, double imag)
{
    const double sum = real * real + imag * imag;
    double guess = sum;
    for (size_t iter = 0; iter < K_NEWTON_MAX_ITER; iter++) {
        // Safe variable guarantees a strictly positive divisor for Newton step.
        double safeGuess = (guess > K_DOUBLE_ZERO) ? guess : K_DOUBLE_ONE;
        guess = (guess + sum / safeGuess) * K_DOUBLE_HALF;
    }
    return guess;
}

// === Helpers: Complex Derived Arithmetic ===

void ComplexConjugate(double real, double imag, double* outReal, double* outImag)
{
    *outReal = real;
    *outImag = -imag;
}

void ComplexScale(double real, double imag, double factor,
    double* outReal, double* outImag)
{
    *outReal = real * factor;
    *outImag = imag * factor;
}

double ApproxAngle(double ratio)
{
    // Bounded atan(ratio) via Taylor series on a clamped magnitude; each
    // running denominator is replaced by a safe copy before dividing.
    double absRatio = (ratio < K_DOUBLE_ZERO) ? -ratio : ratio;
    double safeAbsRatio = (absRatio > K_DOUBLE_ZERO) ? absRatio : K_DOUBLE_ONE;
    double sign = (ratio < K_DOUBLE_ZERO) ? -K_DOUBLE_ONE : K_DOUBLE_ONE;
    double baseRatio = absRatio;
    double offset = K_DOUBLE_ZERO;
    if (absRatio > K_DOUBLE_ONE) {
        baseRatio = K_DOUBLE_ONE / safeAbsRatio;
        offset = K_DOUBLE_HALF_PI;
    }
    double xSquared = baseRatio * baseRatio;
    double term = baseRatio;
    double sum = term;
    size_t denomLow = K_ATAN_DENOM_BASE + K_ATAN_DENOM_STEP;
    for (size_t iter = 0; iter < K_ATAN_SERIES_TERMS; iter++) {
        double denomK = static_cast<double>(denomLow);
        term = -term * xSquared;
        double safeDenomK = (denomK > K_DOUBLE_ZERO) ? denomK : K_DOUBLE_ONE;
        sum += term / safeDenomK;
        denomLow += K_ATAN_DENOM_STEP;
    }
    return sign * (offset + sum);
}

double ComplexArgument(double real, double imag)
{
    // Safe divisor for the ratio imag/real; falls back to one near zero.
    double safeReal = (real > K_COMPLEX_TOLERANCE || real < -K_COMPLEX_TOLERANCE) ? real : K_DOUBLE_ONE;
    double ratio = imag / safeReal;
    return ApproxAngle(ratio);
}

// === Helpers: Per-Op Result Dispatch ===

void ComputeAddResult(const ComplexCaseSpec& spec, double* outReal, double* outImag)
{
    ComplexAdd(spec.realA, spec.imagA, spec.realB, spec.imagB, outReal, outImag);
}

void ComputeMultiplyResult(const ComplexCaseSpec& spec, double* outReal, double* outImag)
{
    ComplexMultiply(spec.realA, spec.imagA, spec.realB, spec.imagB, outReal, outImag);
}

void ComputeMagnitudeEchoResult(const ComplexCaseSpec& spec, double* outReal, double* outImag)
{
    // For the magnitude op, the real/imag outputs echo the input coefficients.
    *outReal = spec.realA;
    *outImag = spec.imagA;
}

void ComputeComplexResult(const ComplexCaseSpec& spec, double* outReal, double* outImag)
{
    *outReal = K_DOUBLE_ZERO;
    *outImag = K_DOUBLE_ZERO;
    if (spec.op == ComplexOp::ADD) {
        ComputeAddResult(spec, outReal, outImag);
        return;
    }
    if (spec.op == ComplexOp::MULTIPLY) {
        ComputeMultiplyResult(spec, outReal, outImag);
        return;
    }
    ComputeMagnitudeEchoResult(spec, outReal, outImag);
}

// === Helpers: Magnitude Reporting ===

double ComputeInputMagnitude(const ComplexCaseSpec& spec)
{
    return ComplexMagnitude(spec.realA, spec.imagA);
}

double ComputeOutputMagnitude(double outReal, double outImag)
{
    return ComplexMagnitude(outReal, outImag);
}

// === Result Building ===

napi_value BuildComplexResult(napi_env env, const ComplexCaseSpec& spec,
    double outReal, double outImag, double magnitudeIn, double magnitudeOut)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    // Derived diagnostics computed from the input complex A.
    double conjReal = K_DOUBLE_ZERO;
    double conjImag = K_DOUBLE_ZERO;
    ComplexConjugate(spec.realA, spec.imagA, &conjReal, &conjImag);
    double scaledReal = K_DOUBLE_ZERO;
    double scaledImag = K_DOUBLE_ZERO;
    ComplexScale(spec.realA, spec.imagA, K_COMPLEX_SCALE_FACTOR, &scaledReal, &scaledImag);
    const double argument = ComplexArgument(spec.realA, spec.imagA);
    const double scaleResult = ComplexMagnitude(scaledReal, scaledImag);
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedDouble(env, result, "arIn", spec.realA);
    SetNamedDouble(env, result, "aiIn", spec.imagA);
    SetNamedDouble(env, result, "brIn", spec.realB);
    SetNamedDouble(env, result, "biIn", spec.imagB);
    SetNamedDouble(env, result, "realOut", outReal);
    SetNamedDouble(env, result, "imagOut", outImag);
    SetNamedDouble(env, result, "magnitudeIn", magnitudeIn);
    SetNamedDouble(env, result, "magnitudeOut", magnitudeOut);
    SetNamedDouble(env, result, "conjugateReal", conjReal);
    SetNamedDouble(env, result, "conjugateImag", conjImag);
    SetNamedDouble(env, result, "argument", argument);
    SetNamedDouble(env, result, "scaleResult", scaleResult);
    return result;
}

// === Test Entry ===

static napi_value RunComplexCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (!ValidateComplexCaseIndex(caseIndex)) {
        napi_throw_error(env, nullptr, "invalid complex case");
        return nullptr;
    }
    const auto spec = GetComplexCaseSpec(caseIndex);
    double outReal = K_DOUBLE_ZERO;
    double outImag = K_DOUBLE_ZERO;
    ComputeComplexResult(spec, &outReal, &outImag);
    const double magnitudeIn = ComputeInputMagnitude(spec);
    const double magnitudeOut = ComputeOutputMagnitude(outReal, outImag);
    return BuildComplexResult(env, spec, outReal, outImag, magnitudeIn, magnitudeOut);
}

}  // namespace

static napi_value InitComplexSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_COMPLEX_CASE_COUNT);
    exportNames.reserve(K_COMPLEX_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_COMPLEX_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testComplexCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunComplexCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_complexSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitComplexSuite,
    .nm_modname = "complex_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterComplexSuiteModule(void)
{
    napi_module_register(&g_complexSuiteModule);
}
