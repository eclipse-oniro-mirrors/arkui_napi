/**
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
#include <vector>

#include "morse_suite_common.h"
#include "napi/native_node_api.h"

// Dit units of a single code: every dot keys 1 unit and every dash keys
// 3 units, e.g. "--." -> 7.
napi_value MorseCodeUnits(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], code));

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32Value(env, MorseCodeUnitCount(code.c_str()), result));
    return result;
}

// Character of the text whose code needs the most dit units; the first one
// wins ties. Text without encodable characters throws, e.g. "SOS" -> "O".
napi_value LongestMorseChar(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    char best = MORSE_NOT_FOUND;
    int32_t bestUnits = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == TEXT_WORD_SEPARATOR) {
            continue;
        }
        int32_t units = MorseCodeUnitCount(LookupMorseCode(symbol));
        if (best == MORSE_NOT_FOUND || units > bestUnits) {
            best = symbol;
            bestUnits = units;
        }
    }
    NAPI_ASSERT(env, best != MORSE_NOT_FOUND, "Text contains no encodable characters");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, std::string(MORSE_SINGLE_CHAR_LENGTH, best), result));
    return result;
}

// Character of the text whose code needs the fewest dit units; the first
// one wins ties. Text without encodable characters throws, e.g. "SEA" -> "E".
napi_value ShortestMorseChar(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    char best = MORSE_NOT_FOUND;
    int32_t bestUnits = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == TEXT_WORD_SEPARATOR) {
            continue;
        }
        int32_t units = MorseCodeUnitCount(LookupMorseCode(symbol));
        if (best == MORSE_NOT_FOUND || units < bestUnits) {
            best = symbol;
            bestUnits = units;
        }
    }
    NAPI_ASSERT(env, best != MORSE_NOT_FOUND, "Text contains no encodable characters");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, std::string(MORSE_SINGLE_CHAR_LENGTH, best), result));
    return result;
}

// Counts how many characters of the text use codes of 1..5 symbols,
// returned as a five-element array, e.g. "SOS" -> [0, 0, 3, 0, 0].
napi_value MorseLengthHistogram(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    std::vector<int32_t> histogram(MORSE_HISTOGRAM_BINS, MORSE_COUNT_ZERO);
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == TEXT_WORD_SEPARATOR) {
            continue;
        }
        int32_t symbols = MorseCodeSymbolCount(LookupMorseCode(symbol));
        size_t binIndex = static_cast<size_t>(symbols) - MORSE_HISTOGRAM_INDEX_BASE;
        ++histogram[binIndex];
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32ArrayValue(env, histogram, result));
    return result;
}

// Counts the dots and dashes of a morse string, returned as the pair
// [dots, dashes], e.g. ".- .-" -> [2, 2].
napi_value DotDashCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: morse");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    int32_t dots = MORSE_COUNT_ZERO;
    int32_t dashes = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < morse.size(); ++i) {
        if (morse[i] == MORSE_DOT) {
            ++dots;
        } else if (morse[i] == MORSE_DASH) {
            ++dashes;
        }
    }

    std::vector<int32_t> counts;
    counts.push_back(dots);
    counts.push_back(dashes);

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32ArrayValue(env, counts, result));
    return result;
}

// Ratio of dashes to all symbols of a morse string, e.g. "--.." -> 0.5.
napi_value DashRatio(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: morse");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    int32_t dots = MORSE_COUNT_ZERO;
    int32_t dashes = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < morse.size(); ++i) {
        if (morse[i] == MORSE_DOT) {
            ++dots;
        } else if (morse[i] == MORSE_DASH) {
            ++dashes;
        }
    }

    double total = static_cast<double>(dots + dashes);
    double safeTotal = total > MORSE_ZERO_DOUBLE ? total : MORSE_SAFE_DIVISOR;
    double ratio = static_cast<double>(dashes) / safeTotal;

    napi_value result = nullptr;
    NAPI_CALL(env, CreateDoubleValue(env, ratio, result));
    return result;
}

// Reverses the symbol order of a code, e.g. ".--" -> "--.".
napi_value ReverseMorseCode(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], code));

    std::string reversed(code.rbegin(), code.rend());

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, reversed, result));
    return result;
}

// True when a code reads the same forwards and backwards,
// e.g. ".-." -> true and ".--" -> false.
napi_value IsMorsePalindrome(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], code));

    std::string reversed(code.rbegin(), code.rend());

    napi_value result = nullptr;
    NAPI_CALL(env, CreateBoolValue(env, reversed == code, result));
    return result;
}

// Swaps the dots and dashes of a code, e.g. ".--" -> "-..".
napi_value InvertMorseCode(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], code));

    std::string inverted;
    for (size_t i = 0; i < code.size(); ++i) {
        inverted += (code[i] == MORSE_DOT) ? MORSE_DASH : MORSE_DOT;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, inverted, result));
    return result;
}

// Longest common symbol prefix of two codes, e.g. ".-.." and ".--" -> ".-".
napi_value MorseCommonPrefix(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: codeA, codeB");

    std::string codeA;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], codeA));

    std::string codeB;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ONE], codeB));

    size_t limit = codeA.size() < codeB.size() ? codeA.size() : codeB.size();
    std::string prefix;
    for (size_t i = 0; i < limit; ++i) {
        if (codeA[i] != codeB[i]) {
            break;
        }
        prefix += codeA[i];
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, prefix, result));
    return result;
}

// Similarity of two codes within 0..1: the shorter code is padded with
// zero-unit spaces, the unit distance (dot 1, dah 3) is summed over the
// longer code and normalized by its maximum possible value, so identical
// codes score 1.0 and ".-" against "--." scores 0.5.
napi_value MorseSimilarity(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: codeA, codeB");

    std::string codeA;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], codeA));

    std::string codeB;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ONE], codeB));

    size_t maxLength = codeA.size() > codeB.size() ? codeA.size() : codeB.size();
    double distance = MORSE_ZERO_DOUBLE;
    for (size_t i = 0; i < maxLength; ++i) {
        double unitA = i < codeA.size() ? static_cast<double>(MorseSymbolUnits(codeA[i])) : MORSE_PAD_UNITS;
        double unitB = i < codeB.size() ? static_cast<double>(MorseSymbolUnits(codeB[i])) : MORSE_PAD_UNITS;
        double difference = unitA - unitB;
        distance += difference < MORSE_ZERO_DOUBLE ? -difference : difference;
    }

    double divisor = static_cast<double>(MORSE_UNIT_SPAN) * static_cast<double>(maxLength);
    double safeDivisor = divisor > MORSE_ZERO_DOUBLE ? divisor : MORSE_SAFE_DIVISOR;
    double similarity = MORSE_FULL_SIMILARITY - distance / safeDivisor;

    napi_value result = nullptr;
    NAPI_CALL(env, CreateDoubleValue(env, similarity, result));
    return result;
}

// Decodes the index-th code token of a morse string back to its character.
// Word separators are skipped and out-of-range indexes throw,
// e.g. ("--- ..", 1) -> "D".
napi_value MorseCharAt(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: morse, index");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    int32_t index = MORSE_COUNT_ZERO;
    NAPI_CALL(env, ExtractInt32Arg(env, argv[ARG_INDEX_ONE], index));

    std::vector<std::vector<std::string>> words = SplitMorseWords(morse);
    std::vector<std::string> codes;
    for (size_t w = 0; w < words.size(); ++w) {
        for (size_t c = 0; c < words[w].size(); ++c) {
            codes.push_back(words[w][c]);
        }
    }

    NAPI_ASSERT(env, index >= MORSE_COUNT_ZERO && static_cast<size_t>(index) < codes.size(), "Index out of range");

    char letter = LookupMorseChar(codes[static_cast<size_t>(index)]);
    NAPI_ASSERT(env, letter != MORSE_NOT_FOUND, "Invalid morse code token");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, std::string(MORSE_SINGLE_CHAR_LENGTH, letter), result));
    return result;
}

// Splits morse into its word code groups; the codes of each group are
// joined by single spaces, e.g. ".. . / ---" -> [".. .", "---"].
napi_value MorseSplitWords(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: morse");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    std::vector<std::vector<std::string>> words = SplitMorseWords(morse);
    std::vector<std::string> groups;
    for (size_t w = 0; w < words.size(); ++w) {
        std::string group;
        for (size_t c = 0; c < words[w].size(); ++c) {
            if (c > 0) {
                group += MORSE_CHAR_GAP;
            }
            group += words[w][c];
        }
        groups.push_back(group);
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringArrayValue(env, groups, result));
    return result;
}

// Splits morse into its individual code tokens; word separators are
// skipped, e.g. ".. . / ---" -> ["..", ".", "---"].
napi_value MorseSplitChars(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: morse");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    std::vector<std::vector<std::string>> words = SplitMorseWords(morse);
    std::vector<std::string> codes;
    for (size_t w = 0; w < words.size(); ++w) {
        for (size_t c = 0; c < words[w].size(); ++c) {
            codes.push_back(words[w][c]);
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringArrayValue(env, codes, result));
    return result;
}

// Joins an array of code tokens with single spaces; every element must be
// a valid dot/dash token, e.g. ["..", "---"] -> ".. ---".
napi_value JoinMorseCodes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: codesArray");

    bool isArray = false;
    NAPI_CALL(env, napi_is_array(env, argv[ARG_INDEX_ZERO], &isArray));
    NAPI_ASSERT(env, isArray, "Code list must be an array");

    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[ARG_INDEX_ZERO], &length));

    std::string joined;
    for (uint32_t i = 0; i < length; ++i) {
        napi_value item = nullptr;
        NAPI_CALL(env, napi_get_element(env, argv[ARG_INDEX_ZERO], i, &item));
        std::string token;
        NAPI_CALL(env, ExtractStringArg(env, item, token));
        NAPI_ASSERT(env, !token.empty(), "Code tokens must not be empty");
        NAPI_ASSERT(env, token.size() <= MORSE_MAX_CODE_SYMBOLS, "Code tokens must hold at most 5 symbols");
        for (size_t j = 0; j < token.size(); ++j) {
            NAPI_ASSERT(env, token[j] == MORSE_DOT || token[j] == MORSE_DASH,
                        "Code tokens must contain only dots and dashes");
        }
        if (i > 0) {
            joined += MORSE_CHAR_GAP;
        }
        joined += token;
    }
    NAPI_ASSERT(env, joined.size() <= MORSE_MAX_CODE_LENGTH, "Joined morse exceeds maximum length");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, joined, result));
    return result;
}

// Radio-style grouping: spaces are dropped and the remaining characters
// are split into groups of at most groupSize characters, the last group may
// be short, e.g. ("HELLO WORLD", 2) -> ["HE", "LL", "OW", "OR", "LD"].
napi_value MorseChunks(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: text, groupSize");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    int32_t groupSize = MORSE_COUNT_ZERO;
    NAPI_CALL(env, ExtractInt32Arg(env, argv[ARG_INDEX_ONE], groupSize));
    NAPI_ASSERT(env, groupSize >= MORSE_MIN_CHUNK_SIZE && groupSize <= MORSE_MAX_CHUNK_SIZE,
                "Group size must be within 1..10");

    std::string compact;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != TEXT_WORD_SEPARATOR) {
            compact += text[i];
        }
    }

    std::vector<std::string> chunks;
    size_t chunkSize = static_cast<size_t>(groupSize);
    for (size_t i = 0; i < compact.size(); i += chunkSize) {
        chunks.push_back(compact.substr(i, chunkSize));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringArrayValue(env, chunks, result));
    return result;
}

// Drops every character the suite cannot encode; letters are uppercased on
// the way through, e.g. "Héllo! 2 4 You" -> "HLLO 2 4 YOU".
napi_value StripUnsupportedChars(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    std::string filtered;
    for (size_t i = 0; i < text.size(); ++i) {
        char upper = UppercaseAsciiChar(text[i]);
        if (upper == TEXT_WORD_SEPARATOR || IsEncodableChar(upper)) {
            filtered += upper;
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, filtered, result));
    return result;
}

// Counts the characters that stripUnsupportedChars would drop,
// e.g. "Hé2!" -> 2.
napi_value CountUnsupportedChars(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    int32_t count = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < text.size(); ++i) {
        char upper = UppercaseAsciiChar(text[i]);
        bool supported = upper == TEXT_WORD_SEPARATOR || IsEncodableChar(upper);
        if (!supported) {
            ++count;
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32Value(env, count, result));
    return result;
}

// NATO word of a single letter; lowercase input is accepted and uppercased
// on the way through, e.g. "b" -> "Bravo".
napi_value NatoWordFor(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: letter");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));
    NAPI_ASSERT(env, text.size() == MORSE_SINGLE_CHAR_LENGTH, "Letter must be a single character");

    char upper = UppercaseAsciiChar(text[0]);
    bool isLetter = upper >= NATO_LETTER_BASE && upper <= NATO_LETTER_LAST;
    NAPI_ASSERT(env, isLetter, "Letter must be within A..Z");

    size_t index = static_cast<size_t>(upper - NATO_LETTER_BASE);

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, NATO_WORDS[index], result));
    return result;
}

// Letter of a single NATO word, matched case-insensitively; unknown words
// throw, e.g. "oscar" -> "O".
napi_value NatoLetterFor(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: word");

    std::string word;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], word));

    int32_t index = LookupNatoWordIndex(word);
    NAPI_ASSERT(env, index != NATO_WORD_NOT_FOUND, "Unknown NATO word");

    char letter = static_cast<char>(NATO_LETTER_BASE + index);

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, std::string(MORSE_SINGLE_CHAR_LENGTH, letter), result));
    return result;
}

// True when the word belongs to the NATO alphabet, matched
// case-insensitively, e.g. "zulu" -> true.
napi_value IsNatoWord(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: word");

    std::string word;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], word));

    int32_t index = LookupNatoWordIndex(word);

    napi_value result = nullptr;
    NAPI_CALL(env, CreateBoolValue(env, index != NATO_WORD_NOT_FOUND, result));
    return result;
}

napi_status RegisterMorseOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("morseCodeUnits", MorseCodeUnits),
        DECLARE_NAPI_FUNCTION("longestMorseChar", LongestMorseChar),
        DECLARE_NAPI_FUNCTION("shortestMorseChar", ShortestMorseChar),
        DECLARE_NAPI_FUNCTION("morseLengthHistogram", MorseLengthHistogram),
        DECLARE_NAPI_FUNCTION("dotDashCount", DotDashCount),
        DECLARE_NAPI_FUNCTION("dashRatio", DashRatio),
        DECLARE_NAPI_FUNCTION("reverseMorseCode", ReverseMorseCode),
        DECLARE_NAPI_FUNCTION("isMorsePalindrome", IsMorsePalindrome),
        DECLARE_NAPI_FUNCTION("invertMorseCode", InvertMorseCode),
        DECLARE_NAPI_FUNCTION("morseCommonPrefix", MorseCommonPrefix),
        DECLARE_NAPI_FUNCTION("morseSimilarity", MorseSimilarity),
        DECLARE_NAPI_FUNCTION("morseCharAt", MorseCharAt),
        DECLARE_NAPI_FUNCTION("morseSplitWords", MorseSplitWords),
        DECLARE_NAPI_FUNCTION("morseSplitChars", MorseSplitChars),
        DECLARE_NAPI_FUNCTION("joinMorseCodes", JoinMorseCodes),
        DECLARE_NAPI_FUNCTION("morseChunks", MorseChunks),
        DECLARE_NAPI_FUNCTION("stripUnsupportedChars", StripUnsupportedChars),
        DECLARE_NAPI_FUNCTION("countUnsupportedChars", CountUnsupportedChars),
        DECLARE_NAPI_FUNCTION("natoWordFor", NatoWordFor),
        DECLARE_NAPI_FUNCTION("natoLetterFor", NatoLetterFor),
        DECLARE_NAPI_FUNCTION("isNatoWord", IsNatoWord),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
