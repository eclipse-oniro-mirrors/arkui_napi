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
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "graph_suite_common.h"
#include "napi/native_node_api.h"

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
    if (number < static_cast<double>(GRAPH_MIN_INT_ARG) || number > static_cast<double>(GRAPH_MAX_INT_ARG)) {
        napi_throw_error(env, nullptr, "Argument is out of the supported integer range");
        return false;
    }

    result = static_cast<int32_t>(number);
    return true;
}

bool ExtractInt32Array(napi_env env, napi_value value, std::vector<int32_t>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > static_cast<uint32_t>(GRAPH_MAX_ARRAY_LENGTH)) {
        napi_throw_error(env, nullptr, "Array exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = GRAPH_UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        int32_t number = 0;
        if (!ExtractInt32Arg(env, element, number)) {
            return false;
        }
        result.push_back(number);
    }
    return true;
}

// Parses one edge: a 2-element array whose entries are in-range vertex labels.
static bool ExtractSingleEdge(napi_env env, napi_value element, std::pair<int32_t, int32_t>& edge)
{
    bool pairIsArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, element, &pairIsArray), false);
    if (!pairIsArray) {
        napi_throw_error(env, nullptr, "Each edge must be a 2-element array");
        return false;
    }

    uint32_t pairLength = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, element, &pairLength), false);
    if (pairLength != static_cast<uint32_t>(GRAPH_EDGE_PAIR_SIZE)) {
        napi_throw_error(env, nullptr, "Each edge must be a 2-element array");
        return false;
    }

    napi_value firstValue = nullptr;
    NAPI_CALL_BASE(env, napi_get_element(env, element, GRAPH_UINT32_INDEX_ZERO, &firstValue), false);
    int32_t first = 0;
    if (!ExtractInt32Arg(env, firstValue, first)) {
        return false;
    }

    napi_value secondValue = nullptr;
    NAPI_CALL_BASE(env, napi_get_element(env, element, GRAPH_UINT32_INDEX_ONE, &secondValue), false);
    int32_t second = 0;
    if (!ExtractInt32Arg(env, secondValue, second)) {
        return false;
    }

    if (first < GRAPH_MIN_VERTEX_LABEL || first > GRAPH_MAX_VERTEX_LABEL || second < GRAPH_MIN_VERTEX_LABEL ||
        second > GRAPH_MAX_VERTEX_LABEL) {
        napi_throw_error(env, nullptr, "Vertex label out of range");
        return false;
    }

    edge = std::make_pair(first, second);
    return true;
}

bool ExtractEdgeList(napi_env env, napi_value value, std::vector<std::pair<int32_t, int32_t>>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be an edge list array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length > static_cast<uint32_t>(GRAPH_MAX_EDGES)) {
        napi_throw_error(env, nullptr, "Edge list exceeds maximum length");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = GRAPH_UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        std::pair<int32_t, int32_t> edge;
        if (!ExtractSingleEdge(env, element, edge)) {
            return false;
        }
        result.push_back(edge);
    }
    return true;
}

bool ExtractInt32Matrix(napi_env env, napi_value value, std::vector<std::vector<int32_t>>& result)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, value, &isArray), false);
    if (!isArray) {
        napi_throw_error(env, nullptr, "Argument must be a matrix array");
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, value, &length), false);
    if (length < static_cast<uint32_t>(GRAPH_MIN_MATRIX_SIZE) ||
        length > static_cast<uint32_t>(GRAPH_MAX_MATRIX_SIZE)) {
        napi_throw_error(env, nullptr, "Matrix size out of range");
        return false;
    }

    result.clear();
    result.reserve(length);
    for (uint32_t i = GRAPH_UINT32_INDEX_ZERO; i < length; ++i) {
        napi_value element = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, value, i, &element), false);
        std::vector<int32_t> row;
        if (!ExtractInt32Array(env, element, row)) {
            return false;
        }
        if (row.size() != static_cast<size_t>(length)) {
            napi_throw_error(env, nullptr, "Matrix must be square");
            return false;
        }
        for (size_t j = 0; j < row.size(); ++j) {
            if (row[j] != GRAPH_MATRIX_VALUE_ZERO && row[j] != GRAPH_MATRIX_VALUE_ONE) {
                napi_throw_error(env, nullptr, "Matrix values must be 0 or 1");
                return false;
            }
        }
        result.push_back(row);
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

