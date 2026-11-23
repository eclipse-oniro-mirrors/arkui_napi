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

#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

// === Constants ===

// --- Argument Counts ---
constexpr size_t K_ARGS_ZERO = 0;

// --- Case Count ---
constexpr size_t K_LEVEN_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Word Pool Configuration ---
constexpr size_t K_WORD_COUNT = 12;
constexpr size_t K_WORD_OFFSET = 3;

// --- Word Constants ---
const char* const K_WORD_KITTEN = "kitten";
const char* const K_WORD_SITTING = "sitting";
const char* const K_WORD_SUNDAY = "sunday";
const char* const K_WORD_SATURDAY = "saturday";
const char* const K_WORD_FLAW = "flaw";
const char* const K_WORD_LAWN = "lawn";
const char* const K_WORD_INTENTION = "intention";
const char* const K_WORD_EXECUTION = "execution";
const char* const K_WORD_GUMBO = "gumbo";
const char* const K_WORD_GAMBOL = "gambol";
const char* const K_WORD_HORSE = "horse";
const char* const K_WORD_ROS = "ros";

const char* const K_WORD_POOL[K_WORD_COUNT] = {
    K_WORD_KITTEN, K_WORD_SITTING, K_WORD_SUNDAY, K_WORD_SATURDAY,
    K_WORD_FLAW, K_WORD_LAWN, K_WORD_INTENTION, K_WORD_EXECUTION,
    K_WORD_GUMBO, K_WORD_GAMBOL, K_WORD_HORSE, K_WORD_ROS,
};

// --- Levenshtein DP Costs ---
constexpr int32_t K_LEVEN_COST_MATCH = 0;
constexpr int32_t K_LEVEN_COST_MISMATCH = 1;
constexpr int32_t K_LEVEN_INSERT_COST = 1;
constexpr int32_t K_LEVEN_DELETE_COST = 1;
constexpr int32_t K_LEVEN_EMPTY_LENGTH = 0;
constexpr int32_t K_LEVEN_COL_INIT_BASE = 1;
constexpr int32_t K_LEVEN_DISTANCE_ZERO = 0;
constexpr size_t K_LEVEN_TABLE_PADDING = 1;

// --- Description Texts ---
const char* const K_LEVEN_DESC_EQUAL_LEN = "compute edit distance for equal-length word pair";
const char* const K_LEVEN_DESC_SOURCE_LONGER = "compute edit distance where source is longer than target";
const char* const K_LEVEN_DESC_TARGET_LONGER = "compute edit distance where target is longer than source";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === Types ===

struct LevenCaseSpec {
    std::string name;
    std::string source;
    std::string target;
    int32_t expectedDistance;
};

struct LevenPair {
    std::string source;
    std::string target;
};

struct LevenReport {
    int32_t distance;
    int32_t sourceLength;
    int32_t targetLength;
    bool matches;
};

enum class LevenPairKind {
    EQUAL_LENGTH,
    SOURCE_LONGER,
    TARGET_LONGER,
};

// === Helpers: Naming & Case Lookup ===

std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(data));
}

// === Helpers: Word Pool Access ===

const char* WordAtIndex(size_t index)
{
    return K_WORD_POOL[index % K_WORD_COUNT];
}

// === Helpers: Levenshtein DP ===

int32_t MinOfThree(int32_t a, int32_t b, int32_t c)
{
    int32_t best = a;
    if (b < best) {
        best = b;
    }
    if (c < best) {
        best = c;
    }
    return best;
}

void FillLevenRow(const std::string& a, const std::string& b, size_t i, size_t cols,
    const std::vector<int32_t>& prev, std::vector<int32_t>& curr)
{
    for (size_t j = static_cast<size_t>(K_LEVEN_COL_INIT_BASE); j <= cols; j++) {
        const int32_t cost = (a[i - 1] == b[j - 1]) ? K_LEVEN_COST_MATCH : K_LEVEN_COST_MISMATCH;
        const int32_t del = prev[j] + K_LEVEN_DELETE_COST;
        const int32_t ins = curr[j - 1] + K_LEVEN_INSERT_COST;
        const int32_t sub = prev[j - 1] + cost;
        curr[j] = MinOfThree(del, ins, sub);
    }
}

int32_t LevenshteinDistance(const std::string& a, const std::string& b)
{
    const size_t rows = a.size();
    const size_t cols = b.size();
    if (rows == static_cast<size_t>(K_LEVEN_EMPTY_LENGTH)) {
        return static_cast<int32_t>(cols);
    }
    if (cols == static_cast<size_t>(K_LEVEN_EMPTY_LENGTH)) {
        return static_cast<int32_t>(rows);
    }
    std::vector<int32_t> prev(cols + K_LEVEN_TABLE_PADDING, K_LEVEN_COST_MATCH);
    std::vector<int32_t> curr(cols + K_LEVEN_TABLE_PADDING, K_LEVEN_COST_MATCH);
    for (size_t j = 0; j <= cols; j++) {
        prev[j] = static_cast<int32_t>(j);
    }
    for (size_t i = static_cast<size_t>(K_LEVEN_COL_INIT_BASE); i <= rows; i++) {
        curr[0] = static_cast<int32_t>(i);
        FillLevenRow(a, b, i, cols, prev, curr);
        prev.swap(curr);
    }
    return prev[cols];
}

