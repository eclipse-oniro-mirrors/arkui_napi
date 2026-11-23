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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "audio_suite_common.h"
#include "napi/native_node_api.h"

bool ExtractDoubleArg(napi_env env, napi_value value, double& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    double number = 0.0;
    NAPI_CALL_BASE(env, napi_get_value_double(env, value, &number), false);
    if (!std::isfinite(number)) {
        napi_throw_error(env, nullptr, "Argument must be a finite number");
        return false;
    }

    result = number;
    return true;
}

bool ExtractInt32Arg(napi_env env, napi_value value, int32_t& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_number) {
        napi_throw_error(env, nullptr, "Argument must be a number");
        return false;
    }

    double number = 0.0;
    NAPI_CALL_BASE(env, napi_get_value_double(env, value, &number), false);
    if (!std::isfinite(number) || std::floor(number) != number) {
        napi_throw_error(env, nullptr, "Argument must be a finite integer");
        return false;
    }
    if (number < static_cast<double>(AUDIO_MIN_MIDI_NUMBER) || number > static_cast<double>(AUDIO_MAX_MIDI_NUMBER)) {
        napi_throw_error(env, nullptr, "Argument is out of the supported integer range");
        return false;
    }

    result = static_cast<int32_t>(number);
    return true;
}

bool ExtractStringArg(napi_env env, napi_value value, std::string& result)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &type), false);
    if (type != napi_string) {
        napi_throw_error(env, nullptr, "Argument must be a string");
        return false;
    }

    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, nullptr, 0, &length), false);
    if (length < AUDIO_NOTE_NAME_MIN_LENGTH || length > AUDIO_NOTE_NAME_MAX_LENGTH) {
        napi_throw_error(env, nullptr, "Note name length is invalid");
        return false;
    }

    std::string buffer(length + AUDIO_NOTE_NAME_MIN_LENGTH, '\0');
    size_t copied = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, &buffer[0], buffer.size(), &copied), false);
    result.assign(buffer, 0, copied);
    return true;
}

bool ExtractDoubleArray(napi_env env, napi_value value, std::vector<double>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > AUDIO_MAX_SAMPLE_COUNT) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = AUDIO_UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        double number = 0.0;
        if (!ExtractDoubleArg(env, element, number)) {
            return false;
        }
        result.push_back(number);
    }
    return true;
}

napi_value CreateDoubleValue(napi_env env, double value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_double(env, value, &result));
    return result;
}

napi_value CreateInt32Value(napi_env env, int32_t value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, value, &result));
    return result;
}

napi_value CreateBoolValue(napi_env env, bool value)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result));
    return result;
}

napi_value CreateStringValue(napi_env env, const std::string& text)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, text.c_str(), text.size(), &result));
    return result;
}

napi_value CreateDoubleArrayValue(napi_env env, const std::vector<double>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateDoubleValue(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

double WrapPhase(double phase)
{
    double wrapped = std::fmod(phase, AUDIO_PHASE_WRAP);
    if (wrapped < 0.0) {
        wrapped += AUDIO_PHASE_WRAP;
    }
    return wrapped;
}

static double DbToLinearValue(double db)
{
    return std::pow(AUDIO_LOG_BASE, db / AUDIO_DB_EXPONENT);
}

static double LinearToDbValue(double linear)
{
    return AUDIO_DB_EXPONENT * std::log10(linear);
}

napi_value DbToLinear(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: decibels");

    double db = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], db)) {
        return nullptr;
    }

    return CreateDoubleValue(env, DbToLinearValue(db));
}

napi_value LinearToDb(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: linear amplitude");

    double linear = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], linear)) {
        return nullptr;
    }
    NAPI_ASSERT(env, linear >= AUDIO_MIN_LINEAR, "Linear amplitude must be positive");

    return CreateDoubleValue(env, LinearToDbValue(linear));
}

napi_value SemitonesToRatio(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: semitones");

    double semitones = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], semitones)) {
        return nullptr;
    }

    double ratio = std::pow(AUDIO_SEMITONE_RATIO_BASE, semitones / AUDIO_SEMITONES_PER_OCTAVE);
    return CreateDoubleValue(env, ratio);
}

napi_value RatioToSemitones(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: frequency ratio");

    double ratio = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], ratio)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ratio >= AUDIO_MIN_LINEAR, "Frequency ratio must be positive");

    double semitones = AUDIO_SEMITONES_PER_OCTAVE * std::log2(ratio);
    return CreateDoubleValue(env, semitones);
}

