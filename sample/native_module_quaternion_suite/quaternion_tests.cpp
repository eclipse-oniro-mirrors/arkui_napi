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
constexpr size_t K_QUATERNION_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Quaternion Op Cycle ---
constexpr size_t K_QUAT_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_MULTIPLY = 0;
constexpr size_t K_OP_INDEX_CONJUGATE = 1;
constexpr size_t K_OP_INDEX_NORMALIZE = 2;

// --- Quaternion Derivation ---
constexpr double K_QUAT_W_SCALE = 1.0;
constexpr double K_QUAT_X_SCALE = 0.5;
constexpr double K_QUAT_Y_SCALE = -0.25;
constexpr double K_QUAT_Z_SCALE = 0.125;

// --- Second Quaternion (MULTIPLY operand) ---
constexpr double K_QUAT_B_W = 0.5;
constexpr double K_QUAT_B_X = 0.5;
constexpr double K_QUAT_B_Y = 0.5;
constexpr double K_QUAT_B_Z = 0.5;

// --- Numeric Doubles ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;

// --- Square Root (Newton-Raphson) ---
constexpr double K_SQRT_HALF = 0.5;
constexpr size_t K_SQRT_ITERATIONS = 20;

// --- Unit Magnitude Tolerance ---
constexpr double K_QUAT_UNIT_TOL = 1e-9;

// --- Op Labels ---
const char* const K_OP_LABEL_MULTIPLY = "multiply";
const char* const K_OP_LABEL_CONJUGATE = "conjugate";
const char* const K_OP_LABEL_NORMALIZE = "normalize";

// --- Op Descriptions ---
const char* const K_OP_DESC_MULTIPLY = "multiply input quaternion by a fixed unit quaternion";
const char* const K_OP_DESC_CONJUGATE = "negate the vector part of the input quaternion";
const char* const K_OP_DESC_NORMALIZE = "scale the quaternion to unit magnitude";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class QuatOp {
    MULTIPLY,
    CONJUGATE,
    NORMALIZE,
};