napi_value CreateInt32ArrayValue(napi_env env, const std::vector<int32_t>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = CreateInt32Value(env, values[i]);
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

napi_value CreateStringArrayValue(napi_env env, const std::vector<std::string>& values)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, values.size(), &result));
    for (size_t i = 0; i < values.size(); ++i) {
        napi_value element = nullptr;
        NAPI_CALL(env, napi_create_string_utf8(env, values[i].c_str(), values[i].size(), &element));
        NAPI_CALL(env, napi_set_element(env, result, static_cast<uint32_t>(i), element));
    }
    return result;
}

void BuildAdjacency(const std::vector<std::pair<int32_t, int32_t>>& edges,
                    bool directed,
                    std::vector<std::vector<int32_t>>& adjacency,
                    int32_t maxVertex)
{
    adjacency.assign(static_cast<size_t>(maxVertex + GRAPH_VERTEX_COUNT_OFFSET), std::vector<int32_t>());
    for (size_t i = 0; i < edges.size(); ++i) {
        adjacency[static_cast<size_t>(edges[i].first)].push_back(edges[i].second);
        if (!directed) {
            adjacency[static_cast<size_t>(edges[i].second)].push_back(edges[i].first);
        }
    }
}

void SortNeighbors(std::vector<int32_t>& neighbors)
{
    std::sort(neighbors.begin(), neighbors.end());
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
}

static int32_t MaxVertexLabel(const std::vector<std::pair<int32_t, int32_t>>& edges)
{
    int32_t maxLabel = GRAPH_NO_VERTEX;
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].first > maxLabel) {
            maxLabel = edges[i].first;
        }
        if (edges[i].second > maxLabel) {
            maxLabel = edges[i].second;
        }
    }
    return maxLabel;
}

static void BuildSortedAdjacency(const std::vector<std::pair<int32_t, int32_t>>& edges,
                                 bool directed,
                                 std::vector<std::vector<int32_t>>& adjacency)
{
    int32_t maxLabel = MaxVertexLabel(edges);
    if (maxLabel == GRAPH_NO_VERTEX) {
        adjacency.clear();
        return;
    }

    BuildAdjacency(edges, directed, adjacency, maxLabel);
    for (size_t i = 0; i < adjacency.size(); ++i) {
        SortNeighbors(adjacency[i]);
    }
}

static void DfsPreorder(const std::vector<std::vector<int32_t>>& adjacency,
                        int32_t vertex,
                        std::vector<bool>& visited,
                        std::vector<int32_t>& order)
{
    visited[static_cast<size_t>(vertex)] = true;
    order.push_back(vertex);
    const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(vertex)];
    for (size_t i = 0; i < neighbors.size(); ++i) {
        int32_t next = neighbors[i];
        if (!visited[static_cast<size_t>(next)]) {
            DfsPreorder(adjacency, next, visited, order);
        }
    }
}

static bool DfsCycleVisit(const std::vector<std::vector<int32_t>>& adjacency,
                          int32_t vertex,
                          std::vector<int32_t>& color)
{
    color[static_cast<size_t>(vertex)] = GRAPH_DFS_GRAY;
    const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(vertex)];
    for (size_t i = 0; i < neighbors.size(); ++i) {
        int32_t next = neighbors[i];
        if (color[static_cast<size_t>(next)] == GRAPH_DFS_GRAY) {
            return true;
        }
        if (color[static_cast<size_t>(next)] == GRAPH_DFS_WHITE && DfsCycleVisit(adjacency, next, color)) {
            return true;
        }
    }
    color[static_cast<size_t>(vertex)] = GRAPH_DFS_BLACK;
    return false;
}

