#ifndef DUST3D_CYCLE_FINDER
#define DUST3D_CYCLE_FINDER
#include <vector>
#include <set>
#include <QVector3D>

class CycleFinder
{
public:
    CycleFinder(const std::vector<QVector3D> &nodePositions,
        const std::vector<std::pair<size_t, size_t>> &edges);
    void find();
    const std::vector<std::vector<size_t>> &getCycles();
private:
    size_t m_nodeNum = 0;
    std::vector<QVector3D> m_nodePositions;
    std::vector<std::pair<size_t, size_t>> m_edges;
    std::vector<int> m_edgeLengths;
    std::map<std::pair<size_t, size_t>, int> m_edgeLengthMap;
    std::vector<std::vector<size_t>> m_cycles;
    std::set<std::pair<size_t, size_t>> m_cycleEdges;
    std::set<std::pair<size_t, size_t>> m_halfEdges;
    void removeEdgeFrom(const std::pair<size_t, size_t> &edge,
        std::vector<std::pair<size_t, size_t>> *edges,
        std::vector<int> *edgeLengths);
    void addPathToCycleEdges(const std::vector<size_t> &path,
        std::set<std::pair<size_t, size_t>> *cycleEdges,
        std::set<std::pair<size_t, size_t>> *halfEdges);
    bool validateCycle(const std::set<std::pair<size_t, size_t>> &newPathEdges,
        const std::set<std::pair<size_t, size_t>> &oldPathEdges);
    void prepareWeights();
};

#endif

