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
constexpr size_t K_TTT_CASE_COUNT = 24;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

// --- Board Configuration ---
constexpr size_t K_TTT_BOARD_SIZE = 9;
constexpr size_t K_TTT_WIN_LINE_COUNT = 8;
constexpr size_t K_TTT_WIN_LINE_LEN = 3;
constexpr size_t K_TTT_OP_CYCLE = 3;
constexpr size_t K_TTT_BOARD_TYPE_COUNT = 6;

// --- Cell Values ---
constexpr int32_t K_TTT_CELL_EMPTY = 0;
constexpr int32_t K_TTT_CELL_X = 1;
constexpr int32_t K_TTT_CELL_O = 2;

// --- Board Positions ---
constexpr size_t K_TTT_POS_TOP_LEFT = 0;
constexpr size_t K_TTT_POS_TOP_MID = 1;
constexpr size_t K_TTT_POS_TOP_RIGHT = 2;
constexpr size_t K_TTT_POS_MID_LEFT = 3;
constexpr size_t K_TTT_POS_CENTER = 4;
constexpr size_t K_TTT_POS_MID_RIGHT = 5;
constexpr size_t K_TTT_POS_BOT_LEFT = 6;
constexpr size_t K_TTT_POS_BOT_MID = 7;
constexpr size_t K_TTT_POS_BOT_RIGHT = 8;

// --- Win Line Indices ---
constexpr size_t K_TTT_LINE_INDEX_FIRST = 0;
constexpr size_t K_TTT_LINE_INDEX_SECOND = 1;
constexpr size_t K_TTT_LINE_INDEX_THIRD = 2;

// --- Op Cycle Indices ---
constexpr size_t K_TTT_OP_INDEX_CHECK_WINNER = 0;
constexpr size_t K_TTT_OP_INDEX_IS_DRAW = 1;
constexpr size_t K_TTT_OP_INDEX_IS_VALID = 2;

// --- Board Type Indices ---
constexpr size_t K_TTT_BOARD_INDEX_EMPTY = 0;
constexpr size_t K_TTT_BOARD_INDEX_X_ROW = 1;
constexpr size_t K_TTT_BOARD_INDEX_O_COL = 2;
constexpr size_t K_TTT_BOARD_INDEX_X_DIAG = 3;
constexpr size_t K_TTT_BOARD_INDEX_DRAW = 4;
constexpr size_t K_TTT_BOARD_INDEX_BOTH_WIN = 5;

// --- Op Labels ---
const char* const K_TTT_OP_LABEL_CHECK_WINNER = "CHECK_WINNER";
const char* const K_TTT_OP_LABEL_IS_DRAW = "IS_DRAW";
const char* const K_TTT_OP_LABEL_IS_VALID = "IS_VALID";

// --- Op Descriptions ---
const char* const K_TTT_OP_DESC_CHECK_WINNER = "detect the winner code on a tic-tac-toe board";
const char* const K_TTT_OP_DESC_IS_DRAW = "check whether the board is a finished draw with no winner";
const char* const K_TTT_OP_DESC_IS_VALID = "check whether the board is a legal tic-tac-toe position";

// --- Winner Names ---
const char* const K_TTT_WINNER_NAME_NONE = "none";
const char* const K_TTT_WINNER_NAME_X = "X";
const char* const K_TTT_WINNER_NAME_O = "O";

// --- Module Configuration ---
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

// === Win Lines ===

const int32_t K_WIN_LINES[K_TTT_WIN_LINE_COUNT][K_TTT_WIN_LINE_LEN] = {
    {K_TTT_POS_TOP_LEFT, K_TTT_POS_TOP_MID, K_TTT_POS_TOP_RIGHT},
    {K_TTT_POS_MID_LEFT, K_TTT_POS_CENTER, K_TTT_POS_MID_RIGHT},
    {K_TTT_POS_BOT_LEFT, K_TTT_POS_BOT_MID, K_TTT_POS_BOT_RIGHT},
    {K_TTT_POS_TOP_LEFT, K_TTT_POS_MID_LEFT, K_TTT_POS_BOT_LEFT},
    {K_TTT_POS_TOP_MID, K_TTT_POS_CENTER, K_TTT_POS_BOT_MID},
    {K_TTT_POS_TOP_RIGHT, K_TTT_POS_MID_RIGHT, K_TTT_POS_BOT_RIGHT},
    {K_TTT_POS_TOP_LEFT, K_TTT_POS_CENTER, K_TTT_POS_BOT_RIGHT},
    {K_TTT_POS_TOP_RIGHT, K_TTT_POS_CENTER, K_TTT_POS_BOT_LEFT},
};

// === Board Fixtures ===

const int32_t K_BOARD_EMPTY[K_TTT_BOARD_SIZE] = {
    K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY,
    K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY,
    K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY,
};

