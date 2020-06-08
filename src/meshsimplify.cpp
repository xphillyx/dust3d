#include <QDebug>
#include <Fast-Quadric-Mesh-Simplification/Simplify.h>
#include "meshsimplify.h"

bool meshSimplifyFromQuads(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &quads,
    std::vector<QVector3D> *simplifiedVertices, std::vector<std::vector<size_t>> *simplifiedFaces)
{
    Simplify simplifier;
    
    simplifier.vertices.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        auto &dest = simplifier.vertices[i];
        const auto &src = vertices[i];
        dest.p.x = src.x();
        dest.p.y = src.y();
        dest.p.z = src.z();
    }
    
    simplifier.triangles.resize(quads.size() * 2);
    for (size_t i = 0, j = 0; i < quads.size(); ++i) {
        {
            auto &dest = simplifier.triangles[j++];
            const auto &src = quads[i];
            dest.v[0] = src[0];
            dest.v[1] = src[1];
            dest.v[2] = src[2];
        }
        {
            auto &dest = simplifier.triangles[j++];
            const auto &src = quads[i];
            dest.v[0] = src[2];
            dest.v[1] = src[3];
            dest.v[2] = src[0];
        }
    }
    
    if (simplifier.triangles.size() < 3 || simplifier.vertices.size() < 3) {
        qDebug() << "meshSimplifyFromQuads failed, simplifier.vertices:" << simplifier.vertices.size();
        return false;
    }
    
    float reduceFraction = 0.01;
    int targetCount = std::round((float)simplifier.triangles.size() * reduceFraction);
    int startSize = simplifier.triangles.size();
    float agressiveness = 7.0f;
    
    if (targetCount < 4) {
        qDebug() << "meshSimplifyFromQuads failed, targetCount:" << targetCount;
        return false;
    }
    
    qDebug() << "meshSimplifyFromQuads targetCount:" << targetCount << "startSize:" << startSize;
    
    simplifier.simplify_mesh(targetCount, agressiveness, true);
    if (simplifier.triangles.size() >= startSize) {
        qDebug() << "meshSimplifyFromQuads failed, simplifier.triangles:" << simplifier.vertices.size();
        return false;
    }
    
    simplifiedVertices->resize(simplifier.vertices.size());
    for (size_t i = 0; i < simplifier.vertices.size(); ++i) {
        auto &dest = (*simplifiedVertices)[i];
        const auto &src = simplifier.vertices[i];
        dest.setX(src.p.x);
        dest.setY(src.p.y);
        dest.setZ(src.p.z);
    }
    
    simplifiedFaces->resize(simplifier.triangles.size());
    for (size_t i = 0, j = 0; i < simplifier.triangles.size(); ++i) {
        auto &dest = (*simplifiedFaces)[j++];
        const auto &src = simplifier.triangles[i];
        dest.resize(3);
        dest[0] = src.v[0];
        dest[1] = src.v[1];
        dest[2] = src.v[2];
    }
    
    return true;
}


