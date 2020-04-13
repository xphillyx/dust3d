#ifndef DUST3D_FIX_MESH_H
#define DUST3D_FIX_MESH_H
#include <QVector3D>
#include <vector>

bool fixMesh(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &triangles);

#endif
