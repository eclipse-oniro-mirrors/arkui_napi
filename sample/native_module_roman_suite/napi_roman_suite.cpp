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

#include <cstddef>
#include <cstdint>
#include <string>

#include "napi/native_node_api.h"
#include "roman_suite_common.h"

int32_t RomanCharToValue(char symbol, bool& valid)
{
    for (size_t i = 0; i < ROMAN_SYMBOL_COUNT; ++i) {
        if (ROMAN_SYMBOL_CHARS[i] == symbol) {
            valid = true;
            return ROMAN_SYMBOL_VALUES[i];
        }
    }
    valid = false;
    return ROMAN_INVALID_VALUE;
}

// Lenient parser: accepts any symbol sequence built with subtractive pairs,
// even when it is not the canonical form of its value, e.g. "IIII" -> 4.
void ParseRomanString(const std::string& text, bool& valid, int32_t& value)
{
    valid = !text.empty();
    int32_t total = ROMAN_ZERO;
    size_t index = 0;
    while (valid && index < text.size()) {
        bool currentValid = false;
        bool nextValid = true;
        int32_t current = RomanCharToValue(text[index], currentValid);
        int32_t next = ROMAN_ZERO;
        if (index + NEXT_SYMBOL_OFFSET < text.size()) {
            next = RomanCharToValue(text[index + NEXT_SYMBOL_OFFSET], nextValid);
        }
        valid = currentValid && nextValid;
        if (valid) {
            if (current >= next) {
                total += current;
                index += PARSE_STEP_SINGLE;
            } else {
                total += next - current;
                index += PARSE_STEP_PAIR;
            }
        }
    }
    value = total;
}

void ParseRomanInRange(const std::string& text, bool& valid, int32_t& value)
{
    bool parsed = false;
    int32_t number = ROMAN_ZERO;
    ParseRomanString(text, parsed, number);
    valid = parsed && number >= ROMAN_MIN_VALUE && number <= ROMAN_MAX_VALUE;
    value = number;
}

// Emits the canonical form of a value already checked against the roman range.
std::string EmitRomanString(int32_t value)
{
    std::string result;
    int32_t remaining = value;
    for (size_t i = 0; i < ROMAN_PAIR_COUNT; ++i) {
        while (remaining >= ROMAN_PAIR_VALUES[i]) {
            result += ROMAN_PAIR_SYMBOLS[i];
            remaining -= ROMAN_PAIR_VALUES[i];
        }
    }
    return result;
}

napi_status ExtractInt32Arg(napi_env env, napi_value value, int32_t& result)
{
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) {
        return status;
    }
    if (type != napi_number) {
        return napi_number_expected;
    }
    status = napi_get_value_int32(env, value, &result);
    if (status != napi_ok) {
        return status;
    }
    return napi_ok;
}

napi_status ExtractStringArg(napi_env env, napi_value value, std::string& result)
{
    size_t bufferSize = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufferSize);
    if (status != napi_ok) {
        return status;
    }
    std::string buffer(bufferSize + STRING_NULL_TERMINATOR, '\0');
    size_t copiedLength = 0;
    status = napi_get_value_string_utf8(env, value, &buffer[0], bufferSize, &copiedLength);
    if (status != napi_ok) {
        return status;
    }
    result.assign(buffer.c_str(), copiedLength);
    return napi_ok;
}

// Reads a roman numeral argument and requires it to be inside the suite range.
napi_status ExtractRomanArg(napi_env env, napi_value value, int32_t& result)
{
    std::string text;
    napi_status status = ExtractStringArg(env, value, text);
    if (status != napi_ok) {
        return status;
    }
    bool valid = false;
    int32_t parsed = ROMAN_ZERO;
    ParseRomanInRange(text, valid, parsed);
    NAPI_ASSERT_BASE(env, valid, "Argument must be a roman numeral in range", napi_invalid_arg);
    result = parsed;
    return napi_ok;
}

napi_status CreateStringValue(napi_env env, const std::string& text, napi_value& result)
{
    napi_status status = napi_create_string_utf8(env, text.c_str(), text.size(), &result);
    return status;
}

napi_status CreateRomanResult(napi_env env, int32_t value, napi_value& result)
{
    napi_status status = CreateStringValue(env, EmitRomanString(value), result);
    return status;
}

napi_value ToRoman(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: value");

    int32_t value = ROMAN_ZERO;
    NAPI_CALL(env, ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], value));
    NAPI_ASSERT(env, value >= ROMAN_MIN_VALUE && value <= ROMAN_MAX_VALUE, "Value must be within roman range");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateRomanResult(env, value, result));
    return result;
}

// Lenient conversion: returns the parsed value, or -1 when the text is not a
// roman numeral inside the suite range, e.g. "IIII" -> 4 but "" or "MMMCMXCIY" -> -1.
napi_value FromRoman(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    bool valid = false;
    int32_t value = ROMAN_ZERO;
    ParseRomanInRange(text, valid, value);
    int32_t parsed = valid ? value : ROMAN_INVALID_VALUE;

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, parsed, &result));
    return result;
}

// Strict check: the text must be the canonical form of its own value,
// so "IV" is a roman numeral while the lenient-parseable "IIII" is not.
napi_value IsRoman(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    bool valid = false;
    int32_t value = ROMAN_ZERO;
    ParseRomanInRange(text, valid, value);
    bool canonical = valid && EmitRomanString(value) == text;

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, canonical, &result));
    return result;
}

napi_value RomanCharValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: symbol");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));
    NAPI_ASSERT(env, text.size() == ROMAN_SINGLE_SYMBOL_LENGTH, "Symbol must be a single roman character");

    bool valid = false;
    int32_t value = RomanCharToValue(text.front(), valid);
    if (!valid) {
        value = ROMAN_INVALID_VALUE;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, value, &result));
    return result;
}

napi_value RomanMinValue(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, ROMAN_MIN_VALUE, &result));
    return result;
}

napi_value RomanMaxValue(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, ROMAN_MAX_VALUE, &result));
    return result;
}

napi_value RomanSymbols(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, ROMAN_SYMBOL_CHARS, result));
    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("toRoman", ToRoman),
        DECLARE_NAPI_FUNCTION("fromRoman", FromRoman),
        DECLARE_NAPI_FUNCTION("isRoman", IsRoman),
        DECLARE_NAPI_FUNCTION("romanCharValue", RomanCharValue),
        DECLARE_NAPI_FUNCTION("romanMinValue", RomanMinValue),
        DECLARE_NAPI_FUNCTION("romanMaxValue", RomanMaxValue),
        DECLARE_NAPI_FUNCTION("romanSymbols", RomanSymbols),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterRomanOpsFunctions(env, exports), nullptr);
    return exports;
}

static napi_module romanSuiteModule = {
    .nm_version = ROMAN_MODULE_VERSION,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "roman_suite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RomanSuiteRegisterModule(void)
{
    napi_module_register(&romanSuiteModule);
}