napi_value NoteToFrequency(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: midi note number");

    int32_t midi = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], midi)) {
        return nullptr;
    }
    NAPI_ASSERT(env, midi >= AUDIO_MIN_MIDI_NUMBER && midi <= AUDIO_MAX_MIDI_NUMBER, "Midi number out of range");

    double exponent = static_cast<double>(midi - AUDIO_A4_MIDI_NUMBER) / AUDIO_SEMITONES_PER_OCTAVE;
    double frequency = AUDIO_A4_FREQUENCY_HZ * std::pow(AUDIO_SEMITONE_RATIO_BASE, exponent);
    return CreateDoubleValue(env, frequency);
}

napi_value FrequencyToMidi(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: frequency in hertz");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double octaveOffset = AUDIO_SEMITONES_PER_OCTAVE * std::log2(frequency / AUDIO_A4_FREQUENCY_HZ);
    double midi = static_cast<double>(AUDIO_A4_MIDI_NUMBER) + octaveOffset + AUDIO_DOUBLE_ROUND_OFFSET;
    return CreateInt32Value(env, static_cast<int32_t>(std::floor(midi)));
}

static void BuildNoteName(int32_t midi, std::string& name)
{
    int32_t pitchClass = ((midi % AUDIO_SHARP_NOTE_COUNT) + AUDIO_SHARP_NOTE_COUNT) % AUDIO_SHARP_NOTE_COUNT;
    int32_t octave = midi / AUDIO_OCTAVE_DIVISOR - AUDIO_OCTAVE_OFFSET;
    name = AUDIO_NOTE_NAMES[pitchClass];
    name += std::to_string(octave);
}

napi_value MidiToNoteName(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: midi note number");

    int32_t midi = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], midi)) {
        return nullptr;
    }
    NAPI_ASSERT(env, midi >= AUDIO_MIN_MIDI_NUMBER && midi <= AUDIO_MAX_MIDI_NUMBER, "Midi number out of range");

    std::string name;
    BuildNoteName(midi, name);
    return CreateStringValue(env, name);
}

static int LetterToPitchClass(char letter, bool& valid)
{
    valid = false;
    for (int i = 0; i < AUDIO_NOTE_LETTER_COUNT; ++i) {
        if (AUDIO_NOTE_LETTERS[i] == letter) {
            valid = true;
            return AUDIO_NOTE_LETTER_SEMITONES[i];
        }
    }
    return -1;
}

napi_value NoteNameToMidi(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: note name");

    std::string name;
    if (!ExtractStringArg(env, argv[ARG_INDEX_ZERO], name)) {
        return nullptr;
    }

    bool valid = false;
    int pitchClass = LetterToPitchClass(name[AUDIO_NOTE_LETTER_INDEX], valid);
    NAPI_ASSERT(env, valid, "Note letter is invalid");
    if (name.size() == AUDIO_NOTE_NAME_MAX_LENGTH) {
        NAPI_ASSERT(env, name[AUDIO_NOTE_ACCIDENTAL_INDEX] == AUDIO_NOTE_SHARP_CHAR, "Only sharp notes are supported");
        pitchClass += 1;
    }

    size_t octaveStart = name.size() - 1;
    std::string octaveText = name.substr(octaveStart);
    NAPI_ASSERT(env, octaveText == "-" || (octaveText[0] >= '0' && octaveText[0] <= '9'), "Octave digit is invalid");
    int octave = 0;
    if (octaveText == "-") {
        octave = AUDIO_MIN_OCTAVE;
    } else {
        octave = octaveText[0] - '0';
    }
    NAPI_ASSERT(env, octave >= AUDIO_MIN_OCTAVE && octave <= AUDIO_MAX_OCTAVE, "Octave out of range");

    int32_t midi = (octave + AUDIO_OCTAVE_OFFSET) * AUDIO_OCTAVE_DIVISOR + pitchClass;
    NAPI_ASSERT(env, midi >= AUDIO_MIN_MIDI_NUMBER && midi <= AUDIO_MAX_MIDI_NUMBER, "Midi number out of range");
    return CreateInt32Value(env, midi);
}

napi_value BpmToBeatMs(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: tempo in bpm");

    double bpm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bpm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bpm >= AUDIO_MIN_BPM && bpm <= AUDIO_MAX_BPM, "Tempo out of range");

    return CreateDoubleValue(env, AUDIO_BEAT_MS_PER_MINUTE / bpm);
}

napi_value BeatMsToBpm(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: beat duration in milliseconds");

    double beatMs = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], beatMs)) {
        return nullptr;
    }
    NAPI_ASSERT(env, beatMs >= AUDIO_MIN_BEAT_MS && beatMs <= AUDIO_MAX_BEAT_MS, "Beat duration out of range");

    return CreateDoubleValue(env, AUDIO_BEAT_MS_PER_MINUTE / beatMs);
}

