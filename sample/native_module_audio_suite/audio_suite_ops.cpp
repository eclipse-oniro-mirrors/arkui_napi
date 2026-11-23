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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "audio_suite_common.h"
#include "napi/native_node_api.h"

static double SineWaveSample(double phase)
{
    return AUDIO_WAVE_AMPLITUDE * std::sin(AUDIO_TWO_PI * WrapPhase(phase));
}

static double SquareWaveSample(double phase)
{
    double wrapped = WrapPhase(phase);
    if (wrapped < AUDIO_WAVE_SIGN_BOUNDARY) {
        return AUDIO_WAVE_HIGH_LEVEL;
    }
    return AUDIO_WAVE_LOW_LEVEL;
}

static double SawtoothWaveSample(double phase)
{
    double wrapped = WrapPhase(phase);
    return AUDIO_WAVE_TRI_DOUBLE_PEAK * (wrapped - AUDIO_WAVE_SAW_OFFSET);
}

static double TriangleWaveSample(double phase)
{
    double wrapped = WrapPhase(phase);
    if (wrapped < AUDIO_WAVE_SIGN_BOUNDARY) {
        return AUDIO_WAVE_TRI_PEAK * wrapped - AUDIO_WAVE_AMPLITUDE;
    }
    return AUDIO_WAVE_AMPLITUDE - AUDIO_WAVE_TRI_PEAK * (wrapped - AUDIO_WAVE_SIGN_BOUNDARY);
}

static void GenerateWaveform(double frequency,
                             double sampleRate,
                             size_t count,
                             double (*generator)(double),
                             std::vector<double>& out)
{
    out.clear();
    out.reserve(count);
    double safeRate = sampleRate > 0.0 ? sampleRate : 1.0;
    double step = frequency / safeRate;
    double phase = 0.0;
    for (size_t i = 0; i < count; ++i) {
        out.push_back(generator(phase));
        phase += step;
        if (phase >= AUDIO_PHASE_WRAP) {
            phase -= AUDIO_PHASE_WRAP;
        }
    }
}

static bool ExtractSampleCount(napi_env env, napi_value value, size_t& count)
{
    double number = 0.0;
    if (!ExtractDoubleArg(env, value, number)) {
        return false;
    }
    if (std::floor(number) != number || number < static_cast<double>(AUDIO_MIN_SAMPLE_COUNT) ||
        number > static_cast<double>(AUDIO_MAX_SAMPLE_COUNT)) {
        napi_throw_error(env, nullptr, "Sample count is out of range");
        return false;
    }
    count = static_cast<size_t>(number);
    return true;
}

napi_value SineWaveValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: phase in cycles");

    double phase = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], phase)) {
        return nullptr;
    }

    return CreateDoubleValue(env, SineWaveSample(phase));
}

napi_value SquareWaveValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: phase in cycles");

    double phase = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], phase)) {
        return nullptr;
    }

    return CreateDoubleValue(env, SquareWaveSample(phase));
}

napi_value SawtoothWaveValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: phase in cycles");

    double phase = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], phase)) {
        return nullptr;
    }

    return CreateDoubleValue(env, SawtoothWaveSample(phase));
}

napi_value TriangleWaveValue(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: phase in cycles");

    double phase = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], phase)) {
        return nullptr;
    }

    return CreateDoubleValue(env, TriangleWaveSample(phase));
}

napi_value GenerateSineWave(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: frequency, sampleRate, count");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double sampleRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], sampleRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, sampleRate >= AUDIO_MIN_SAMPLE_RATE && sampleRate <= AUDIO_MAX_SAMPLE_RATE,
                "Sample rate out of range");

    size_t count = 0;
    if (!ExtractSampleCount(env, argv[ARG_INDEX_TWO], count)) {
        return nullptr;
    }

    std::vector<double> samples;
    GenerateWaveform(frequency, sampleRate, count, SineWaveSample, samples);
    return CreateDoubleArrayValue(env, samples);
}