static int32_t FindRoot(std::vector<int32_t>& parent, int32_t vertex)
{
    while (parent[static_cast<size_t>(vertex)] != vertex) {
        vertex = parent[static_cast<size_t>(vertex)];
    }
    return vertex;
}

static bool ExtractEdgesAndVertex(napi_env env,
                                  napi_value edgesValue,
                                  napi_value vertexValue,
                                  std::vector<std::pair<int32_t, int32_t>>& edges,
                                  int32_t& vertex)
{
    if (!ExtractEdgeList(env, edgesValue, edges) || !ExtractInt32Arg(env, vertexValue, vertex)) {
        return false;
    }

    int32_t maxLabel = MaxVertexLabel(edges);
    if (vertex < GRAPH_MIN_VERTEX_LABEL || vertex > maxLabel) {
        napi_throw_error(env, nullptr, "Vertex out of range");
        return false;
    }

    return true;
}

static bool ExtractEdgesAndTwoVertices(napi_env env,
                                       napi_value edgesValue,
                                       napi_value firstValue,
                                       napi_value secondValue,
                                       std::vector<std::pair<int32_t, int32_t>>& edges,
                                       int32_t& first,
                                       int32_t& second)
{
    if (!ExtractEdgeList(env, edgesValue, edges) || !ExtractInt32Arg(env, firstValue, first) ||
        !ExtractInt32Arg(env, secondValue, second)) {
        return false;
    }

    int32_t maxLabel = MaxVertexLabel(edges);
    if (first < GRAPH_MIN_VERTEX_LABEL || first > maxLabel || second < GRAPH_MIN_VERTEX_LABEL || second > maxLabel) {
        napi_throw_error(env, nullptr, "Vertex out of range");
        return false;
    }

    return true;
}

void BfsOrderFrom(const std::vector<std::vector<int32_t>>& adjacency, int32_t start, std::vector<int32_t>& order)
{
    if (adjacency.empty()) {
        return;
    }

    std::vector<bool> visited(adjacency.size(), false);
    std::vector<int32_t> queue;
    queue.push_back(start);
    visited[static_cast<size_t>(start)] = true;
    size_t head = 0;
    while (head < queue.size()) {
        int32_t current = queue[head];
        ++head;
        order.push_back(current);
        const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(current)];
        for (size_t i = 0; i < neighbors.size(); ++i) {
            int32_t next = neighbors[i];
            if (!visited[static_cast<size_t>(next)]) {
                visited[static_cast<size_t>(next)] = true;
                queue.push_back(next);
            }
        }
    }
}

void DfsOrderFrom(const std::vector<std::vector<int32_t>>& adjacency, int32_t start, std::vector<int32_t>& order)
{
    if (adjacency.empty()) {
        return;
    }

    std::vector<bool> visited(adjacency.size(), false);
    DfsPreorder(adjacency, start, visited, order);
}

void BfsDistanceFrom(const std::vector<std::vector<int32_t>>& adjacency, int32_t start, std::vector<int32_t>& distance)
{
    distance.assign(adjacency.size(), GRAPH_NO_PATH);
    if (adjacency.empty()) {
        return;
    }

    distance[static_cast<size_t>(start)] = GRAPH_ORIGIN_DISTANCE;
    std::vector<int32_t> queue;
    queue.push_back(start);
    size_t head = 0;
    while (head < queue.size()) {
        int32_t current = queue[head];
        ++head;
        const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(current)];
        for (size_t i = 0; i < neighbors.size(); ++i) {
            int32_t next = neighbors[i];
            if (distance[static_cast<size_t>(next)] == GRAPH_NO_PATH) {
                distance[static_cast<size_t>(next)] = distance[static_cast<size_t>(current)] + GRAPH_HOP_STEP;
                queue.push_back(next);
            }
        }
    }
}

