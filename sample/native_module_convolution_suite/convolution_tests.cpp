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
constexpr size_t K_CONV_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Convolution Configuration ---
constexpr size_t K_CONV_OP_CYCLE = 2;
constexpr size_t K_CONV_MAX_SIGNAL = 8;
constexpr size_t K_CONV_MAX_KERNEL = 4;
constexpr size_t K_CONV_MAX_OUTPUT = K_CONV_MAX_SIGNAL + K_CONV_MAX_KERNEL - 1;
constexpr size_t K_CONV_ZERO = 0;
constexpr size_t K_CONV_OP_INDEX_CONVOLVE = 0;
constexpr size_t K_CONV_OP_INDEX_MOVING_AVERAGE = 1;

// --- Signal Generation ---
constexpr double K_CONV_SIGNAL_STEP = 1.5;

// --- Kernel Generation ---
constexpr double K_CONV_KERNEL_BASE = 0.5;

// --- Test Values (Doubles) ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;

// --- Op Labels ---
const char* const K_CONV_OP_LABEL_CONVOLVE = "convolve";
const char* const K_CONV_OP_LABEL_MOVING_AVERAGE = "moving_average";

// --- Op Descriptions ---
const char* const K_CONV_OP_DESC_CONVOLVE = "1D convolution of signal with kernel";
const char* const K_CONV_OP_DESC_MOVING_AVERAGE = "moving average of signal over window";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class ConvOp {
    CONVOLVE,
    MOVING_AVERAGE,
};

struct ConvCaseSpec {
    std::string name;
    ConvOp op;
    size_t signalLen;
    size_t kernelLen;
    double signal[K_CONV_MAX_SIGNAL];
    double kernel[K_CONV_MAX_KERNEL];
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

void FillSignal(ConvCaseSpec& spec, size_t caseNumber)
{
    for (size_t i = 0; i < spec.signalLen; i++) {
        spec.signal[i] = static_cast<double>(caseNumber + i) * K_CONV_SIGNAL_STEP;
    }
}

void FillKernel(ConvCaseSpec& spec)
{
    for (size_t i = 0; i < spec.kernelLen; i++) {
        spec.kernel[i] = K_CONV_KERNEL_BASE + static_cast<double>(i);
    }
}

ConvCaseSpec GetConvCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    ConvCaseSpec spec;
    spec.name = BuildIndexedName("convolutionCase", caseNumber);
    spec.signalLen = caseNumber % K_CONV_MAX_SIGNAL + 1;
    spec.kernelLen = caseNumber % K_CONV_MAX_KERNEL + 1;
    const size_t opIndex = caseNumber % K_CONV_OP_CYCLE;
    spec.op = (opIndex == K_CONV_OP_INDEX_CONVOLVE) ? ConvOp::CONVOLVE : ConvOp::MOVING_AVERAGE;
    if (spec.op == ConvOp::CONVOLVE && spec.signalLen < spec.kernelLen) {
        spec.signalLen = spec.kernelLen;
    }
    FillSignal(spec, caseNumber);
    FillKernel(spec);
    return spec;
}

const char* OpLabel(ConvOp op)
{
    if (op == ConvOp::CONVOLVE) {
        return K_CONV_OP_LABEL_CONVOLVE;
    }
    return K_CONV_OP_LABEL_MOVING_AVERAGE;
}

const char* OpDescription(ConvOp op)
{
    if (op == ConvOp::CONVOLVE) {
        return K_CONV_OP_DESC_CONVOLVE;
    }
    return K_CONV_OP_DESC_MOVING_AVERAGE;
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

bool SetNamedDouble(napi_env env, napi_value object, const char* name, double value)
{
    napi_value napiValue = nullptr;
    if (napi_create_double(env, value, &napiValue) != napi_ok) {
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

// === Helpers: Convolution & Moving Average ===

double ConvolveAt(const double* sig, size_t sigLen, const double* ker, size_t kerLen, size_t k)
{
    double sum = K_DOUBLE_ZERO;
    for (size_t i = 0; i < sigLen; i++) {
        if (k >= i && (k - i) < kerLen) {
            sum += sig[i] * ker[k - i];
        }
    }
    return sum;
}

size_t Convolve1D(const double* sig, size_t sigLen, const double* ker, size_t kerLen, double* out)
{
    if (sigLen == K_CONV_ZERO || kerLen == K_CONV_ZERO) {
        return K_CONV_ZERO;
    }
    const size_t outLen = sigLen + kerLen - 1;
    for (size_t k = 0; k < outLen; k++) {
        out[k] = ConvolveAt(sig, sigLen, ker, kerLen, k);
    }
    return outLen;
}

size_t MovingAverage(const double* sig, size_t sigLen, size_t window, double* out)
{
    double safeWindow = (window > K_CONV_ZERO) ? static_cast<double>(window) : K_DOUBLE_ONE;
    const size_t outLen = (sigLen >= window) ? (sigLen - window + 1) : K_CONV_ZERO;
    for (size_t i = 0; i < outLen; i++) {
        double sum = K_DOUBLE_ZERO;
        for (size_t j = 0; j < window; j++) {
            sum += sig[i + j];
        }
        out[i] = sum / safeWindow;
    }
    return outLen;
}

// === Helpers: Output Statistics ===

double ComputeArraySum(const double* values, size_t count)
{
    double sum = K_DOUBLE_ZERO;
    for (size_t i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum;
}

double ComputeArrayMean(const double* values, size_t count)
{
    const double sum = ComputeArraySum(values, count);
    double safeCount = (count > K_CONV_ZERO) ? static_cast<double>(count) : K_DOUBLE_ONE;
    return sum / safeCount;
}

double ComputeArrayMin(const double* values, size_t count)
{
    if (count == K_CONV_ZERO) {
        return K_DOUBLE_ZERO;
    }
    double minVal = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] < minVal) {
            minVal = values[i];
        }
    }
    return minVal;
}

double ComputeArrayMax(const double* values, size_t count)
{
    if (count == K_CONV_ZERO) {
        return K_DOUBLE_ZERO;
    }
    double maxVal = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] > maxVal) {
            maxVal = values[i];
        }
    }
    return maxVal;
}

