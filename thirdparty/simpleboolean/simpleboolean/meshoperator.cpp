#include <queue>
#include <map>
#include <set>
#include <simpleboolean/meshoperator.h>
#include <simpleboolean/retriangulation.h>
#include <simpleboolean/edgeloop.h>
#include <simpleboolean/subsurface.h>
#include <simpleboolean/subblock.h>
#include <simpleboolean/axisalignedboundingboxtree.h>
#include <thirdparty/GuigueDevillers03/tri_tri_intersect.h>
#include <QDebug>
#include <QElapsedTimer>

namespace simpleboolean
{

int MeshOperator::m_vertexToKeyMultiplyFactor = 1000;

MeshOperator::~MeshOperator()
{
    delete m_potentialIntersectedPairs;
}

void MeshOperator::setMeshes(const Mesh &first, const Mesh &second)
{
    m_firstMesh = first;
    m_secondMesh = second;
    m_firstMeshFaceAABBs.resize(m_firstMesh.faces.size());
    m_secondMeshFaceAABBs.resize(m_secondMesh.faces.size());
    m_firstOldToNewVertexMap.resize(m_firstMesh.faces.size());
    m_secondOldToNewVertexMap.resize(m_secondMesh.faces.size());
    for (size_t i = 0; i < m_firstMesh.faces.size(); ++i) {
        addFaceToAxisAlignedBoundingBox(m_firstMesh, m_firstMesh.faces[i], m_firstMeshFaceAABBs[i]);
        m_firstMeshFaceAABBs[i].updateCenter();
    }
    for (size_t i = 0; i < m_secondMesh.faces.size(); ++i) {
        addFaceToAxisAlignedBoundingBox(m_secondMesh, m_secondMesh.faces[i], m_secondMeshFaceAABBs[i]);
        m_secondMeshFaceAABBs[i].updateCenter();
    }
}

void MeshOperator::addMeshToAxisAlignedBoundingBox(const Mesh &mesh, AxisAlignedBoudingBox &box)
{
    for (const auto &vertex: mesh.vertices) {
        box.update(vertex);
    }
}

void MeshOperator::addFaceToAxisAlignedBoundingBox(const Mesh &mesh, const Face &face, AxisAlignedBoudingBox &box)
{
    for (size_t i = 0; i < 3; ++i) {
        box.update(mesh.vertices[face.indices[i]]);
    }
}

void MeshOperator::searchPotentialIntersectedPairs()
{
    AxisAlignedBoudingBox firstBox;
    AxisAlignedBoudingBox secondBox;
    AxisAlignedBoudingBox intersectedBox;
    addMeshToAxisAlignedBoundingBox(m_firstMesh, firstBox);
    addMeshToAxisAlignedBoundingBox(m_secondMesh, secondBox);
    firstBox.intersectWithAt(secondBox, &intersectedBox);
    std::vector<size_t> firstGroupOfFacesIn;
    std::vector<size_t> secondGroupOfFacesIn;
    for (size_t i = 0; i < m_firstMeshFaceAABBs.size(); ++i) {
        if (intersectedBox.intersectWith(m_firstMeshFaceAABBs[i])) {
            firstGroupOfFacesIn.push_back(i);
        }
    }
    for (size_t i = 0; i < m_secondMeshFaceAABBs.size(); ++i) {
        if (intersectedBox.intersectWith(m_secondMeshFaceAABBs[i])) {
            secondGroupOfFacesIn.push_back(i);
        }
    }
    AxisAlignedBoudingBox firstGroupBox;
    for (const auto &i: firstGroupOfFacesIn) {
        addFaceToAxisAlignedBoundingBox(m_firstMesh, m_firstMesh.faces[i], firstGroupBox);
    }
    firstGroupBox.updateCenter();
    AxisAlignedBoudingBox secondGroupBox;
    for (const auto &i: secondGroupOfFacesIn) {
        addFaceToAxisAlignedBoundingBox(m_secondMesh, m_secondMesh.faces[i], secondGroupBox);
    }
    secondGroupBox.updateCenter();
    AxisAlignedBoudingBoxTree leftTree(&m_firstMeshFaceAABBs, firstGroupOfFacesIn, firstGroupBox);
    AxisAlignedBoudingBoxTree rightTree(&m_secondMeshFaceAABBs, secondGroupOfFacesIn, secondGroupBox);
    m_potentialIntersectedPairs = leftTree.test(leftTree.root(), rightTree.root(), &m_secondMeshFaceAABBs);
}

bool MeshOperator::intersectTwoFaces(size_t firstIndex, size_t secondIndex, std::pair<Vertex, Vertex> &newEdge)
{
    const auto &firstFace = m_firstMesh.faces[firstIndex];
    const auto &secondFace = m_secondMesh.faces[secondIndex];
    int coplanar = 0;
    if (!tri_tri_intersection_test_3d(m_firstMesh.vertices[firstFace.indices[0]].xyz,
            m_firstMesh.vertices[firstFace.indices[1]].xyz,
            m_firstMesh.vertices[firstFace.indices[2]].xyz,
            m_secondMesh.vertices[secondFace.indices[0]].xyz,
            m_secondMesh.vertices[secondFace.indices[1]].xyz,
            m_secondMesh.vertices[secondFace.indices[2]].xyz,
            &coplanar,
            newEdge.first.xyz,
            newEdge.second.xyz)) {
        return false;
    }
    if (coplanar)
        return false;
    return true;
}

std::tuple<int, int, int> MeshOperator::vertexToKey(const Vertex &vertex)
{
    return {vertex.xyz[0] * MeshOperator::m_vertexToKeyMultiplyFactor,
        vertex.xyz[1] * MeshOperator::m_vertexToKeyMultiplyFactor,
        vertex.xyz[2] * MeshOperator::m_vertexToKeyMultiplyFactor
    };
}

size_t MeshOperator::newVertexToIndex(const Vertex &vertex, std::pair<size_t, size_t> oldVertex)
{
    if (0 == oldVertex.first) {
        auto key = vertexToKey(vertex);
        const auto &findResult = m_newVertexToIndexMap.find(key);
        if (findResult != m_newVertexToIndexMap.end()) {
            return findResult->second;
        }
        size_t newIndex = m_newVertices.size();
        m_newVertexToIndexMap.insert({key, newIndex});
        m_newVertices.push_back(vertex);
        return newIndex;
    }
    auto &oldToNewMap = (1 == oldVertex.first) ?
        m_firstOldToNewVertexMap : m_secondOldToNewVertexMap;
    auto &findNew = oldToNewMap[oldVertex.second];
    if (findNew)
        return findNew - 1;
    auto key = vertexToKey(vertex);
    const auto &findResult = m_newVertexToIndexMap.find(key);
    if (findResult != m_newVertexToIndexMap.end()) {
        findNew = findResult->second + 1;
        return findResult->second;
    }
    size_t newIndex = m_newVertices.size();
    m_newVertexToIndexMap.insert({key, newIndex});
    m_newVertices.push_back(vertex);
    findNew = newIndex + 1;
    return newIndex;
}

void MeshOperator::groupEdgesToLoops(const std::vector<std::pair<size_t, size_t>> &edges,
        std::vector<std::vector<size_t>> &edgeLoops)
{
    EdgeLoop::buildEdgeLoopsFromDirectedEdges(edges, &edgeLoops, true, false);
}

bool MeshOperator::combine()
{
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    auto searchPotentialIntersectedPairsStartTime = elapsedTimer.elapsed();
    searchPotentialIntersectedPairs();
    qDebug() << "Search potential intersected pairs took" << (elapsedTimer.elapsed() - searchPotentialIntersectedPairsStartTime) << "milliseconds";
    
    auto checkPotentialIntersectedPairsStartTime = elapsedTimer.elapsed();
    std::map<size_t, std::vector<std::pair<size_t, size_t>>> newEdgesPerTriangleInFirstMesh;
    std::map<size_t, std::vector<std::pair<size_t, size_t>>> newEdgesPerTriangleInSecondMesh;
    std::vector<bool> reTriangulatedFacesInFirstMesh(m_firstMesh.faces.size());
    std::vector<bool> reTriangulatedFacesInSecondMesh(m_secondMesh.faces.size());
    for (const auto &pair: *m_potentialIntersectedPairs) {
        std::pair<Vertex, Vertex> newEdge;
        if (intersectTwoFaces(pair.first, pair.second, newEdge)) {
            if (vertexToKey(newEdge.first) == vertexToKey(newEdge.second))
                continue;
            std::pair<size_t, size_t> newVertexPair = {
                newVertexToIndex(newEdge.first, std::make_pair(0, 0)),
                newVertexToIndex(newEdge.second, std::make_pair(0, 0))
            };
            std::pair<size_t, size_t> newVertexOppositePair = {
                newVertexPair.second,
                newVertexPair.first
            };
            
            newEdgesPerTriangleInFirstMesh[pair.first].push_back(newVertexPair);
            newEdgesPerTriangleInSecondMesh[pair.second].push_back(newVertexPair);

            newEdgesPerTriangleInFirstMesh[pair.first].push_back(newVertexOppositePair);
            newEdgesPerTriangleInSecondMesh[pair.second].push_back(newVertexOppositePair);
            
            reTriangulatedFacesInFirstMesh[pair.first] = true;
            reTriangulatedFacesInSecondMesh[pair.second] = true;
        }
    }
    qDebug() << "Check potential intersected pairs took" << (elapsedTimer.elapsed() - checkPotentialIntersectedPairsStartTime) << "milliseconds";
    auto doReTriangulation = [&](const Mesh *mesh, const std::map<size_t, std::vector<std::pair<size_t, size_t>>> &newEdgesPerTriangle, std::vector<Face> &toTriangles, std::vector<std::vector<size_t>> &edgeLoops,
            size_t meshIndex) {
        for (const auto &it: newEdgesPerTriangle) {
            const auto &face = mesh->faces[it.first];
            const std::vector<std::pair<size_t, size_t>> &newEdges = it.second;
            std::vector<std::vector<size_t>> edgeLoopsPerFace;
            groupEdgesToLoops(newEdges, edgeLoopsPerFace);
            std::vector<size_t> triangleVertices = {
                newVertexToIndex(mesh->vertices[face.indices[0]], std::make_pair(meshIndex + 1, face.indices[0])),
                newVertexToIndex(mesh->vertices[face.indices[1]], std::make_pair(meshIndex + 1, face.indices[1])),
                newVertexToIndex(mesh->vertices[face.indices[2]], std::make_pair(meshIndex + 1, face.indices[2])),
            };
            ReTriangulation re(m_newVertices, triangleVertices, edgeLoopsPerFace);
            re.reTriangulate();
            const auto &reTriangulatedTriangles = re.getResult();
            for (const auto &loop: edgeLoopsPerFace) {
                edgeLoops.push_back(loop);
            }
            for (const auto &triangle: reTriangulatedTriangles) {
                toTriangles.push_back(triangle);
            }
        }
    };
    auto addUnIntersectedFaces = [&](const Mesh *mesh, const std::vector<bool> &reTriangulatedFaces,
            std::vector<Face> &toTriangles,
            size_t meshIndex) {
        for (size_t i = 0; i < mesh->faces.size(); ++i) {
            if (reTriangulatedFaces[i])
                continue;
            const auto &face = mesh->faces[i];
            Face triangle = {{
                newVertexToIndex(mesh->vertices[face.indices[0]], std::make_pair(meshIndex + 1, face.indices[0])),
                newVertexToIndex(mesh->vertices[face.indices[1]], std::make_pair(meshIndex + 1, face.indices[1])),
                newVertexToIndex(mesh->vertices[face.indices[2]], std::make_pair(meshIndex + 1, face.indices[2])),
            }};
            toTriangles.push_back(triangle);
        }
    };
    auto createSubSurfacesAndOthersStartTime = elapsedTimer.elapsed();
    std::vector<std::vector<size_t>> firstMergedEdgeLoops;
    {
        std::vector<std::vector<size_t>> edgeLoops;
        auto reTriangulationStartTime = elapsedTimer.elapsed();
        doReTriangulation(&m_firstMesh, newEdgesPerTriangleInFirstMesh, m_firstTriangles, edgeLoops, 0);
        qDebug() << "    Do retriangulation took" << (elapsedTimer.elapsed() - reTriangulationStartTime) << "milliseconds";
        auto addUnIntersectedStartTime = elapsedTimer.elapsed();
        addUnIntersectedFaces(&m_firstMesh, reTriangulatedFacesInFirstMesh, m_firstTriangles, 0);
        m_debugFirstMesh.vertices = m_newVertices;
        m_debugFirstMesh.faces = m_firstTriangles;
        qDebug() << "    Add unintsersected took" << (elapsedTimer.elapsed() - addUnIntersectedStartTime) << "milliseconds";
        EdgeLoop::merge(edgeLoops, &firstMergedEdgeLoops);
        exportEdgeLoopsAsObj(m_newVertices, edgeLoops, "/Users/jeremy/Desktop/debug-edgeloop-first.obj");
        auto createSubSurfacesStartTime = elapsedTimer.elapsed();
        m_firstVisitedTriangles.resize(m_firstTriangles.size(), false);
        SubSurface::createSubSurfaces(firstMergedEdgeLoops, m_firstTriangles, m_firstVisitedTriangles, m_firstSubSurfaces);
        qDebug() << "    Create subsurfaces took" << (elapsedTimer.elapsed() - createSubSurfacesStartTime) << "milliseconds";
    }
    {
        std::vector<std::vector<size_t>> edgeLoops;
        auto reTriangulationStartTime = elapsedTimer.elapsed();
        doReTriangulation(&m_secondMesh, newEdgesPerTriangleInSecondMesh, m_secondTriangles, edgeLoops, 1);
        qDebug() << "    Do retriangulation took" << (elapsedTimer.elapsed() - reTriangulationStartTime) << "milliseconds";
        auto addUnIntersectedStartTime = elapsedTimer.elapsed();
        addUnIntersectedFaces(&m_secondMesh, reTriangulatedFacesInSecondMesh, m_secondTriangles, 1);
        m_debugSecondMesh.vertices = m_newVertices;
        m_debugSecondMesh.faces = m_secondTriangles;
        qDebug() << "    Add unintsersected took" << (elapsedTimer.elapsed() - addUnIntersectedStartTime) << "milliseconds";
        std::vector<std::vector<size_t>> secondMergedEdgeLoops;
        EdgeLoop::merge(edgeLoops, &secondMergedEdgeLoops);
        exportEdgeLoopsAsObj(m_newVertices, secondMergedEdgeLoops, "/Users/jeremy/Desktop/debug-edgeloop-second.obj");
        auto createSubSurfacesStartTime = elapsedTimer.elapsed();
        //EdgeLoop::unifyDirection(firstMergedEdgeLoops, &secondMergedEdgeLoops);
        //auto testFirstLoops = firstMergedEdgeLoops;
        //for (auto &it: testFirstLoops)
        //    std::sort(it.begin(), it.end());
        //auto testSecondLoops = secondMergedEdgeLoops;
        //for (auto &it: testSecondLoops)
        //    std::sort(it.begin(), it.end());
        //std::sort(testFirstLoops.begin(), testFirstLoops.end());
        //std::sort(testSecondLoops.begin(), testSecondLoops.end());
        //qDebug() << "[0]]:" << testFirstLoops;
        //qDebug() << "[1]]:" << testSecondLoops;
        m_secondVisitedTriangles.resize(m_secondTriangles.size(), false);
        SubSurface::createSubSurfaces(firstMergedEdgeLoops, m_secondTriangles, m_secondVisitedTriangles, m_secondSubSurfaces);
        qDebug() << "    Create subsurfaces took" << (elapsedTimer.elapsed() - createSubSurfacesStartTime) << "milliseconds";
    }
    qDebug() << "Create subsurfaces and others took" << (elapsedTimer.elapsed() - createSubSurfacesAndOthersStartTime) << "milliseconds";
    
    auto createSubBlocksStartTime = elapsedTimer.elapsed();
    SubBlock::createSubBlocks(m_firstSubSurfaces, m_secondSubSurfaces, m_subBlocks);
    qDebug() << "Create subblocks took" << (elapsedTimer.elapsed() - createSubBlocksStartTime) << "milliseconds";
    
    m_debugSubBlocks.resize(m_subBlocks.size());
    for (size_t i = 0; i < m_subBlocks.size(); ++i) {
        m_debugSubBlocks[i].vertices = m_newVertices;
        for (const auto &it: m_subBlocks[i].faces) {
            if (-1 == it.second)
                m_debugSubBlocks[i].faces.push_back(Face {{it.first[2], it.first[1], it.first[0]}});
            else
                m_debugSubBlocks[i].faces.push_back(Face {{it.first[0], it.first[1], it.first[2]}});
        }
    }
    
    auto distinguishStartTime = elapsedTimer.elapsed();
    if (!Distinguish::distinguish(m_subBlocks, m_newVertices, &m_indicesToSubBlocks))
        return false;
    qDebug() << "Distinguish took" << (elapsedTimer.elapsed() - distinguishStartTime) << "milliseconds";
    
    return true;
}

void MeshOperator::getResult(Type booleanType, Mesh *result)
{
    size_t index = (size_t)booleanType;
    if (index >= m_indicesToSubBlocks.size())
        return;
    
    int subBlockIndex = m_indicesToSubBlocks[index];
    if (-1 == subBlockIndex)
        return;
    
    const auto &subBlock = m_subBlocks[subBlockIndex];
    result->vertices = m_newVertices;
    result->faces.reserve(subBlock.faces.size());
    for (const auto &it: subBlock.faces) {
        if (-1 == it.second)
            result->faces.push_back(Face {{it.first[2], it.first[1], it.first[0]}});
        else
            result->faces.push_back(Face {{it.first[0], it.first[1], it.first[2]}});
    }
    
    auto addNoneSubSurfaceFaces = [&](const std::vector<Face> &triangles,
            const std::vector<bool> &visitedTriangles) {
        for (size_t i = 0; i < triangles.size(); ++i) {
            if (visitedTriangles[i])
                continue;
            result->faces.push_back(triangles[i]);
        }
    };
    
    if (Type::Union == booleanType) {
        addNoneSubSurfaceFaces(m_firstTriangles, m_firstVisitedTriangles);
        addNoneSubSurfaceFaces(m_secondTriangles, m_secondVisitedTriangles);
    } else if (Type::Subtraction == booleanType) {
        addNoneSubSurfaceFaces(m_firstTriangles, m_firstVisitedTriangles);
    } else if (Type::InversedSubtraction == booleanType) {
        addNoneSubSurfaceFaces(m_secondTriangles, m_secondVisitedTriangles);
    }
}

}
