#ifndef SIMPLEBOOLEAN_DISTINGUISH_H
#define SIMPLEBOOLEAN_DISTINGUISH_H
#include <simpleboolean/subblock.h>

namespace simpleboolean
{

enum class Type
{
    Union = 0,
    Intersection = 1,
    Subtraction = 2,
    InversedSubtraction = 3
};

class Distinguish
{
public:
    static bool distinguish(std::vector<SubBlock> &subBlocks,
        const std::vector<Vertex> &vertices,
        std::vector<int> *indicesToSubBlocks);
};

}

#endif
