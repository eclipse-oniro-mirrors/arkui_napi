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
#include <utility>
#include <vector>

#include "graph_suite_common.h"
#include "napi/native_node_api.h"

static napi_value CreateInt32MatrixValue(napi_env env, const std::vector<std::vector<int32_t>>& matrix)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, matrix.size(), &result));
    for (size_t i = 0; i < matrix.size(); ++i) {
        napi_value row = CreateInt32ArrayValue(env, matrix[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), row));
    }
    return result;
}

static void BuildMatrixAdjacency(const std::vector<std::vector<int32_t>>& matrix,
                                 bool directed,
                                 std::vector<std::vector<int32_t>>& adjacency)
{
    adjacency.assign(matrix.size(), std::vector<int32_t>());
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix.size(); ++j) {
            bool forward = matrix[i][j] == GRAPH_MATRIX_VALUE_ONE;
            bool backward = matrix[j][i] == GRAPH_MATRIX_VALUE_ONE;
            if (forward || (!directed && backward)) {
                adjacency[i].push_back(static_cast<int32_t>(j));
            }
        }
    }
}

static bool IsValidGridCoordinate(int32_t coordinate)
{
    return coordinate >= GRID_MIN_COORDINATE && coordinate <= GRID_MAX_COORDINATE;
}

static void AppendGridNeighbor(std::vector<int32_t>& neighbors, int32_t row, int32_t col, int32_t rows, int32_t cols)
{
    if (row < GRID_MIN_COORDINATE || row >= rows || col < GRID_MIN_COORDINATE || col >= cols) {
        return;
    }

    neighbors.push_back(row * cols + col);
}

static bool ExtractMatrixAndVertex(napi_env env,
                                   napi_value matrixValue,
                                   napi_value vertexValue,
                                   std::vector<std::vector<int32_t>>& matrix,
                                   int32_t& vertex)
{
    if (!ExtractInt32Matrix(env, matrixValue, matrix) || !ExtractInt32Arg(env, vertexValue, vertex)) {
        return false;
    }

    if (vertex < GRAPH_MIN_VERTEX_LABEL || vertex >= static_cast<int32_t>(matrix.size())) {
        napi_throw_error(env, nullptr, "Vertex out of range");
        return false;
    }

    return true;
}

static bool ExtractGridCoordinates(napi_env env,
                                   napi_value firstRowValue,
                                   napi_value firstColValue,
                                   napi_value secondRowValue,
                                   napi_value secondColValue,
                                   int32_t& firstRow,
                                   int32_t& firstCol,
                                   int32_t& secondRow,
                                   int32_t& secondCol)
{
    if (!ExtractInt32Arg(env, firstRowValue, firstRow) || !ExtractInt32Arg(env, firstColValue, firstCol) ||
        !ExtractInt32Arg(env, secondRowValue, secondRow) || !ExtractInt32Arg(env, secondColValue, secondCol)) {
        return false;
    }

    if (!IsValidGridCoordinate(firstRow) || !IsValidGridCoordinate(firstCol) || !IsValidGridCoordinate(secondRow) ||
        !IsValidGridCoordinate(secondCol)) {
        napi_throw_error(env, nullptr, "Grid coordinates out of range");
        return false;
    }

    return true;
}

napi_value BuildAdjacencyMatrix(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, size");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    int32_t size = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], size)) {
        return nullptr;
    }
    NAPI_ASSERT(env, size >= GRAPH_MIN_MATRIX_SIZE && size <= GRAPH_MAX_MATRIX_SIZE, "Matrix size out of range");

    for (size_t i = 0; i < edges.size(); ++i) {
        NAPI_ASSERT(env, edges[i].first < size && edges[i].second < size, "Vertex label exceeds matrix size");
    }

    std::vector<int32_t> emptyRow(static_cast<size_t>(size), GRAPH_MATRIX_VALUE_ZERO);
    std::vector<std::vector<int32_t>> matrix(static_cast<size_t>(size), emptyRow);
    for (size_t i = 0; i < edges.size(); ++i) {
        matrix[static_cast<size_t>(edges[i].first)][static_cast<size_t>(edges[i].second)] = GRAPH_MATRIX_VALUE_ONE;
        matrix[static_cast<size_t>(edges[i].second)][static_cast<size_t>(edges[i].first)] = GRAPH_MATRIX_VALUE_ONE;
    }
    return CreateInt32MatrixValue(env, matrix);
}

