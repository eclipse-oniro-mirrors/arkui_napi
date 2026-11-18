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

// Uppercases the ASCII letters; every other character passes through.
char UppercaseAsciiChar(char symbol)
{
    if (symbol >= 'a' && symbol <= 'z') {
        return static_cast<char>(symbol - 'a' + 'A');
    }
    return symbol;
}

// Encodable characters are the letters and digits of the morse alphabet.
bool IsEncodableChar(char symbol)
{
    char upper = UppercaseAsciiChar(symbol);
    bool isLetter = upper >= NATO_LETTER_BASE && upper <= NATO_LETTER_LAST;
    bool isDigit = upper >= '0' && upper <= '9';
    return isLetter || isDigit;
}

// Looks up the dot/dash code of a letter or digit, or nullptr when the
// character has no morse code.
const char* LookupMorseCode(char letter)
{
    char upper = UppercaseAsciiChar(letter);
    for (size_t i = 0; i < MORSE_ALPHABET_SIZE; ++i) {
        if (MORSE_LETTERS[i][0] == upper) {
            return MORSE_CODES[i];
        }
    }
    return nullptr;
}

// Looks up the character encoded by a dot/dash code, or 0 when the code
// sequence belongs to no character of the alphabet.
char LookupMorseChar(const std::string& code)
{
    for (size_t i = 0; i < MORSE_ALPHABET_SIZE; ++i) {
        if (code == MORSE_CODES[i]) {
            return MORSE_LETTERS[i][0];
        }
    }
    return MORSE_NOT_FOUND;
}

// Case-insensitive NATO word lookup; returns the word index or -1.
int32_t LookupNatoWordIndex(const std::string& word)
{
    for (size_t i = 0; i < NATO_ALPHABET_SIZE; ++i) {
        std::string candidate = NATO_WORDS[i];
        bool matches = candidate.size() == word.size();
        for (size_t j = 0; matches && j < candidate.size(); ++j) {
            matches = UppercaseAsciiChar(word[j]) == UppercaseAsciiChar(candidate[j]);
        }
        if (matches) {
            return static_cast<int32_t>(i);
        }
    }
    return NATO_WORD_NOT_FOUND;
}

// Dit units of a single symbol: dots key 1 unit, dahs key 3 units.
int32_t MorseSymbolUnits(char symbol)
{
    if (symbol == MORSE_DASH) {
        return MORSE_DAH_UNITS;
    }
    return MORSE_DIT_UNITS;
}

// Sums the dit units of a table code: every dot adds 1, every dah adds 3.
int32_t MorseCodeUnitCount(const char* code)
{
    int32_t units = MORSE_COUNT_ZERO;
    for (size_t i = 0; code[i] != '\0'; ++i) {
        units += MorseSymbolUnits(code[i]);
    }
    return units;
}

// Counts the dot/dash symbols of a table code.
int32_t MorseCodeSymbolCount(const char* code)
{
    int32_t symbols = MORSE_COUNT_ZERO;
    while (code[symbols] != '\0') {
        ++symbols;
    }
    return symbols;
}

// Builds a run of key-down units in the binary signal alphabet.
std::string SignalMarkString(int32_t units)
{
    return std::string(static_cast<size_t>(units), SIGNAL_MARK_CHAR);
}

// Builds a run of gap units in the binary signal alphabet.
std::string SignalGapString(int32_t units)
{
    return std::string(static_cast<size_t>(units), SIGNAL_SPACE_CHAR);
}

// Expands one code to its binary signal: marks are spelled at dit
// resolution and intra-character gaps separate them, e.g. ".-" -> "10111".
std::string MorseCodeSignalString(const std::string& code)
{
    std::string signal;
    for (size_t i = 0; i < code.size(); ++i) {
        if (i > 0) {
            signal += SignalGapString(MORSE_INTRA_CHAR_GAP_UNITS);
        }
        if (code[i] == MORSE_DASH) {
            signal += SignalMarkString(MORSE_DAH_UNITS);
        } else {
            signal += SignalMarkString(MORSE_DIT_UNITS);
        }
    }
    return signal;
}

