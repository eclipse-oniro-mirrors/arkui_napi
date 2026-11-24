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
constexpr size_t K_NQ_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- N-Queens Configuration ---
constexpr int32_t K_NQ_MAX_N = 8;
constexpr size_t K_NQ_OP_CYCLE = 3;
constexpr int32_t K_NQ_INVALID_ROW = -1;

// --- Op Cycle Indices ---
constexpr size_t K_NQ_OP_INDEX_COUNT = 0;
constexpr size_t K_NQ_OP_INDEX_HAS = 1;
constexpr size_t K_NQ_OP_INDEX_FIRST = 2;

// --- Op Labels ---
const char* const K_NQ_OP_LABEL_COUNT = "COUNT_SOLUTIONS";
const char* const K_NQ_OP_LABEL_HAS = "HAS_SOLUTION";
const char* const K_NQ_OP_LABEL_FIRST = "FIRST_SOLUTION_ROW";

// --- Op Descriptions ---
const char* const K_NQ_OP_DESC_COUNT = "count total solutions for n-queens of size n";
const char* const K_NQ_OP_DESC_HAS = "check whether at least one solution exists for size n";
const char* const K_NQ_OP_DESC_FIRST = "row of queen in a query column for the first solution of size n";

// --- Known Solution Counts (n=1..8) ---
constexpr int32_t K_NQ_SOLUTIONS_N1 = 1;
constexpr int32_t K_NQ_SOLUTIONS_N2 = 0;
constexpr int32_t K_NQ_SOLUTIONS_N3 = 0;
constexpr int32_t K_NQ_SOLUTIONS_N4 = 2;
constexpr int32_t K_NQ_SOLUTIONS_N5 = 10;
constexpr int32_t K_NQ_SOLUTIONS_N6 = 4;
constexpr int32_t K_NQ_SOLUTIONS_N7 = 40;
constexpr int32_t K_NQ_SOLUTIONS_N8 = 92;

// --- Board Size Category & Difficulty ---
constexpr int32_t K_NQ_CATEGORY_SMALL_MAX = 3;
const char* const K_NQ_CATEGORY_LABEL_SMALL = "small";
const char* const K_NQ_CATEGORY_LABEL_LARGE = "large";
const char* const K_NQ_DIFFICULTY_LABEL = "exponential";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === CaseSpec ===

enum class NqOp {
    COUNT_SOLUTIONS,
    HAS_SOLUTION,
    FIRST_SOLUTION_ROW,
};

struct NqCaseSpec {
    std::string name;
    NqOp op;
    int32_t n;
    size_t caseNumber;
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

NqCaseSpec GetNqCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    NqCaseSpec spec;
    spec.name = BuildIndexedName("nqueensCase", caseNumber);
    spec.caseNumber = caseNumber;
    spec.n = static_cast<int32_t>(caseNumber % static_cast<size_t>(K_NQ_MAX_N)) + 1;
    const size_t cycle = caseNumber % K_NQ_OP_CYCLE;
    if (cycle == K_NQ_OP_INDEX_COUNT) {
        spec.op = NqOp::COUNT_SOLUTIONS;
    } else if (cycle == K_NQ_OP_INDEX_HAS) {
        spec.op = NqOp::HAS_SOLUTION;
    } else {
        spec.op = NqOp::FIRST_SOLUTION_ROW;
    }
    return spec;
}

const char* NqOpLabel(NqOp op)
{
    if (op == NqOp::COUNT_SOLUTIONS) {
        return K_NQ_OP_LABEL_COUNT;
    }
    if (op == NqOp::HAS_SOLUTION) {
        return K_NQ_OP_LABEL_HAS;
    }
    return K_NQ_OP_LABEL_FIRST;
}