napi_value MatrixDegree(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: matrix, vertex");

    std::vector<std::vector<int32_t>> matrix;
    int32_t vertex = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, vertex)) {
        return nullptr;
    }

    const std::vector<int32_t>& row = matrix[static_cast<size_t>(vertex)];
    int32_t degree = 0;
    for (size_t j = 0; j < row.size(); ++j) {
        degree += row[j];
    }
    return CreateInt32Value(env, degree);
}

napi_value MatrixNeighbors(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: matrix, vertex");

    std::vector<std::vector<int32_t>> matrix;
    int32_t vertex = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, vertex)) {
        return nullptr;
    }

    const std::vector<int32_t>& row = matrix[static_cast<size_t>(vertex)];
    std::vector<int32_t> neighbors;
    for (size_t j = 0; j < row.size(); ++j) {
        if (row[j] == GRAPH_MATRIX_VALUE_ONE) {
            neighbors.push_back(static_cast<int32_t>(j));
        }
    }
    return CreateInt32ArrayValue(env, neighbors);
}

napi_value MatrixEdgeCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: matrix");

    std::vector<std::vector<int32_t>> matrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], matrix)) {
        return nullptr;
    }

    int32_t count = 0;
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            count += matrix[i][j];
        }
    }
    return CreateInt32Value(env, count);
}

napi_value MatrixHasEdge(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: matrix, u, v");

    std::vector<std::vector<int32_t>> matrix;
    int32_t first = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, first)) {
        return nullptr;
    }

    int32_t second = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_TWO], second)) {
        return nullptr;
    }
    NAPI_ASSERT(env, second >= GRAPH_MIN_VERTEX_LABEL && second < static_cast<int32_t>(matrix.size()),
                "Vertex out of range");

    const std::vector<int32_t>& row = matrix[static_cast<size_t>(first)];
    return CreateBoolValue(env, row[static_cast<size_t>(second)] == GRAPH_MATRIX_VALUE_ONE);
}

napi_value MatrixTranspose(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: matrix");

    std::vector<std::vector<int32_t>> matrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], matrix)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> result(matrix.size(),
                                             std::vector<int32_t>(matrix.size(), GRAPH_MATRIX_VALUE_ZERO));
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix[i].size(); ++j) {
            result[j][i] = matrix[i][j];
        }
    }
    return CreateInt32MatrixValue(env, result);
}

napi_value MatrixIsSymmetric(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: matrix");

    std::vector<std::vector<int32_t>> matrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], matrix)) {
        return nullptr;
    }

    bool symmetric = true;
    for (size_t i = 0; symmetric && i < matrix.size(); ++i) {
        for (size_t j = 0; symmetric && j < matrix[i].size(); ++j) {
            symmetric = matrix[i][j] == matrix[j][i];
        }
    }
    return CreateBoolValue(env, symmetric);
}

napi_value MatrixUnion(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: matrix1, matrix2");

    std::vector<std::vector<int32_t>> firstMatrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], firstMatrix)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> secondMatrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ONE], secondMatrix)) {
        return nullptr;
    }
    NAPI_ASSERT(env, firstMatrix.size() == secondMatrix.size(), "Matrices must have the same size");

    std::vector<std::vector<int32_t>> result(firstMatrix.size(),
                                             std::vector<int32_t>(firstMatrix.size(), GRAPH_MATRIX_VALUE_ZERO));
    for (size_t i = 0; i < firstMatrix.size(); ++i) {
        for (size_t j = 0; j < firstMatrix[i].size(); ++j) {
            bool present = firstMatrix[i][j] == GRAPH_MATRIX_VALUE_ONE || secondMatrix[i][j] == GRAPH_MATRIX_VALUE_ONE;
            result[i][j] = present ? GRAPH_MATRIX_VALUE_ONE : GRAPH_MATRIX_VALUE_ZERO;
        }
    }
    return CreateInt32MatrixValue(env, result);
}

