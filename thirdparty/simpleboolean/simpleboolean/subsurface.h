#ifndef SIMPLEBOOLEAN_SUB_SURFACE_H
#define SIMPLEBOOLEAN_SUB_SURFACE_H
#include <vector>
#include <simpleboolean/meshdatatype.h>
#include <QString>
#include <map>
#include <set>

namespace simpleboolean
{

class SubSurface
{
public:
    size_t edgeLoopIndex;
    std::vector<Face> faces;
    bool isFrontSide = true;
    bool isSharedByOthers = false;
    std::set<std::pair<size_t, bool>> owners;
    
    static QString createEdgeLoopName(const std::vector<size_t> &edgeLoop);
    static void createSubSurfaces(std::vector<std::vector<size_t>> &edgeLoops,
        const std::vector<Face> &triangles,
        std::vector<bool> &visitedTriangles,
        std::vector<SubSurface> &subSurfaces);
};

}

#endif
