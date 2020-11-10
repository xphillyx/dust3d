#ifndef DUST3D_TEXTURE_PAINTER_H
#define DUST3D_TEXTURE_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include <QColor>
#include <QImage>
#include "outcome.h"
#include "paintmode.h"
#include "model.h"

class TexturePainter : public QObject
{
    Q_OBJECT
public:
    TexturePainter(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setColorImage(QImage *colorImage);
    void setRadius(float radius);
    void setBrushColor(const QColor &color);
    void setBrushMetalness(float value);
    void setBrushRoughness(float value);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    
    QImage *takeColorImage(void);
    
    ~TexturePainter();
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
    QColor m_brushColor;
    QImage *m_colorImage = nullptr;
    float m_brushMetalness = Model::m_defaultMetalness;
    float m_brushRoughness = Model::m_defaultRoughness;
};

#endif