// Splits a validated morse string into words (separated by '/') and each
// word into its dot/dash codes (separated by spaces). Empty segments
// collapse, so repeated separators are tolerated.
std::vector<std::vector<std::string>> SplitMorseWords(const std::string& morse)
{
    std::vector<std::vector<std::string>> words;
    std::vector<std::string> currentWord;
    std::string currentCode;
    for (size_t i = 0; i < morse.size(); ++i) {
        char symbol = morse[i];
        if (symbol == MORSE_WORD_SEPARATOR) {
            if (!currentCode.empty()) {
                currentWord.push_back(currentCode);
                currentCode.clear();
            }
            if (!currentWord.empty()) {
                words.push_back(currentWord);
                currentWord.clear();
            }
        } else if (symbol == MORSE_CHAR_GAP) {
            if (!currentCode.empty()) {
                currentWord.push_back(currentCode);
                currentCode.clear();
            }
        } else {
            currentCode += symbol;
        }
    }
    if (!currentCode.empty()) {
        currentWord.push_back(currentCode);
    }
    if (!currentWord.empty()) {
        words.push_back(currentWord);
    }
    return words;
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

// Reads a plain-text argument, uppercases it and requires every character
// to be a letter, a digit or a word-separator space.
napi_status ExtractMorseTextArg(napi_env env, napi_value value, std::string& result)
{
    std::string text;
    napi_status status = ExtractStringArg(env, value, text);
    if (status != napi_ok) {
        return status;
    }
    NAPI_ASSERT_BASE(env, text.size() <= MORSE_MAX_TEXT_LENGTH, "Text exceeds maximum length", napi_invalid_arg);
    std::string upper;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = UppercaseAsciiChar(text[i]);
        bool supported = symbol == TEXT_WORD_SEPARATOR || IsEncodableChar(symbol);
        NAPI_ASSERT_BASE(env, supported, "Text must contain only letters, digits and spaces", napi_invalid_arg);
        upper += symbol;
    }
    result = upper;
    return napi_ok;
}

// Reads a morse argument built from dots, dashes, code-separator spaces and
// word-separator slashes.
napi_status ExtractMorseCodeArg(napi_env env, napi_value value, std::string& result)
{
    std::string morse;
    napi_status status = ExtractStringArg(env, value, morse);
    if (status != napi_ok) {
        return status;
    }
    NAPI_ASSERT_BASE(env, morse.size() <= MORSE_MAX_CODE_LENGTH, "Morse exceeds maximum length", napi_invalid_arg);
    for (size_t i = 0; i < morse.size(); ++i) {
        char symbol = morse[i];
        bool valid =
            symbol == MORSE_DOT || symbol == MORSE_DASH || symbol == MORSE_CHAR_GAP || symbol == MORSE_WORD_SEPARATOR;
        NAPI_ASSERT_BASE(env, valid, "Morse must contain only dots, dashes, spaces and slashes", napi_invalid_arg);
    }
    result = morse;
    return napi_ok;
}

// Reads a single dot/dash code token: non-empty and free of separators.
napi_status ExtractMorseTokenArg(napi_env env, napi_value value, std::string& result)
{
    std::string code;
    napi_status status = ExtractStringArg(env, value, code);
    if (status != napi_ok) {
        return status;
    }
    NAPI_ASSERT_BASE(env, !code.empty(), "Code must not be empty", napi_invalid_arg);
    NAPI_ASSERT_BASE(env, code.size() <= MORSE_MAX_CODE_LENGTH, "Code exceeds maximum length", napi_invalid_arg);
    for (size_t i = 0; i < code.size(); ++i) {
        bool valid = code[i] == MORSE_DOT || code[i] == MORSE_DASH;
        NAPI_ASSERT_BASE(env, valid, "Code must contain only dots and dashes", napi_invalid_arg);
    }
    result = code;
    return napi_ok;
}

// Reads a binary signal argument: a sequence of '0' and '1' characters.
napi_status ExtractSignalStringArg(napi_env env, napi_value value, std::string& result)
{
    std::string signal;
    napi_status status = ExtractStringArg(env, value, signal);
    if (status != napi_ok) {
        return status;
    }
    NAPI_ASSERT_BASE(env, signal.size() <= MORSE_MAX_SIGNAL_LENGTH, "Signal exceeds maximum length", napi_invalid_arg);
    for (size_t i = 0; i < signal.size(); ++i) {
        bool valid = signal[i] == SIGNAL_MARK_CHAR || signal[i] == SIGNAL_SPACE_CHAR;
        NAPI_ASSERT_BASE(env, valid, "Signal must contain only '0' and '1' characters", napi_invalid_arg);
    }
    result = signal;
    return napi_ok;
}

