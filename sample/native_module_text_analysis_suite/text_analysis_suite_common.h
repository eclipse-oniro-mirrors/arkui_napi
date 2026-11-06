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

#ifndef SAMPLE_NATIVE_MODULE_TEXT_ANALYSIS_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_TEXT_ANALYSIS_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

constexpr size_t MAX_TEXT_LENGTH = 4096;
constexpr size_t MAX_WORD_COUNT = 1024;
constexpr size_t MAX_TRUNCATE_LENGTH = 4096;

constexpr char CHAR_SPACE = ' ';
constexpr char CHAR_TAB = '\t';
constexpr char CHAR_NEWLINE = '\n';
constexpr char CHAR_CARRIAGE_RETURN = '\r';
constexpr char CHAR_FORM_FEED = '\f';
constexpr char CHAR_VERTICAL_TAB = '\v';
constexpr char CHAR_UNDERSCORE = '_';
constexpr char CHAR_UPPER_A = 'A';
constexpr char CHAR_UPPER_Z = 'Z';
constexpr char CHAR_LOWER_A = 'a';
constexpr char CHAR_LOWER_E = 'e';
constexpr char CHAR_LOWER_I = 'i';
constexpr char CHAR_LOWER_O = 'o';
constexpr char CHAR_LOWER_U = 'u';
constexpr char CHAR_LOWER_Z = 'z';
constexpr char CHAR_DIGIT_ZERO = '0';
constexpr char CHAR_DIGIT_NINE = '9';
constexpr char CHAR_CASE_DELTA = 32;
constexpr uint32_t UINT32_ONE = 1;
constexpr size_t HALF_DIVISOR = 2;

constexpr size_t WORDS_PER_LINE_DEFAULT = 1;
constexpr size_t WORD_INDEX_ZERO = 0;
constexpr size_t WORD_INDEX_SECOND = 1;
constexpr size_t EMPTY_RESULT_SIZE = 0;
constexpr uint32_t UINT32_INDEX_ZERO = 0;

bool ExtractTextArg(napi_env env, napi_value value, std::string& result);
bool ExtractStringArrayArg(napi_env env, napi_value value, std::vector<std::string>& result);
std::string JoinStrings(const std::vector<std::string>& parts, const std::string& delimiter);
napi_value CreateStringValue(napi_env env, const std::string& value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateUint32Value(napi_env env, uint32_t value);
napi_value CreateStringArrayValue(napi_env env, const std::vector<std::string>& values);

bool IsWhitespaceChar(char c);
bool IsAsciiLetter(char c);
char ToLowerAscii(char c);
char ToUpperAscii(char c);
std::string ToLowerCopy(const std::string& input);
std::vector<std::string> SplitOnWhitespace(const std::string& input);

inline std::string JoinWithSpace(const std::vector<std::string>& parts)
{
    return JoinStrings(parts, " ");
}

napi_value CountVowels(napi_env env, napi_callback_info info);
napi_value ReverseWords(napi_env env, napi_callback_info info);
napi_value SnakeToCamel(napi_env env, napi_callback_info info);
napi_value Capitalize(napi_env env, napi_callback_info info);
napi_value ToTitleCase(napi_env env, napi_callback_info info);
napi_value Truncate(napi_env env, napi_callback_info info);
napi_value Initials(napi_env env, napi_callback_info info);
napi_value WordsToString(napi_env env, napi_callback_info info);
napi_value StringToWords(napi_env env, napi_callback_info info);
napi_value JoinWithDelimiter(napi_env env, napi_callback_info info);

napi_value IsPalindrome(napi_env env, napi_callback_info info);
napi_value IsAnagram(napi_env env, napi_callback_info info);
napi_value CountWords(napi_env env, napi_callback_info info);
napi_value WordFrequency(napi_env env, napi_callback_info info);
napi_value CharacterFrequency(napi_env env, napi_callback_info info);
napi_value LongestCommonPrefix(napi_env env, napi_callback_info info);
napi_value Levenshtein(napi_env env, napi_callback_info info);
napi_value RunLengthEncode(napi_env env, napi_callback_info info);
napi_value WrapWords(napi_env env, napi_callback_info info);
napi_status RegisterTextAnalysisOpsFunctions(napi_env env, napi_value exports);

#endif  // SAMPLE_NATIVE_MODULE_TEXT_ANALYSIS_SUITE_COMMON_H
