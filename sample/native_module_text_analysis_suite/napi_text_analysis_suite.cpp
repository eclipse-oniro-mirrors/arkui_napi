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

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_node_api.h"

#include "text_analysis_suite_common.h"

std::string JoinStrings(const std::vector<std::string>& parts, const std::string& delimiter)
{
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > EMPTY_RESULT_SIZE) {
            result += delimiter;
        }
        result += parts[i];
    }
    return result;
}

bool ExtractStringArrayArg(napi_env env, napi_value value, std::vector<std::string>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > MAX_WORD_COUNT) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        std::string item;
        if (!ExtractTextArg(env, element, item)) {
            return false;
        }
        result.push_back(item);
    }
    return true;
}

bool ExtractTextArg(napi_env env, napi_value value, std::string& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_string) {
        napi_throw_error(env, nullptr, "Argument must be a string");
        return false;
    }

    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, nullptr, 0, &length), false);
    if (length > MAX_TEXT_LENGTH) {
        napi_throw_error(env, nullptr, "String exceeds maximum length");
        return false;
    }

    result.assign(length + 1, '\0');
    size_t copied = 0;
    NAPI_CALL_BASE(env,
                   napi_get_value_string_utf8(env, value, &result[0], length + 1, &copied), false);
    result.resize(copied);
    return true;
}

napi_value CreateStringValue(napi_env env, const std::string& value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, value.c_str(), value.size(), &result));
    return result;
}

napi_value CreateBoolValue(napi_env env, bool value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result));
    return result;
}

napi_value CreateUint32Value(napi_env env, uint32_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, value, &result));
    return result;
}

napi_value CreateStringArrayValue(napi_env env, const std::vector<std::string>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateStringValue(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

bool IsWhitespaceChar(char c)
{
    return c == CHAR_SPACE || c == CHAR_TAB || c == CHAR_NEWLINE || c == CHAR_CARRIAGE_RETURN ||
           c == CHAR_FORM_FEED || c == CHAR_VERTICAL_TAB;
}

bool IsAsciiLetter(char c)
{
    return (c >= CHAR_UPPER_A && c <= CHAR_UPPER_Z) || (c >= CHAR_LOWER_A && c <= CHAR_LOWER_Z);
}

char ToLowerAscii(char c)
{
    if (c >= CHAR_UPPER_A && c <= CHAR_UPPER_Z) {
        return static_cast<char>(c + CHAR_CASE_DELTA);
    }
    return c;
}

char ToUpperAscii(char c)
{
    if (c >= CHAR_LOWER_A && c <= CHAR_LOWER_Z) {
        return static_cast<char>(c - CHAR_CASE_DELTA);
    }
    return c;
}

std::string ToLowerCopy(const std::string& input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ToLowerAscii);
    return result;
}

std::vector<std::string> SplitOnWhitespace(const std::string& input)
{
    std::vector<std::string> words;
    size_t position = 0;
    while (position < input.size()) {
        while (position < input.size() && IsWhitespaceChar(input[position])) {
            ++position;
        }
        size_t start = position;
        while (position < input.size() && !IsWhitespaceChar(input[position])) {
            ++position;
        }
        if (position > start) {
            words.push_back(input.substr(start, position - start));
        }
    }
    return words;
}

static bool IsVowelChar(char c)
{
    char lower = ToLowerAscii(c);
    return lower == CHAR_LOWER_A || lower == CHAR_LOWER_E || lower == CHAR_LOWER_I ||
           lower == CHAR_LOWER_O || lower == CHAR_LOWER_U;
}

napi_value CountVowels(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    uint32_t count = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (IsVowelChar(text[i])) {
            ++count;
        }
    }
    return CreateUint32Value(env, count);
}

napi_value ReverseWords(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::string> words = SplitOnWhitespace(text);
    std::reverse(words.begin(), words.end());
    return CreateStringValue(env, JoinWithSpace(words));
}

