#include <simpleboolean/edgeloop.h>
#include <map>
#include <QDebug>
#include <set>
#include <algorithm>

namespace simpleboolean
{

void EdgeLoop::merge(const std::vector<std::vector<size_t>> &sourceEdgeLoops,
        std::vector<std::vector<size_t>> *targetEdgeLoops)
{
    std::vector<std::pair<size_t, size_t>> edges;
    for (size_t m = 0; m < sourceEdgeLoops.size(); ++m) {
        const auto &source = sourceEdgeLoops[m];
        //qDebug() << "source[" << m << "]:" << source;
        if (source.front() == source.back()) {
            auto newEdgeLoop = source;
            newEdgeLoop.pop_back();
            targetEdgeLoops->push_back(newEdgeLoop);
            continue;
        }
        for (size_t i = 1; i < source.size(); ++i) {
            edges.push_back(std::make_pair(source[i - 1], source[i]));
            edges.push_back(std::make_pair(source[i], source[i - 1]));
        }
    }
    buildEdgeLoopsFromDirectedEdges(edges, targetEdgeLoops, false, false);
    //for (size_t i = 0; i < targetEdgeLoops->size(); ++i) {
    //    qDebug() << "target[" << i << "]:" << (*targetEdgeLoops)[i];
    //}
}

void EdgeLoop::buildEdgeLoopsFromDirectedEdges(const std::vector<std::pair<size_t, size_t>> &edges,
        std::vector<std::vector<size_t>> *edgeLoops,
        bool allowOpenEdgeLoop,
        bool allowOppositeEdgeLoop)
{
    //qDebug() << "================== allowOpenEdgeLoop:" << allowOpenEdgeLoop << "==================";
    //qDebug() << "edges:" << edges;

    std::map<size_t, std::vector<size_t>> nodeNeighborMap;
    std::set<std::pair<size_t, size_t>> edgeSet;
    for (const auto &it: edges) {
        nodeNeighborMap[it.first].push_back(it.second);
        edgeSet.insert(it);
    }
    
    // After this sorting, the endpoint will come first in the following edge loop searching
    auto sortedEdges = edges;
    std::sort(sortedEdges.begin(), sortedEdges.end(), [&](const std::pair<size_t, size_t> &a,
            const std::pair<size_t, size_t> &b) {
        return nodeNeighborMap[a.first].size() < nodeNeighborMap[b.first].size();
    });
    
    std::set<std::pair<size_t, size_t>> visitedEdges;
    for (const auto &currentEdge: sortedEdges) {
        if (visitedEdges.find(currentEdge) != visitedEdges.end())
            continue;
        std::set<std::pair<size_t, size_t>> localVisitedEdges;
        std::vector<size_t> edgeLoop;
        visitedEdges.insert(currentEdge);
        if (!allowOppositeEdgeLoop) {
            if (visitedEdges.find(std::make_pair(currentEdge.second, currentEdge.first)) != visitedEdges.end()) {
                //qDebug() << "Ignore edge because of opposite edge exists:" << currentEdge;
                continue;
            }
        }
        localVisitedEdges.insert(currentEdge);
        edgeLoop.push_back(currentEdge.first);
        //qDebug() << "------ Current edge:" << currentEdge;
        size_t loopNode = currentEdge.second;
        for (;;) {
            edgeLoop.push_back(loopNode);
            if (loopNode == currentEdge.first) {
                //qDebug() << "Looped back, finish";
                break;
            }
            auto findNext = nodeNeighborMap.find(loopNode);
            if (findNext == nodeNeighborMap.end()) {
                //qDebug() << "No neighbor for node:" << loopNode;
                break;
            }
            if (1 == findNext->second.size()) {
                auto nextNode = findNext->second[0];
                auto edge = std::make_pair(loopNode, nextNode);
                if (visitedEdges.find(edge) != visitedEdges.end()) {
                    //qDebug() << "Only one neighbor and the edge already visited";
                    break;
                }
                if (!allowOppositeEdgeLoop) {
                    auto oppositeEdge = std::make_pair(nextNode, loopNode);
                    if (visitedEdges.find(oppositeEdge) != visitedEdges.end()) {
                        //qDebug() << "Ignore edge because of opposite edge exists:" << edge;
                        break;
                    }
                }
                visitedEdges.insert(edge);
                localVisitedEdges.insert(edge);
                loopNode = nextNode;
                //qDebug() << "Next node:" << loopNode;
                continue;
            }
            bool nextResolved = false;
            // First try the edge which has opposite edge, this means the new edge is most likely been picked first, then triangle edges
            for (size_t round = 0; !nextResolved && round < 2; ++round) {
                for (const auto &nextNode: findNext->second) {
                    auto oppositeEdge = std::make_pair(nextNode, loopNode);
                    if (0 == round) {
                        if (edgeSet.find(oppositeEdge) == edgeSet.end()) {
                            //qDebug() << "Node ignored because of no opposite edge:" << nextNode;
                            continue;
                        }
                    }
                    auto edge = std::make_pair(loopNode, nextNode);
                    if (visitedEdges.find(edge) != visitedEdges.end()) {
                        //qDebug() << "Visited edge:" << edge;
                        continue;
                    }
                    if (localVisitedEdges.find(oppositeEdge) != localVisitedEdges.end()) {
                        //qDebug() << "Ignore edge because of opposite edge exists:" << edge;
                        continue;
                    }
                    if (!allowOppositeEdgeLoop) {
                        if (visitedEdges.find(oppositeEdge) != visitedEdges.end()) {
                            //qDebug() << "Ignore edge because of opposite edge exists:" << edge;
                            continue;
                        }
                    }
                    visitedEdges.insert(edge);
                    localVisitedEdges.insert(edge);
                    loopNode = nextNode;
                    nextResolved = true;
                    //qDebug() << "Next choosen node:" << loopNode;
                    break;
                }
            }
            if (!nextResolved) {
                break;
            }
        }
        if (allowOpenEdgeLoop) {
            if (3 == edgeLoop.size() && edgeLoop.front() == edgeLoop.back()) {
                edgeLoop.pop_back();
            }
        } else {
            if (edgeLoop.size() < 4) {
                //qDebug() << "Invalid edge loop size:" << edgeLoop.size() << "loop:" << edgeLoop;
                continue;
            }
            if (edgeLoop.front() != edgeLoop.back()) {
                //qDebug() << "Invalid edge loop:" << edgeLoop;
                continue;
            }
            edgeLoop.pop_back();
        }
        //qDebug() << "Good edgeLoop:" << edgeLoop;
        edgeLoops->push_back(edgeLoop);
    }
}

void EdgeLoop::unifyDirection(const std::vector<std::vector<size_t>> &reference,
        std::vector<std::vector<size_t>> *target)
{
    std::vector<std::set<std::pair<size_t, size_t>>> referenceHalfEdges;
    for (const auto &it: reference) {
        std::set<std::pair<size_t, size_t>> halfEdges;
        for (size_t i = 0; i < it.size(); ++i) {
            size_t j = (i + 1) % it.size();
            halfEdges.insert(std::make_pair(it[i], it[j]));
        }
        referenceHalfEdges.push_back(halfEdges);
    }
    
    auto isHalfEdgeInReference = [&](const std::pair<size_t, size_t> &halfEdge) {
        for (const auto &it: referenceHalfEdges) {
            if (it.find(halfEdge) != it.end())
                return true;
        }
        return false;
    };
    
    for (auto &it: *target) {
        if (it.size() < 2)
            return;
        if (isHalfEdgeInReference(std::make_pair(it[0], it[1])))
            continue;
        std::reverse(it.begin(), it.end());
    }
}

}
