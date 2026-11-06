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
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "napi/native_node_api.h"

#include "text_analysis_suite_common.h"

static bool IsAlnumChar(char c)
{
    return IsAsciiLetter(c) || (c >= CHAR_DIGIT_ZERO && c <= CHAR_DIGIT_NINE);
}

static std::string KeepAlnumLower(const std::string& input)
{
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (IsAlnumChar(input[i])) {
            result += ToLowerAscii(input[i]);
        }
    }
    return result;
}

static bool IsPermutationOf(std::string left, std::string right)
{
    if (left.size() != right.size()) {
        return false;
    }
    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());
    return left == right;
}

napi_value IsPalindrome(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::string normalized = KeepAlnumLower(text);
    size_t halfLength = normalized.size() / HALF_DIVISOR;
    bool result = std::equal(normalized.begin(), normalized.begin() + static_cast<ptrdiff_t>(halfLength),
                             normalized.rbegin());
    return CreateBoolValue(env, result);
}

napi_value IsAnagram(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: text1, text2");

    std::string first;
    std::string second;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], first), "First text extraction failed");
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ONE], second), "Second text extraction failed");

    bool result = IsPermutationOf(KeepAlnumLower(first), KeepAlnumLower(second));
    return CreateBoolValue(env, result);
}

napi_value CountWords(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::string> words = SplitOnWhitespace(text);
    return CreateUint32Value(env, static_cast<uint32_t>(words.size()));
}

static void AddWordCount(std::vector<std::pair<std::string, uint32_t>>& counts, const std::string& word)
{
    for (size_t i = 0; i < counts.size(); ++i) {
        if (counts[i].first == word) {
            ++counts[i].second;
            return;
        }
    }
    counts.emplace_back(word, UINT32_ONE);
}

static napi_value CreateCountsArray(napi_env env, const std::vector<std::pair<std::string, uint32_t>>& counts)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, counts.size(), &result));
    for (size_t i = 0; i < counts.size(); ++i) {
        napi_value entry = nullptr;
        NAPI_CALL(env, napi_create_array_with_length(env, WORD_INDEX_SECOND + 1, &entry));
        napi_value word = CreateStringValue(env, counts[i].first);
        napi_value count = CreateUint32Value(env, counts[i].second);
        NAPI_CALL(env, napi_set_element(env, entry, UINT32_INDEX_ZERO, word));
        NAPI_CALL(env, napi_set_element(env, entry, UINT32_ONE, count));
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), entry));
    }
    return result;
}

napi_value WordFrequency(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::string> words = SplitOnWhitespace(ToLowerCopy(text));
    NAPI_ASSERT(env, words.size() <= MAX_WORD_COUNT, "Too many words");

    std::vector<std::pair<std::string, uint32_t>> counts;
    for (size_t i = 0; i < words.size(); ++i) {
        AddWordCount(counts, words[i]);
    }
    return CreateCountsArray(env, counts);
}

static size_t FindCharEntry(const std::vector<std::pair<std::string, uint32_t>>& counts, char c)
{
    std::string key(1, c);
    for (size_t i = 0; i < counts.size(); ++i) {
        if (counts[i].first == key) {
            return i;
        }
    }
    return counts.size();
}

napi_value CharacterFrequency(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::vector<std::pair<std::string, uint32_t>> counts;
    for (size_t i = 0; i < text.size(); ++i) {
        size_t entry = FindCharEntry(counts, text[i]);
        if (entry == counts.size()) {
            counts.emplace_back(std::string(1, text[i]), UINT32_ONE);
        } else {
            ++counts[entry].second;
        }
    }
    return CreateCountsArray(env, counts);
}

static std::string CommonPrefixOfTwo(const std::string& left, const std::string& right)
{
    size_t limit = std::min(left.size(), right.size());
    size_t position = EMPTY_RESULT_SIZE;
    while (position < limit && left[position] == right[position]) {
        ++position;
    }
    return left.substr(EMPTY_RESULT_SIZE, position);
}

napi_value LongestCommonPrefix(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: words");

    std::vector<std::string> words;
    NAPI_ASSERT(env, ExtractStringArrayArg(env, argv[ARG_INDEX_ZERO], words),
                "Argument must be an array of strings");
    NAPI_ASSERT(env, !words.empty(), "Array must not be empty");

    std::string prefix = words[WORD_INDEX_ZERO];
    for (size_t i = WORD_INDEX_SECOND; i < words.size(); ++i) {
        prefix = CommonPrefixOfTwo(prefix, words[i]);
    }
    return CreateStringValue(env, prefix);
}

