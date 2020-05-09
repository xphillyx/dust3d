#ifndef VOXEL_PAINTER_H
#define VOXEL_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include "outcome.h"
#include "paintmode.h"
#include "model.h"

class VoxelGrid;

class VoxelPainterContext
{
public:
	~VoxelPainterContext();
	VoxelGrid *sourceVoxelGrid = nullptr;
	VoxelGrid *positiveVoxelGrid = nullptr;
    VoxelGrid *negativeVoxelGrid = nullptr;
};

class VoxelPainter : public QObject
{
    Q_OBJECT
public:
    VoxelPainter(Outcome *outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setRadius(float radius);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    void setContext(VoxelPainterContext *context);
    ~VoxelPainter();
    Outcome *takeOutcome();
    const QVector3D &targetPosition();
    VoxelPainterContext *takeContext();
    VoxelGrid *takeResultVoxelGrid();
signals:
    void finished();
public slots:
    void process();
    void paint();
private:
    float m_radius = 0.0;
    float m_brushWeight = 0.001;
    PaintMode m_paintMode = PaintMode::None;
    std::set<QUuid> m_mousePickMaskNodeIds;
    Outcome *m_outcome = nullptr;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    bool calculateMouseModelPosition(QVector3D &mouseModelPosition);
    void paintToVoxelGrid();
public:
    VoxelPainterContext *m_context = nullptr;
    VoxelGrid *m_resultVoxelGrid = nullptr;
};

#endif
