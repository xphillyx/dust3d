#ifndef DUST3D_VERTEX_COLOR_PAINTER_H
#define DUST3D_VERTEX_COLOR_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include <QColor>
#include "outcome.h"
#include "paintmode.h"
#include "voxelgrid.h"
#include "model.h"

QColor operator+(const QColor &first, const QColor &second);
QColor operator-(const QColor &first, const QColor &second);

class VertexColorPainter : public QObject
{
    Q_OBJECT
public:
    VertexColorPainter(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setRadius(float radius);
    void setBrushColor(const QColor &color);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    void setVoxelGrid(VoxelGrid<QColor> *voxelGrid);
    
    ~VertexColorPainter();
    Model *takePaintedModel();
    const QVector3D &targetPosition();
signals:
    void finished();
public slots:
    void process();
    void paint();
private:
    float m_radius = 0.0;
    PaintMode m_paintMode = PaintMode::None;
    std::set<QUuid> m_mousePickMaskNodeIds;
    Outcome m_outcome;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    QColor m_brushColor = Qt::green;
    VoxelGrid<QColor> *m_voxelGrid = nullptr;
    Model *m_model = nullptr;
    bool calculateMouseModelPosition(QVector3D &mouseModelPosition);
    void paintToVoxelGrid();
    int toVoxelLength(float length);
    void createPaintedModel();
public:
    static const int m_gridSize;
};

#endif