napi_value GenerateSquareWave(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: frequency, sampleRate, count");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double sampleRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], sampleRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, sampleRate >= AUDIO_MIN_SAMPLE_RATE && sampleRate <= AUDIO_MAX_SAMPLE_RATE,
                "Sample rate out of range");

    size_t count = 0;
    if (!ExtractSampleCount(env, argv[ARG_INDEX_TWO], count)) {
        return nullptr;
    }

    std::vector<double> samples;
    GenerateWaveform(frequency, sampleRate, count, SquareWaveSample, samples);
    return CreateDoubleArrayValue(env, samples);
}

napi_value GenerateSawtoothWave(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: frequency, sampleRate, count");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double sampleRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], sampleRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, sampleRate >= AUDIO_MIN_SAMPLE_RATE && sampleRate <= AUDIO_MAX_SAMPLE_RATE,
                "Sample rate out of range");

    size_t count = 0;
    if (!ExtractSampleCount(env, argv[ARG_INDEX_TWO], count)) {
        return nullptr;
    }

    std::vector<double> samples;
    GenerateWaveform(frequency, sampleRate, count, SawtoothWaveSample, samples);
    return CreateDoubleArrayValue(env, samples);
}

napi_value GenerateTriangleWave(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: frequency, sampleRate, count");

    double frequency = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], frequency)) {
        return nullptr;
    }
    NAPI_ASSERT(env, frequency >= AUDIO_MIN_FREQUENCY_HZ && frequency <= AUDIO_MAX_FREQUENCY_HZ,
                "Frequency out of range");

    double sampleRate = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], sampleRate)) {
        return nullptr;
    }
    NAPI_ASSERT(env, sampleRate >= AUDIO_MIN_SAMPLE_RATE && sampleRate <= AUDIO_MAX_SAMPLE_RATE,
                "Sample rate out of range");

    size_t count = 0;
    if (!ExtractSampleCount(env, argv[ARG_INDEX_TWO], count)) {
        return nullptr;
    }

    std::vector<double> samples;
    GenerateWaveform(frequency, sampleRate, count, TriangleWaveSample, samples);
    return CreateDoubleArrayValue(env, samples);
}

napi_value MixSignals(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: left, right");

    std::vector<double> left;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], left)) {
        return nullptr;
    }

    std::vector<double> right;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ONE], right)) {
        return nullptr;
    }
    NAPI_ASSERT(env, left.size() == right.size(), "Both signals must have the same length");
    NAPI_ASSERT(env, !left.empty(), "Signals must not be empty");

    std::vector<double> mixed;
    mixed.reserve(left.size());
    for (size_t i = 0; i < left.size(); ++i) {
        mixed.push_back((left[i] + right[i]) / AUDIO_MIX_DIVISOR);
    }
    return CreateDoubleArrayValue(env, mixed);
}

napi_value ApplyGain(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: samples, gain");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double gain = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], gain)) {
        return nullptr;
    }
    NAPI_ASSERT(env, gain >= AUDIO_MIN_GAIN_LINEAR, "Gain must not be negative");

    std::vector<double> result;
    result.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        result.push_back(samples[i] * gain);
    }
    return CreateDoubleArrayValue(env, result);
}

napi_value ApplyGainDb(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: samples, gainDb");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double gainDb = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], gainDb)) {
        return nullptr;
    }

    double gain = std::pow(AUDIO_LOG_BASE, gainDb / AUDIO_DB_EXPONENT);
    std::vector<double> result;
    result.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        result.push_back(samples[i] * gain);
    }
    return CreateDoubleArrayValue(env, result);
}

napi_value NormalizePeak(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: samples, targetPeak");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }

    double targetPeak = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], targetPeak)) {
        return nullptr;
    }
    NAPI_ASSERT(env, targetPeak >= AUDIO_MIN_NORMALIZE_PEAK && targetPeak <= AUDIO_MAX_NORMALIZE_PEAK,
                "Target peak out of range");

    double peak = 0.0;
    for (size_t i = 0; i < samples.size(); ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    if (peak <= 0.0) {
        napi_throw_error(env, nullptr, "Signal peak is too small to normalize");
        return nullptr;
    }

    double safePeak = peak > 0.0 ? peak : 1.0;
    double scale = targetPeak / safePeak;
    std::vector<double> result;
    result.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        result.push_back(samples[i] * scale);
    }
    return CreateDoubleArrayValue(env, result);
}

