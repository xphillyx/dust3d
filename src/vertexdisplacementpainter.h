#ifndef DUST3D_VERTEX_DISPLACEMENT_PAINTER_H
#define DUST3D_VERTEX_DISPLACEMENT_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include "outcome.h"
#include "paintmode.h"
#include "voxelgrid.h"
#include "model.h"

class VertexDisplacementPainter : public QObject
{
    Q_OBJECT
public:
    VertexDisplacementPainter(Outcome *outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setRadius(float radius);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    void setVoxelGrid(VoxelGrid<int> *voxelGrid);
    
    ~VertexDisplacementPainter();
    Model *takePaintedModel();
    Outcome *takeOutcome();
    const QVector3D &targetPosition();
signals:
    void finished();
public slots:
    void process();
    void paint();
private:
    float m_radius = 0.0;
    int m_brushWeight = 10;
    PaintMode m_paintMode = PaintMode::None;
    std::set<QUuid> m_mousePickMaskNodeIds;
    Outcome *m_outcome = nullptr;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    VoxelGrid<int> *m_voxelGrid = nullptr;
    Model *m_model = nullptr;
    bool calculateMouseModelPosition(QVector3D &mouseModelPosition);
    void paintToVoxelGrid();
    int toVoxelLength(float length);
    void createPaintedModel();
public:
    static const int m_gridSize;
};

#endif