napi_value FrequencyToWavelength(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: frequency, wave speed");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double speed = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], speed)) {
        return nullptr;
    }
    NAPI_ASSERT(env, speed >= AUDIO_MIN_SPEED_OF_SOUND && speed <= AUDIO_MAX_SPEED_OF_SOUND, "Wave speed out of range");

    return CreateDoubleValue(env, speed / frequency);
}

napi_value SampleRateToNyquist(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: sample rate");

    double sampleRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], sampleRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, sampleRate >= AUDIO_MIN_SAMPLE_RATE && sampleRate <= AUDIO_MAX_SAMPLE_RATE,
                "Sample rate out of range");

    return CreateDoubleValue(env, sampleRate / AUDIO_NYQUIST_DIVISOR);
}

napi_value SemitonesBetweenFrequencies(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: from, to");

    double fromFrequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], fromFrequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fromFrequency >= AUDIO_MIN_FREQUENCY_HZ && fromFrequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double toFrequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], toFrequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, toFrequency >= AUDIO_MIN_FREQUENCY_HZ && toFrequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double semitones = AUDIO_SEMITONES_PER_OCTAVE * std::log2(toFrequency / fromFrequency);
    return CreateDoubleValue(env, semitones);
}

napi_value CentsBetweenFrequencies(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: from, to");

    double fromFrequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], fromFrequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fromFrequency >= AUDIO_MIN_FREQUENCY_HZ && fromFrequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double toFrequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], toFrequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, toFrequency >= AUDIO_MIN_FREQUENCY_HZ && toFrequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double cents = AUDIO_CENTS_PER_OCTAVE * std::log2(toFrequency / fromFrequency);
    return CreateDoubleValue(env, cents);
}

napi_value FrequencyToNoteName(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: frequency in hertz");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double octaveOffset = AUDIO_SEMITONES_PER_OCTAVE * std::log2(frequency / AUDIO_A4_FREQUENCY_HZ);
    double midi = static_cast<double>(AUDIO_A4_MIDI_NUMBER) + octaveOffset;
    if (midi < static_cast<double>(AUDIO_MIN_MIDI_NUMBER) - AUDIO_QUANTIZE_TIE_BOUNDARY ||
        midi > static_cast<double>(AUDIO_MAX_MIDI_NUMBER) + AUDIO_QUANTIZE_TIE_BOUNDARY) {
        napi_throw_error(env, nullptr, "Frequency has no matching note name");
        return nullptr;
    }
    int32_t rounded = static_cast<int32_t>(std::floor(midi + AUDIO_DOUBLE_ROUND_OFFSET));

    std::string name;
    BuildNoteName(rounded, name);
    return CreateStringValue(env, name);
}

napi_value QuantizeToNearestNote(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: frequency in hertz");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double octaveOffset = AUDIO_SEMITONES_PER_OCTAVE * std::log2(frequency / AUDIO_A4_FREQUENCY_HZ);
    double midi = static_cast<double>(AUDIO_A4_MIDI_NUMBER) + octaveOffset;
    if (midi < static_cast<double>(AUDIO_MIN_MIDI_NUMBER) - AUDIO_QUANTIZE_TIE_BOUNDARY ||
        midi > static_cast<double>(AUDIO_MAX_MIDI_NUMBER) + AUDIO_QUANTIZE_TIE_BOUNDARY) {
        napi_throw_error(env, nullptr, "Frequency cannot be quantized to a note");
        return nullptr;
    }
    int32_t rounded = static_cast<int32_t>(std::floor(midi + AUDIO_DOUBLE_ROUND_OFFSET));

    double exponent = static_cast<double>(rounded - AUDIO_A4_MIDI_NUMBER) / AUDIO_SEMITONES_PER_OCTAVE;
    double quantized = AUDIO_A4_FREQUENCY_HZ * std::pow(AUDIO_SEMITONE_RATIO_BASE, exponent);
    return CreateDoubleValue(env, quantized);
}

napi_value BpmToBarMs(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: bpm, beatsPerBar");

    double bpm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bpm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bpm >= AUDIO_MIN_BPM && bpm <= AUDIO_MAX_BPM, "Tempo out of range");

    int32_t beatsPerBar = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], beatsPerBar)) {
        return nullptr;
    }
    NAPI_ASSERT(env, beatsPerBar >= AUDIO_MIN_BEATS_PER_BAR && beatsPerBar <= AUDIO_MAX_BEATS_PER_BAR,
                "Beats per bar out of range");

    return CreateDoubleValue(env, AUDIO_BEAT_MS_PER_MINUTE * beatsPerBar / bpm);
}