int32_t CountComponents(const std::vector<std::vector<int32_t>>& adjacency)
{
    std::vector<int32_t> sizes;
    CollectComponentSizes(adjacency, sizes);
    return static_cast<int32_t>(sizes.size());
}

void CollectComponentSizes(const std::vector<std::vector<int32_t>>& adjacency, std::vector<int32_t>& sizes)
{
    std::vector<bool> visited(adjacency.size(), false);
    std::vector<int32_t> order;
    for (size_t i = 0; i < adjacency.size(); ++i) {
        if (visited[i]) {
            continue;
        }

        order.clear();
        BfsOrderFrom(adjacency, static_cast<int32_t>(i), order);
        for (size_t j = 0; j < order.size(); ++j) {
            visited[static_cast<size_t>(order[j])] = true;
        }
        sizes.push_back(static_cast<int32_t>(order.size()));
    }
}

static bool ColorComponent(const std::vector<std::vector<int32_t>>& adjacency,
                           int32_t start,
                           std::vector<int32_t>& color)
{
    color[static_cast<size_t>(start)] = GRAPH_COLOR_FIRST;
    std::vector<int32_t> queue;
    queue.push_back(start);
    size_t head = 0;
    while (head < queue.size()) {
        int32_t current = queue[head];
        ++head;
        int32_t currentColor = color[static_cast<size_t>(current)];
        const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(current)];
        for (size_t j = 0; j < neighbors.size(); ++j) {
            int32_t next = neighbors[j];
            if (color[static_cast<size_t>(next)] == GRAPH_COLOR_UNCOLORED) {
                int32_t nextColor = currentColor == GRAPH_COLOR_FIRST ? GRAPH_COLOR_SECOND : GRAPH_COLOR_FIRST;
                color[static_cast<size_t>(next)] = nextColor;
                queue.push_back(next);
            } else if (color[static_cast<size_t>(next)] == currentColor) {
                return false;
            }
        }
    }
    return true;
}

bool IsBipartiteAdjacency(const std::vector<std::vector<int32_t>>& adjacency)
{
    std::vector<int32_t> color(adjacency.size(), GRAPH_COLOR_UNCOLORED);
    for (size_t i = 0; i < adjacency.size(); ++i) {
        if (color[i] == GRAPH_COLOR_UNCOLORED && !ColorComponent(adjacency, static_cast<int32_t>(i), color)) {
            return false;
        }
    }
    return true;
}

bool HasDirectedCycle(const std::vector<std::vector<int32_t>>& adjacency)
{
    std::vector<int32_t> color(adjacency.size(), GRAPH_DFS_WHITE);
    for (size_t i = 0; i < adjacency.size(); ++i) {
        if (color[i] == GRAPH_DFS_WHITE && DfsCycleVisit(adjacency, static_cast<int32_t>(i), color)) {
            return true;
        }
    }
    return false;
}

napi_value VertexCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    int32_t maxLabel = MaxVertexLabel(edges);
    int32_t count = maxLabel == GRAPH_NO_VERTEX ? GRAPH_EMPTY_VERTEX_COUNT : maxLabel + GRAPH_VERTEX_COUNT_OFFSET;
    return CreateInt32Value(env, count);
}

napi_value EdgeCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    return CreateInt32Value(env, static_cast<int32_t>(edges.size()));
}

napi_value DegreeOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, vertex");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t vertex = 0;
    if (!ExtractEdgesAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], edges, vertex)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildAdjacency(edges, false, adjacency, MaxVertexLabel(edges));
    return CreateInt32Value(env, static_cast<int32_t>(adjacency[static_cast<size_t>(vertex)].size()));
}

napi_value NeighborsOf(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, vertex");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t vertex = 0;
    if (!ExtractEdgesAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], edges, vertex)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildAdjacency(edges, false, adjacency, MaxVertexLabel(edges));
    std::vector<int32_t> neighbors = adjacency[static_cast<size_t>(vertex)];
    SortNeighbors(neighbors);
    return CreateInt32ArrayValue(env, neighbors);
}

