#ifndef DUST3D_MESH_STROKETIFIER_H
#define DUST3D_MESH_STROKETIFIER_H
#include <QObject>
#include <QVector3D>
#include "outcome.h"

class MeshStroketifier : public QObject
{
    Q_OBJECT
public:
    struct StrokeNode
    {
        QVector3D position;
        float radius;
    };
    MeshStroketifier(Outcome *mesh);
    void stroketify();
private:
    Outcome *m_mesh = nullptr;
    std::vector<StrokeNode> m_strokeNodes;
    float calculateStrokeLengths(std::vector<float> *lengths);
    void calculateBoundingBox(float *minX, float *maxX,
        float *minY, float *maxY,
        float *minZ, float *maxZ);
    void translate(const QVector3D &offset);
    void scale(float amount);
};

#endif