static std::string UpperFirstOfWord(const std::string& word)
{
    if (word.empty()) {
        return word;
    }
    std::string result = word;
    result[WORD_INDEX_ZERO] = ToUpperAscii(result[WORD_INDEX_ZERO]);
    return result;
}

napi_value Capitalize(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");
    NAPI_ASSERT(env, !text.empty(), "Text must not be empty");

    return CreateStringValue(env, UpperFirstOfWord(text));
}

napi_value ToTitleCase(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::string> words = SplitOnWhitespace(text);
    for (size_t i = 0; i < words.size(); ++i) {
        words[i] = UpperFirstOfWord(ToLowerCopy(words[i]));
    }
    return CreateStringValue(env, JoinWithSpace(words));
}

static std::string ConvertSnakeToCamel(const std::string& input)
{
    std::string result;
    bool upperNext = false;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == CHAR_UNDERSCORE) {
            upperNext = true;
            continue;
        }
        char c = input[i];
        if (upperNext && IsAsciiLetter(c)) {
            result += ToUpperAscii(c);
            upperNext = false;
        } else {
            result += ToLowerAscii(c);
        }
    }
    return result;
}

napi_value SnakeToCamel(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    return CreateStringValue(env, ConvertSnakeToCamel(text));
}

napi_value Truncate(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: text, maxLength");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    double limit = 0.0;
    NAPI_CALL(env, napi_get_value_double(env, argv[ARG_INDEX_ONE], &limit));
    NAPI_ASSERT(env, limit >= 0.0 && limit <= static_cast<double>(MAX_TRUNCATE_LENGTH),
                "maxLength out of range");

    size_t maxLength = static_cast<size_t>(limit);
    std::string suffix = text.size() > maxLength ? text.substr(0, maxLength) : text;
    if (suffix.size() < text.size()) {
        suffix += "...";
    }
    return CreateStringValue(env, suffix);
}

napi_value Initials(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::string> words = SplitOnWhitespace(text);
    std::string result;
    for (size_t i = 0; i < words.size(); ++i) {
        result += ToUpperAscii(words[i][WORD_INDEX_ZERO]);
    }
    return CreateStringValue(env, result);
}

napi_value WordsToString(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: words");

    std::vector<std::string> words;
    NAPI_ASSERT(env, ExtractStringArrayArg(env, argv[ARG_INDEX_ZERO], words),
                "Argument must be an array of strings");

    return CreateStringValue(env, JoinWithSpace(words));
}

napi_value StringToWords(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::string> words = SplitOnWhitespace(text);
    NAPI_ASSERT(env, words.size() <= MAX_WORD_COUNT, "Too many words");
    return CreateStringArrayValue(env, words);
}

napi_value JoinWithDelimiter(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: words, delimiter");

    std::vector<std::string> words;
    NAPI_ASSERT(env, ExtractStringArrayArg(env, argv[ARG_INDEX_ZERO], words),
                "First argument must be an array of strings");

    std::string delimiter;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ONE], delimiter), "Delimiter extraction failed");

    return CreateStringValue(env, JoinStrings(words, delimiter));
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("countVowels", CountVowels),
        DECLARE_NAPI_FUNCTION("reverseWords", ReverseWords),
        DECLARE_NAPI_FUNCTION("capitalize", Capitalize),
        DECLARE_NAPI_FUNCTION("toTitleCase", ToTitleCase),
        DECLARE_NAPI_FUNCTION("snakeToCamel", SnakeToCamel),
        DECLARE_NAPI_FUNCTION("truncate", Truncate),
        DECLARE_NAPI_FUNCTION("initials", Initials),
        DECLARE_NAPI_FUNCTION("wordsToString", WordsToString),
        DECLARE_NAPI_FUNCTION("stringToWords", StringToWords),
        DECLARE_NAPI_FUNCTION("joinWithDelimiter", JoinWithDelimiter),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterTextAnalysisOpsFunctions(env, exports), nullptr);
    return exports;
}

static napi_module textAnalysisSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "textAnalysisSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void TextAnalysisSuiteRegisterModule(void)
{
    napi_module_register(&textAnalysisSuiteModule);
}
