#ifndef MESH_RESULT_CONTEXT_H
#define MESH_RESULT_CONTEXT_H
#include <vector>
#include <set>
#include <QVector3D>
#include <QUuid>
#include <QColor>
#include "positionmap.h"

#define MAX_WEIGHT_NUM  4

struct BmeshNode
{
    int bmeshId = 0;
    int nodeId = 0;
    QVector3D origin;
    float radius = 0;
    QColor color;
};

struct BmeshVertex
{
    QVector3D position;
    int bmeshId = 0;
    int nodeId = 0;
};

struct BmeshEdge
{
    int fromBmeshId = 0;
    int fromNodeId = 0;
    int toBmeshId = 0;
    int toNodeId = 0;
};

struct ResultVertex
{
    QVector3D position;
};

struct ResultTriangle
{
    int indicies[3] = {0, 0, 0};
    QVector3D normal;
};

struct ResultTriangleUv
{
    float uv[3][2] = {{0, 0}, {0, 0}, {0, 0}};
    bool resolved = false;
};

struct ResultVertexUv
{
    float uv[2] = {0, 0};
};

struct ResultPart
{
    QColor color;
    std::vector<ResultVertex> vertices;
    std::vector<QVector3D> interpolatedVertexNormals;
    std::vector<ResultTriangle> triangles;
    std::vector<ResultTriangleUv> uvs;
    std::vector<ResultVertexUv> vertexUvs;
};

struct ResultRearrangedVertex
{
    QVector3D position;
    int originalIndex;
};

struct ResultRearrangedTriangle
{
    int indicies[3];
    QVector3D normal;
    int originalIndex;
};

class MeshResultContext
{
public:
    std::vector<BmeshNode> bmeshNodes;
    std::vector<BmeshVertex> bmeshVertices;
    std::vector<ResultVertex> vertices;
    std::vector<ResultTriangle> triangles;
    MeshResultContext();
public:
    const std::vector<std::pair<int, int>> &triangleSourceNodes();
    const std::vector<QColor> &triangleColors();
    const std::map<std::pair<int, int>, std::pair<int, int>> &triangleEdgeSourceMap();
    const std::map<std::pair<int, int>, BmeshNode *> &bmeshNodeMap();
    const std::map<int, ResultPart> &parts();
    const std::vector<ResultTriangleUv> &triangleUvs();
    const std::vector<ResultRearrangedVertex> &rearrangedVertices();
    const std::vector<ResultRearrangedTriangle> &rearrangedTriangles();
    const std::map<int, std::pair<int, int>> &vertexSourceMap();
private:
    bool m_triangleSourceResolved;
    bool m_triangleColorResolved;
    bool m_triangleEdgeSourceMapResolved;
    bool m_bmeshNodeMapResolved;
    bool m_resultPartsResolved;
    bool m_resultTriangleUvsResolved;
    bool m_resultRearrangedVerticesResolved;
private:
    std::vector<std::pair<int, int>> m_triangleSourceNodes;
    std::vector<QColor> m_triangleColors;
    std::map<std::pair<int, int>, std::pair<int, int>> m_triangleEdgeSourceMap;
    std::map<std::pair<int, int>, BmeshNode *> m_bmeshNodeMap;
    std::map<int, ResultPart> m_resultParts;
    std::vector<ResultTriangleUv> m_resultTriangleUvs;
    std::set<int> m_seamVertices;
    std::vector<ResultRearrangedVertex> m_rearrangedVertices;
    std::vector<ResultRearrangedTriangle> m_rearrangedTriangles;
    std::map<int, std::pair<int, int>> m_vertexSourceMap;
private:
    void calculateTriangleSourceNodes(std::vector<std::pair<int, int>> &triangleSourceNodes, std::map<int, std::pair<int, int>> &vertexSourceMap);
    void calculateRemainingVertexSourceNodesAfterTriangleSourceNodesSolved(std::map<int, std::pair<int, int>> &vertexSourceMap);
    void calculateTriangleColors(std::vector<QColor> &triangleColors);
    void calculateTriangleEdgeSourceMap(std::map<std::pair<int, int>, std::pair<int, int>> &triangleEdgeSourceMap);
    void calculateBmeshNodeMap(std::map<std::pair<int, int>, BmeshNode *> &bmeshNodeMap);
    void calculateResultParts(std::map<int, ResultPart> &parts);
    void calculateResultTriangleUvs(std::vector<ResultTriangleUv> &uvs, std::set<int> &seamVertices);
    void calculateResultRearrangedVertices(std::vector<ResultRearrangedVertex> &rearrangedVertices, std::vector<ResultRearrangedTriangle> &rearrangedTriangles);
};

#endif
