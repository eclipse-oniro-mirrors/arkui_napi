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

#ifndef SAMPLE_NATIVE_MODULE_GRAPH_SUITE_COMMON_H
#define SAMPLE_NATIVE_MODULE_GRAPH_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "napi/native_api.h"

constexpr int REQUIRED_ARGS_ONE = 1;
constexpr int REQUIRED_ARGS_TWO = 2;
constexpr int REQUIRED_ARGS_THREE = 3;
constexpr int REQUIRED_ARGS_FOUR = 4;
constexpr int ARG_INDEX_ZERO = 0;
constexpr int ARG_INDEX_ONE = 1;
constexpr int ARG_INDEX_TWO = 2;
constexpr int ARG_INDEX_THREE = 3;

constexpr int GRAPH_MAX_VERTICES = 256;
constexpr int GRAPH_VERTEX_COUNT_OFFSET = 1;
constexpr int GRAPH_MAX_VERTEX_LABEL = GRAPH_MAX_VERTICES - GRAPH_VERTEX_COUNT_OFFSET;
constexpr int GRAPH_MIN_VERTEX_LABEL = 0;
constexpr int GRAPH_MAX_EDGES = 1024;
constexpr int GRAPH_EDGE_PAIR_SIZE = 2;
constexpr int GRAPH_MIN_INT_ARG = 0;
constexpr int GRAPH_MAX_INT_ARG = 1024;
constexpr int GRAPH_MAX_ARRAY_LENGTH = 1024;
constexpr int GRAPH_EMPTY_VERTEX_COUNT = 0;
constexpr int GRAPH_NO_PATH = -1;
constexpr int GRAPH_NO_VERTEX = -1;
constexpr int GRAPH_MAX_CONNECTED_COMPONENTS = 1;
constexpr int GRAPH_ORIGIN_DISTANCE = 0;
constexpr int GRAPH_HOP_STEP = 1;
constexpr int GRAPH_INDEGREE_ZERO = 0;
constexpr uint32_t GRAPH_UINT32_INDEX_ZERO = 0;
constexpr uint32_t GRAPH_UINT32_INDEX_ONE = 1;

constexpr int GRAPH_MIN_MATRIX_SIZE = 1;
constexpr int GRAPH_MAX_MATRIX_SIZE = 64;
constexpr int GRAPH_MATRIX_VALUE_ZERO = 0;
constexpr int GRAPH_MATRIX_VALUE_ONE = 1;

constexpr int GRAPH_COLOR_UNCOLORED = -1;
constexpr int GRAPH_COLOR_FIRST = 0;
constexpr int GRAPH_COLOR_SECOND = 1;

constexpr int GRAPH_DFS_WHITE = 0;
constexpr int GRAPH_DFS_GRAY = 1;
constexpr int GRAPH_DFS_BLACK = 2;

constexpr int GRID_MIN_DIMENSION = 1;
constexpr int GRID_MAX_DIMENSION = 1024;
constexpr int GRID_MIN_COORDINATE = 0;
constexpr int GRID_MAX_COORDINATE = 1023;
constexpr int GRID_MIN_RADIUS = 0;
constexpr int GRID_MAX_RADIUS = 32;
constexpr int GRID_RADIUS_SPAN = 2;
constexpr int GRID_RADIUS_OFFSET = 1;
constexpr int GRID_AXIS_STEP = 1;

bool ExtractInt32Arg(napi_env env, napi_value value, int32_t& result);
bool ExtractInt32Array(napi_env env, napi_value value, std::vector<int32_t>& result);
bool ExtractEdgeList(napi_env env, napi_value value, std::vector<std::pair<int32_t, int32_t>>& result);
bool ExtractInt32Matrix(napi_env env, napi_value value, std::vector<std::vector<int32_t>>& result);
napi_value CreateDoubleValue(napi_env env, double value);
napi_value CreateInt32Value(napi_env env, int32_t value);
napi_value CreateBoolValue(napi_env env, bool value);
napi_value CreateInt32ArrayValue(napi_env env, const std::vector<int32_t>& values);
napi_value CreateStringArrayValue(napi_env env, const std::vector<std::string>& values);
void BuildAdjacency(const std::vector<std::pair<int32_t, int32_t>>& edges,
                    bool directed,
                    std::vector<std::vector<int32_t>>& adjacency,
                    int32_t maxVertex);