napi_value HasEdge(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: edge list, u, v");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t first = 0;
    int32_t second = 0;
    if (!ExtractEdgesAndTwoVertices(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO], edges, first,
                                    second)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildAdjacency(edges, false, adjacency, MaxVertexLabel(edges));
    const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(first)];
    bool found = std::find(neighbors.begin(), neighbors.end(), second) != neighbors.end();
    return CreateBoolValue(env, found);
}

napi_value BfsOrder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, start vertex");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t start = 0;
    if (!ExtractEdgesAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], edges, start)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    std::vector<int32_t> order;
    BfsOrderFrom(adjacency, start, order);
    return CreateInt32ArrayValue(env, order);
}

napi_value DfsOrder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, start vertex");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t start = 0;
    if (!ExtractEdgesAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], edges, start)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    std::vector<int32_t> order;
    DfsOrderFrom(adjacency, start, order);
    return CreateInt32ArrayValue(env, order);
}

napi_value ReachableCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, start vertex");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t start = 0;
    if (!ExtractEdgesAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], edges, start)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    std::vector<int32_t> order;
    BfsOrderFrom(adjacency, start, order);
    return CreateInt32Value(env, static_cast<int32_t>(order.size()));
}

napi_value ComponentCount(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    return CreateInt32Value(env, CountComponents(adjacency));
}

napi_value IsConnected(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    int32_t components = CountComponents(adjacency);
    return CreateBoolValue(env, components <= GRAPH_MAX_CONNECTED_COMPONENTS);
}

napi_value ShortestPathHops(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: edge list, from, to");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t from = 0;
    int32_t to = 0;
    if (!ExtractEdgesAndTwoVertices(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO], edges, from,
                                    to)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    std::vector<int32_t> distance;
    BfsDistanceFrom(adjacency, from, distance);
    return CreateInt32Value(env, distance[static_cast<size_t>(to)]);
}

napi_value PathSequence(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: edge list, from, to");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t from = 0;
    int32_t to = 0;
    if (!ExtractEdgesAndTwoVertices(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO], edges, from,
                                    to)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    std::vector<int32_t> parent(adjacency.size(), GRAPH_NO_VERTEX);
    std::vector<int32_t> queue;
    queue.push_back(from);
    size_t head = 0;
    while (head < queue.size()) {
        int32_t current = queue[head];
        ++head;
        const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(current)];
        for (size_t i = 0; i < neighbors.size(); ++i) {
            int32_t next = neighbors[i];
            if (parent[static_cast<size_t>(next)] == GRAPH_NO_VERTEX && next != from) {
                parent[static_cast<size_t>(next)] = current;
                queue.push_back(next);
            }
        }
    }

    if (parent[static_cast<size_t>(to)] == GRAPH_NO_VERTEX && to != from) {
        return CreateInt32ArrayValue(env, std::vector<int32_t>());
    }

    std::vector<int32_t> path;
    int32_t current = to;
    while (current != GRAPH_NO_VERTEX) {
        path.push_back(current);
        if (current == from) {
            break;
        }
        current = parent[static_cast<size_t>(current)];
    }
    std::reverse(path.begin(), path.end());
    return CreateInt32ArrayValue(env, path);
}

napi_value HasCycleUndirected(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    int32_t maxLabel = MaxVertexLabel(edges);
    if (maxLabel == GRAPH_NO_VERTEX) {
        return CreateBoolValue(env, false);
    }

    int32_t vertexCount = maxLabel + GRAPH_VERTEX_COUNT_OFFSET;
    std::vector<int32_t> parent(static_cast<size_t>(vertexCount), GRAPH_NO_VERTEX);
    std::iota(parent.begin(), parent.end(), GRAPH_MIN_VERTEX_LABEL);

    for (size_t i = 0; i < edges.size(); ++i) {
        int32_t rootFirst = FindRoot(parent, edges[i].first);
        int32_t rootSecond = FindRoot(parent, edges[i].second);
        if (rootFirst == rootSecond) {
            return CreateBoolValue(env, true);
        }
        parent[static_cast<size_t>(rootFirst)] = rootSecond;
    }
    return CreateBoolValue(env, false);
}

