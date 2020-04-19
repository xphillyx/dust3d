#ifndef DUST3D_SUBDIVISION_H
#define DUST3D_SUBDIVISION_H
#include <QVector3D>
#include <vector>

void subdivision(const std::vector<QVector3D> &vertices, 
    const std::vector<std::vector<size_t>> &faces,
    std::vector<QVector3D> *outputVertices,
    std::vector<std::vector<size_t>> *outputFaces);

#endif