napi_value MatrixBfsOrder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: matrix, start vertex");

    std::vector<std::vector<int32_t>> matrix;
    int32_t start = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, start)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, false, adjacency);
    std::vector<int32_t> order;
    BfsOrderFrom(adjacency, start, order);
    return CreateInt32ArrayValue(env, order);
}

napi_value MatrixDfsOrder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: matrix, start vertex");

    std::vector<std::vector<int32_t>> matrix;
    int32_t start = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, start)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, false, adjacency);
    std::vector<int32_t> order;
    DfsOrderFrom(adjacency, start, order);
    return CreateInt32ArrayValue(env, order);
}

napi_value MatrixComponentCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: matrix");

    std::vector<std::vector<int32_t>> matrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], matrix)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, false, adjacency);
    return CreateInt32Value(env, CountComponents(adjacency));
}

napi_value MatrixShortestHops(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: matrix, from, to");

    std::vector<std::vector<int32_t>> matrix;
    int32_t from = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, from)) {
        return nullptr;
    }

    int32_t to = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_TWO], to)) {
        return nullptr;
    }
    NAPI_ASSERT(env, to >= GRAPH_MIN_VERTEX_LABEL && to < static_cast<int32_t>(matrix.size()),
                "To vertex out of range");

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, false, adjacency);
    std::vector<int32_t> distance;
    BfsDistanceFrom(adjacency, from, distance);
    return CreateInt32Value(env, distance[static_cast<size_t>(to)]);
}

napi_value MatrixReachableCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: matrix, start vertex");

    std::vector<std::vector<int32_t>> matrix;
    int32_t start = 0;
    if (!ExtractMatrixAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], matrix, start)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, false, adjacency);
    std::vector<int32_t> order;
    BfsOrderFrom(adjacency, start, order);
    return CreateInt32Value(env, static_cast<int32_t>(order.size()));
}

napi_value MatrixHasCycleDirected(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: matrix");

    std::vector<std::vector<int32_t>> matrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], matrix)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, true, adjacency);
    return CreateBoolValue(env, HasDirectedCycle(adjacency));
}

napi_value MatrixIsBipartite(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: matrix");

    std::vector<std::vector<int32_t>> matrix;
    if (!ExtractInt32Matrix(env, argv[ARG_INDEX_ZERO], matrix)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildMatrixAdjacency(matrix, false, adjacency);
    return CreateBoolValue(env, IsBipartiteAdjacency(adjacency));
}

napi_value GridNeighbors(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FOUR;
    napi_value argv[REQUIRED_ARGS_FOUR] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FOUR, "Requires 4 arguments: row, col, rows, cols");

    int32_t row = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], row)) {
        return nullptr;
    }

    int32_t col = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], col)) {
        return nullptr;
    }

    int32_t rows = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_TWO], rows)) {
        return nullptr;
    }

    int32_t cols = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_THREE], cols)) {
        return nullptr;
    }

    NAPI_ASSERT(env, rows >= GRID_MIN_DIMENSION && rows <= GRID_MAX_DIMENSION, "Grid row count out of range");
    NAPI_ASSERT(env, cols >= GRID_MIN_DIMENSION && cols <= GRID_MAX_DIMENSION, "Grid column count out of range");
    NAPI_ASSERT(env, row >= GRID_MIN_COORDINATE && row < rows, "Cell row out of range");
    NAPI_ASSERT(env, col >= GRID_MIN_COORDINATE && col < cols, "Cell column out of range");

    std::vector<int32_t> neighbors;
    AppendGridNeighbor(neighbors, row - GRID_AXIS_STEP, col, rows, cols);
    AppendGridNeighbor(neighbors, row + GRID_AXIS_STEP, col, rows, cols);
    AppendGridNeighbor(neighbors, row, col - GRID_AXIS_STEP, rows, cols);
    AppendGridNeighbor(neighbors, row, col + GRID_AXIS_STEP, rows, cols);
    std::sort(neighbors.begin(), neighbors.end());
    return CreateInt32ArrayValue(env, neighbors);
}