napi_value ReverseSamples(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }

    std::vector<double> result = samples;
    std::reverse(result.begin(), result.end());
    return CreateDoubleArrayValue(env, result);
}

napi_value PeakAmplitude(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double peak = 0.0;
    for (size_t i = 0; i < samples.size(); ++i) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return CreateDoubleValue(env, peak);
}

static double RmsValue(const std::vector<double>& samples)
{
    double sumSquares = 0.0;
    for (size_t i = 0; i < samples.size(); ++i) {
        sumSquares += samples[i] * samples[i];
    }
    return std::sqrt(sumSquares / static_cast<double>(samples.size()));
}

napi_value RmsLevel(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    return CreateDoubleValue(env, RmsValue(samples));
}

napi_value RmsToDb(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double rms = RmsValue(samples);
    NAPI_ASSERT(env, rms >= AUDIO_MIN_ABSOLUTE_SAMPLE, "Signal is silent");

    return CreateDoubleValue(env, AUDIO_DB_EXPONENT * std::log10(rms));
}

napi_value RemoveDcOffset(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double mean = 0.0;
    for (size_t i = 0; i < samples.size(); ++i) {
        mean += samples[i];
    }
    mean /= static_cast<double>(samples.size());

    std::vector<double> result;
    result.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        result.push_back(samples[i] - mean);
    }
    return CreateDoubleArrayValue(env, result);
}

napi_value ClipSamples(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: samples, ceiling");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double ceiling = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], ceiling)) {
        return nullptr;
    }
    NAPI_ASSERT(env, ceiling >= AUDIO_MIN_CEILING && ceiling <= AUDIO_MAX_CEILING, "Ceiling out of range");

    std::vector<double> result;
    result.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        result.push_back(std::max(-ceiling, std::min(ceiling, samples[i])));
    }
    return CreateDoubleArrayValue(env, result);
}

static void ApplyFade(std::vector<double>& samples, size_t fadeLength, bool fadeFromSilence)
{
    if (fadeLength == 0) {
        return;
    }
    for (size_t i = 0; i < fadeLength && i < samples.size(); ++i) {
        double progress = static_cast<double>(i) / static_cast<double>(fadeLength);
        if (fadeFromSilence) {
            samples[i] *= progress;
        } else {
            samples[samples.size() - 1 - i] *= AUDIO_WAVE_AMPLITUDE - progress;
        }
    }
}

napi_value FadeIn(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: samples, fadeLength");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double fadeLengthArg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], fadeLengthArg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fadeLengthArg >= AUDIO_MIN_FADE_LENGTH && fadeLengthArg <= static_cast<double>(samples.size()),
                "Fade length exceeds sample count");

    size_t fadeLength = static_cast<size_t>(fadeLengthArg);
    std::vector<double> result = samples;
    ApplyFade(result, fadeLength, true);
    return CreateDoubleArrayValue(env, result);
}

napi_value FadeOut(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: samples, fadeLength");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double fadeLengthArg = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ONE], fadeLengthArg)) {
        return nullptr;
    }
    NAPI_ASSERT(env, fadeLengthArg >= AUDIO_MIN_FADE_LENGTH && fadeLengthArg <= static_cast<double>(samples.size()),
                "Fade length exceeds sample count");

    size_t fadeLength = static_cast<size_t>(fadeLengthArg);
    std::vector<double> result = samples;
    ApplyFade(result, fadeLength, false);
    return CreateDoubleArrayValue(env, result);
}

napi_value ZeroCrossingCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    int64_t crossings = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        bool wasNonNegative = samples[i - 1] >= 0.0;
        bool isNonNegative = samples[i] >= 0.0;
        if (wasNonNegative != isNonNegative) {
            ++crossings;
        }
    }
    return CreateInt32Value(env, static_cast<int32_t>(crossings));
}

napi_value SignalEnergy(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    double energy = 0.0;
    for (size_t i = 0; i < samples.size(); ++i) {
        energy += samples[i] * samples[i];
    }
    return CreateDoubleValue(env, energy);
}

