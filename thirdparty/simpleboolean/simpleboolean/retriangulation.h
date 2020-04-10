#ifndef SIMPLEBOOLEAN_RETRIANGULATION_H
#define SIMPLEBOOLEAN_RETRIANGULATION_H
#include <simpleboolean/meshdatatype.h>
#include <vector>
#include <map>
#include <set>

namespace simpleboolean
{
  
class ReTriangulation
{
public:
    ReTriangulation(const std::vector<Vertex> &vertices,
        const std::vector<size_t> &triangle,
        const std::vector<std::vector<size_t>> &edgeLoops);
    void reTriangulate();
    const std::vector<Face> &getResult();
    static void buildEdgeLoopsFromDirectedEdges(const std::vector<std::pair<size_t, size_t>> &edges,
        std::vector<std::vector<size_t>> *edgeLoops);
    
private:
    std::vector<Vertex> m_vertices;
    std::vector<size_t> m_triangle;
    std::vector<std::vector<size_t>> m_edgeLoops;
    std::vector<Face> m_reTriangulatedTriangles;
    std::vector<std::vector<size_t>> m_closedEdgeLoops;
    std::vector<std::vector<size_t>> m_recalculatedEdgeLoops;
    std::map<size_t, std::vector<size_t>> m_innerEdgeLoopsMap;
    std::map<size_t, Vertex> m_vertices2D;
    std::vector<std::vector<Vertex>> m_closedEdgeLoopsVertices2D;
    std::vector<std::vector<Vertex>> m_recalculatedEdgeLoopsVertices2D;
    size_t m_errors = 0;
    
    void recalculateEdgeLoops();
    void convertVerticesTo2D();
    void convertEdgeLoopsToVertices2D();
    bool attachClosedEdgeLoopsToOutter();
    void addFacesToHalfEdges(const std::vector<Face> &faces,
        std::set<std::pair<size_t, size_t>> *halfEdges);
    void unifyFaceDirections(const std::set<std::pair<size_t, size_t>> &halfEdges,
        std::vector<Face> *newFaces);
    //void fillHoles(std::set<std::pair<size_t, size_t>> *halfEdges);
};
  
}

#endif