napi_status CreateStringValue(napi_env env, const std::string& text, napi_value& result)
{
    napi_status status = napi_create_string_utf8(env, text.c_str(), text.size(), &result);
    return status;
}

napi_status CreateInt32Value(napi_env env, int32_t value, napi_value& result)
{
    napi_status status = napi_create_int32(env, value, &result);
    return status;
}

napi_status CreateDoubleValue(napi_env env, double value, napi_value& result)
{
    napi_status status = napi_create_double(env, value, &result);
    return status;
}

napi_status CreateBoolValue(napi_env env, bool value, napi_value& result)
{
    napi_status status = napi_get_boolean(env, value, &result);
    return status;
}

napi_status CreateStringArrayValue(napi_env env, const std::vector<std::string>& values, napi_value& result)
{
    napi_status status = napi_create_array_with_length(env, values.size(), &result);
    if (status != napi_ok) {
        return status;
    }
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value item = nullptr;
        status = CreateStringValue(env, values[i], item);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, result, static_cast<uint32_t>(i), item);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

napi_status CreateInt32ArrayValue(napi_env env, const std::vector<int32_t>& values, napi_value& result)
{
    napi_status status = napi_create_array_with_length(env, values.size(), &result);
    if (status != napi_ok) {
        return status;
    }
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value item = nullptr;
        status = CreateInt32Value(env, values[i], item);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, result, static_cast<uint32_t>(i), item);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

// Encodes plain text: character codes are joined by single spaces and words
// are joined by " / ", e.g. "sos" -> "... --- ..." and "HI YOU" ->
// ".... .. / -.-- --- ..-".
napi_value TextToMorse(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    std::string morse;
    std::string word;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        bool isWordSeparator = symbol == TEXT_WORD_SEPARATOR;
        if (isWordSeparator && !word.empty()) {
            if (!morse.empty()) {
                morse += MORSE_WORD_GAP_TEXT;
            }
            morse += word;
            word.clear();
        } else if (!isWordSeparator) {
            if (!word.empty()) {
                word += MORSE_CHAR_GAP;
            }
            word += LookupMorseCode(symbol);
        }
    }
    if (!word.empty()) {
        if (!morse.empty()) {
            morse += MORSE_WORD_GAP_TEXT;
        }
        morse += word;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, morse, result));
    return result;
}

// Decodes morse back to text: code separators split codes and word
// separators map back to spaces. Repeated separators are tolerated, but an
// unknown code token throws, e.g. ".... ..  ---" -> "HI O".
napi_value MorseToText(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: morse");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    std::vector<std::vector<std::string>> words = SplitMorseWords(morse);
    std::string text;
    for (size_t w = 0; w < words.size(); ++w) {
        if (w > 0) {
            text += TEXT_WORD_SEPARATOR;
        }
        for (size_t c = 0; c < words[w].size(); ++c) {
            char letter = LookupMorseChar(words[w][c]);
            NAPI_ASSERT(env, letter != MORSE_NOT_FOUND, "Invalid morse code token");
            text += letter;
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, text, result));
    return result;
}

// True when every character of the input is a dot, dash, code-separator
// space or word-separator slash; the empty string is not a morse message.
napi_value IsMorseCode(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    bool valid = !text.empty();
    for (size_t i = 0; valid && i < text.size(); ++i) {
        char symbol = text[i];
        valid =
            symbol == MORSE_DOT || symbol == MORSE_DASH || symbol == MORSE_CHAR_GAP || symbol == MORSE_WORD_SEPARATOR;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateBoolValue(env, valid, result));
    return result;
}

// Returns the code of a single letter or digit; lowercase input is accepted
// and uppercased on the way through, e.g. "a" -> ".-".
napi_value CharToMorse(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: character");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));
    NAPI_ASSERT(env, text.size() == MORSE_SINGLE_CHAR_LENGTH, "Character must be a single letter or digit");

    char upper = UppercaseAsciiChar(text[0]);
    const char* code = LookupMorseCode(upper);
    NAPI_ASSERT(env, code != nullptr, "Character must be a letter or a digit");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, code, result));
    return result;
}