void SortNeighbors(std::vector<int32_t>& neighbors);
void BfsOrderFrom(const std::vector<std::vector<int32_t>>& adjacency, int32_t start, std::vector<int32_t>& order);
void DfsOrderFrom(const std::vector<std::vector<int32_t>>& adjacency, int32_t start, std::vector<int32_t>& order);
void BfsDistanceFrom(const std::vector<std::vector<int32_t>>& adjacency, int32_t start, std::vector<int32_t>& distance);
int32_t CountComponents(const std::vector<std::vector<int32_t>>& adjacency);
void CollectComponentSizes(const std::vector<std::vector<int32_t>>& adjacency, std::vector<int32_t>& sizes);
bool IsBipartiteAdjacency(const std::vector<std::vector<int32_t>>& adjacency);
bool HasDirectedCycle(const std::vector<std::vector<int32_t>>& adjacency);

napi_value VertexCount(napi_env env, napi_callback_info info);
napi_value EdgeCount(napi_env env, napi_callback_info info);
napi_value DegreeOf(napi_env env, napi_callback_info info);
napi_value NeighborsOf(napi_env env, napi_callback_info info);
napi_value HasEdge(napi_env env, napi_callback_info info);
napi_value BfsOrder(napi_env env, napi_callback_info info);
napi_value DfsOrder(napi_env env, napi_callback_info info);
napi_value ReachableCount(napi_env env, napi_callback_info info);
napi_value ComponentCount(napi_env env, napi_callback_info info);
napi_value IsConnected(napi_env env, napi_callback_info info);
napi_value ShortestPathHops(napi_env env, napi_callback_info info);
napi_value PathSequence(napi_env env, napi_callback_info info);
napi_value HasCycleUndirected(napi_env env, napi_callback_info info);
napi_value HasCycleDirected(napi_env env, napi_callback_info info);
napi_value IsBipartite(napi_env env, napi_callback_info info);
napi_value TopologicalOrder(napi_env env, napi_callback_info info);
napi_value MaxDegree(napi_env env, napi_callback_info info);
napi_value MaxDegreeVertex(napi_env env, napi_callback_info info);
napi_value IsIsolatedVertex(napi_env env, napi_callback_info info);
napi_value CommonNeighbors(napi_env env, napi_callback_info info);
napi_value ComponentSizes(napi_env env, napi_callback_info info);
napi_value HasParallelEdges(napi_env env, napi_callback_info info);

napi_value BuildAdjacencyMatrix(napi_env env, napi_callback_info info);
napi_value MatrixDegree(napi_env env, napi_callback_info info);
napi_value MatrixNeighbors(napi_env env, napi_callback_info info);
napi_value MatrixEdgeCount(napi_env env, napi_callback_info info);
napi_value MatrixHasEdge(napi_env env, napi_callback_info info);
napi_value MatrixTranspose(napi_env env, napi_callback_info info);
napi_value MatrixIsSymmetric(napi_env env, napi_callback_info info);
napi_value MatrixUnion(napi_env env, napi_callback_info info);
napi_value MatrixBfsOrder(napi_env env, napi_callback_info info);
napi_value MatrixDfsOrder(napi_env env, napi_callback_info info);
napi_value MatrixComponentCount(napi_env env, napi_callback_info info);
napi_value MatrixShortestHops(napi_env env, napi_callback_info info);
napi_value MatrixReachableCount(napi_env env, napi_callback_info info);
napi_value MatrixHasCycleDirected(napi_env env, napi_callback_info info);
napi_value MatrixIsBipartite(napi_env env, napi_callback_info info);
napi_value GridNeighbors(napi_env env, napi_callback_info info);
napi_value GridDistanceManhattan(napi_env env, napi_callback_info info);
napi_value GridDistanceChebyshev(napi_env env, napi_callback_info info);
napi_value GridDistanceEuclidean(napi_env env, napi_callback_info info);
napi_value GridToIndex(napi_env env, napi_callback_info info);
napi_value IndexToGrid(napi_env env, napi_callback_info info);
napi_value GridCellsInRadius(napi_env env, napi_callback_info info);
napi_value GridLineCells(napi_env env, napi_callback_info info);

napi_status RegisterGraphOpsFunctions(napi_env env, napi_value exports);

#endif // SAMPLE_NATIVE_MODULE_GRAPH_SUITE_COMMON_H