const int32_t K_BOARD_X_WINS_ROW[K_TTT_BOARD_SIZE] = {
    K_TTT_CELL_X, K_TTT_CELL_X, K_TTT_CELL_X,
    K_TTT_CELL_O, K_TTT_CELL_EMPTY, K_TTT_CELL_O,
    K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY,
};

const int32_t K_BOARD_O_WINS_COL[K_TTT_BOARD_SIZE] = {
    K_TTT_CELL_X, K_TTT_CELL_O, K_TTT_CELL_X,
    K_TTT_CELL_EMPTY, K_TTT_CELL_O, K_TTT_CELL_EMPTY,
    K_TTT_CELL_X, K_TTT_CELL_O, K_TTT_CELL_EMPTY,
};

const int32_t K_BOARD_X_WINS_DIAG[K_TTT_BOARD_SIZE] = {
    K_TTT_CELL_X, K_TTT_CELL_O, K_TTT_CELL_O,
    K_TTT_CELL_X, K_TTT_CELL_X, K_TTT_CELL_EMPTY,
    K_TTT_CELL_EMPTY, K_TTT_CELL_O, K_TTT_CELL_X,
};

const int32_t K_BOARD_DRAW[K_TTT_BOARD_SIZE] = {
    K_TTT_CELL_X, K_TTT_CELL_O, K_TTT_CELL_X,
    K_TTT_CELL_X, K_TTT_CELL_O, K_TTT_CELL_O,
    K_TTT_CELL_O, K_TTT_CELL_X, K_TTT_CELL_X,
};

const int32_t K_BOARD_BOTH_WIN[K_TTT_BOARD_SIZE] = {
    K_TTT_CELL_X, K_TTT_CELL_X, K_TTT_CELL_X,
    K_TTT_CELL_O, K_TTT_CELL_O, K_TTT_CELL_O,
    K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY, K_TTT_CELL_EMPTY,
};

// === CaseSpec ===

enum class TttOp {
    CHECK_WINNER,
    IS_DRAW,
    IS_VALID,
};

struct TttCaseSpec {
    std::string name;
    TttOp op;
    int32_t board[K_TTT_BOARD_SIZE];
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

const int32_t* GetBoardForCase(size_t caseNumber)
{
    const size_t boardType = caseNumber % K_TTT_BOARD_TYPE_COUNT;
    if (boardType == K_TTT_BOARD_INDEX_EMPTY) {
        return K_BOARD_EMPTY;
    }
    if (boardType == K_TTT_BOARD_INDEX_X_ROW) {
        return K_BOARD_X_WINS_ROW;
    }
    if (boardType == K_TTT_BOARD_INDEX_O_COL) {
        return K_BOARD_O_WINS_COL;
    }
    if (boardType == K_TTT_BOARD_INDEX_X_DIAG) {
        return K_BOARD_X_WINS_DIAG;
    }
    if (boardType == K_TTT_BOARD_INDEX_DRAW) {
        return K_BOARD_DRAW;
    }
    return K_BOARD_BOTH_WIN;
}

TttCaseSpec GetTttCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + K_FIRST_CASE_NUMBER;
    TttCaseSpec spec;
    spec.name = BuildIndexedName("ticTacToeCase", caseNumber);
    spec.caseNumber = caseNumber;
    const size_t cycle = caseNumber % K_TTT_OP_CYCLE;
    if (cycle == K_TTT_OP_INDEX_CHECK_WINNER) {
        spec.op = TttOp::CHECK_WINNER;
    } else if (cycle == K_TTT_OP_INDEX_IS_DRAW) {
        spec.op = TttOp::IS_DRAW;
    } else {
        spec.op = TttOp::IS_VALID;
    }
    const int32_t* board = GetBoardForCase(caseNumber);
    for (size_t i = 0; i < K_TTT_BOARD_SIZE; i++) {
        spec.board[i] = board[i];
    }
    return spec;
}

const char* TttOpLabel(TttOp op)
{
    if (op == TttOp::CHECK_WINNER) {
        return K_TTT_OP_LABEL_CHECK_WINNER;
    }
    if (op == TttOp::IS_DRAW) {
        return K_TTT_OP_LABEL_IS_DRAW;
    }
    return K_TTT_OP_LABEL_IS_VALID;
}

