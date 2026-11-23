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
constexpr size_t K_COLOR_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Color Mode Configuration ---
constexpr size_t K_COLOR_MODE_CYCLE = 3;
constexpr size_t K_COLOR_MODE_INDEX_HSL = 0;
constexpr size_t K_COLOR_MODE_INDEX_HSV = 1;
constexpr size_t K_COLOR_MODE_INDEX_HEX = 2;

// --- Channel Derivation ---
constexpr size_t K_CHANNEL_MULTIPLIER_R = 7;
constexpr size_t K_CHANNEL_MULTIPLIER_G = 13;
constexpr size_t K_CHANNEL_MULTIPLIER_B = 19;
constexpr size_t K_CHANNEL_MOD = 256;
constexpr int32_t K_CHANNEL_MIN = 0;
constexpr int32_t K_CHANNEL_MAX = 255;

// --- Normalization & Hue Math ---
constexpr double K_DOUBLE_ZERO = 0.0;
constexpr double K_DOUBLE_ONE = 1.0;
constexpr double K_DOUBLE_TWO = 2.0;
constexpr double K_DOUBLE_THREE = 3.0;
constexpr double K_DOUBLE_SIX = 6.0;
constexpr double K_DOUBLE_SIXTY = 60.0;
constexpr double K_DOUBLE_THREE_SIXTY = 360.0;
constexpr double K_NORM_DIVISOR = 255.0;

// --- Hex Formatting ---
constexpr int K_HEX_HIGH_SHIFT = 4;
constexpr int K_HEX_LOW_MASK = 0x0F;
constexpr int K_HEX_COMPONENT_WIDTH = 2;
constexpr int K_HEX_NULL_TERMINATOR = 1;
const char* const K_HEX_PREFIX = "#";
const char* const K_HEX_DIGITS = "0123456789ABCDEF";

// --- Mode Labels ---
const char* const K_COLOR_MODE_LABEL_HSL = "rgb_to_hsl";
const char* const K_COLOR_MODE_LABEL_HSV = "rgb_to_hsv";
const char* const K_COLOR_MODE_LABEL_HEX = "hex_format";

// --- Mode Descriptions ---
const char* const K_COLOR_MODE_DESC_HSL = "convert rgb channels to hue-saturation-lightness";
const char* const K_COLOR_MODE_DESC_HSV = "convert rgb channels to hue-saturation-value";
const char* const K_COLOR_MODE_DESC_HEX = "format rgb channels as uppercase hex string";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class ColorMode {
    RGB_TO_HSL,
    RGB_TO_HSV,
    HEX_FORMAT,
};

