#include <simpleboolean/subsurface.h>
#include <QStringList>
#include <set>
#include <queue>
#include <array>
#include <QDebug>
#include <QElapsedTimer>

namespace simpleboolean
{

void SubSurface::createSubSurfaces(std::vector<std::vector<size_t>> &edgeLoops,
        const std::vector<Face> &triangles,
        std::vector<bool> &visitedTriangles,
        std::vector<SubSurface> &subSurfaces)
{
    if (edgeLoops.empty())
        return;
    
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    
    auto createHalfEdgesStartTime = elapsedTimer.elapsed();
    std::map<std::pair<size_t, size_t>, size_t> halfEdges;
    for (size_t m = 0; m < triangles.size(); ++m) {
        const auto &triangle = triangles[m];
        for (size_t i = 0; i < 3; ++i) {
            size_t j = (i + 1) % 3;
            auto edge = std::make_pair(triangle.indices[i], triangle.indices[j]);
            halfEdges.insert({edge, m});
        }
    }
    //qDebug() << "Create halfedges took" << (elapsedTimer.elapsed() - createHalfEdgesStartTime) << "milliseconds";
    
    auto createEdgeLoopMapStartTime = elapsedTimer.elapsed();
    std::vector<bool> edgeLoopFlippedMap(edgeLoops.size(), false);
    std::map<std::pair<size_t, size_t>, std::pair<size_t, bool>> edgeToLoopMap;
    for (size_t m = 0; m < edgeLoops.size(); ++m) {
        auto &edgeLoop = edgeLoops[m];
        for (size_t i = 0; i < edgeLoop.size(); ++i) {
            size_t j = (i + 1) % edgeLoop.size();
            edgeToLoopMap.insert({std::make_pair(edgeLoop[i], edgeLoop[j]), std::make_pair(m, true)});
            edgeToLoopMap.insert({std::make_pair(edgeLoop[j], edgeLoop[i]), std::make_pair(m, false)});
        }
    }
    
    for (const auto &edgeToLoop: edgeToLoopMap) {
        auto findTriangle = halfEdges.find(edgeToLoop.first);
        if (findTriangle == halfEdges.end())
            continue;
        if (visitedTriangles[findTriangle->second])
            continue;
        std::queue<size_t> waitTriangles;
        waitTriangles.push(findTriangle->second);
        SubSurface subSurface;
        subSurface.edgeLoopIndex = edgeToLoop.second.first;
        subSurface.isFrontSide = edgeToLoop.second.second;
        while (!waitTriangles.empty()) {
            auto triangleIndex = waitTriangles.front();
            waitTriangles.pop();
            if (visitedTriangles[triangleIndex])
                continue;
            visitedTriangles[triangleIndex] = true;
            const auto &triangle = triangles[triangleIndex];
            subSurface.faces.push_back(triangle);
            for (size_t i = 0; i < 3; ++i) {
                size_t j = (i + 1) % 3;
                auto edge = std::make_pair(triangle.indices[i], triangle.indices[j]);
                auto findEdgeLoop = edgeToLoopMap.find(edge);
                if (findEdgeLoop != edgeToLoopMap.end()) {
                    subSurface.owners.insert(findEdgeLoop->second);
                } else {
                    auto findNeighbor = halfEdges.find(std::make_pair(edge.second, edge.first));
                    if (findNeighbor != halfEdges.end() &&
                            !visitedTriangles[findNeighbor->second]) {
                        waitTriangles.push(findNeighbor->second);
                    }
                }
            }
        }
        if (subSurface.faces.empty())
            continue;
        if (subSurface.owners.size() <= 1) {
            subSurfaces.push_back(subSurface);
            continue;
        }
        for (const auto &owner: subSurface.owners) {
            SubSurface neighbor = subSurface;
            neighbor.edgeLoopIndex = owner.first;
            neighbor.isFrontSide = owner.second;
            neighbor.isSharedByOthers = true;
            subSurfaces.push_back(neighbor);
        }
    }
    
    //qDebug() << "subSurfaces.size:" << subSurfaces.size();
    //for (const auto &it: subSurfaces) {
    //    qDebug() << "[" << it.edgeLoopIndex << "] [" << (it.isFrontSide ? "Front" : "Back") << "] faces:" << it.faces.size();
    //}
}

}