static uint32_t EditDistanceStep(uint32_t above, uint32_t diagonal, uint32_t left, bool sameChar)
{
    uint32_t keep = sameChar ? diagonal : above + UINT32_ONE;
    return std::min(std::min(left + UINT32_ONE, above + UINT32_ONE), keep);
}

static uint32_t ComputeLevenshtein(const std::string& left, const std::string& right)
{
    std::vector<uint32_t> previous(right.size() + UINT32_ONE);
    std::vector<uint32_t> current(right.size() + UINT32_ONE);
    for (size_t j = 0; j <= right.size(); ++j) {
        previous[j] = static_cast<uint32_t>(j);
    }

    for (size_t i = 1; i <= left.size(); ++i) {
        current[WORD_INDEX_ZERO] = static_cast<uint32_t>(i);
        for (size_t j = 1; j <= right.size(); ++j) {
            bool sameChar = ToLowerAscii(left[i - 1]) == ToLowerAscii(right[j - 1]);
            current[j] = EditDistanceStep(previous[j], previous[j - 1], current[j - 1], sameChar);
        }
        previous.swap(current);
    }
    return previous[right.size()];
}

napi_value Levenshtein(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: text1, text2");

    std::string first;
    std::string second;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], first), "First text extraction failed");
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ONE], second), "Second text extraction failed");

    uint32_t distance = ComputeLevenshtein(first, second);
    return CreateUint32Value(env, distance);
}

static void AppendRunLength(std::string& output, char c, uint32_t runLength)
{
    output += c;
    if (runLength > UINT32_ONE) {
        output += std::to_string(runLength);
    }
}

napi_value RunLengthEncode(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    std::string result;
    size_t position = EMPTY_RESULT_SIZE;
    while (position < text.size()) {
        size_t runEnd = position + UINT32_ONE;
        while (runEnd < text.size() && text[runEnd] == text[position]) {
            ++runEnd;
        }
        uint32_t runLength = static_cast<uint32_t>(runEnd - position);
        AppendRunLength(result, text[position], runLength);
        position = runEnd;
    }
    return CreateStringValue(env, result);
}

static napi_value WrapLines(napi_env env, const std::string& text, size_t width)
{
    std::vector<std::string> words = SplitOnWhitespace(text);
    std::vector<std::string> lines;
    std::string line;
    for (size_t i = 0; i < words.size(); ++i) {
        bool fits = line.empty() || line.size() + UINT32_ONE + words[i].size() <= width;
        if (!fits) {
            lines.push_back(line);
            line.clear();
        }
        line = line.empty() ? words[i] : line + " " + words[i];
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    return CreateStringArrayValue(env, lines);
}

napi_value WrapWords(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: text, width");

    std::string text;
    NAPI_ASSERT(env, ExtractTextArg(env, argv[ARG_INDEX_ZERO], text), "Text extraction failed");

    double rawWidth = 0.0;
    NAPI_CALL(env, napi_get_value_double(env, argv[ARG_INDEX_ONE], &rawWidth));
    NAPI_ASSERT(env, rawWidth >= static_cast<double>(WORDS_PER_LINE_DEFAULT) &&
                         rawWidth <= static_cast<double>(MAX_TEXT_LENGTH),
                "width out of range");

    size_t width = static_cast<size_t>(rawWidth);
    return WrapLines(env, text, width);
}

napi_status RegisterTextAnalysisOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("isPalindrome", IsPalindrome),
        DECLARE_NAPI_FUNCTION("isAnagram", IsAnagram),
        DECLARE_NAPI_FUNCTION("countWords", CountWords),
        DECLARE_NAPI_FUNCTION("wordFrequency", WordFrequency),
        DECLARE_NAPI_FUNCTION("characterFrequency", CharacterFrequency),
        DECLARE_NAPI_FUNCTION("longestCommonPrefix", LongestCommonPrefix),
        DECLARE_NAPI_FUNCTION("levenshtein", Levenshtein),
        DECLARE_NAPI_FUNCTION("runLengthEncode", RunLengthEncode),
        DECLARE_NAPI_FUNCTION("wrapWords", WrapWords),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