struct QuaternionCaseSpec {
    std::string name;
    QuatOp op;
    double w;
    double x;
    double y;
    double z;
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

QuaternionCaseSpec GetQuaternionCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    const double cn = static_cast<double>(caseNumber);
    QuaternionCaseSpec spec;
    spec.name = BuildIndexedName("quaternionCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.w = cn * K_QUAT_W_SCALE;
    spec.x = cn * K_QUAT_X_SCALE;
    spec.y = cn * K_QUAT_Y_SCALE;
    spec.z = cn * K_QUAT_Z_SCALE;
    spec.op = QuatOp::MULTIPLY;
    const size_t opCycle = caseNumber % K_QUAT_OP_CYCLE;
    if (opCycle == K_OP_INDEX_MULTIPLY) {
        spec.op = QuatOp::MULTIPLY;
    } else if (opCycle == K_OP_INDEX_CONJUGATE) {
        spec.op = QuatOp::CONJUGATE;
    } else {
        spec.op = QuatOp::NORMALIZE;
    }
    return spec;
}

const char* OpLabel(QuatOp op)
{
    if (op == QuatOp::MULTIPLY) {
        return K_OP_LABEL_MULTIPLY;
    }
    if (op == QuatOp::CONJUGATE) {
        return K_OP_LABEL_CONJUGATE;
    }
    return K_OP_LABEL_NORMALIZE;
}

const char* OpDescription(QuatOp op)
{
    if (op == QuatOp::MULTIPLY) {
        return K_OP_DESC_MULTIPLY;
    }
    if (op == QuatOp::CONJUGATE) {
        return K_OP_DESC_CONJUGATE;
    }
    return K_OP_DESC_NORMALIZE;
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

// === Helpers: Quaternion Math Primitives ===

double QuatSqrt(double value)
{
    if (value <= K_DOUBLE_ZERO) {
        return K_DOUBLE_ZERO;
    }
    double estimate = value;
    for (size_t i = 0; i < K_SQRT_ITERATIONS; i++) {
        estimate = K_SQRT_HALF * (estimate + value / estimate);
    }
    return estimate;
}

double QuatMagnitude(double w, double x, double y, double z)
{
    const double sum = w * w + x * x + y * y + z * z;
    return QuatSqrt(sum);
}

void QuatMultiply(double aw, double ax, double ay, double az,
    double bw, double bx, double by, double bz,
    double* outW, double* outX, double* outY, double* outZ)
{
    *outW = aw * bw - ax * bx - ay * by - az * bz;
    *outX = aw * bx + ax * bw + ay * bz - az * by;
    *outY = aw * by - ax * bz + ay * bw + az * bx;
    *outZ = aw * bz + ax * by - ay * bx + az * bw;
}

void QuatConjugate(double w, double x, double y, double z,
    double* outW, double* outX, double* outY, double* outZ)
{
    *outW = w;
    *outX = -x;
    *outY = -y;
    *outZ = -z;
}

void QuatNormalize(double w, double x, double y, double z,
    double* outW, double* outX, double* outY, double* outZ)
{
    const double mag = QuatMagnitude(w, x, y, z);
    const double safeMag = (mag > K_DOUBLE_ZERO) ? mag : K_DOUBLE_ONE;
    *outW = w / safeMag;
    *outX = x / safeMag;
    *outY = y / safeMag;
    *outZ = z / safeMag;
}

// === Helpers: Op Dispatch ===

void ComputeQuaternionOp(const QuaternionCaseSpec& spec,
    double* outW, double* outX, double* outY, double* outZ)
{
    if (spec.op == QuatOp::MULTIPLY) {
        QuatMultiply(spec.w, spec.x, spec.y, spec.z,
            K_QUAT_B_W, K_QUAT_B_X, K_QUAT_B_Y, K_QUAT_B_Z,
            outW, outX, outY, outZ);
        return;
    }
    if (spec.op == QuatOp::CONJUGATE) {
        QuatConjugate(spec.w, spec.x, spec.y, spec.z, outW, outX, outY, outZ);
        return;
    }
    QuatNormalize(spec.w, spec.x, spec.y, spec.z, outW, outX, outY, outZ);
}

bool IsUnitMagnitude(double magnitude)
{
    double diff = magnitude - K_DOUBLE_ONE;
    if (diff < K_DOUBLE_ZERO) {
        diff = -diff;
    }
    return diff < K_QUAT_UNIT_TOL;
}

// === Result Building ===

napi_value BuildQuaternionResult(napi_env env, const QuaternionCaseSpec& spec,
    double outW, double outX, double outY, double outZ,
    double magIn, double magOut, bool isUnitOutput)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedDouble(env, result, "wIn", spec.w);
    SetNamedDouble(env, result, "xIn", spec.x);
    SetNamedDouble(env, result, "yIn", spec.y);
    SetNamedDouble(env, result, "zIn", spec.z);
    SetNamedDouble(env, result, "wOut", outW);
    SetNamedDouble(env, result, "xOut", outX);
    SetNamedDouble(env, result, "yOut", outY);
    SetNamedDouble(env, result, "zOut", outZ);
    SetNamedDouble(env, result, "magnitudeIn", magIn);
    SetNamedDouble(env, result, "magnitudeOut", magOut);
    SetNamedBool(env, result, "isUnitOutput", isUnitOutput);
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    return result;
}

// === Test Entry ===

static napi_value RunQuaternionCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_QUATERNION_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid quaternion case");
        return nullptr;
    }
    const auto spec = GetQuaternionCaseSpec(caseIndex);
    double outW = K_DOUBLE_ZERO;
    double outX = K_DOUBLE_ZERO;
    double outY = K_DOUBLE_ZERO;
    double outZ = K_DOUBLE_ZERO;
    ComputeQuaternionOp(spec, &outW, &outX, &outY, &outZ);
    const double magIn = QuatMagnitude(spec.w, spec.x, spec.y, spec.z);
    const double magOut = QuatMagnitude(outW, outX, outY, outZ);
    const bool isUnitOutput = IsUnitMagnitude(magOut);
    return BuildQuaternionResult(env, spec, outW, outX, outY, outZ, magIn, magOut, isUnitOutput);
}

}  // namespace

static napi_value InitQuaternionSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_QUATERNION_CASE_COUNT);
    exportNames.reserve(K_QUATERNION_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_QUATERNION_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testQuaternionCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunQuaternionCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_quaternionSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitQuaternionSuite,
    .nm_modname = "quaternion_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterQuaternionSuiteModule(void)
{
    napi_module_register(&g_quaternionSuiteModule);
}