napi_value GridDistanceManhattan(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FOUR;
    napi_value argv[REQUIRED_ARGS_FOUR] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FOUR, "Requires 4 arguments: row1, col1, row2, col2");

    int32_t firstRow = 0;
    int32_t firstCol = 0;
    int32_t secondRow = 0;
    int32_t secondCol = 0;
    if (!ExtractGridCoordinates(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO],
                                argv[ARG_INDEX_THREE], firstRow, firstCol, secondRow, secondCol)) {
        return nullptr;
    }

    int32_t rowDelta = firstRow > secondRow ? firstRow - secondRow : secondRow - firstRow;
    int32_t colDelta = firstCol > secondCol ? firstCol - secondCol : secondCol - firstCol;
    return CreateInt32Value(env, rowDelta + colDelta);
}

napi_value GridDistanceChebyshev(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FOUR;
    napi_value argv[REQUIRED_ARGS_FOUR] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FOUR, "Requires 4 arguments: row1, col1, row2, col2");

    int32_t firstRow = 0;
    int32_t firstCol = 0;
    int32_t secondRow = 0;
    int32_t secondCol = 0;
    if (!ExtractGridCoordinates(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO],
                                argv[ARG_INDEX_THREE], firstRow, firstCol, secondRow, secondCol)) {
        return nullptr;
    }

    int32_t rowDelta = firstRow > secondRow ? firstRow - secondRow : secondRow - firstRow;
    int32_t colDelta = firstCol > secondCol ? firstCol - secondCol : secondCol - firstCol;
    return CreateInt32Value(env, rowDelta > colDelta ? rowDelta : colDelta);
}

napi_value GridDistanceEuclidean(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FOUR;
    napi_value argv[REQUIRED_ARGS_FOUR] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FOUR, "Requires 4 arguments: row1, col1, row2, col2");

    int32_t firstRow = 0;
    int32_t firstCol = 0;
    int32_t secondRow = 0;
    int32_t secondCol = 0;
    if (!ExtractGridCoordinates(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO],
                                argv[ARG_INDEX_THREE], firstRow, firstCol, secondRow, secondCol)) {
        return nullptr;
    }

    int32_t rowDelta = firstRow > secondRow ? firstRow - secondRow : secondRow - firstRow;
    int32_t colDelta = firstCol > secondCol ? firstCol - secondCol : secondCol - firstCol;
    double squareDistance = static_cast<double>(rowDelta * rowDelta + colDelta * colDelta);
    return CreateDoubleValue(env, std::sqrt(squareDistance));
}

napi_value GridToIndex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: row, col, cols");

    int32_t row = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], row)) {
        return nullptr;
    }

    int32_t col = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], col)) {
        return nullptr;
    }

    int32_t cols = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_TWO], cols)) {
        return nullptr;
    }

    NAPI_ASSERT(env, cols >= GRID_MIN_DIMENSION && cols <= GRID_MAX_DIMENSION, "Grid column count out of range");
    NAPI_ASSERT(env, IsValidGridCoordinate(row), "Grid row out of range");
    NAPI_ASSERT(env, col >= GRID_MIN_COORDINATE && col < cols, "Grid column out of range");
    return CreateInt32Value(env, row * cols + col);
}

napi_value IndexToGrid(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: index, cols");

    int32_t index = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], index)) {
        return nullptr;
    }

    int32_t cols = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ONE], cols)) {
        return nullptr;
    }

    NAPI_ASSERT(env, IsValidGridCoordinate(index), "Grid index out of range");
    NAPI_ASSERT(env, cols >= GRID_MIN_DIMENSION && cols <= GRID_MAX_DIMENSION, "Grid column count out of range");

    int32_t safeCols = cols > 0 ? cols : 1;
    std::vector<int32_t> cell;
    cell.push_back(index / safeCols);
    cell.push_back(index % safeCols);
    return CreateInt32ArrayValue(env, cell);
}

