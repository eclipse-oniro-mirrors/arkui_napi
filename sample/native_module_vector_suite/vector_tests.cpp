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
constexpr size_t K_VEC_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Vector Operation Configuration ---
constexpr size_t K_VEC_OP_CYCLE = 3;
constexpr size_t K_VEC_OP_INDEX_DOT = 0;
constexpr size_t K_VEC_OP_INDEX_CROSS = 1;
constexpr size_t K_VEC_OP_INDEX_MAGNITUDE = 2;

// --- Vector Component Scales ---
constexpr double K_VEC_SCALE_A_X = 1.0;
constexpr double K_VEC_SCALE_A_Y = 2.0;
constexpr double K_VEC_SCALE_A_Z = 3.0;
constexpr double K_VEC_SCALE_B_X = 0.5;
constexpr double K_VEC_SCALE_B_Y = 1.5;
constexpr double K_VEC_SCALE_B_Z = 2.5;

// --- Vector Math Values ---
constexpr double K_VEC_MAGNITUDE_INIT = 0.0;
constexpr double K_VEC_MAGNITUDE_ZERO = 0.0;
constexpr double K_VEC_SQRT_HALF = 0.5;
constexpr double K_VEC_SQRT_TOLERANCE = 1e-12;
constexpr size_t K_VEC_SQRT_ITERATIONS = 20;
constexpr double K_VEC_UNIT_FALLBACK = 1.0;

// --- Vector Auxiliary Values ---
constexpr double K_VEC_SQUARED_INIT = 0.0;
constexpr double K_VEC_COMPONENT_SUM_INIT = 0.0;
constexpr double K_VEC_ANGLE_UNDEFINED = 0.0;
constexpr size_t K_VEC_COMPONENT_COUNT = 3;

// --- Operation Labels ---
const char* const K_OP_LABEL_DOT = "dot";
const char* const K_OP_LABEL_CROSS = "cross";
const char* const K_OP_LABEL_MAGNITUDE = "magnitude";

// --- Operation Descriptions ---
const char* const K_OP_DESC_DOT = "compute the dot product of two 3D vectors";
const char* const K_OP_DESC_CROSS = "compute the cross product of two 3D vectors";
const char* const K_OP_DESC_MAGNITUDE = "compute the magnitude of the first 3D vector";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class VecOp {
    DOT,
    CROSS,
    MAGNITUDE,
};

struct VectorCaseSpec {
    std::string name;
    VecOp op;
    size_t caseNumber;
    double ax;
    double ay;
    double az;
    double bx;
    double by;
    double bz;
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

VectorCaseSpec GetVectorCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    VectorCaseSpec spec;
    spec.name = BuildIndexedName("vectorCase", caseNumber);
    spec.caseNumber = caseNumber;
    const double n = static_cast<double>(caseNumber);
    spec.ax = n * K_VEC_SCALE_A_X;
    spec.ay = n * K_VEC_SCALE_A_Y;
    spec.az = n * K_VEC_SCALE_A_Z;
    spec.bx = n * K_VEC_SCALE_B_X;
    spec.by = n * K_VEC_SCALE_B_Y;
    spec.bz = n * K_VEC_SCALE_B_Z;
    const size_t cycle = caseNumber % K_VEC_OP_CYCLE;
    if (cycle == K_VEC_OP_INDEX_DOT) {
        spec.op = VecOp::DOT;
    } else if (cycle == K_VEC_OP_INDEX_CROSS) {
        spec.op = VecOp::CROSS;
    } else {
        spec.op = VecOp::MAGNITUDE;
    }
    return spec;
}

const char* VecOpLabel(VecOp op)
{
    if (op == VecOp::DOT) {
        return K_OP_LABEL_DOT;
    }
    if (op == VecOp::CROSS) {
        return K_OP_LABEL_CROSS;
    }
    return K_OP_LABEL_MAGNITUDE;
}

