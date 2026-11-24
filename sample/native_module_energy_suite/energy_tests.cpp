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
constexpr size_t K_ENERGY_CASE_COUNT = 27;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Energy Op Cycle ---
constexpr size_t K_ENERGY_OP_CYCLE = 3;
constexpr size_t K_OP_INDEX_JOULE_TO_CALORIE = 0;
constexpr size_t K_OP_INDEX_CALORIE_TO_JOULE = 1;
constexpr size_t K_OP_INDEX_JOULE_TO_KWH = 2;

// --- Energy Base Doubles ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;
constexpr double K_ROUND_TOLERANCE = 1.0e-9;

// --- Energy Conversion Factors ---
constexpr double K_CAL_PER_JOULE = 0.239005736;
constexpr double K_JOULE_PER_CAL = 4.184;
constexpr double K_JOULE_PER_KWH = 3.6e6;
constexpr double K_KWH_PER_JOULE = K_DOUBLE_ONE / K_JOULE_PER_KWH;
constexpr double K_ENERGY_STEP = 1000.0;

// --- Extended Energy Conversion Factors ---
constexpr double K_JOULE_PER_BTU = 1055.06;
constexpr double K_FOOT_POUND_PER_JOULE = 0.737562;

// --- Energy Reference Scales ---
constexpr double K_LARGE_SCALE_THRESHOLD_JOULE = 1000000.0;

// --- Op Labels ---
const char* const K_OP_LABEL_JOULE_TO_CALORIE = "joule_to_calorie";
const char* const K_OP_LABEL_CALORIE_TO_JOULE = "calorie_to_joule";
const char* const K_OP_LABEL_JOULE_TO_KWH = "joule_to_kwh";

// --- Op Descriptions ---
const char* const K_OP_DESC_JOULE_TO_CALORIE = "convert joules to calories via multiplication by factor";
const char* const K_OP_DESC_CALORIE_TO_JOULE = "convert calories to joules via multiplication by factor";
const char* const K_OP_DESC_JOULE_TO_KWH = "convert joules to kilowatt-hours by constant divisor";

// --- Unit Labels ---
const char* const K_UNIT_LABEL_JOULE = "joules";
const char* const K_UNIT_LABEL_CALORIE = "calories";
const char* const K_UNIT_LABEL_KWH = "kilowatt-hours";

// --- Domain & Category Labels ---
const char* const K_DOMAIN_LABEL = "unit_conversion";
const char* const K_CATEGORY_LABEL = "energy";

// --- Unit Category Labels ---
const char* const K_UNIT_CATEGORY_SI_MECHANICAL = "si_mechanical";
const char* const K_UNIT_CATEGORY_METRIC_THERMAL = "metric_thermal";
const char* const K_UNIT_CATEGORY_SI_ELECTRICAL = "si_electrical";

// --- Conversion Family Labels ---
const char* const K_CATEGORY_LABEL_MECH_TO_THERMAL = "mechanical_to_thermal";
const char* const K_CATEGORY_LABEL_THERMAL_TO_MECH = "thermal_to_mechanical";
const char* const K_CATEGORY_LABEL_MECH_TO_ELECTRICAL = "mechanical_to_electrical";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class EnergyOp {
    JOULE_TO_CALORIE,
    CALORIE_TO_JOULE,
    JOULE_TO_KWH,
};

struct EnergyCaseSpec {
    std::string name;
    EnergyOp op;
    size_t caseNumber;
    double value;
};

struct EnergyDerived {
    double jouleEquivalent;
    double btuEquivalent;
    double footPoundEquivalent;
    double kwhValue;
    bool isLargeScale;
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

EnergyOp ResolveEnergyOp(size_t caseNumber)
{
    const size_t cycle = caseNumber % K_ENERGY_OP_CYCLE;
    if (cycle == K_OP_INDEX_JOULE_TO_CALORIE) {
        return EnergyOp::JOULE_TO_CALORIE;
    }
    if (cycle == K_OP_INDEX_CALORIE_TO_JOULE) {
        return EnergyOp::CALORIE_TO_JOULE;
    }
    return EnergyOp::JOULE_TO_KWH;
}

EnergyCaseSpec GetEnergyCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    EnergyCaseSpec spec;
    spec.name = BuildIndexedName("energyCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.value = static_cast<double>(caseNumber) * K_ENERGY_STEP;
    spec.op = ResolveEnergyOp(caseNumber);
    return spec;
}

const char* OpLabel(EnergyOp op)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return K_OP_LABEL_JOULE_TO_CALORIE;
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_OP_LABEL_CALORIE_TO_JOULE;
    }
    return K_OP_LABEL_JOULE_TO_KWH;
}

const char* OpDescription(EnergyOp op)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return K_OP_DESC_JOULE_TO_CALORIE;
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_OP_DESC_CALORIE_TO_JOULE;
    }
    return K_OP_DESC_JOULE_TO_KWH;
}

const char* SourceUnitLabel(EnergyOp op)
{
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_UNIT_LABEL_CALORIE;
    }
    return K_UNIT_LABEL_JOULE;
}

