#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H
#include <QVector3D>
#include <vector>
#include "meshresultcontext.h"
#include "meshloader.h"
#include "jointnodetree.h"

struct SkinnedMeshWeight
{
    int jointIndex;
    float amount;
};

struct SkinnedMeshVertex
{
    QVector3D position;
    QVector3D normal;
    QVector3D posPosition;
    QVector3D posNormal;
    SkinnedMeshWeight weights[MAX_WEIGHT_NUM];
};

struct SkinnedMeshTriangle
{
    int indicies[3];
    QColor color;
};

class SkinnedMesh
{
public:
    SkinnedMesh(const MeshResultContext &resultContext, const JointNodeTree &jointNodeTree);
    ~SkinnedMesh();
    void applyRigFrameToMesh(const RigFrame &frame);
    MeshLoader *toMeshLoader();
private:
    void fromMeshResultContext(MeshResultContext &resultContext);
    void frameToMatrices(const RigFrame &frame, std::vector<QMatrix4x4> &matrices);
    void frameToMatricesAtJoint(const RigFrame &frame, std::vector<QMatrix4x4> &matrices, int jointIndex, const QMatrix4x4 &parentWorldMatrix);
private:
    MeshResultContext m_resultContext;
    JointNodeTree *m_jointNodeTree;
private:
    Q_DISABLE_COPY(SkinnedMesh);
    std::vector<SkinnedMeshVertex> m_vertices;
    std::vector<SkinnedMeshTriangle> m_triangles;
};

#endif