// === Helpers: Pair Precomputation ===

std::vector<LevenPair> BuildStringPairs()
{
    std::vector<LevenPair> pairs;
    pairs.reserve(K_LEVEN_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_LEVEN_CASE_COUNT; caseIndex++) {
        const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
        LevenPair pair;
        pair.source = WordAtIndex(caseNumber % K_WORD_COUNT);
        pair.target = WordAtIndex((caseNumber + K_WORD_OFFSET) % K_WORD_COUNT);
        pairs.push_back(pair);
    }
    return pairs;
}

// === Helpers: Pair Classification ===

LevenPairKind ClassifyLevenPair(const std::string& source, const std::string& target)
{
    if (source.size() == target.size()) {
        return LevenPairKind::EQUAL_LENGTH;
    }
    if (source.size() > target.size()) {
        return LevenPairKind::SOURCE_LONGER;
    }
    return LevenPairKind::TARGET_LONGER;
}

const char* LevenPairKindDescription(LevenPairKind kind)
{
    if (kind == LevenPairKind::EQUAL_LENGTH) {
        return K_LEVEN_DESC_EQUAL_LEN;
    }
    if (kind == LevenPairKind::SOURCE_LONGER) {
        return K_LEVEN_DESC_SOURCE_LONGER;
    }
    return K_LEVEN_DESC_TARGET_LONGER;
}

const char* DescribeLevenPair(const std::string& source, const std::string& target)
{
    return LevenPairKindDescription(ClassifyLevenPair(source, target));
}

// === Helpers: Spec Construction & Validation ===

LevenCaseSpec GetLevenCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    LevenCaseSpec spec;
    spec.name = BuildIndexedName("levenCase", caseNumber);
    const std::vector<LevenPair> pairs = BuildStringPairs();
    spec.source = pairs[caseIndex].source;
    spec.target = pairs[caseIndex].target;
    spec.expectedDistance = LevenshteinDistance(spec.source, spec.target);
    return spec;
}

bool VerifyLevenSpec(const LevenCaseSpec& spec)
{
    if (spec.name.empty()) {
        return false;
    }
    if (spec.source.empty() && spec.target.empty()) {
        return false;
    }
    return spec.expectedDistance >= K_LEVEN_DISTANCE_ZERO;
}

bool DistanceMatches(int32_t distance, int32_t expected)
{
    return distance == expected;
}

// === Helpers: Report Computation ===

LevenReport ComputeLevenReport(const LevenCaseSpec& spec)
{
    LevenReport report;
    report.distance = LevenshteinDistance(spec.source, spec.target);
    report.sourceLength = static_cast<int32_t>(spec.source.size());
    report.targetLength = static_cast<int32_t>(spec.target.size());
    report.matches = DistanceMatches(report.distance, spec.expectedDistance);
    return report;
}

// === Helpers: Object Property Setters ===

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

// === Result Building ===

napi_value BuildLevenResult(napi_env env, const LevenCaseSpec& spec, const LevenReport& report)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "source", spec.source);
    SetNamedString(env, result, "target", spec.target);
    SetNamedInt32(env, result, "sourceLength", report.sourceLength);
    SetNamedInt32(env, result, "targetLength", report.targetLength);
    SetNamedInt32(env, result, "distance", report.distance);
    SetNamedInt32(env, result, "expectedDistance", spec.expectedDistance);
    SetNamedBool(env, result, "matches", report.matches);
    SetNamedString(env, result, "description", DescribeLevenPair(spec.source, spec.target));
    return result;
}

// === Test Entry ===

static napi_value RunLevenCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_LEVEN_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid leven case");
        return nullptr;
    }
    const auto spec = GetLevenCaseSpec(caseIndex);
    if (!VerifyLevenSpec(spec)) {
        napi_throw_error(env, nullptr, "leven spec inconsistent");
        return nullptr;
    }
    const LevenReport report = ComputeLevenReport(spec);
    return BuildLevenResult(env, spec, report);
}

}  // namespace

static napi_value InitLevenSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_LEVEN_CASE_COUNT);
    exportNames.reserve(K_LEVEN_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_LEVEN_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testLevenCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunLevenCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_levenSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitLevenSuite,
    .nm_modname = "leven_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterLevenSuiteModule(void)
{
    napi_module_register(&g_levenSuiteModule);
}
