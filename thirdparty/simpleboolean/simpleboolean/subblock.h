#ifndef SIMPLEBOOLEAN_SUB_BLOCK_H
#define SIMPLEBOOLEAN_SUB_BLOCK_H
#include <simpleboolean/subsurface.h>
#include <array>
#include <map>
#include <QString>
#include <vector>

namespace simpleboolean
{

class SubBlock
{
public:
    std::map<std::array<size_t, 3>, int> faces;
    std::map<size_t, std::map<int, bool>> cycles;

    static bool createSubBlocks(const std::vector<SubSurface> &firstSubSurfaces,
        const std::vector<SubSurface> &secondSubSurfaces,
        std::vector<SubBlock> &subBlocks);
};

}

#endif