napi_value PeakIndex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    size_t index = 0;
    double peak = 0.0;
    for (size_t i = 0; i < samples.size(); ++i) {
        double magnitude = std::abs(samples[i]);
        if (magnitude > peak) {
            peak = magnitude;
            index = i;
        }
    }
    return CreateInt32Value(env, static_cast<int32_t>(index));
}

napi_value InvertPhase(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: samples");

    std::vector<double> samples;
    if (!ExtractDoubleArray(env, argv[ARG_INDEX_ZERO], samples)) {
        return nullptr;
    }
    NAPI_ASSERT(env, !samples.empty(), "Samples must not be empty");

    std::vector<double> result;
    result.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        result.push_back(-samples[i]);
    }
    return CreateDoubleArrayValue(env, result);
}

napi_value AmplitudeToVelocity(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: amplitude");

    double amplitude = 0.0;
    if (!ExtractDoubleArg(env, argv[ARG_INDEX_ZERO], amplitude)) {
        return nullptr;
    }
    NAPI_ASSERT(env, amplitude >= AUDIO_MIN_GAIN_LINEAR && amplitude <= AUDIO_WAVE_AMPLITUDE, "Amplitude out of range");

    int32_t velocity = static_cast<int32_t>(std::round(amplitude * AUDIO_VELOCITY_DIVISOR));
    return CreateInt32Value(env, velocity);
}

napi_value VelocityToAmplitude(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: velocity");

    int32_t velocity = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], velocity)) {
        return nullptr;
    }
    NAPI_ASSERT(env, velocity >= AUDIO_MIN_VELOCITY && velocity <= AUDIO_MAX_VELOCITY, "Velocity out of range");

    double amplitude = static_cast<double>(velocity) / AUDIO_VELOCITY_DIVISOR;
    return CreateDoubleValue(env, amplitude);
}

napi_status RegisterAudioOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("sineWaveValue", SineWaveValue),
        DECLARE_NAPI_FUNCTION("squareWaveValue", SquareWaveValue),
        DECLARE_NAPI_FUNCTION("sawtoothWaveValue", SawtoothWaveValue),
        DECLARE_NAPI_FUNCTION("triangleWaveValue", TriangleWaveValue),
        DECLARE_NAPI_FUNCTION("generateSineWave", GenerateSineWave),
        DECLARE_NAPI_FUNCTION("generateSquareWave", GenerateSquareWave),
        DECLARE_NAPI_FUNCTION("generateSawtoothWave", GenerateSawtoothWave),
        DECLARE_NAPI_FUNCTION("generateTriangleWave", GenerateTriangleWave),
        DECLARE_NAPI_FUNCTION("mixSignals", MixSignals),
        DECLARE_NAPI_FUNCTION("applyGain", ApplyGain),
        DECLARE_NAPI_FUNCTION("applyGainDb", ApplyGainDb),
        DECLARE_NAPI_FUNCTION("normalizePeak", NormalizePeak),
        DECLARE_NAPI_FUNCTION("reverseSamples", ReverseSamples),
        DECLARE_NAPI_FUNCTION("peakAmplitude", PeakAmplitude),
        DECLARE_NAPI_FUNCTION("rmsLevel", RmsLevel),
        DECLARE_NAPI_FUNCTION("rmsToDb", RmsToDb),
        DECLARE_NAPI_FUNCTION("removeDcOffset", RemoveDcOffset),
        DECLARE_NAPI_FUNCTION("clipSamples", ClipSamples),
        DECLARE_NAPI_FUNCTION("fadeIn", FadeIn),
        DECLARE_NAPI_FUNCTION("fadeOut", FadeOut),
        DECLARE_NAPI_FUNCTION("zeroCrossingCount", ZeroCrossingCount),
        DECLARE_NAPI_FUNCTION("signalEnergy", SignalEnergy),
        DECLARE_NAPI_FUNCTION("peakIndex", PeakIndex),
        DECLARE_NAPI_FUNCTION("invertPhase", InvertPhase),
        DECLARE_NAPI_FUNCTION("amplitudeToVelocity", AmplitudeToVelocity),
        DECLARE_NAPI_FUNCTION("velocityToAmplitude", VelocityToAmplitude),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
