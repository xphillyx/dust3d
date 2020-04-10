#ifndef SIMPLEBOOLEAN_EDGE_LOOP_H
#define SIMPLEBOOLEAN_EDGE_LOOP_H
#include <vector>

namespace simpleboolean
{

class EdgeLoop
{
public:
    static void merge(const std::vector<std::vector<size_t>> &sourceEdgeLoops,
        std::vector<std::vector<size_t>> *targetEdgeLoops);
    static void buildEdgeLoopsFromDirectedEdges(const std::vector<std::pair<size_t, size_t>> &edges,
        std::vector<std::vector<size_t>> *edgeLoops,
        bool allowOpenEdgeLoop=false,
        bool allowOppositeEdgeLoop=true);
    static void unifyDirection(const std::vector<std::vector<size_t>> &reference,
        std::vector<std::vector<size_t>> *target);
};

}

#endif