napi_value HasCycleDirected(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, true, adjacency);
    return CreateBoolValue(env, HasDirectedCycle(adjacency));
}

napi_value IsBipartite(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    return CreateBoolValue(env, IsBipartiteAdjacency(adjacency));
}

napi_value TopologicalOrder(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, true, adjacency);
    int32_t vertexCount = static_cast<int32_t>(adjacency.size());
    std::vector<int32_t> indegree(static_cast<size_t>(vertexCount), GRAPH_INDEGREE_ZERO);
    for (size_t i = 0; i < adjacency.size(); ++i) {
        for (size_t j = 0; j < adjacency[i].size(); ++j) {
            ++indegree[static_cast<size_t>(adjacency[i][j])];
        }
    }

    std::set<int32_t> ready;
    for (int32_t i = 0; i < vertexCount; ++i) {
        if (indegree[static_cast<size_t>(i)] == GRAPH_INDEGREE_ZERO) {
            ready.insert(i);
        }
    }

    std::vector<int32_t> order;
    while (!ready.empty()) {
        int32_t current = *ready.begin();
        ready.erase(ready.begin());
        order.push_back(current);
        const std::vector<int32_t>& neighbors = adjacency[static_cast<size_t>(current)];
        for (size_t i = 0; i < neighbors.size(); ++i) {
            int32_t next = neighbors[i];
            --indegree[static_cast<size_t>(next)];
            if (indegree[static_cast<size_t>(next)] == GRAPH_INDEGREE_ZERO) {
                ready.insert(next);
            }
        }
    }

    if (static_cast<int32_t>(order.size()) < vertexCount) {
        return CreateInt32ArrayValue(env, std::vector<int32_t>());
    }
    return CreateInt32ArrayValue(env, order);
}