const char* NqOpDescription(NqOp op)
{
    if (op == NqOp::COUNT_SOLUTIONS) {
        return K_NQ_OP_DESC_COUNT;
    }
    if (op == NqOp::HAS_SOLUTION) {
        return K_NQ_OP_DESC_HAS;
    }
    return K_NQ_OP_DESC_FIRST;
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

// === Helpers: N-Queens Backtracking ===

bool NqIsSafe(const int32_t cols[], int32_t col, int32_t row)
{
    for (int32_t c = 0; c < col; c++) {
        if (cols[c] == row) {
            return false;
        }
        const int32_t diff = col - c;
        const int32_t rowDiff = cols[c] - row;
        if (rowDiff == diff || rowDiff == -diff) {
            return false;
        }
    }
    return true;
}

void NqCountHelper(int32_t cols[], int32_t col, int32_t n, int32_t* count)
{
    if (col == n) {
        (*count)++;
        return;
    }
    for (int32_t row = 0; row < n; row++) {
        if (NqIsSafe(cols, col, row)) {
            cols[col] = row;
            NqCountHelper(cols, col + 1, n, count);
        }
    }
}

int32_t CountQueens(int32_t n)
{
    if (n <= 0 || n > K_NQ_MAX_N) {
        return 0;
    }
    int32_t cols[K_NQ_MAX_N] = {0};
    int32_t count = 0;
    NqCountHelper(cols, 0, n, &count);
    return count;
}

bool NqFindFirstSolution(int32_t cols[], int32_t col, int32_t n)
{
    if (col == n) {
        return true;
    }
    for (int32_t row = 0; row < n; row++) {
        if (NqIsSafe(cols, col, row)) {
            cols[col] = row;
            if (NqFindFirstSolution(cols, col + 1, n)) {
                return true;
            }
        }
    }
    return false;
}

struct NqSolution {
    int32_t board[K_NQ_MAX_N];
    bool found;
};

NqSolution FindFirstSolutionBoard(int32_t n)
{
    NqSolution sol;
    for (int32_t i = 0; i < K_NQ_MAX_N; i++) {
        sol.board[i] = K_NQ_INVALID_ROW;
    }
    sol.found = false;
    if (n <= 0 || n > K_NQ_MAX_N) {
        return sol;
    }
    int32_t cols[K_NQ_MAX_N] = {0};
    sol.found = NqFindFirstSolution(cols, 0, n);
    if (sol.found) {
        for (int32_t i = 0; i < n; i++) {
            sol.board[i] = cols[i];
        }
    }
    return sol;
}

bool HasSolution(int32_t n)
{
    return FindFirstSolutionBoard(n).found;
}

// === Helpers: Aggregation, Validation & Symmetry ===

int32_t CountQueensUpTo(int32_t n)
{
    int32_t total = 0;
    for (int32_t k = 1; k <= n; k++) {
        total += CountQueens(k);
    }
    return total;
}

bool IsSolutionBoard(const int32_t cols[], int32_t n)
{
    for (int32_t c = 0; c < n; c++) {
        const int32_t row = cols[c];
        if (row < 0 || row >= n) {
            return false;
        }
        if (!NqIsSafe(cols, c, row)) {
            return false;
        }
    }
    return true;
}

bool IsSymmetricBoard(const int32_t cols[], int32_t n)
{
    for (int32_t c = 0; c < n - 1 - c; c++) {
        if (cols[c] != cols[n - 1 - c]) {
            return false;
        }
    }
    return true;
}

int32_t QueryColumnForCase(int32_t n, size_t caseNumber)
{
    if (n <= 0) {
        return 0;
    }
    return static_cast<int32_t>((caseNumber - K_FIRST_CASE_NUMBER) % static_cast<size_t>(n));
}

int32_t FirstSolutionRow(int32_t n, int32_t queryCol)
{
    if (n <= 0 || n > K_NQ_MAX_N || queryCol < 0 || queryCol >= n) {
        return K_NQ_INVALID_ROW;
    }
    const NqSolution sol = FindFirstSolutionBoard(n);
    if (!sol.found) {
        return K_NQ_INVALID_ROW;
    }
    return sol.board[queryCol];
}

// === Helpers: Outcome & Result Building ===

struct NqOutcome {
    int32_t result;
    int32_t solutionsFound;
    int32_t queryCol;
    int32_t totalUpTo;
    bool isValidFirstSolution;
    bool isSymmetricFirst;
    NqSolution firstSolution;
};

NqOutcome SolveNq(const NqCaseSpec& spec)
{
    NqOutcome outcome;
    outcome.queryCol = K_NQ_INVALID_ROW;
    outcome.solutionsFound = CountQueens(spec.n);
    outcome.firstSolution = FindFirstSolutionBoard(spec.n);
    outcome.totalUpTo = CountQueensUpTo(spec.n);
    outcome.isValidFirstSolution = false;
    outcome.isSymmetricFirst = false;
    if (outcome.firstSolution.found) {
        outcome.isValidFirstSolution = IsSolutionBoard(outcome.firstSolution.board, spec.n);
        outcome.isSymmetricFirst = IsSymmetricBoard(outcome.firstSolution.board, spec.n);
    }
    if (spec.op == NqOp::COUNT_SOLUTIONS) {
        outcome.result = outcome.solutionsFound;
    } else if (spec.op == NqOp::HAS_SOLUTION) {
        outcome.result = outcome.firstSolution.found ? 1 : 0;
    } else {
        outcome.queryCol = QueryColumnForCase(spec.n, spec.caseNumber);
        outcome.result = FirstSolutionRow(spec.n, outcome.queryCol);
    }
    return outcome;
}

napi_value BuildBoardArray(napi_env env, const NqSolution& sol)
{
    napi_value array = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, static_cast<size_t>(K_NQ_MAX_N), &array));
    for (int32_t i = 0; i < K_NQ_MAX_N; i++) {
        napi_value cell = nullptr;
        NAPI_CALL(env, napi_create_int32(env, sol.board[i], &cell));
        NAPI_CALL(env, napi_set_element(env, array, static_cast<uint32_t>(i), cell));
    }
    return array;
}