const char* VecOpDescription(VecOp op)
{
    if (op == VecOp::DOT) {
        return K_OP_DESC_DOT;
    }
    if (op == VecOp::CROSS) {
        return K_OP_DESC_CROSS;
    }
    return K_OP_DESC_MAGNITUDE;
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

// === Helpers: Vector Math ===

double VecSqrt(double value)
{
    if (value <= K_VEC_SQRT_TOLERANCE) {
        return K_VEC_MAGNITUDE_ZERO;
    }
    double guess = value;
    for (size_t i = 0; i < K_VEC_SQRT_ITERATIONS; i++) {
        const double safeGuess = (guess > K_VEC_MAGNITUDE_ZERO) ? guess : K_VEC_UNIT_FALLBACK;
        guess = K_VEC_SQRT_HALF * (guess + value / safeGuess);
    }
    return guess;
}

double VecDot(double ax, double ay, double az, double bx, double by, double bz)
{
    return ax * bx + ay * by + az * bz;
}

void VecCross(double ax, double ay, double az, double bx, double by, double bz,
    double* outX, double* outY, double* outZ)
{
    *outX = ay * bz - az * by;
    *outY = az * bx - ax * bz;
    *outZ = ax * by - ay * bx;
}

double VecMagnitude(double x, double y, double z)
{
    const double sum = x * x + y * y + z * z;
    return VecSqrt(sum);
}

void VecNormalize(double x, double y, double z, double* outX, double* outY, double* outZ)
{
    const double mag = VecMagnitude(x, y, z);
    const double safeMag = (mag > K_VEC_MAGNITUDE_ZERO) ? mag : K_VEC_UNIT_FALLBACK;
    *outX = x / safeMag;
    *outY = y / safeMag;
    *outZ = z / safeMag;
}

double VecSqrMagnitude(double x, double y, double z)
{
    return x * x + y * y + z * z;
}

double VecComponentSum(double x, double y, double z)
{
    return x + y + z;
}

double VecAngleCosine(double ax, double ay, double az, double bx, double by, double bz)
{
    const double magA = VecMagnitude(ax, ay, az);
    const double magB = VecMagnitude(bx, by, bz);
    if (magA <= K_VEC_SQRT_TOLERANCE || magB <= K_VEC_SQRT_TOLERANCE) {
        return K_VEC_ANGLE_UNDEFINED;
    }
    return VecDot(ax, ay, az, bx, by, bz) / (magA * magB);
}

// === Result Building ===

napi_value BuildVectorResult(napi_env env, const VectorCaseSpec& spec, double dot,
    double crossX, double crossY, double crossZ)
{
    const double magnitude = VecMagnitude(spec.ax, spec.ay, spec.az);
    const double squaredMagnitude = VecSqrMagnitude(spec.ax, spec.ay, spec.az);
    const double componentSumA = VecComponentSum(spec.ax, spec.ay, spec.az);
    const double cosAngle = VecAngleCosine(spec.ax, spec.ay, spec.az,
        spec.bx, spec.by, spec.bz);
    double normX = K_VEC_MAGNITUDE_ZERO;
    double normY = K_VEC_MAGNITUDE_ZERO;
    double normZ = K_VEC_MAGNITUDE_ZERO;
    VecNormalize(spec.ax, spec.ay, spec.az, &normX, &normY, &normZ);
    const bool isDegenerate = (magnitude <= K_VEC_SQRT_TOLERANCE);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", VecOpLabel(spec.op));
    SetNamedString(env, result, "description", VecOpDescription(spec.op));
    SetNamedInt32(env, result, "seed", static_cast<int32_t>(spec.caseNumber));
    SetNamedDouble(env, result, "ax", spec.ax);
    SetNamedDouble(env, result, "ay", spec.ay);
    SetNamedDouble(env, result, "az", spec.az);
    SetNamedDouble(env, result, "bx", spec.bx);
    SetNamedDouble(env, result, "by", spec.by);
    SetNamedDouble(env, result, "bz", spec.bz);
    SetNamedDouble(env, result, "magnitude", magnitude);
    SetNamedDouble(env, result, "squaredMagnitude", squaredMagnitude);
    SetNamedDouble(env, result, "componentSumA", componentSumA);
    SetNamedDouble(env, result, "normalizedX", normX);
    SetNamedDouble(env, result, "normalizedY", normY);
    SetNamedDouble(env, result, "normalizedZ", normZ);
    SetNamedBool(env, result, "isDegenerate", isDegenerate);
    if (spec.op == VecOp::DOT) {
        SetNamedDouble(env, result, "dot", dot);
        SetNamedDouble(env, result, "cosAngle", cosAngle);
    } else if (spec.op == VecOp::CROSS) {
        SetNamedDouble(env, result, "resultX", crossX);
        SetNamedDouble(env, result, "resultY", crossY);
        SetNamedDouble(env, result, "resultZ", crossZ);
    }
    return result;
}

// === Test Entry ===

static napi_value RunVectorCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_VEC_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid vector case");
        return nullptr;
    }
    const auto spec = GetVectorCaseSpec(caseIndex);
    double dot = K_VEC_MAGNITUDE_ZERO;
    double crossX = K_VEC_MAGNITUDE_ZERO;
    double crossY = K_VEC_MAGNITUDE_ZERO;
    double crossZ = K_VEC_MAGNITUDE_ZERO;
    if (spec.op == VecOp::DOT) {
        dot = VecDot(spec.ax, spec.ay, spec.az, spec.bx, spec.by, spec.bz);
    } else if (spec.op == VecOp::CROSS) {
        VecCross(spec.ax, spec.ay, spec.az, spec.bx, spec.by, spec.bz, &crossX, &crossY, &crossZ);
    }
    return BuildVectorResult(env, spec, dot, crossX, crossY, crossZ);
}

}  // namespace

static napi_value InitVectorSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_VEC_CASE_COUNT);
    exportNames.reserve(K_VEC_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_VEC_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testVectorCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunVectorCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_vectorSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitVectorSuite,
    .nm_modname = "vector_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterVectorSuiteModule(void)
{
    napi_module_register(&g_vectorSuiteModule);
}