struct ColorCaseSpec {
    std::string name;
    ColorMode mode;
    int32_t r;
    int32_t g;
    int32_t b;
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

int32_t ClampChannel(int32_t value)
{
    if (value < K_CHANNEL_MIN) {
        return K_CHANNEL_MIN;
    }
    if (value > K_CHANNEL_MAX) {
        return K_CHANNEL_MAX;
    }
    return value;
}

ColorCaseSpec GetColorCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    ColorCaseSpec spec;
    spec.name = BuildIndexedName("colorCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.r = ClampChannel(static_cast<int32_t>((caseNumber * K_CHANNEL_MULTIPLIER_R) % K_CHANNEL_MOD));
    spec.g = ClampChannel(static_cast<int32_t>((caseNumber * K_CHANNEL_MULTIPLIER_G) % K_CHANNEL_MOD));
    spec.b = ClampChannel(static_cast<int32_t>((caseNumber * K_CHANNEL_MULTIPLIER_B) % K_CHANNEL_MOD));
    const size_t cycle = caseNumber % K_COLOR_MODE_CYCLE;
    if (cycle == K_COLOR_MODE_INDEX_HSL) {
        spec.mode = ColorMode::RGB_TO_HSL;
    } else if (cycle == K_COLOR_MODE_INDEX_HSV) {
        spec.mode = ColorMode::RGB_TO_HSV;
    } else {
        spec.mode = ColorMode::HEX_FORMAT;
    }
    return spec;
}

const char* ColorModeLabel(ColorMode mode)
{
    if (mode == ColorMode::RGB_TO_HSL) {
        return K_COLOR_MODE_LABEL_HSL;
    }
    if (mode == ColorMode::RGB_TO_HSV) {
        return K_COLOR_MODE_LABEL_HSV;
    }
    return K_COLOR_MODE_LABEL_HEX;
}

const char* DescribeMode(ColorMode mode)
{
    if (mode == ColorMode::RGB_TO_HSL) {
        return K_COLOR_MODE_DESC_HSL;
    }
    if (mode == ColorMode::RGB_TO_HSV) {
        return K_COLOR_MODE_DESC_HSV;
    }
    return K_COLOR_MODE_DESC_HEX;
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

// === Helpers: Channel Normalization & Extremes ===

double NormalizeChannel(int32_t channel)
{
    return static_cast<double>(channel) / K_NORM_DIVISOR;
}

double MaxOf3(double a, double b, double c)
{
    double m = a;
    if (b > m) {
        m = b;
    }
    if (c > m) {
        m = c;
    }
    return m;
}

double MinOf3(double a, double b, double c)
{
    double m = a;
    if (b < m) {
        m = b;
    }
    if (c < m) {
        m = c;
    }
    return m;
}

// === Helpers: Hue Computation ===

double HueFromRgb(double normRed, double normGreen, double normBlue, double maxC, double d)
{
    if (d == K_DOUBLE_ZERO) {
        return K_DOUBLE_ZERO;
    }
    const double safeDelta = (d > K_DOUBLE_ZERO) ? d : K_DOUBLE_ONE;
    double seg = K_DOUBLE_ZERO;
    if (maxC == normRed) {
        seg = (normGreen - normBlue) / safeDelta;
    } else if (maxC == normGreen) {
        seg = (normBlue - normRed) / safeDelta + K_DOUBLE_TWO;
    } else {
        seg = (normRed - normGreen) / safeDelta + K_DOUBLE_THREE;
    }
    double deg = seg * K_DOUBLE_SIXTY;
    if (deg < K_DOUBLE_ZERO) {
        deg += K_DOUBLE_THREE_SIXTY;
    }
    return deg;
}

// === Helpers: RGB -> HSL ===

bool RgbToHsl(int32_t r, int32_t g, int32_t b, double* h, double* s, double* l)
{
    const double normRed = NormalizeChannel(r);
    const double normGreen = NormalizeChannel(g);
    const double normBlue = NormalizeChannel(b);
    const double maxC = MaxOf3(normRed, normGreen, normBlue);
    const double minC = MinOf3(normRed, normGreen, normBlue);
    const double d = maxC - minC;
    *l = (maxC + minC) / K_DOUBLE_TWO;
    if (d == K_DOUBLE_ZERO) {
        *h = K_DOUBLE_ZERO;
        *s = K_DOUBLE_ZERO;
        return true;
    }
    double denom = K_DOUBLE_TWO * (*l) - K_DOUBLE_ONE;
    if (denom < K_DOUBLE_ZERO) {
        denom = -denom;
    }
    const double saturationDenom = K_DOUBLE_ONE - denom;
    const double safeSatDenom = (saturationDenom > K_DOUBLE_ZERO) ? saturationDenom : K_DOUBLE_ONE;
    *s = d / safeSatDenom;
    *h = HueFromRgb(normRed, normGreen, normBlue, maxC, d);
    return true;
}

// === Helpers: RGB -> HSV ===

bool RgbToHsv(int32_t r, int32_t g, int32_t b, double* h, double* s, double* v)
{
    const double normRed = NormalizeChannel(r);
    const double normGreen = NormalizeChannel(g);
    const double normBlue = NormalizeChannel(b);
    const double maxC = MaxOf3(normRed, normGreen, normBlue);
    const double minC = MinOf3(normRed, normGreen, normBlue);
    const double d = maxC - minC;
    *v = maxC;
    const double safeMax = (maxC > K_DOUBLE_ZERO) ? maxC : K_DOUBLE_ONE;
    *s = d / safeMax;
    *h = HueFromRgb(normRed, normGreen, normBlue, maxC, d);
    return true;
}

// === Helpers: Hex Formatting ===

std::string FormatHexComponent(int32_t value)
{
    char buf[K_HEX_COMPONENT_WIDTH + K_HEX_NULL_TERMINATOR];
    buf[0] = K_HEX_DIGITS[(value >> K_HEX_HIGH_SHIFT) & K_HEX_LOW_MASK];
    buf[1] = K_HEX_DIGITS[value & K_HEX_LOW_MASK];
    buf[K_HEX_COMPONENT_WIDTH] = '\0';
    return std::string(buf);
}

std::string FormatHex(int32_t r, int32_t g, int32_t b)
{
    std::string result = K_HEX_PREFIX;
    result += FormatHexComponent(r);
    result += FormatHexComponent(g);
    result += FormatHexComponent(b);
    return result;
}

// === Result Building ===

napi_value BuildColorResult(napi_env env, const ColorCaseSpec& spec, double h, double s,
    double lOrV, const std::string& hex)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "mode", ColorModeLabel(spec.mode));
    SetNamedString(env, result, "description", DescribeMode(spec.mode));
    SetNamedInt32(env, result, "r", spec.r);
    SetNamedInt32(env, result, "g", spec.g);
    SetNamedInt32(env, result, "b", spec.b);
    SetNamedDouble(env, result, "h", h);
    SetNamedDouble(env, result, "s", s);
    SetNamedDouble(env, result, "lOrV", lOrV);
    SetNamedString(env, result, "hex", hex);
    SetNamedInt32(env, result, "caseNumber", static_cast<int32_t>(spec.caseNumber));
    return result;
}

// === Test Entry ===

static napi_value RunColorCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_COLOR_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid color case");
        return nullptr;
    }
    const auto spec = GetColorCaseSpec(caseIndex);
    double hL = K_DOUBLE_ZERO;
    double sL = K_DOUBLE_ZERO;
    double lL = K_DOUBLE_ZERO;
    RgbToHsl(spec.r, spec.g, spec.b, &hL, &sL, &lL);
    double hV = K_DOUBLE_ZERO;
    double sV = K_DOUBLE_ZERO;
    double vV = K_DOUBLE_ZERO;
    RgbToHsv(spec.r, spec.g, spec.b, &hV, &sV, &vV);
    const double h = (spec.mode == ColorMode::RGB_TO_HSV) ? hV : hL;
    const double s = (spec.mode == ColorMode::RGB_TO_HSV) ? sV : sL;
    const double lOrV = (spec.mode == ColorMode::RGB_TO_HSV) ? vV : lL;
    const std::string hex = FormatHex(spec.r, spec.g, spec.b);
    return BuildColorResult(env, spec, h, s, lOrV, hex);
}

}  // namespace

static napi_value InitColorSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_COLOR_CASE_COUNT);
    exportNames.reserve(K_COLOR_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_COLOR_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testColorCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunColorCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_colorSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitColorSuite,
    .nm_modname = "color_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterColorSuiteModule(void)
{
    napi_module_register(&g_colorSuiteModule);
}