napi_value GridCellsInRadius(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: radius");

    int32_t radius = 0;
    if (!ExtractInt32Arg(env, argv[ARG_INDEX_ZERO], radius)) {
        return nullptr;
    }
    NAPI_ASSERT(env, radius >= GRID_MIN_RADIUS && radius <= GRID_MAX_RADIUS, "Radius out of range");

    int32_t side = GRID_RADIUS_SPAN * radius + GRID_RADIUS_OFFSET;
    return CreateInt32Value(env, side * side);
}

napi_value GridLineCells(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_FOUR;
    napi_value argv[REQUIRED_ARGS_FOUR] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_FOUR, "Requires 4 arguments: row1, col1, row2, col2");

    int32_t firstRow = 0;
    int32_t firstCol = 0;
    int32_t secondRow = 0;
    int32_t secondCol = 0;
    if (!ExtractGridCoordinates(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO],
                                argv[ARG_INDEX_THREE], firstRow, firstCol, secondRow, secondCol)) {
        return nullptr;
    }

    std::vector<int32_t> cells;
    int32_t currentRow = firstRow;
    int32_t currentCol = firstCol;
    cells.push_back(currentRow);
    cells.push_back(currentCol);
    while (currentRow != secondRow) {
        currentRow += currentRow < secondRow ? GRID_AXIS_STEP : -GRID_AXIS_STEP;
        cells.push_back(currentRow);
        cells.push_back(currentCol);
    }
    while (currentCol != secondCol) {
        currentCol += currentCol < secondCol ? GRID_AXIS_STEP : -GRID_AXIS_STEP;
        cells.push_back(currentRow);
        cells.push_back(currentCol);
    }
    return CreateInt32ArrayValue(env, cells);
}

napi_status RegisterGraphOpsFunctions(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("buildAdjacencyMatrix", BuildAdjacencyMatrix),
        DECLARE_NAPI_FUNCTION("matrixDegree", MatrixDegree),
        DECLARE_NAPI_FUNCTION("matrixNeighbors", MatrixNeighbors),
        DECLARE_NAPI_FUNCTION("matrixEdgeCount", MatrixEdgeCount),
        DECLARE_NAPI_FUNCTION("matrixHasEdge", MatrixHasEdge),
        DECLARE_NAPI_FUNCTION("matrixTranspose", MatrixTranspose),
        DECLARE_NAPI_FUNCTION("matrixIsSymmetric", MatrixIsSymmetric),
        DECLARE_NAPI_FUNCTION("matrixUnion", MatrixUnion),
        DECLARE_NAPI_FUNCTION("matrixBfsOrder", MatrixBfsOrder),
        DECLARE_NAPI_FUNCTION("matrixDfsOrder", MatrixDfsOrder),
        DECLARE_NAPI_FUNCTION("matrixComponentCount", MatrixComponentCount),
        DECLARE_NAPI_FUNCTION("matrixShortestHops", MatrixShortestHops),
        DECLARE_NAPI_FUNCTION("matrixReachableCount", MatrixReachableCount),
        DECLARE_NAPI_FUNCTION("matrixHasCycleDirected", MatrixHasCycleDirected),
        DECLARE_NAPI_FUNCTION("matrixIsBipartite", MatrixIsBipartite),
        DECLARE_NAPI_FUNCTION("gridNeighbors", GridNeighbors),
        DECLARE_NAPI_FUNCTION("gridDistanceManhattan", GridDistanceManhattan),
        DECLARE_NAPI_FUNCTION("gridDistanceChebyshev", GridDistanceChebyshev),
        DECLARE_NAPI_FUNCTION("gridDistanceEuclidean", GridDistanceEuclidean),
        DECLARE_NAPI_FUNCTION("gridToIndex", GridToIndex),
        DECLARE_NAPI_FUNCTION("indexToGrid", IndexToGrid),
        DECLARE_NAPI_FUNCTION("gridCellsInRadius", GridCellsInRadius),
        DECLARE_NAPI_FUNCTION("gridLineCells", GridLineCells),
    };
    return napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
