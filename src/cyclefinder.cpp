#include <QString>
#include <vector>
#include <QDebug>
#include <algorithm>
#include <queue>
#include "cyclefinder.h"
#include "shortestpath.h"

//
// The cycle finder implement the following method:
//      1. Remove edge
//      2. Find shortest alternative path between the removed edge
// https://www.geeksforgeeks.org/find-minimum-weight-cycle-undirected-graph/
//

CycleFinder::CycleFinder(const std::vector<QVector3D> &nodePositions,
        const std::vector<std::pair<size_t, size_t>> &edges) :
    m_nodeNum(nodePositions.size()),
    m_nodePositions(nodePositions),
    m_edges(edges)
{
}

void CycleFinder::prepareWeights()
{
    m_edgeLengths.resize(m_edges.size(), 1);
    for (size_t i = 0; i < m_edges.size(); ++i) {
        const auto &edge = m_edges[i];
        auto length = m_nodePositions[edge.first].distanceToPoint(
            m_nodePositions[edge.second]) * 10000;
        m_edgeLengths[i] = length;
        m_edgeLengthMap.insert({std::make_pair(edge.first, edge.second), length});
        m_edgeLengthMap.insert({std::make_pair(edge.second, edge.first), length});
    }
}

void CycleFinder::find()
{
    prepareWeights();
    
    if (m_edges.empty())
        return;
    
    std::queue<std::pair<size_t, size_t>> waitEdges;
    std::set<std::pair<size_t, size_t>> visited;
    std::map<std::pair<size_t, size_t>, size_t> halfEdgeToCycleMap;
    
    auto isPathValid = [&](const std::vector<size_t> &path) {
        for (size_t i = 0; i < path.size(); ++i) {
            size_t j = (i + 1) % path.size();
            auto edge = std::make_pair(path[i], path[j]);
            if (m_halfEdges.find(edge) != m_halfEdges.end()) {
                return false;
            }
        }
        /*
        std::set<size_t> neighborCycles;
        for (size_t i = 0; i < path.size(); ++i) {
            size_t j = (i + 1) % path.size();
            auto oppositeEdge = std::make_pair(path[j], path[i]);
            auto findOpposite = halfEdgeToCycleMap.find(oppositeEdge);
            if (findOpposite == halfEdgeToCycleMap.end())
                continue;
            neighborCycles.insert(findOpposite->second);
        }
        if (neighborCycles.size() > 4) {
            // Should not have so many neighbors
            return false;
        }*/
        return true;
    };
    
    std::map<std::pair<size_t, size_t>, size_t> edgeIndexMap;
    for (size_t i = 0; i < m_edges.size(); ++i) {
        const auto &edge = m_edges[i];
        edgeIndexMap.insert({edge, i});
        edgeIndexMap.insert({std::make_pair(edge.second, edge.first), i});
    }
    
    waitEdges.push(m_edges[0]);
    while (!waitEdges.empty()) {
        auto currentEdge = waitEdges.front();
        waitEdges.pop();
        if (visited.find(currentEdge) != visited.end())
            continue;
        visited.insert(currentEdge);
        
        auto edges = m_edges;
        auto weights = m_edgeLengths;
        std::vector<size_t> path;
        removeEdgeFrom(currentEdge, &edges, &weights);
        if (!shortestPath(m_nodeNum, edges, weights, currentEdge.first, currentEdge.second, &path))
            continue;
        
        if (!isPathValid(path))
            continue;
        
        for (size_t i = 0; i < path.size(); ++i) {
            size_t j = (i + 1) % path.size();
            auto edge = std::make_pair(path[i], path[j]);
            m_halfEdges.insert(edge);
            halfEdgeToCycleMap.insert({edge, m_cycles.size()});
        }
        
        m_cycles.push_back(path);
        
        // Update weights
        for (size_t i = 0; i < path.size(); ++i) {
            size_t j = (i + 1) % path.size();
            auto edge = std::make_pair(path[i], path[j]);
            auto index = edgeIndexMap[edge];
            m_edgeLengths[index] += 100000;
        }
        
        // Add opposite edges to wait queue
        for (size_t i = 0; i < path.size(); ++i) {
            size_t j = (i + 1) % path.size();
            auto oppositeEdge = std::make_pair(path[j], path[i]);
            if (visited.find(oppositeEdge) != visited.end())
                continue;
            waitEdges.push(oppositeEdge);
        }
    }
    
    auto cycleLength = [&](const std::vector<size_t> &path) {
        float length = 0;
        for (size_t i = 0; i < path.size(); ++i) {
            size_t j = (i + 1) % path.size();
            auto edge = std::make_pair(path[i], path[j]);
            length += m_edgeLengthMap[edge];
        }
        return length;
    };
    
    // Remove the longest cycle
    if (m_cycles.size() > 1) {
        float maxCycleLength = 0;
        size_t maxCycle = 0;
        for (size_t i = 0; i < m_cycles.size(); ++i) {
            float length = cycleLength(m_cycles[i]);
            if (length > maxCycleLength) {
                maxCycle = i;
                maxCycleLength = length;
            }
        }
        m_cycles.erase(m_cycles.begin() + maxCycle);
    }
}

const std::vector<std::vector<size_t>> &CycleFinder::getCycles()
{
    return m_cycles;
}

void CycleFinder::removeEdgeFrom(const std::pair<size_t, size_t> &edge,
        std::vector<std::pair<size_t, size_t>> *edges,
        std::vector<int> *edgeLengths)
{
    auto oppositeEdge = std::make_pair(edge.second, edge.first);
    for (auto it = edges->begin(); it != edges->end(); ++it) {
        if (*it == edge || *it == oppositeEdge) {
            auto index = it - edges->begin();
            edgeLengths->erase(edgeLengths->begin() + index);
            edges->erase(it);
            break;
        }
    }
}
