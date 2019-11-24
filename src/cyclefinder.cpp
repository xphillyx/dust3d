#include <QString>
#include <vector>
#include <QDebug>
#include <algorithm>
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
            m_nodePositions[edge.second]) * 1000;
        m_edgeLengths[i] = length;
        m_edgeLengthMap.insert({std::make_pair(edge.first, edge.second), length});
        m_edgeLengthMap.insert({std::make_pair(edge.second, edge.first), length});
    }
}

bool CycleFinder::validateCycle(const std::set<std::pair<size_t, size_t>> &newPathEdges,
        const std::set<std::pair<size_t, size_t>> &oldPathEdges)
{
    auto diff = [](const std::set<std::pair<size_t, size_t>> &first,
            const std::set<std::pair<size_t, size_t>> &second) {
        auto diffResult = second;
        for (const auto &it: first) {
            diffResult.erase(std::make_pair(it.first, it.second));
            diffResult.erase(std::make_pair(it.second, it.first));
        }
        return diffResult;
    };
    
    auto diffEdges = diff(newPathEdges, oldPathEdges);
    auto diffDiffEdges = diff(diffEdges, oldPathEdges);
    
    auto sumOfEdgeLengths = [&](const std::set<std::pair<size_t, size_t>> &edges) {
        int totalLength = 0;
        for (const auto &it: edges) {
            totalLength += m_edgeLengthMap[it];
        }
        return totalLength;
    };
    
    if (sumOfEdgeLengths(diffEdges) < sumOfEdgeLengths(diffDiffEdges))
        return false;

    return true;
}

void CycleFinder::find()
{
    prepareWeights();
    
    for (const auto &it: m_edges) {
        if (m_cycleEdges.find(it) != m_cycleEdges.end())
            continue;
        auto edges = m_edges;
        auto weights = m_edgeLengths;
        std::vector<size_t> path;
        removeEdgeFrom(it, &edges, &weights);
        if (shortestPath(m_nodeNum, edges, weights, it.first, it.second, &path)) {
            m_cycles.push_back(path);
            addPathToCycleEdges(path, &m_cycleEdges);
            
            /*
            // Check the opposite cycles
            auto reweightedEdges = m_edges;
            auto reweightedWeights = m_edgeLengths;
            std::set<std::pair<size_t, size_t>> pathEdges;
            addPathToCycleEdges(path, &pathEdges);
            for (size_t i = 0; i < reweightedWeights.size(); ++i) {
                const auto &edge = reweightedEdges[i];
                if (pathEdges.find(edge) != pathEdges.end())
                    reweightedWeights[i] = 100000;
            }
            for (size_t i = 0; i < path.size(); ++i) {
                size_t j = (i + 1) % path.size();
                std::pair<size_t, size_t> edgeInPath = std::make_pair(path[i], path[j]);
                auto tryEdges = reweightedEdges;
                auto tryWeights = reweightedWeights;
                std::vector<size_t> oppositePath;
                removeEdgeFrom(edgeInPath, &tryEdges, &tryWeights);
                if (shortestPath(m_nodeNum, tryEdges, tryWeights, edgeInPath.first, edgeInPath.second,
                        &oppositePath)) {
                    for (size_t m = 0; m < oppositePath.size(); ++m) {
                        size_t n = (m + 1) % oppositePath.size();
                        std::pair<size_t, size_t> possibleNewEdge = std::make_pair(oppositePath[m], oppositePath[n]);
                        if (m_cycleEdges.find(possibleNewEdge) == m_cycleEdges.end()) {
                            std::set<std::pair<size_t, size_t>> oppositePathEdges;
                            addPathToCycleEdges(oppositePath, &oppositePathEdges);
                            if (validateCycle(oppositePathEdges, pathEdges)) {
                                m_cycles.push_back(oppositePath);
                                addPathToCycleEdges(oppositePath, &m_cycleEdges);
                            }
                            break;
                        }
                    }
                }
            }
            */
        }
    }
}

const std::vector<std::vector<size_t>> &CycleFinder::getCycles()
{
    return m_cycles;
}

void CycleFinder::addPathToCycleEdges(const std::vector<size_t> &path,
    std::set<std::pair<size_t, size_t>> *cycleEdges)
{
    for (size_t i = 0; i < path.size(); ++i) {
        size_t j = (i + 1) % path.size();
        cycleEdges->insert({path[i], path[j]});
        cycleEdges->insert({path[j], path[i]});
    }
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