const char* TargetUnitLabel(EnergyOp op)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return K_UNIT_LABEL_CALORIE;
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_UNIT_LABEL_JOULE;
    }
    return K_UNIT_LABEL_KWH;
}

double ConversionFactor(EnergyOp op)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return K_CAL_PER_JOULE;
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_JOULE_PER_CAL;
    }
    return K_KWH_PER_JOULE;
}

const char* InputCategoryLabel(EnergyOp op)
{
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_UNIT_CATEGORY_METRIC_THERMAL;
    }
    return K_UNIT_CATEGORY_SI_MECHANICAL;
}

const char* OutputCategoryLabel(EnergyOp op)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return K_UNIT_CATEGORY_METRIC_THERMAL;
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_UNIT_CATEGORY_SI_MECHANICAL;
    }
    return K_UNIT_CATEGORY_SI_ELECTRICAL;
}

const char* CategoryLabelForOp(EnergyOp op)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return K_CATEGORY_LABEL_MECH_TO_THERMAL;
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return K_CATEGORY_LABEL_THERMAL_TO_MECH;
    }
    return K_CATEGORY_LABEL_MECH_TO_ELECTRICAL;
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

// === Helpers: Energy Conversions ===

double JouleToCalorie(double joules)
{
    return joules * K_CAL_PER_JOULE;
}

double CalorieToJoule(double calories)
{
    return calories * K_JOULE_PER_CAL;
}

double JouleToKWh(double joules)
{
    return joules / K_JOULE_PER_KWH;
}

double JouleToBtu(double joules)
{
    return joules / K_JOULE_PER_BTU;
}

double JouleToFootPound(double joules)
{
    return joules * K_FOOT_POUND_PER_JOULE;
}

double ConvertByOp(EnergyOp op, double value)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return JouleToCalorie(value);
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return CalorieToJoule(value);
    }
    return JouleToKWh(value);
}

double InverseConvert(EnergyOp op, double output)
{
    if (op == EnergyOp::JOULE_TO_CALORIE) {
        return CalorieToJoule(output);
    }
    if (op == EnergyOp::CALORIE_TO_JOULE) {
        return JouleToCalorie(output);
    }
    return output * K_JOULE_PER_KWH;
}

bool IsClose(double left, double right)
{
    const double diff = left - right;
    const double absDiff = (diff < K_DOUBLE_ZERO) ? -diff : diff;
    return absDiff < K_ROUND_TOLERANCE;
}

bool RoundTripMatches(EnergyOp op, double originalInput, double output)
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

EnergyDerived ComputeEnergyDerived(EnergyOp op, double value)
{
    EnergyDerived derived;
    const double joules = (op == EnergyOp::CALORIE_TO_JOULE) ? CalorieToJoule(value) : value;
    derived.jouleEquivalent = joules;
    derived.btuEquivalent = JouleToBtu(joules);
    derived.footPoundEquivalent = JouleToFootPound(joules);
    derived.kwhValue = JouleToKWh(joules);
    derived.isLargeScale = joules > K_LARGE_SCALE_THRESHOLD_JOULE;
    return derived;
}

// === Result Building ===

napi_value BuildEnergyResult(napi_env env, const EnergyCaseSpec& spec, double output, double factor,
    bool roundTrip, bool isSane, const EnergyDerived& derived)
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
    SetNamedString(env, result, "categoryLabel", CategoryLabelForOp(spec.op));
    SetNamedString(env, result, "inputCategory", InputCategoryLabel(spec.op));
    SetNamedString(env, result, "outputCategory", OutputCategoryLabel(spec.op));
    SetNamedDouble(env, result, "jouleEquivalent", derived.jouleEquivalent);
    SetNamedDouble(env, result, "btuEquivalent", derived.btuEquivalent);
    SetNamedDouble(env, result, "footPoundEquivalent", derived.footPoundEquivalent);
    SetNamedDouble(env, result, "kwhValue", derived.kwhValue);
    SetNamedBool(env, result, "isLargeScale", derived.isLargeScale);
    return result;
}

// === Test Entry ===

static napi_value RunEnergyCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_ENERGY_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid energy case");
        return nullptr;
    }
    const auto spec = GetEnergyCaseSpec(caseIndex);
    const double output = ConvertByOp(spec.op, spec.value);
    const double factor = ConversionFactor(spec.op);
    const bool roundTrip = RoundTripMatches(spec.op, spec.value, output);
    const bool isSane = IsOutputSane(spec.value, output);
    const EnergyDerived derived = ComputeEnergyDerived(spec.op, spec.value);
    return BuildEnergyResult(env, spec, output, factor, roundTrip, isSane, derived);
}

}  // namespace

static napi_value InitEnergySuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_ENERGY_CASE_COUNT);
    exportNames.reserve(K_ENERGY_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_ENERGY_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testEnergyCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunEnergyCase,
            nullptr, nullptr, nullptr, napi_default,
            reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_energySuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitEnergySuite,
    .nm_modname = "energy_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterEnergySuiteModule(void)
{
    napi_module_register(&g_energySuiteModule);
}
