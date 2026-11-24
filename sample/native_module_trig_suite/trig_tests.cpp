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

// This suite exercises trigonometric functions (sin, cos, tan) computed from
// Taylor series rather than the platform math library. Each case derives a
// small angle in radians from the case number, keeps it within [0, pi/2)
// using a subtraction-based modulo, and cycles through the three operations.
// The tangent divides the sine by a safe copy of the cosine, which falls
// back to one when the cosine is too close to zero, so the divisor is never
// zero. Each Taylor recurrence divides by a safe denominator variable too.

namespace {

// === Constants ===

// --- Argument Counts ---
constexpr size_t K_ARGS_ZERO = 0;

// --- Case Count ---
constexpr size_t K_TRIG_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_TRIG_MAX_CASE_INDEX = 23;

// --- Trig Op Cycle ---
// The operation cycles every three cases across the full case set, so each
// of SIN, COS and TAN is exercised by exactly eight cases.
constexpr size_t K_TRIG_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_SIN = 0;
constexpr size_t K_OP_INDEX_COS = 1;
constexpr size_t K_OP_INDEX_TAN = 2;

// --- Taylor Series Term Counts ---
// Eight total terms per function: the leading term plus the extra iterations.
constexpr size_t K_TAYLOR_SIN_TERMS = 7;
constexpr size_t K_TAYLOR_COS_TERMS = 7;

// --- Taylor Denominator Stepping ---
// The running denominator factors grow by two each iteration so that no
// magic literal appears in the recurrence (two and one are named below).
constexpr size_t K_TAYLOR_DENOM_LOW_START_SIN = 2;
constexpr size_t K_TAYLOR_DENOM_LOW_START_COS = 1;
constexpr size_t K_TAYLOR_DENOM_STEP_ONE = 1;
constexpr size_t K_TAYLOR_DENOM_STEP_TWO = 2;

// --- Test Values (Doubles) ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;
constexpr double K_DOUBLE_PI = 3.14159265358979;
constexpr double K_DOUBLE_TWO_PI = 6.28318530717959;
constexpr double K_DOUBLE_HALF_PI = 1.57079632679490;

// --- Trig Angle Derivation ---
constexpr double K_TRIG_ANGLE_STEP = 0.2;

// --- Op Labels ---
const char* const K_OP_LABEL_SIN = "sin";
const char* const K_OP_LABEL_COS = "cos";
const char* const K_OP_LABEL_TAN = "tan";

// --- Op Descriptions ---
const char* const K_OP_DESC_SIN = "sine via Taylor series with angle reduction";
const char* const K_OP_DESC_COS = "cosine via Taylor series with angle reduction";
const char* const K_OP_DESC_TAN = "tangent as sine divided by safe cosine";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// --- Degree/Radian Conversion ---
constexpr double K_TRIG_HALF_CIRCLE_DEGREES = 180.0;
// --- Quadrant Classification ---
constexpr size_t K_TRIG_QUADRANT_ONE = 1;
constexpr size_t K_TRIG_QUADRANT_TWO = 2;
constexpr size_t K_TRIG_QUADRANT_THREE = 3;
constexpr size_t K_TRIG_QUADRANT_FOUR = 4;

// --- Reciprocal Safety ---
// Reciprocals divide by the trig value via a safe-variable ternary.
constexpr double K_TRIG_RECIP_TOLERANCE = 1.0e-6;

// === CaseSpec ===

enum class TrigOp {
    SIN,
    COS,
    TAN,
};

struct TrigCaseSpec {
    std::string name;
    TrigOp op;
    size_t caseNumber;
    double angle;
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

bool ValidateTrigCaseIndex(size_t caseIndex)
{
    return caseIndex <= K_TRIG_MAX_CASE_INDEX;
}

double DeriveAngle(size_t caseNumber)
{
    double angle = static_cast<double>(caseNumber) * K_TRIG_ANGLE_STEP;
    // Keep the angle within [0, pi/2) using subtraction (a modulo form),
    // which keeps the Taylor series accurate and avoids library fmod.
    while (angle >= K_DOUBLE_HALF_PI) {
        angle -= K_DOUBLE_HALF_PI;
    }
    return angle;
}

TrigOp DetermineTrigOp(size_t caseNumber)
{
    const size_t cycle = caseNumber % K_TRIG_OP_CYCLE;
    if (cycle == K_OP_INDEX_SIN) {
        return TrigOp::SIN;
    }
    if (cycle == K_OP_INDEX_COS) {
        return TrigOp::COS;
    }
    return TrigOp::TAN;
}

TrigCaseSpec GetTrigCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    TrigCaseSpec spec;
    spec.name = BuildIndexedName("trigCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.angle = DeriveAngle(caseNumber);
    spec.op = DetermineTrigOp(caseNumber);
    return spec;
}

const char* OpLabel(TrigOp op)
{
    if (op == TrigOp::SIN) {
        return K_OP_LABEL_SIN;
    }
    if (op == TrigOp::COS) {
        return K_OP_LABEL_COS;
    }
    return K_OP_LABEL_TAN;
}

const char* OpDescription(TrigOp op)
{
    if (op == TrigOp::SIN) {
        return K_OP_DESC_SIN;
    }
    if (op == TrigOp::COS) {
        return K_OP_DESC_COS;
    }
    return K_OP_DESC_TAN;
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

// === Helpers: Angle Reduction & Taylor Series ===

double ReduceAngle(double angle)
{
    // Reduce to [-pi, pi] using a subtraction loop with no division.
    double reduced = angle;
    while (reduced > K_DOUBLE_PI) {
        reduced -= K_DOUBLE_TWO_PI;
    }
    while (reduced < -K_DOUBLE_PI) {
        reduced += K_DOUBLE_TWO_PI;
    }
    return reduced;
}

double SinTaylor(double angle)
{
    const double x = ReduceAngle(angle);
    const double xSquared = x * x;
    double term = x;
    double sum = term;
    size_t denomLow = K_TAYLOR_DENOM_LOW_START_SIN;
    for (size_t iter = 0; iter < K_TAYLOR_SIN_TERMS; iter++) {
        double factorLow = static_cast<double>(denomLow);
        double factorHigh = static_cast<double>(denomLow + K_TAYLOR_DENOM_STEP_ONE);
        double denom = factorLow * factorHigh;
        // Safe variable guarantees a strictly positive divisor each step.
        double safeDenom = (denom > K_DOUBLE_ZERO) ? denom : K_DOUBLE_ONE;
        term = -term * xSquared / safeDenom;
        sum += term;
        denomLow += K_TAYLOR_DENOM_STEP_TWO;
    }
    return sum;
}

double CosTaylor(double angle)
{
    const double x = ReduceAngle(angle);
    const double xSquared = x * x;
    double term = K_DOUBLE_ONE;
    double sum = term;
    size_t denomLow = K_TAYLOR_DENOM_LOW_START_COS;
    for (size_t iter = 0; iter < K_TAYLOR_COS_TERMS; iter++) {
        double factorLow = static_cast<double>(denomLow);
        double factorHigh = static_cast<double>(denomLow + K_TAYLOR_DENOM_STEP_ONE);
        double denom = factorLow * factorHigh;
        // Safe variable guarantees a strictly positive divisor each step.
        double safeDenom = (denom > K_DOUBLE_ZERO) ? denom : K_DOUBLE_ONE;
        term = -term * xSquared / safeDenom;
        sum += term;
        denomLow += K_TAYLOR_DENOM_STEP_TWO;
    }
    return sum;
}

double TanTaylor(double angle)
{
    const double sinVal = SinTaylor(angle);
    const double cosVal = CosTaylor(angle);
    // Safe variable: fall back to one when cosine is too close to zero.
    double safeCos = (cosVal > K_DOUBLE_ZERO || cosVal < -K_DOUBLE_ZERO) ? cosVal : K_DOUBLE_ONE;
    return sinVal / safeCos;
}

// === Helpers: Angle Conversion & Classification ===

double RadianToDegree(double radians)
{
    return radians * K_TRIG_HALF_CIRCLE_DEGREES / K_DOUBLE_PI;
}

size_t AngleQuadrant(double angle)
{
    // Classify the angle into one of four quadrants using the reduced angle
    // in [-pi, pi]; no division or wrapping loop is required here.
    const double reduced = ReduceAngle(angle);
    if (reduced >= K_DOUBLE_ZERO && reduced < K_DOUBLE_HALF_PI) {
        return K_TRIG_QUADRANT_ONE;
    }
    if (reduced >= K_DOUBLE_HALF_PI) {
        return K_TRIG_QUADRANT_TWO;
    }
    if (reduced < -K_DOUBLE_HALF_PI) {
        return K_TRIG_QUADRANT_THREE;
    }
    return K_TRIG_QUADRANT_FOUR;
}

// === Helpers: Reciprocal Trigonometric Functions ===

double SecTaylor(double angle)
{
    const double cosVal = CosTaylor(angle);
    double safeCosVal = (cosVal > K_TRIG_RECIP_TOLERANCE || cosVal < -K_TRIG_RECIP_TOLERANCE)
        ? cosVal : K_DOUBLE_ONE;
    return K_DOUBLE_ONE / safeCosVal;
}

double CscTaylor(double angle)
{
    const double sinVal = SinTaylor(angle);
    double safeSinVal = (sinVal > K_TRIG_RECIP_TOLERANCE || sinVal < -K_TRIG_RECIP_TOLERANCE)
        ? sinVal : K_DOUBLE_ONE;
    return K_DOUBLE_ONE / safeSinVal;
}

double CotTaylor(double angle)
{
    const double sinVal = SinTaylor(angle);
    const double cosVal = CosTaylor(angle);
    double safeSinVal = (sinVal > K_TRIG_RECIP_TOLERANCE || sinVal < -K_TRIG_RECIP_TOLERANCE)
        ? sinVal : K_DOUBLE_ONE;
    return cosVal / safeSinVal;
}

double ComputeReciprocal(TrigOp op, double angle)
{
    if (op == TrigOp::SIN) {
        return CscTaylor(angle);
    }
    if (op == TrigOp::COS) {
        return SecTaylor(angle);
    }
    return CotTaylor(angle);
}

// === Helpers: Per-Op Value Dispatch ===

double ComputeSinValue(double angle)
{
    return SinTaylor(angle);
}

double ComputeCosValue(double angle)
{
    return CosTaylor(angle);
}

double ComputeTanValue(double angle)
{
    return TanTaylor(angle);
}

double ComputeTrigValue(TrigOp op, double angle)
{
    if (op == TrigOp::SIN) {
        return ComputeSinValue(angle);
    }
    if (op == TrigOp::COS) {
        return ComputeCosValue(angle);
    }
    return ComputeTanValue(angle);
}

// === Result Building ===

napi_value BuildTrigResult(napi_env env, const TrigCaseSpec& spec, double value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    const double angleDegrees = RadianToDegree(spec.angle);
    const size_t quadrant = AngleQuadrant(spec.angle);
    const double reducedAngle = ReduceAngle(spec.angle);
    const double reciprocal = ComputeReciprocal(spec.op, spec.angle);
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedDouble(env, result, "angle", spec.angle);
    SetNamedDouble(env, result, "value", value);
    SetNamedDouble(env, result, "angleDegrees", angleDegrees);
    SetNamedDouble(env, result, "quadrant", static_cast<double>(quadrant));
    SetNamedDouble(env, result, "reducedAngle", reducedAngle);
    SetNamedDouble(env, result, "reciprocal", reciprocal);
    return result;
}

// === Test Entry ===

static napi_value RunTrigCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (!ValidateTrigCaseIndex(caseIndex)) {
        napi_throw_error(env, nullptr, "invalid trig case");
        return nullptr;
    }
    const auto spec = GetTrigCaseSpec(caseIndex);
    const double value = ComputeTrigValue(spec.op, spec.angle);
    return BuildTrigResult(env, spec, value);
}

}  // namespace

static napi_value InitTrigSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_TRIG_CASE_COUNT);
    exportNames.reserve(K_TRIG_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_TRIG_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testTrigCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunTrigCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_trigSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitTrigSuite,
    .nm_modname = "trig_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterTrigSuiteModule(void)
{
    napi_module_register(&g_trigSuiteModule);
}
