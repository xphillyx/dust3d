#include <config.hpp>
#include <field-math.hpp>
#include <optimizer.hpp>
#include <parametrizer.hpp>
#ifdef WITH_CUDA
#include <cuda_runtime.h>
#endif
#include <QDebug>
#include <Fast-Quadric-Mesh-Simplification/Simplify.h>
#include "meshsimplify.h"

using namespace qflow;

#if 0
bool meshSimplifyFromQuadsUsingFQMS(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &quads,
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
#endif

bool meshSimplifyFromQuads(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &quads,
    std::vector<QVector3D> *simplifiedVertices, std::vector<std::vector<size_t>> *simplifiedFaces)
{
    Parametrizer field;
    
#ifdef WITH_CUDA
    cudaFree(0);
#endif
    
    field.V.resize(3, vertices.size());
    field.F.resize(3, quads.size() * 2);
    for (decltype(vertices.size()) i = 0; i < vertices.size(); i++) {
        const auto &vertex = vertices[i];
        field.V.col(i) << (double)vertex.x(), (double)vertex.y(), (double)vertex.z();
    }
    for (decltype(quads.size()) i = 0; i < quads.size(); i++) {
        const auto &face = quads[i];
        field.F.col(i) << (uint32_t)face[0], (uint32_t)face[1], (uint32_t)face[2];
        field.F.col(i) << (uint32_t)face[2], (uint32_t)face[3], (uint32_t)face[0];
    }
    field.NormalizeMesh();
    
    int faces = -1;
    field.Initialize(faces);
    
    if (field.flag_preserve_boundary) {
        Hierarchy& hierarchy = field.hierarchy;
        hierarchy.clearConstraints();
        for (uint32_t i = 0; i < 3 * hierarchy.mF.cols(); ++i) {
            if (hierarchy.mE2E[i] == -1) {
                uint32_t i0 = hierarchy.mF(i % 3, i / 3);
                uint32_t i1 = hierarchy.mF((i + 1) % 3, i / 3);
                Vector3d p0 = hierarchy.mV[0].col(i0), p1 = hierarchy.mV[0].col(i1);
                Vector3d edge = p1 - p0;
                if (edge.squaredNorm() > 0) {
                    edge.normalize();
                    hierarchy.mCO[0].col(i0) = p0;
                    hierarchy.mCO[0].col(i1) = p1;
                    hierarchy.mCQ[0].col(i0) = hierarchy.mCQ[0].col(i1) = edge;
                    hierarchy.mCQw[0][i0] = hierarchy.mCQw[0][i1] = hierarchy.mCOw[0][i0] = hierarchy.mCOw[0][i1] =
                        1.0;
                }
            }
        }
        hierarchy.propagateConstraints();
    }
    
    Optimizer::optimize_orientations(field.hierarchy);
    field.ComputeOrientationSingularities();
    
    if (field.flag_adaptive_scale == 1) {
        field.EstimateSlope();
    }
    
    Optimizer::optimize_scale(field.hierarchy, field.rho, field.flag_adaptive_scale);
    field.flag_adaptive_scale = 1;
    
    Optimizer::optimize_positions(field.hierarchy, field.flag_adaptive_scale);
    field.ComputePositionSingularities();
    
    field.ComputeIndexMap();
    
    simplifiedVertices->resize(field.O_compact.size());
    for (size_t i = 0; i < field.O_compact.size(); ++i) {
        auto t = field.O_compact[i] * field.normalize_scale + field.normalize_offset;
        (*simplifiedVertices)[i] = QVector3D(t[0], t[1], t[2]);
    }
    simplifiedFaces->resize(field.F_compact.size());
    for (size_t i = 0; i < field.F_compact.size(); ++i) {
        (*simplifiedFaces)[i] = std::vector<size_t> {
            (size_t)field.F_compact[i][0],
            (size_t)field.F_compact[i][1],
            (size_t)field.F_compact[i][2],
            (size_t)field.F_compact[i][3]
        };
    }
    
    return true;
}