napi_value MaxDegree(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    int32_t maxLabel = MaxVertexLabel(edges);
    if (maxLabel == GRAPH_NO_VERTEX) {
        return CreateInt32Value(env, GRAPH_EMPTY_VERTEX_COUNT);
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildAdjacency(edges, false, adjacency, MaxVertexLabel(edges));
    int32_t maxDegree = 0;
    for (size_t i = 0; i < adjacency.size(); ++i) {
        maxDegree = std::max(maxDegree, static_cast<int32_t>(adjacency[i].size()));
    }
    return CreateInt32Value(env, maxDegree);
}

napi_value MaxDegreeVertex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    int32_t maxLabel = MaxVertexLabel(edges);
    if (maxLabel == GRAPH_NO_VERTEX) {
        return CreateInt32Value(env, GRAPH_NO_VERTEX);
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildAdjacency(edges, false, adjacency, MaxVertexLabel(edges));
    int32_t bestVertex = GRAPH_NO_VERTEX;
    int32_t bestDegree = 0;
    for (size_t i = 0; i < adjacency.size(); ++i) {
        int32_t degree = static_cast<int32_t>(adjacency[i].size());
        if (degree > bestDegree) {
            bestDegree = degree;
            bestVertex = static_cast<int32_t>(i);
        }
    }
    return CreateInt32Value(env, bestVertex);
}

napi_value IsIsolatedVertex(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: edge list, vertex");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t vertex = 0;
    if (!ExtractEdgesAndVertex(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], edges, vertex)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildAdjacency(edges, false, adjacency, MaxVertexLabel(edges));
    return CreateBoolValue(env, adjacency[static_cast<size_t>(vertex)].empty());
}

napi_value CommonNeighbors(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: edge list, u, v");

    std::vector<std::pair<int32_t, int32_t>> edges;
    int32_t first = 0;
    int32_t second = 0;
    if (!ExtractEdgesAndTwoVertices(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO], edges, first,
                                    second)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    const std::vector<int32_t>& firstNeighbors = adjacency[static_cast<size_t>(first)];
    const std::vector<int32_t>& secondNeighbors = adjacency[static_cast<size_t>(second)];
    std::vector<int32_t> common;
    size_t i = 0;
    size_t j = 0;
    while (i < firstNeighbors.size() && j < secondNeighbors.size()) {
        if (firstNeighbors[i] < secondNeighbors[j]) {
            ++i;
        } else if (firstNeighbors[i] > secondNeighbors[j]) {
            ++j;
        } else {
            common.push_back(firstNeighbors[i]);
            ++i;
            ++j;
        }
    }
    return CreateInt32ArrayValue(env, common);
}

napi_value ComponentSizes(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::vector<std::vector<int32_t>> adjacency;
    BuildSortedAdjacency(edges, false, adjacency);
    std::vector<int32_t> sizes;
    CollectComponentSizes(adjacency, sizes);
    std::sort(sizes.begin(), sizes.end());
    return CreateInt32ArrayValue(env, sizes);
}

napi_value HasParallelEdges(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: edge list");

    std::vector<std::pair<int32_t, int32_t>> edges;
    if (!ExtractEdgeList(env, argv[ARG_INDEX_ZERO], edges)) {
        return nullptr;
    }

    std::set<std::pair<int32_t, int32_t>> seen;
    for (size_t i = 0; i < edges.size(); ++i) {
        int32_t low = edges[i].first <= edges[i].second ? edges[i].first : edges[i].second;
        int32_t high = edges[i].first <= edges[i].second ? edges[i].second : edges[i].first;
        if (!seen.insert(std::make_pair(low, high)).second) {
            return CreateBoolValue(env, true);
        }
    }
    return CreateBoolValue(env, false);
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("vertexCount", VertexCount),
        DECLARE_NAPI_FUNCTION("edgeCount", EdgeCount),
        DECLARE_NAPI_FUNCTION("degreeOf", DegreeOf),
        DECLARE_NAPI_FUNCTION("neighborsOf", NeighborsOf),
        DECLARE_NAPI_FUNCTION("hasEdge", HasEdge),
        DECLARE_NAPI_FUNCTION("bfsOrder", BfsOrder),
        DECLARE_NAPI_FUNCTION("dfsOrder", DfsOrder),
        DECLARE_NAPI_FUNCTION("reachableCount", ReachableCount),
        DECLARE_NAPI_FUNCTION("componentCount", ComponentCount),
        DECLARE_NAPI_FUNCTION("isConnected", IsConnected),
        DECLARE_NAPI_FUNCTION("shortestPathHops", ShortestPathHops),
        DECLARE_NAPI_FUNCTION("pathSequence", PathSequence),
        DECLARE_NAPI_FUNCTION("hasCycleUndirected", HasCycleUndirected),
        DECLARE_NAPI_FUNCTION("hasCycleDirected", HasCycleDirected),
        DECLARE_NAPI_FUNCTION("isBipartite", IsBipartite),
        DECLARE_NAPI_FUNCTION("topologicalOrder", TopologicalOrder),
        DECLARE_NAPI_FUNCTION("maxDegree", MaxDegree),
        DECLARE_NAPI_FUNCTION("maxDegreeVertex", MaxDegreeVertex),
        DECLARE_NAPI_FUNCTION("isIsolatedVertex", IsIsolatedVertex),
        DECLARE_NAPI_FUNCTION("commonNeighbors", CommonNeighbors),
        DECLARE_NAPI_FUNCTION("componentSizes", ComponentSizes),
        DECLARE_NAPI_FUNCTION("hasParallelEdges", HasParallelEdges),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    NAPI_CALL_BASE(env, RegisterGraphOpsFunctions(env, exports), nullptr);
    return exports;
}
EXTERN_C_END

static napi_module graphSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "graphSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void GraphSuiteRegisterModule(void)
{
    napi_module_register(&graphSuiteModule);
}
