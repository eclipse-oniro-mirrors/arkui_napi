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

#ifndef SAMPLE_NATIVE_MODULE_AUDIO_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_AUDIO_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int REQUIRED_ARGS_THREE = 3;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;
constexpr int ARG_INDEX_TWO = 2;

constexpr double AUDIO_DB_EXPONENT = 20.0;
constexpr double AUDIO_LOG_BASE = 10.0;
constexpr double AUDIO_MIN_LINEAR = 1e-12;
constexpr double AUDIO_SEMITONE_RATIO_BASE = 2.0;
constexpr int AUDIO_SEMITONES_PER_OCTAVE = 12;
constexpr double AUDIO_DOUBLE_ROUND_OFFSET = 0.5;

constexpr int AUDIO_A4_MIDI_NUMBER = 69;
constexpr double AUDIO_A4_FREQUENCY_HZ = 440.0;
constexpr int AUDIO_MIN_MIDI_NUMBER = 0;
constexpr int AUDIO_MAX_MIDI_NUMBER = 127;
constexpr int AUDIO_SHARP_NOTE_COUNT = 12;
constexpr int AUDIO_OCTAVE_DIVISOR = 12;
constexpr int AUDIO_OCTAVE_OFFSET = 1;
constexpr size_t AUDIO_NOTE_NAME_MIN_LENGTH = 2;
constexpr size_t AUDIO_NOTE_NAME_MAX_LENGTH = 3;
constexpr size_t AUDIO_NOTE_LETTER_INDEX = 0;
constexpr size_t AUDIO_NOTE_ACCIDENTAL_INDEX = 1;
constexpr char AUDIO_NOTE_SHARP_CHAR = '#';
constexpr int AUDIO_NOTE_LETTER_COUNT = 7;
constexpr int AUDIO_MIN_OCTAVE = -1;
constexpr int AUDIO_MAX_OCTAVE = 9;

constexpr double AUDIO_BEAT_MS_PER_MINUTE = 60000.0;
constexpr double AUDIO_MIN_BPM = 1.0;
constexpr double AUDIO_MAX_BPM = 1000.0;
constexpr double AUDIO_MIN_BEAT_MS = 1.0;
constexpr double AUDIO_MAX_BEAT_MS = 60000.0;
constexpr int AUDIO_MIN_BEATS_PER_BAR = 1;
constexpr int AUDIO_MAX_BEATS_PER_BAR = 16;

constexpr double AUDIO_TWO_PI = 6.283185307179586;
constexpr double AUDIO_PHASE_WRAP = 1.0;
constexpr double AUDIO_WAVE_HIGH_LEVEL = 1.0;
constexpr double AUDIO_WAVE_LOW_LEVEL = -1.0;
constexpr double AUDIO_WAVE_SIGN_BOUNDARY = 0.5;
constexpr double AUDIO_WAVE_SAW_OFFSET = 0.5;
constexpr double AUDIO_WAVE_TRI_PEAK = 2.0;
constexpr double AUDIO_WAVE_TRI_DOUBLE_PEAK = 4.0;
constexpr double AUDIO_WAVE_AMPLITUDE = 1.0;

constexpr double AUDIO_MIN_FREQUENCY_HZ = 0.01;
constexpr double AUDIO_MAX_FREQUENCY_HZ = 96000.0;
constexpr double AUDIO_MIN_SAMPLE_RATE = 1.0;
constexpr double AUDIO_MAX_SAMPLE_RATE = 192000.0;
constexpr double AUDIO_NYQUIST_DIVISOR = 2.0;
constexpr double AUDIO_MIN_SPEED_OF_SOUND = 1.0;
constexpr double AUDIO_MAX_SPEED_OF_SOUND = 1000.0;
constexpr size_t AUDIO_MIN_SAMPLE_COUNT = 1;
constexpr size_t AUDIO_MAX_SAMPLE_COUNT = 1024;

constexpr double AUDIO_MIX_DIVISOR = 2.0;
constexpr double AUDIO_MIN_GAIN_LINEAR = 0.0;
constexpr double AUDIO_MIN_NORMALIZE_PEAK = 0.01;
constexpr double AUDIO_MAX_NORMALIZE_PEAK = 4.0;
constexpr double AUDIO_MIN_ABSOLUTE_SAMPLE = 1e-9;
constexpr double AUDIO_RMS_MEAN_SCALE = 0.5;
constexpr uint32_t AUDIO_UINT32_INDEX_ZERO = 0;

const char* const AUDIO_NOTE_NAMES[AUDIO_SHARP_NOTE_COUNT] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B",
};

const char* const AUDIO_NOTE_LETTERS = "CDEFGAB";
const int AUDIO_NOTE_LETTER_SEMITONES[AUDIO_NOTE_LETTER_COUNT] = { 0, 2, 4, 5, 7, 9, 11 };