// === Helpers: Validation & Expected Length ===

bool ValidateSpec(const ConvCaseSpec& spec)
{
    if (spec.signalLen > K_CONV_MAX_SIGNAL) {
        return false;
    }
    if (spec.kernelLen > K_CONV_MAX_KERNEL) {
        return false;
    }
    if (spec.op == ConvOp::CONVOLVE && spec.signalLen < spec.kernelLen) {
        return false;
    }
    return true;
}

size_t ExpectedOutputLength(ConvOp op, size_t signalLen, size_t kernelLen)
{
    if (op == ConvOp::CONVOLVE) {
        return signalLen + kernelLen - 1;
    }
    return (signalLen >= kernelLen) ? (signalLen - kernelLen + 1) : K_CONV_ZERO;
}

// === Helpers: Output Dispatch & Array Building ===

size_t ComputeConvOutput(const ConvCaseSpec& spec, double* output)
{
    if (spec.op == ConvOp::CONVOLVE) {
        return Convolve1D(spec.signal, spec.signalLen, spec.kernel, spec.kernelLen, output);
    }
    return MovingAverage(spec.signal, spec.signalLen, spec.kernelLen, output);
}

napi_value BuildDoubleArray(napi_env env, const double* values, size_t count)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &array));
    for (size_t i = 0; i < count; i++) {
        napi_value elem = nullptr;
        NAPI_CALL(env, napi_create_double(env, values[i], &elem));
        NAPI_CALL(env, napi_set_element(env, array, static_cast<uint32_t>(i), elem));
    }
    return array;
}

// === Result Building ===

napi_value BuildConvResult(napi_env env, const ConvCaseSpec& spec, const double* output,
    size_t outputLen, double outputSum, double outputMean, double outputMin, double outputMax,
    size_t expectedOutputLen, bool lengthMatches)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", OpLabel(spec.op));
    SetNamedString(env, result, "description", OpDescription(spec.op));
    SetNamedInt32(env, result, "signalLen", static_cast<int32_t>(spec.signalLen));
    SetNamedInt32(env, result, "kernelLen", static_cast<int32_t>(spec.kernelLen));
    SetNamedInt32(env, result, "outputLen", static_cast<int32_t>(outputLen));
    SetNamedInt32(env, result, "expectedOutputLen", static_cast<int32_t>(expectedOutputLen));
    SetNamedDouble(env, result, "outputSum", outputSum);
    SetNamedDouble(env, result, "outputMean", outputMean);
    SetNamedDouble(env, result, "outputMin", outputMin);
    SetNamedDouble(env, result, "outputMax", outputMax);
    SetNamedBool(env, result, "lengthMatches", lengthMatches);
    napi_value signalArray = BuildDoubleArray(env, spec.signal, spec.signalLen);
    NAPI_CALL(env, napi_set_named_property(env, result, "signal", signalArray));
    napi_value kernelArray = BuildDoubleArray(env, spec.kernel, spec.kernelLen);
    NAPI_CALL(env, napi_set_named_property(env, result, "kernel", kernelArray));
    napi_value outputArray = BuildDoubleArray(env, output, outputLen);
    NAPI_CALL(env, napi_set_named_property(env, result, "output", outputArray));
    return result;
}

// === Test Entry ===

static napi_value RunConvCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_CONV_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid convolution case");
        return nullptr;
    }
    const auto spec = GetConvCaseSpec(caseIndex);
    if (!ValidateSpec(spec)) {
        napi_throw_error(env, nullptr, "invalid convolution spec");
        return nullptr;
    }
    double output[K_CONV_MAX_OUTPUT] = {K_DOUBLE_ZERO};
    const size_t outputLen = ComputeConvOutput(spec, output);
    const double outputSum = ComputeArraySum(output, outputLen);
    const double outputMean = ComputeArrayMean(output, outputLen);
    const double outputMin = ComputeArrayMin(output, outputLen);
    const double outputMax = ComputeArrayMax(output, outputLen);
    const size_t expectedOutputLen = ExpectedOutputLength(spec.op, spec.signalLen, spec.kernelLen);
    const bool lengthMatches = (outputLen == expectedOutputLen);
    return BuildConvResult(env, spec, output, outputLen, outputSum, outputMean, outputMin, outputMax,
        expectedOutputLen, lengthMatches);
}

}  // namespace

static napi_value InitConvolutionSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_CONV_CASE_COUNT);
    exportNames.reserve(K_CONV_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_CONV_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testConvolutionCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunConvCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_convolutionSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitConvolutionSuite,
    .nm_modname = "convolution_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterConvolutionSuiteModule(void)
{
    napi_module_register(&g_convolutionSuiteModule);
}
