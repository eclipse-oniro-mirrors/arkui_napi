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

#ifndef SAMPLE_NATIVE_MODULE_MORSE_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_MORSE_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;

constexpr int MORSE_MODULE_VERSION = 1;

// Plain text accepts letters, digits and spaces, up to this length.
constexpr size_t MORSE_MAX_TEXT_LENGTH = 256;
// Morse strings hold dot/dash codes separated by spaces and slashes.
constexpr size_t MORSE_MAX_CODE_LENGTH = 1024;
// Signal strings expand morse to binary marks at dit resolution.
constexpr size_t MORSE_MAX_SIGNAL_LENGTH = 4096;

// Morse timing at dit resolution: a dit keys 1 unit, a dah keys 3 units,
// the gap inside a character is 1 unit, between characters 3 units and
// between words 7 units.
constexpr int32_t MORSE_DIT_UNITS = 1;
constexpr int32_t MORSE_DAH_UNITS = 3;
constexpr int32_t MORSE_INTRA_CHAR_GAP_UNITS = 1;
constexpr int32_t MORSE_CHAR_GAP_UNITS = 3;
constexpr int32_t MORSE_WORD_GAP_UNITS = 7;

// A standard PARIS word takes 50 dit units, so one dit lasts
// 1200 / wpm milliseconds at the given words-per-minute speed.
constexpr double MORSE_UNITS_PER_WORD_WPM = 1200.0;
constexpr int32_t MORSE_MIN_WPM = 5;
constexpr int32_t MORSE_MAX_WPM = 60;
constexpr int32_t MORSE_ZERO_WPM = 0;

// Letters A-Z plus digits 0-9 share one combined alphabet table.
constexpr size_t MORSE_ALPHABET_SIZE = 36;
// NATO phonetic words exist for the 26 letters only.
constexpr size_t NATO_ALPHABET_SIZE = 26;

// Valid morse codes hold between one and five dot/dash symbols.
constexpr size_t MORSE_MIN_CODE_SYMBOLS = 1;
constexpr size_t MORSE_MAX_CODE_SYMBOLS = 5;
constexpr size_t MORSE_SINGLE_CHAR_LENGTH = 1;
constexpr size_t STRING_NULL_TERMINATOR = 1;

// Histogram bins: one per possible code length 1..5.
constexpr size_t MORSE_HISTOGRAM_BINS = 5;
constexpr size_t MORSE_HISTOGRAM_INDEX_BASE = 1;

// Radio-style chunking bounds for morseChunks.
constexpr int32_t MORSE_MIN_CHUNK_SIZE = 1;
constexpr int32_t MORSE_MAX_CHUNK_SIZE = 10;

// Signal decoding thresholds: mark runs of 3 or more units are dahs, gap
// runs of 3 or more units separate characters and 7 or more separate words.
constexpr int32_t SIGNAL_DAH_MIN_UNITS = 3;
constexpr int32_t SIGNAL_CHAR_GAP_MIN_UNITS = 3;
constexpr int32_t SIGNAL_WORD_GAP_MIN_UNITS = 7;

// Similarity scoring: per symbol the unit distance (dot 1, dah 3) is at
// most the dah-minus-dit span, so the whole-code distance stays within
// span * maxLength and the normalized score within 0..1.
constexpr int32_t MORSE_UNIT_SPAN = MORSE_DAH_UNITS - MORSE_DIT_UNITS;
constexpr double MORSE_PAD_UNITS = 0.0;
constexpr double MORSE_FULL_SIMILARITY = 1.0;
constexpr double MORSE_ZERO_DOUBLE = 0.0;
constexpr double MORSE_SAFE_DIVISOR = 1.0;

// Sentinel values used by the lookup helpers.
constexpr int32_t MORSE_COUNT_ZERO = 0;
constexpr int32_t MORSE_COUNT_ONE = 1;
constexpr int32_t NATO_WORD_NOT_FOUND = -1;
constexpr char MORSE_NOT_FOUND = 0;

// Morse alphabet characters: dots, dashes, code separators and word separators.
constexpr char MORSE_DOT = '.';
constexpr char MORSE_DASH = '-';
constexpr char MORSE_CHAR_GAP = ' ';
constexpr char MORSE_WORD_SEPARATOR = '/';
// Word gaps are written as " / " between the codes of adjacent words.
const char* const MORSE_WORD_GAP_TEXT = " / ";
// Plain text words are separated by single spaces.
constexpr char TEXT_WORD_SEPARATOR = ' ';
// Signal strings mark key-down units with '1' and gap units with '0'.
constexpr char SIGNAL_MARK_CHAR = '1';
constexpr char SIGNAL_SPACE_CHAR = '0';
// NATO lookups are anchored on the uppercase letter range.
constexpr char NATO_LETTER_BASE = 'A';
constexpr char NATO_LETTER_LAST = 'Z';

// Parallel tables: MORSE_LETTERS[i] is encoded by MORSE_CODES[i], and the
// first NATO_ALPHABET_SIZE letters also map to NATO_WORDS[i].
const char* const MORSE_LETTERS[MORSE_ALPHABET_SIZE] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R",
    "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
};