napi_value DelayTimeMs(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: bpm, noteDivision");

    double bpm = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], bpm)) {
        return nullptr;
    }
    NAPI_ASSERT(env, bpm >= AUDIO_MIN_BPM && bpm <= AUDIO_MAX_BPM, "Tempo out of range");

    int32_t noteDivision = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], noteDivision)) {
        return nullptr;
    }
    NAPI_ASSERT(env, noteDivision >= AUDIO_MIN_NOTE_DIVISION && noteDivision <= AUDIO_MAX_NOTE_DIVISION,
                "Note division out of range");

    double beatMs = AUDIO_BEAT_MS_PER_MINUTE / bpm;
    double delayMs = beatMs * AUDIO_MIN_BEATS_PER_BAR * AUDIO_DOTTED_NOTE_FACTOR / noteDivision;
    return CreateDoubleValue(env, delayMs);
}

napi_value OctaveShift(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: frequency, octaves");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    int32_t octaves = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], octaves)) {
        return nullptr;
    }
    NAPI_ASSERT(env, octaves >= AUDIO_MIN_OCTAVE_SHIFT && octaves <= AUDIO_MAX_OCTAVE_SHIFT,
                "Octave shift out of range");

    double shifted = frequency * std::pow(AUDIO_OCTAVE_RATIO, octaves);
    NAPI_ASSERT(env, shifted >= AUDIO_MIN_FREQUENCY_HZ && shifted <= AUDIO_MAX_FREQUENCY_HZ,
                "Shifted frequency out of range");
    return CreateDoubleValue(env, shifted);
}

napi_value PanGains(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: pan position");

    double pan = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], pan)) {
        return nullptr;
    }
    NAPI_ASSERT(env, pan >= AUDIO_MIN_PAN && pan <= AUDIO_MAX_PAN, "Pan position out of range");

    double angle = (pan + AUDIO_PAN_HALF_RANGE) * AUDIO_TWO_PI * AUDIO_PAN_LAW_FACTOR / AUDIO_WAVE_TRI_PEAK;
    double leftGain = std::cos(angle);
    double rightGain = std::sin(angle);

    std::vector<double> gains;
    gains.push_back(leftGain);
    gains.push_back(rightGain);
    return CreateDoubleArrayValue(env, gains);
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("dbToLinear", DbToLinear),
        DECLARE_NAPI_FUNCTION("linearToDb", LinearToDb),
        DECLARE_NAPI_FUNCTION("semitonesToRatio", SemitonesToRatio),
        DECLARE_NAPI_FUNCTION("ratioToSemitones", RatioToSemitones),
        DECLARE_NAPI_FUNCTION("noteToFrequency", NoteToFrequency),
        DECLARE_NAPI_FUNCTION("frequencyToMidi", FrequencyToMidi),
        DECLARE_NAPI_FUNCTION("midiToNoteName", MidiToNoteName),
        DECLARE_NAPI_FUNCTION("noteNameToMidi", NoteNameToMidi),
        DECLARE_NAPI_FUNCTION("bpmToBeatMs", BpmToBeatMs),
        DECLARE_NAPI_FUNCTION("beatMsToBpm", BeatMsToBpm),
        DECLARE_NAPI_FUNCTION("frequencyToWavelength", FrequencyToWavelength),
        DECLARE_NAPI_FUNCTION("sampleRateToNyquist", SampleRateToNyquist),
        DECLARE_NAPI_FUNCTION("semitonesBetweenFrequencies", SemitonesBetweenFrequencies),
        DECLARE_NAPI_FUNCTION("centsBetweenFrequencies", CentsBetweenFrequencies),
        DECLARE_NAPI_FUNCTION("frequencyToNoteName", FrequencyToNoteName),
        DECLARE_NAPI_FUNCTION("quantizeToNearestNote", QuantizeToNearestNote),
        DECLARE_NAPI_FUNCTION("bpmToBarMs", BpmToBarMs),
        DECLARE_NAPI_FUNCTION("delayTimeMs", DelayTimeMs),
        DECLARE_NAPI_FUNCTION("octaveShift", OctaveShift),
        DECLARE_NAPI_FUNCTION("panGains", PanGains),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterAudioOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module audioSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "audioSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void AudioSuiteRegisterModule(void)
{
    napi_module_register(&audioSuiteModule);
}