napi_value BuildNqResult(napi_env env, const NqCaseSpec& spec, const NqOutcome& outcome)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", NqOpLabel(spec.op));
    SetNamedString(env, result, "description", NqOpDescription(spec.op));
    SetNamedInt32(env, result, "n", spec.n);
    SetNamedInt32(env, result, "result", outcome.result);
    SetNamedInt32(env, result, "solutionsFound", outcome.solutionsFound);
    SetNamedInt32(env, result, "queryCol", outcome.queryCol);
    SetNamedBool(env, result, "hasFirstSolution", outcome.firstSolution.found);
    SetNamedInt32(env, result, "totalUpTo", outcome.totalUpTo);
    SetNamedBool(env, result, "isValidFirstSolution", outcome.isValidFirstSolution);
    SetNamedString(env, result, "symmetryClass", outcome.isSymmetricFirst ? "symmetric" : "asymmetric");
    const char* categoryLabel = (spec.n <= K_NQ_CATEGORY_SMALL_MAX)
        ? K_NQ_CATEGORY_LABEL_SMALL : K_NQ_CATEGORY_LABEL_LARGE;
    SetNamedString(env, result, "boardSizeCategory", categoryLabel);
    SetNamedString(env, result, "difficultyLabel", K_NQ_DIFFICULTY_LABEL);
    const int32_t knownCounts[K_NQ_MAX_N + 1] = {0, K_NQ_SOLUTIONS_N1, K_NQ_SOLUTIONS_N2, K_NQ_SOLUTIONS_N3,
        K_NQ_SOLUTIONS_N4, K_NQ_SOLUTIONS_N5, K_NQ_SOLUTIONS_N6, K_NQ_SOLUTIONS_N7, K_NQ_SOLUTIONS_N8};
    const int32_t knownForN = (spec.n >= 0 && spec.n <= K_NQ_MAX_N) ? knownCounts[spec.n] : 0;
    SetNamedInt32(env, result, "knownSolutionCount", knownForN);
    napi_value boardArray = BuildBoardArray(env, outcome.firstSolution);
    NAPI_CALL(env, napi_set_named_property(env, result, "firstSolutionBoard", boardArray));
    return result;
}

// === Test Entry ===

static napi_value RunNqCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_NQ_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid nqueens case");
        return nullptr;
    }
    const auto spec = GetNqCaseSpec(caseIndex);
    const NqOutcome outcome = SolveNq(spec);
    return BuildNqResult(env, spec, outcome);
}

}  // namespace

static napi_value InitNqueensSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_NQ_CASE_COUNT);
    exportNames.reserve(K_NQ_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_NQ_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testNqueensCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunNqCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_nqueensSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitNqueensSuite,
    .nm_modname = "nqueens_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterNqueensSuiteModule(void)
{
    napi_module_register(&g_nqueensSuiteModule);
}
