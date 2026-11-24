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
constexpr size_t K_PRESSURE_CASE_COUNT = 27;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Pressure Op Cycle ---
constexpr size_t K_PRESSURE_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_PA_TO_BAR = 0;
constexpr size_t K_OP_INDEX_BAR_TO_POUND_PER_SQ_IN = 1;
constexpr size_t K_OP_INDEX_POUND_PER_SQ_IN_TO_KPA = 2;

// --- Pressure Base Doubles ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;
constexpr double K_ROUND_TOLERANCE = 1.0e-9;

// --- Pressure Conversion Factors ---
constexpr double K_PA_PER_BAR = 100000.0;
constexpr double K_BAR_PER_PA = K_DOUBLE_ONE / K_PA_PER_BAR;
constexpr double K_POUND_PER_SQ_IN_PER_BAR = 14.5038;
constexpr double K_BAR_PER_POUND_PER_SQ_IN = K_DOUBLE_ONE / K_POUND_PER_SQ_IN_PER_BAR;
constexpr double K_KPA_PER_POUND_PER_SQ_IN = 6.89476;
constexpr double K_POUND_PER_SQ_IN_PER_KPA = K_DOUBLE_ONE / K_KPA_PER_POUND_PER_SQ_IN;
constexpr double K_PRESSURE_STEP = 1000.0;

// --- Extended Pressure Conversion Factors ---
constexpr double K_PA_PER_MMHG = 133.322;
constexpr double K_MMHG_PER_PA = K_DOUBLE_ONE / K_PA_PER_MMHG;
constexpr double K_PA_PER_ATM = 101325.0;
constexpr double K_ATM_PER_PA = K_DOUBLE_ONE / K_PA_PER_ATM;
constexpr double K_PA_PER_KPA = 1000.0;
constexpr double K_KPA_PER_PA = K_DOUBLE_ONE / K_PA_PER_KPA;
constexpr double K_KPA_PER_BAR = K_PA_PER_BAR / K_PA_PER_KPA;

// --- Pressure Reference Scales ---
constexpr double K_HIGH_PRESSURE_THRESHOLD_PA = 200000.0;
constexpr double K_VACUUM_THRESHOLD_PA = 1000.0;

// --- Op Labels ---
const char* const K_OP_LABEL_PA_TO_BAR = "pa_to_bar";
const char* const K_OP_LABEL_BAR_TO_POUND_PER_SQ_IN = "bar_to_pound_per_sq_in";
const char* const K_OP_LABEL_POUND_PER_SQ_IN_TO_KPA = "pound_per_sq_in_to_kpa";

// --- Op Descriptions ---
const char* const K_OP_DESC_PA_TO_BAR = "convert pascals to bars via constant divisor";
const char* const K_OP_DESC_BAR_TO_POUND_PER_SQ_IN = "convert bars to pound_per_sq_in via multiplication by factor";
const char* const K_OP_DESC_POUND_PER_SQ_IN_TO_KPA =
    "convert pound_per_sq_in to kilopascals via multiplication by factor";

// --- Unit Labels ---
const char* const K_UNIT_LABEL_PASCAL = "pascals";
const char* const K_UNIT_LABEL_BAR = "bars";
const char* const K_UNIT_LABEL_POUND_PER_SQ_IN = "pound_per_sq_in";
const char* const K_UNIT_LABEL_KPA = "kilopascals";

// --- Domain & Category Labels ---
const char* const K_DOMAIN_LABEL = "unit_conversion";
const char* const K_CATEGORY_LABEL = "pressure";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class PressureOp {
    PA_TO_BAR,
    BAR_TO_POUND_PER_SQ_IN,
    POUND_PER_SQ_IN_TO_KPA,
};

struct PressureCaseSpec {
    std::string name;
    PressureOp op;
    size_t caseNumber;
    double value;
};