bool ExtractDoubleArg(napi_env env, napi_value value, double& result);
bool ExtractInt32Arg(napi_env env, napi_value value, int32_t& result);
bool ExtractStringArg(napi_env env, napi_value value, std::string& result);
bool ExtractDoubleArray(napi_env env, napi_value value, std::vector<double>& result);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateInt32Value(napi_env env, int32_t value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateStringValue(napi_env env, const std::string& text);
napi_value CreateDoubleArrayValue(napi_env env, const std::vector<double>& values);
double WrapPhase(double phase);

constexpr double AUDIO_CENTS_PER_OCTAVE = 1200.0;
constexpr double AUDIO_CENTS_PER_SEMITONE = 100.0;
constexpr int AUDIO_MIN_OCTAVE_SHIFT = -4;
constexpr int AUDIO_MAX_OCTAVE_SHIFT = 4;
constexpr double AUDIO_OCTAVE_RATIO = 2.0;
constexpr double AUDIO_QUANTIZE_TIE_BOUNDARY = 0.5;
constexpr int AUDIO_MIN_NOTE_DIVISION = 1;
constexpr int AUDIO_MAX_NOTE_DIVISION = 64;
constexpr double AUDIO_DOTTED_NOTE_FACTOR = 1.5;
constexpr double AUDIO_TRIPLET_NOTE_FACTOR = 2.0 / 3.0;
constexpr double AUDIO_MIN_PAN = -1.0;
constexpr double AUDIO_MAX_PAN = 1.0;
constexpr double AUDIO_PAN_LAW_FACTOR = 0.5;
constexpr double AUDIO_PAN_HALF_RANGE = 0.5;
constexpr int AUDIO_MIN_VELOCITY = 1;
constexpr int AUDIO_MAX_VELOCITY = 127;
constexpr double AUDIO_VELOCITY_DIVISOR = 127.0;
constexpr double AUDIO_MIN_CEILING = 0.01;
constexpr double AUDIO_MAX_CEILING = 16.0;
constexpr double AUDIO_MIN_FADE_LENGTH = 1.0;
constexpr size_t AUDIO_EMPTY_SIGNAL_SIZE = 0;

napi_value DbToLinear(napi_env env, napi_callback_info info);
napi_value LinearToDb(napi_env env, napi_callback_info info);
napi_value SemitonesToRatio(napi_env env, napi_callback_info info);
napi_value RatioToSemitones(napi_env env, napi_callback_info info);
napi_value NoteToFrequency(napi_env env, napi_callback_info info);
napi_value FrequencyToMidi(napi_env env, napi_callback_info info);
napi_value MidiToNoteName(napi_env env, napi_callback_info info);
napi_value NoteNameToMidi(napi_env env, napi_callback_info info);
napi_value BpmToBeatMs(napi_env env, napi_callback_info info);
napi_value BeatMsToBpm(napi_env env, napi_callback_info info);
napi_value FrequencyToWavelength(napi_env env, napi_callback_info info);
napi_value SampleRateToNyquist(napi_env env, napi_callback_info info);
napi_value SemitonesBetweenFrequencies(napi_env env, napi_callback_info info);
napi_value CentsBetweenFrequencies(napi_env env, napi_callback_info info);
napi_value FrequencyToNoteName(napi_env env, napi_callback_info info);
napi_value QuantizeToNearestNote(napi_env env, napi_callback_info info);
napi_value BpmToBarMs(napi_env env, napi_callback_info info);
napi_value DelayTimeMs(napi_env env, napi_callback_info info);
napi_value OctaveShift(napi_env env, napi_callback_info info);
napi_value PanGains(napi_env env, napi_callback_info info);

napi_value SineWaveValue(napi_env env, napi_callback_info info);
napi_value SquareWaveValue(napi_env env, napi_callback_info info);
napi_value SawtoothWaveValue(napi_env env, napi_callback_info info);
napi_value TriangleWaveValue(napi_env env, napi_callback_info info);
napi_value GenerateSineWave(napi_env env, napi_callback_info info);
napi_value GenerateSquareWave(napi_env env, napi_callback_info info);
napi_value GenerateSawtoothWave(napi_env env, napi_callback_info info);
napi_value GenerateTriangleWave(napi_env env, napi_callback_info info);
napi_value MixSignals(napi_env env, napi_callback_info info);
napi_value ApplyGain(napi_env env, napi_callback_info info);
napi_value ApplyGainDb(napi_env env, napi_callback_info info);
napi_value NormalizePeak(napi_env env, napi_callback_info info);
napi_value ReverseSamples(napi_env env, napi_callback_info info);
napi_value PeakAmplitude(napi_env env, napi_callback_info info);
napi_value RmsLevel(napi_env env, napi_callback_info info);
napi_value RmsToDb(napi_env env, napi_callback_info info);
napi_value RemoveDcOffset(napi_env env, napi_callback_info info);
napi_value ClipSamples(napi_env env, napi_callback_info info);
napi_value FadeIn(napi_env env, napi_callback_info info);
napi_value FadeOut(napi_env env, napi_callback_info info);
napi_value ZeroCrossingCount(napi_env env, napi_callback_info info);
napi_value SignalEnergy(napi_env env, napi_callback_info info);
napi_value PeakIndex(napi_env env, napi_callback_info info);
napi_value InvertPhase(napi_env env, napi_callback_info info);
napi_value AmplitudeToVelocity(napi_env env, napi_callback_info info);
napi_value VelocityToAmplitude(napi_env env, napi_callback_info info);

napi_status RegisterAudioOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_AUDIO_SUITE_COMMON_H
