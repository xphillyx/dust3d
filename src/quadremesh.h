#ifndef DUST3D_QUAD_REMESH_H
#define DUST3D_QUAD_REMESH_H
#include <vector>
#include <QVector3D>

bool quadRemesh(const std::vector<QVector3D> &inputVertices,
    std::vector<std::vector<size_t>> &inputTriangles,
    std::vector<std::vector<size_t>> &inputQuads,
    std::vector<QVector3D> *outputVertices,
    std::vector<std::vector<size_t>> *outputQuads);

#endif