// Decodes a single dot/dash token of one to five symbols back to its
// character, e.g. "--." -> "G"; unknown code sequences throw.
napi_value MorseToChar(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: code");

    std::string code;
    NAPI_CALL(env, ExtractMorseTokenArg(env, argv[ARG_INDEX_ZERO], code));
    NAPI_ASSERT(env, code.size() >= MORSE_MIN_CODE_SYMBOLS && code.size() <= MORSE_MAX_CODE_SYMBOLS,
                "Code must hold between 1 and 5 symbols");

    char letter = LookupMorseChar(code);
    NAPI_ASSERT(env, letter != MORSE_NOT_FOUND, "Unknown morse code");

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, std::string(MORSE_SINGLE_CHAR_LENGTH, letter), result));
    return result;
}

// Spells text with NATO words joined by single spaces. Spaces separate the
// words of the input and are not spelled, and digits are rejected because
// the NATO alphabet covers letters only, e.g. "hi you" -> "Hotel India
// Yankee Oscar Uniform".
napi_value TextToNato(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    std::string nato;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == TEXT_WORD_SEPARATOR) {
            continue;
        }
        bool isLetter = symbol >= NATO_LETTER_BASE && symbol <= NATO_LETTER_LAST;
        NAPI_ASSERT(env, isLetter, "NATO alphabet covers letters only");
        size_t index = static_cast<size_t>(symbol - NATO_LETTER_BASE);
        if (!nato.empty()) {
            nato += TEXT_WORD_SEPARATOR;
        }
        nato += NATO_WORDS[index];
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, nato, result));
    return result;
}

// Decodes NATO words back to letters, word by word and case-insensitively;
// unknown words throw, e.g. "hotel INDIA" -> "HI".
napi_value NatoToText(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: nato");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));
    NAPI_ASSERT(env, text.size() <= MORSE_MAX_TEXT_LENGTH, "Nato text exceeds maximum length");

    std::string decoded;
    std::string word;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == TEXT_WORD_SEPARATOR) {
            if (!word.empty()) {
                int32_t index = LookupNatoWordIndex(word);
                NAPI_ASSERT(env, index != NATO_WORD_NOT_FOUND, "Unknown NATO word");
                decoded += static_cast<char>(NATO_LETTER_BASE + index);
                word.clear();
            }
        } else {
            word += symbol;
        }
    }
    if (!word.empty()) {
        int32_t index = LookupNatoWordIndex(word);
        NAPI_ASSERT(env, index != NATO_WORD_NOT_FOUND, "Unknown NATO word");
        decoded += static_cast<char>(NATO_LETTER_BASE + index);
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, decoded, result));
    return result;
}

// Dit duration in milliseconds at the given words-per-minute speed,
// e.g. 20 wpm -> 60.0 ms per dit.
napi_value MorseWpmDuration(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: wpm");

    int32_t wpm = MORSE_COUNT_ZERO;
    NAPI_CALL(env, ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], wpm));
    NAPI_ASSERT(env, wpm >= MORSE_MIN_WPM && wpm <= MORSE_MAX_WPM, "Wpm must be within 5..60");

    double safeWpm = wpm > MORSE_ZERO_WPM ? static_cast<double>(wpm) : MORSE_SAFE_DIVISOR;
    double ditDuration = MORSE_UNITS_PER_WORD_WPM / safeWpm;

    napi_value result = nullptr;
    NAPI_CALL(env, CreateDoubleValue(env, ditDuration, result));
    return result;
}

// Farnsworth timing: characters are keyed at the faster character speed
// while the overall delivery runs at the slower wpm. Returns the dit
// duration in milliseconds at the character speed; characterWpm must not
// be smaller than wpm.
napi_value FarnsworthDitDuration(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: wpm, characterWpm");

    int32_t wpm = MORSE_COUNT_ZERO;
    NAPI_CALL(env, ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], wpm));
    NAPI_ASSERT(env, wpm >= MORSE_MIN_WPM && wpm <= MORSE_MAX_WPM, "Wpm must be within 5..60");

    int32_t characterWpm = MORSE_COUNT_ZERO;
    NAPI_CALL(env, ExtractInt32Arg(env, argv[ARG_INDEX_ONE], characterWpm));
    NAPI_ASSERT(env, characterWpm >= MORSE_MIN_WPM && characterWpm <= MORSE_MAX_WPM,
                "Character wpm must be within 5..60");
    NAPI_ASSERT(env, characterWpm >= wpm, "Character speed must be greater than or equal to overall speed");

    double safeCharacterWpm = characterWpm > MORSE_ZERO_WPM ? static_cast<double>(characterWpm) : MORSE_SAFE_DIVISOR;
    double ditDuration = MORSE_UNITS_PER_WORD_WPM / safeCharacterWpm;

    napi_value result = nullptr;
    NAPI_CALL(env, CreateDoubleValue(env, ditDuration, result));
    return result;
}