struct PressureDerived {
    double atmEquivalent;
    double mmHgEquivalent;
    double kpaValue;
    double poundPerSqInEquivalent;
    bool isHighPressure;
    bool isVacuum;
    bool mmHgRoundTrip;
    bool atmRoundTrip;
    bool kpaCrossPath;
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

PressureOp ResolvePressureOp(size_t caseNumber)
{
    const size_t cycle = caseNumber % K_PRESSURE_OP_CYCLE;
    if (cycle == K_OP_INDEX_PA_TO_BAR) {
        return PressureOp::PA_TO_BAR;
    }
    if (cycle == K_OP_INDEX_BAR_TO_POUND_PER_SQ_IN) {
        return PressureOp::BAR_TO_POUND_PER_SQ_IN;
    }
    return PressureOp::POUND_PER_SQ_IN_TO_KPA;
}

PressureCaseSpec GetPressureCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    PressureCaseSpec spec;
    spec.name = BuildIndexedName("pressureCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.value = static_cast<double>(caseNumber) * K_PRESSURE_STEP;
    spec.op = ResolvePressureOp(caseNumber);
    return spec;
}

const char* OpLabel(PressureOp op)
{
    if (op == PressureOp::PA_TO_BAR) {
        return K_OP_LABEL_PA_TO_BAR;
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return K_OP_LABEL_BAR_TO_POUND_PER_SQ_IN;
    }
    return K_OP_LABEL_POUND_PER_SQ_IN_TO_KPA;
}

const char* OpDescription(PressureOp op)
{
    if (op == PressureOp::PA_TO_BAR) {
        return K_OP_DESC_PA_TO_BAR;
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return K_OP_DESC_BAR_TO_POUND_PER_SQ_IN;
    }
    return K_OP_DESC_POUND_PER_SQ_IN_TO_KPA;
}

const char* SourceUnitLabel(PressureOp op)
{
    if (op == PressureOp::PA_TO_BAR) {
        return K_UNIT_LABEL_PASCAL;
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return K_UNIT_LABEL_BAR;
    }
    return K_UNIT_LABEL_POUND_PER_SQ_IN;
}

const char* TargetUnitLabel(PressureOp op)
{
    if (op == PressureOp::PA_TO_BAR) {
        return K_UNIT_LABEL_BAR;
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return K_UNIT_LABEL_POUND_PER_SQ_IN;
    }
    return K_UNIT_LABEL_KPA;
}

double ConversionFactor(PressureOp op)
{
    if (op == PressureOp::PA_TO_BAR) {
        return K_BAR_PER_PA;
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return K_POUND_PER_SQ_IN_PER_BAR;
    }
    return K_KPA_PER_POUND_PER_SQ_IN;
}

// === Helpers: Object Property Setters ===

bool SetNamedDouble(napi_env env, napi_value object, const char* name, double value)
{
    napi_value napiValue = nullptr;
    if (napi_create_double(env, value, &napiValue) != napi_ok) {
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

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
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

// === Helpers: Pressure Conversions ===

double PascalToBar(double pascals)
{
    return pascals / K_PA_PER_BAR;
}

double BarToPoundPerSqIn(double bars)
{
    return bars * K_POUND_PER_SQ_IN_PER_BAR;
}

double PoundPerSqInToKPa(double poundPerSqIn)
{
    return poundPerSqIn * K_KPA_PER_POUND_PER_SQ_IN;
}

double PascalToMmHg(double pascals)
{
    return pascals * K_MMHG_PER_PA;
}

double MmHgToPascal(double mmHg)
{
    return mmHg * K_PA_PER_MMHG;
}

double AtmosphereToPascal(double atm)
{
    return atm * K_PA_PER_ATM;
}

double BarToKPa(double bars)
{
    return bars * K_KPA_PER_BAR;
}

double KPaToPoundPerSqIn(double kpa)
{
    return kpa * K_POUND_PER_SQ_IN_PER_KPA;
}

double ConvertByOp(PressureOp op, double value)
{
    if (op == PressureOp::PA_TO_BAR) {
        return PascalToBar(value);
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return BarToPoundPerSqIn(value);
    }
    return PoundPerSqInToKPa(value);
}

double InverseConvert(PressureOp op, double output)
{
    if (op == PressureOp::PA_TO_BAR) {
        return output * K_PA_PER_BAR;
    }
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        return output / K_POUND_PER_SQ_IN_PER_BAR;
    }
    return output / K_KPA_PER_POUND_PER_SQ_IN;
}

bool IsClose(double left, double right)
{
    const double diff = left - right;
    const double absDiff = (diff < K_DOUBLE_ZERO) ? -diff : diff;
    return absDiff < K_ROUND_TOLERANCE;
}

bool RoundTripMatches(PressureOp op, double originalInput, double output)
{
    const double roundTrip = InverseConvert(op, output);
    return IsClose(originalInput, roundTrip);
}

bool IsOutputSane(double input, double output)
{
    if (input <= K_DOUBLE_ZERO) {
        return output == K_DOUBLE_ZERO;
    }
    return output > K_DOUBLE_ZERO;
}

PressureDerived ComputePressureDerived(PressureOp op, double value)
{
    PressureDerived derived;
    double pascals = value;
    if (op == PressureOp::BAR_TO_POUND_PER_SQ_IN) {
        pascals = value * K_PA_PER_BAR;
    } else if (op == PressureOp::POUND_PER_SQ_IN_TO_KPA) {
        pascals = PoundPerSqInToKPa(value) * K_PA_PER_KPA;
    }
    derived.atmEquivalent = pascals * K_ATM_PER_PA;
    derived.mmHgEquivalent = PascalToMmHg(pascals);
    derived.kpaValue = pascals * K_KPA_PER_PA;
    derived.poundPerSqInEquivalent = KPaToPoundPerSqIn(derived.kpaValue);
    derived.isHighPressure = pascals > K_HIGH_PRESSURE_THRESHOLD_PA;
    derived.isVacuum = pascals < K_VACUUM_THRESHOLD_PA;
    derived.mmHgRoundTrip = IsClose(pascals, MmHgToPascal(PascalToMmHg(pascals)));
    derived.atmRoundTrip = IsClose(pascals, AtmosphereToPascal(derived.atmEquivalent));
    derived.kpaCrossPath = IsClose(derived.kpaValue, BarToKPa(PascalToBar(pascals)));
    return derived;
}

// === Result Building ===

napi_value BuildPressureResult(napi_env env, const PressureCaseSpec& spec, double output, double factor,
    bool roundTrip, bool isSane, const PressureDerived& derived)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "category", K_CATEGORY_LABEL);
    SetNamedString(env, result, "domain", K_DOMAIN_LABEL);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedString(env, result, "sourceUnit", SourceUnitLabel(spec.op));
    SetNamedString(env, result, "targetUnit", TargetUnitLabel(spec.op));
    SetNamedInt32(env, result, "caseNumber", static_cast<int32_t>(spec.caseNumber));
    SetNamedDouble(env, result, "input", spec.value);
    SetNamedDouble(env, result, "output", output);
    SetNamedDouble(env, result, "factor", factor);
    SetNamedBool(env, result, "roundTrip", roundTrip);
    SetNamedBool(env, result, "isSane", isSane);
    SetNamedDouble(env, result, "atmEquivalent", derived.atmEquivalent);
    SetNamedDouble(env, result, "mmHgEquivalent", derived.mmHgEquivalent);
    SetNamedDouble(env, result, "kpaValue", derived.kpaValue);
    SetNamedDouble(env, result, "poundPerSqInEquivalent", derived.poundPerSqInEquivalent);
    SetNamedBool(env, result, "isHighPressure", derived.isHighPressure);
    SetNamedBool(env, result, "isVacuum", derived.isVacuum);
    SetNamedBool(env, result, "mmHgRoundTrip", derived.mmHgRoundTrip);
    SetNamedBool(env, result, "atmRoundTrip", derived.atmRoundTrip);
    SetNamedBool(env, result, "kpaCrossPath", derived.kpaCrossPath);
    return result;
}

// === Test Entry ===

static napi_value RunPressureCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_PRESSURE_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid pressure case");
        return nullptr;
    }
    const auto spec = GetPressureCaseSpec(caseIndex);
    const double output = ConvertByOp(spec.op, spec.value);
    const double factor = ConversionFactor(spec.op);
    const bool roundTrip = RoundTripMatches(spec.op, spec.value, output);
    const bool isSane = IsOutputSane(spec.value, output);
    const PressureDerived derived = ComputePressureDerived(spec.op, spec.value);
    return BuildPressureResult(env, spec, output, factor, roundTrip, isSane, derived);
}

}  // namespace

static napi_value InitPressureSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_PRESSURE_CASE_COUNT);
    exportNames.reserve(K_PRESSURE_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_PRESSURE_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testPressureCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunPressureCase,
            nullptr, nullptr, nullptr, napi_default,
            reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_pressureSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitPressureSuite,
    .nm_modname = "pressure_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterPressureSuiteModule(void)
{
    napi_module_register(&g_pressureSuiteModule);
}