const char* TttOpDescription(TttOp op)
{
    if (op == TttOp::CHECK_WINNER) {
        return K_TTT_OP_DESC_CHECK_WINNER;
    }
    if (op == TttOp::IS_DRAW) {
        return K_TTT_OP_DESC_IS_DRAW;
    }
    return K_TTT_OP_DESC_IS_VALID;
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

// === Helpers: Board Logic ===

int32_t CountCell(const int32_t board[K_TTT_BOARD_SIZE], int32_t cell)
{
    int32_t count = 0;
    for (size_t i = 0; i < K_TTT_BOARD_SIZE; i++) {
        if (board[i] == cell) {
            count++;
        }
    }
    return count;
}

int32_t CheckWinner(const int32_t board[K_TTT_BOARD_SIZE])
{
    for (size_t i = 0; i < K_TTT_WIN_LINE_COUNT; i++) {
        const int32_t a = board[K_WIN_LINES[i][K_TTT_LINE_INDEX_FIRST]];
        const int32_t b = board[K_WIN_LINES[i][K_TTT_LINE_INDEX_SECOND]];
        const int32_t c = board[K_WIN_LINES[i][K_TTT_LINE_INDEX_THIRD]];
        if (a == K_TTT_CELL_EMPTY) {
            continue;
        }
        if (a == b && b == c) {
            return a;
        }
    }
    return K_TTT_CELL_EMPTY;
}

bool BothPlayersWin(const int32_t board[K_TTT_BOARD_SIZE])
{
    bool xWins = false;
    bool oWins = false;
    for (size_t i = 0; i < K_TTT_WIN_LINE_COUNT; i++) {
        const int32_t a = board[K_WIN_LINES[i][K_TTT_LINE_INDEX_FIRST]];
        const int32_t b = board[K_WIN_LINES[i][K_TTT_LINE_INDEX_SECOND]];
        const int32_t c = board[K_WIN_LINES[i][K_TTT_LINE_INDEX_THIRD]];
        if (a == K_TTT_CELL_EMPTY) {
            continue;
        }
        if (a == b && b == c) {
            if (a == K_TTT_CELL_X) {
                xWins = true;
            } else {
                oWins = true;
            }
        }
    }
    return xWins && oWins;
}

bool IsDraw(const int32_t board[K_TTT_BOARD_SIZE])
{
    if (CheckWinner(board) != K_TTT_CELL_EMPTY) {
        return false;
    }
    if (CountCell(board, K_TTT_CELL_EMPTY) != 0) {
        return false;
    }
    return true;
}

bool IsValidBoard(const int32_t board[K_TTT_BOARD_SIZE])
{
    for (size_t i = 0; i < K_TTT_BOARD_SIZE; i++) {
        if (board[i] != K_TTT_CELL_EMPTY && board[i] != K_TTT_CELL_X && board[i] != K_TTT_CELL_O) {
            return false;
        }
    }
    if (BothPlayersWin(board)) {
        return false;
    }
    const int32_t xCount = CountCell(board, K_TTT_CELL_X);
    const int32_t oCount = CountCell(board, K_TTT_CELL_O);
    if (xCount < oCount || xCount > oCount + 1) {
        return false;
    }
    const int32_t winner = CheckWinner(board);
    if (winner == K_TTT_CELL_X && xCount != oCount + 1) {
        return false;
    }
    if (winner == K_TTT_CELL_O && xCount != oCount) {
        return false;
    }
    return true;
}

const char* WinnerName(int32_t winnerCode)
{
    if (winnerCode == K_TTT_CELL_X) {
        return K_TTT_WINNER_NAME_X;
    }
    if (winnerCode == K_TTT_CELL_O) {
        return K_TTT_WINNER_NAME_O;
    }
    return K_TTT_WINNER_NAME_NONE;
}

// === Helpers: Result Building ===

napi_value BuildTttResult(napi_env env, const TttCaseSpec& spec)
{
    const int32_t winnerCode = CheckWinner(spec.board);
    const bool isDraw = IsDraw(spec.board);
    const bool isValid = IsValidBoard(spec.board);
    const int32_t xCount = CountCell(spec.board, K_TTT_CELL_X);
    const int32_t oCount = CountCell(spec.board, K_TTT_CELL_O);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "op", TttOpLabel(spec.op));
    SetNamedString(env, result, "description", TttOpDescription(spec.op));
    SetNamedString(env, result, "winner", WinnerName(winnerCode));
    SetNamedInt32(env, result, "winnerCode", winnerCode);
    SetNamedBool(env, result, "isDraw", isDraw);
    SetNamedBool(env, result, "isValid", isValid);
    SetNamedInt32(env, result, "xCount", xCount);
    SetNamedInt32(env, result, "oCount", oCount);
    return result;
}

// === Test Entry ===

static napi_value RunTttCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_ARGS_ZERO;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, &data));
    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_TTT_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid tic-tac-toe case");
        return nullptr;
    }
    const auto spec = GetTttCaseSpec(caseIndex);
    return BuildTttResult(env, spec);
}

}  // namespace

static napi_value InitTicTacToeSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_TTT_CASE_COUNT);
    exportNames.reserve(K_TTT_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_TTT_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testTicTacToeCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunTttCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_ticTacToeSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitTicTacToeSuite,
    .nm_modname = "tic_tac_toe_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterTicTacToeSuiteModule(void)
{
    napi_module_register(&g_ticTacToeSuiteModule);
}