// Total dit units of the encoded text: the sum of all code units plus the
// character gaps inside words (3 units each) and the word gaps between
// words (7 units each), so "SOS" -> 21 and "HI YOU" -> 46.
napi_value MorseTimingUnits(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    int32_t codeUnits = MORSE_COUNT_ZERO;
    int32_t charCount = MORSE_COUNT_ZERO;
    int32_t wordCount = MORSE_COUNT_ZERO;
    int32_t charsInWord = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < text.size(); ++i) {
        char symbol = text[i];
        if (symbol == TEXT_WORD_SEPARATOR) {
            if (charsInWord > MORSE_COUNT_ZERO) {
                ++wordCount;
                charsInWord = MORSE_COUNT_ZERO;
            }
        } else {
            codeUnits += MorseCodeUnitCount(LookupMorseCode(symbol));
            ++charCount;
            ++charsInWord;
        }
    }
    if (charsInWord > MORSE_COUNT_ZERO) {
        ++wordCount;
    }

    int32_t totalUnits = codeUnits;
    if (wordCount > MORSE_COUNT_ZERO) {
        int32_t charGaps = (charCount - wordCount) * MORSE_CHAR_GAP_UNITS;
        int32_t wordGaps = (wordCount - MORSE_COUNT_ONE) * MORSE_WORD_GAP_UNITS;
        totalUnits += charGaps + wordGaps;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32Value(env, totalUnits, result));
    return result;
}

// Expands morse to its binary signal at dit resolution: '1' marks a
// key-down unit and '0' a gap unit. The signal covers the marks and the
// gaps between them; no trailing gap is emitted, e.g. ".-" -> "10111".
napi_value MorseSignalString(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: morse");

    std::string morse;
    NAPI_CALL(env, ExtractMorseCodeArg(env, argv[ARG_INDEX_ZERO], morse));

    std::vector<std::vector<std::string>> words = SplitMorseWords(morse);
    std::string signal;
    for (size_t w = 0; w < words.size(); ++w) {
        if (w > 0) {
            signal += SignalGapString(MORSE_WORD_GAP_UNITS);
        }
        for (size_t c = 0; c < words[w].size(); ++c) {
            if (c > 0) {
                signal += SignalGapString(MORSE_CHAR_GAP_UNITS);
            }
            signal += MorseCodeSignalString(words[w][c]);
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, signal, result));
    return result;
}

// Decodes run-length signal units into per-word character codes: mark runs of one or two
// units are dits and longer runs are dahs; gap runs of one or two units stay inside a
// character, three to six units separate characters and seven or more separate words.
static void DecodeSignalRuns(const std::string& signal, std::vector<std::vector<std::string>>& words)
{
    std::vector<std::string> currentWord;
    std::string currentCode;
    size_t position = 0;
    while (position < signal.size()) {
        char runSymbol = signal[position];
        size_t runStart = position;
        while (position < signal.size() && signal[position] == runSymbol) {
            ++position;
        }
        int32_t runLength = static_cast<int32_t>(position - runStart);
        if (runSymbol == SIGNAL_MARK_CHAR) {
            currentCode += (runLength >= SIGNAL_DAH_MIN_UNITS) ? MORSE_DASH : MORSE_DOT;
        } else if (!currentCode.empty() && runLength >= SIGNAL_CHAR_GAP_MIN_UNITS) {
            currentWord.push_back(currentCode);
            currentCode.clear();
            if (runLength >= SIGNAL_WORD_GAP_MIN_UNITS) {
                words.push_back(currentWord);
                currentWord.clear();
            }
        }
    }
    if (!currentCode.empty()) {
        currentWord.push_back(currentCode);
    }
    if (!currentWord.empty()) {
        words.push_back(currentWord);
    }
}

