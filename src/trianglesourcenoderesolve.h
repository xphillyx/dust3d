#ifndef DUST3D_TRIANGLE_SOURCE_NODE_RESOLVE_H
#define DUST3D_TRIANGLE_SOURCE_NODE_RESOLVE_H
#include "object.h"

void triangleSourceNodeResolve(const Object &object, std::vector<std::pair<QUuid, QUuid>> &triangleSourceNodes,
    std::vector<std::pair<QUuid, QUuid>> *vertexSourceNodes=nullptr);

#endif
