#ifndef DUST3D_PART_DEFORM_MAP_PAINTER_H
#define DUST3D_PART_DEFORM_MAP_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include "outcome.h"
#include "paintmode.h"

class PartDeformMapPainter : public QObject
{
    Q_OBJECT
public:
    PartDeformMapPainter(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setRadius(float radius);
    void setPaintImages(const std::map<QUuid, QUuid> &paintImages);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    const std::map<QUuid, QUuid> &resultPaintImages();
    const std::set<QUuid> &changedPartIds();
    
    ~PartDeformMapPainter();
    const QVector3D &targetPosition();
signals:
    void finished();
public slots:
    void process();
    void paint();
private:
    float m_radius = 0.0;
    std::map<QUuid, QUuid> m_paintImages;
    PaintMode m_paintMode = PaintMode::None;
    std::set<QUuid> m_changedPartIds;
    std::set<QUuid> m_mousePickMaskNodeIds;
    Outcome m_outcome;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    bool calculateMouseModelPosition(QVector3D &mouseModelPosition);
    void paintToImage(const QUuid &partId, float x, float y, float radius, bool inverted=false);
};

#endif