const char* const MORSE_CODES[MORSE_ALPHABET_SIZE] = {
    ".-",   "-...", "-.-.",  "-..",   ".",     "..-.",  "--.",   "....",  "..",    ".---",  "-.-",   ".-..",
    "--",   "-.",   "---",   ".--.",  "--.-",  ".-.",   "...",   "-",     "..-",   "...-",  ".--",   "-..-",
    "-.--", "--..", "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----.",
};

const char* const NATO_WORDS[NATO_ALPHABET_SIZE] = {
    "Alfa",    "Bravo", "Charlie", "Delta",  "Echo",     "Foxtrot", "Golf",   "Hotel",  "India",
    "Juliett", "Kilo",  "Lima",    "Mike",   "November", "Oscar",   "Papa",   "Quebec", "Romeo",
    "Sierra",  "Tango", "Uniform", "Victor", "Whiskey",  "X-ray",   "Yankee", "Zulu",
};

// Domain helpers shared by both translation units.
char UppercaseAsciiChar(char symbol);
bool IsEncodableChar(char symbol);
const char* LookupMorseCode(char letter);
char LookupMorseChar(const std::string& code);
int32_t LookupNatoWordIndex(const std::string& word);
int32_t MorseSymbolUnits(char symbol);
int32_t MorseCodeUnitCount(const char* code);
int32_t MorseCodeSymbolCount(const char* code);
std::string SignalMarkString(int32_t units);
std::string SignalGapString(int32_t units);
std::string MorseCodeSignalString(const std::string& code);
std::vector<std::vector<std::string>> SplitMorseWords(const std::string& morse);

// Argument extraction helpers.
napi_status ExtractInt32Arg(napi_env env, napi_value value, int32_t& result);
napi_status ExtractStringArg(napi_env env, napi_value value, std::string& result);
napi_status ExtractMorseTextArg(napi_env env, napi_value value, std::string& result);
napi_status ExtractMorseCodeArg(napi_env env, napi_value value, std::string& result);
napi_status ExtractMorseTokenArg(napi_env env, napi_value value, std::string& result);
napi_status ExtractSignalStringArg(napi_env env, napi_value value, std::string& result);

// Result creation helpers.
napi_status CreateStringValue(napi_env env, const std::string& text, napi_value& result);
napi_status CreateInt32Value(napi_env env, int32_t value, napi_value& result);
napi_status CreateDoubleValue(napi_env env, double value, napi_value& result);
napi_status CreateBoolValue(napi_env env, bool value, napi_value& result);
napi_status CreateStringArrayValue(napi_env env, const std::vector<std::string>& values, napi_value& result);
napi_status CreateInt32ArrayValue(napi_env env, const std::vector<int32_t>& values, napi_value& result);

// Text and morse conversion entry points.
napi_value TextToMorse(napi_env env, napi_callback_info info);
napi_value MorseToText(napi_env env, napi_callback_info info);
napi_value IsMorseCode(napi_env env, napi_callback_info info);
napi_value CharToMorse(napi_env env, napi_callback_info info);
napi_value MorseToChar(napi_env env, napi_callback_info info);
napi_value TextToNato(napi_env env, napi_callback_info info);
napi_value NatoToText(napi_env env, napi_callback_info info);
napi_value MorseWpmDuration(napi_env env, napi_callback_info info);
napi_value FarnsworthDitDuration(napi_env env, napi_callback_info info);
napi_value MorseTimingUnits(napi_env env, napi_callback_info info);
napi_value MorseSignalString(napi_env env, napi_callback_info info);
napi_value SignalStringToMorse(napi_env env, napi_callback_info info);
napi_value MorseCharCount(napi_env env, napi_callback_info info);
napi_value MorseWordCount(napi_env env, napi_callback_info info);
napi_value IsEncodableText(napi_env env, napi_callback_info info);

// Code analysis and splitting entry points.
napi_value MorseCodeUnits(napi_env env, napi_callback_info info);
napi_value LongestMorseChar(napi_env env, napi_callback_info info);
napi_value ShortestMorseChar(napi_env env, napi_callback_info info);
napi_value MorseLengthHistogram(napi_env env, napi_callback_info info);
napi_value DotDashCount(napi_env env, napi_callback_info info);
napi_value DashRatio(napi_env env, napi_callback_info info);
napi_value ReverseMorseCode(napi_env env, napi_callback_info info);
napi_value IsMorsePalindrome(napi_env env, napi_callback_info info);
napi_value InvertMorseCode(napi_env env, napi_callback_info info);
napi_value MorseCommonPrefix(napi_env env, napi_callback_info info);
napi_value MorseSimilarity(napi_env env, napi_callback_info info);
napi_value MorseCharAt(napi_env env, napi_callback_info info);
napi_value MorseSplitWords(napi_env env, napi_callback_info info);
napi_value MorseSplitChars(napi_env env, napi_callback_info info);
napi_value JoinMorseCodes(napi_env env, napi_callback_info info);
napi_value MorseChunks(napi_env env, napi_callback_info info);
napi_value StripUnsupportedChars(napi_env env, napi_callback_info info);
napi_value CountUnsupportedChars(napi_env env, napi_callback_info info);
napi_value NatoWordFor(napi_env env, napi_callback_info info);
napi_value NatoLetterFor(napi_env env, napi_callback_info info);
napi_value IsNatoWord(napi_env env, napi_callback_info info);

napi_status RegisterMorseOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_MORSE_SUITE_COMMON_H