// Joins decoded character codes back into a morse string with word separators.
static void JoinMorseWords(const std::vector<std::vector<std::string>>& words, std::string& morse)
{
    for (size_t w = 0; w < words.size(); ++w) {
        if (w > 0) {
            morse += MORSE_WORD_GAP_TEXT;
        }
        for (size_t c = 0; c < words[w].size(); ++c) {
            if (c > 0) {
                morse += MORSE_CHAR_GAP;
            }
            morse += words[w][c];
        }
    }
}

// Decodes a binary signal back to morse, e.g. "10111" -> ".-".
napi_value SignalStringToMorse(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: signal");

    std::string signal;
    NAPI_CALL(env, ExtractSignalStringArg(env, argv[ARG_INDEX_ZERO], signal));

    std::vector<std::vector<std::string>> words;
    DecodeSignalRuns(signal, words);

    std::string morse;
    JoinMorseWords(words, morse);

    napi_value result = nullptr;
    NAPI_CALL(env, CreateStringValue(env, morse, result));
    return result;
}

// Counts the encodable characters of the text, excluding word-separator
// spaces, e.g. "HELLO WORLD" -> 10.
napi_value MorseCharCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    int32_t count = MORSE_COUNT_ZERO;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != TEXT_WORD_SEPARATOR) {
            ++count;
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32Value(env, count, result));
    return result;
}

// Counts the non-empty space-separated words of the text,
// e.g. "  HELLO   WORLD " -> 2.
napi_value MorseWordCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractMorseTextArg(env, argv[ARG_INDEX_ZERO], text));

    int32_t wordCount = MORSE_COUNT_ZERO;
    bool inWord = false;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == TEXT_WORD_SEPARATOR) {
            inWord = false;
        } else if (!inWord) {
            ++wordCount;
            inWord = true;
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateInt32Value(env, wordCount, result));
    return result;
}

// True when every character survives suite validation after uppercasing:
// letters, digits and spaces. The empty text is vacuously encodable.
napi_value IsEncodableText(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: text");

    std::string text;
    NAPI_CALL(env, ExtractStringArg(env, argv[ARG_INDEX_ZERO], text));

    bool encodable = true;
    for (size_t i = 0; encodable && i < text.size(); ++i) {
        char symbol = UppercaseAsciiChar(text[i]);
        encodable = symbol == TEXT_WORD_SEPARATOR || IsEncodableChar(symbol);
    }

    napi_value result = nullptr;
    NAPI_CALL(env, CreateBoolValue(env, encodable, result));
    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("textToMorse", TextToMorse),
        DECLARE_NAPI_FUNCTION("morseToText", MorseToText),
        DECLARE_NAPI_FUNCTION("isMorseCode", IsMorseCode),
        DECLARE_NAPI_FUNCTION("charToMorse", CharToMorse),
        DECLARE_NAPI_FUNCTION("morseToChar", MorseToChar),
        DECLARE_NAPI_FUNCTION("textToNato", TextToNato),
        DECLARE_NAPI_FUNCTION("natoToText", NatoToText),
        DECLARE_NAPI_FUNCTION("morseWpmDuration", MorseWpmDuration),
        DECLARE_NAPI_FUNCTION("farnsworthDitDuration", FarnsworthDitDuration),
        DECLARE_NAPI_FUNCTION("morseTimingUnits", MorseTimingUnits),
        DECLARE_NAPI_FUNCTION("morseSignalString", MorseSignalString),
        DECLARE_NAPI_FUNCTION("signalStringToMorse", SignalStringToMorse),
        DECLARE_NAPI_FUNCTION("morseCharCount", MorseCharCount),
        DECLARE_NAPI_FUNCTION("morseWordCount", MorseWordCount),
        DECLARE_NAPI_FUNCTION("isEncodableText", IsEncodableText),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterMorseOpsFunctions(env, exports), nullptr);
    return exports;
}

static napi_module morseSuiteModule = {
    .nm_version = MORSE_MODULE_VERSION,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "morseSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void MorseSuiteRegisterModule(void)
{
    napi_module_register(&morseSuiteModule);
}
