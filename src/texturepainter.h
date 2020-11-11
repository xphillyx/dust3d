#ifndef DUST3D_TEXTURE_PAINTER_H
#define DUST3D_TEXTURE_PAINTER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include <map>
#include <set>
#include <QColor>
#include <QImage>
#include <unordered_map>
#include <unordered_set>
#include "outcome.h"
#include "paintmode.h"
#include "model.h"

class TexturePainterContext
{
public:
    Outcome *outcome = nullptr;
    QImage *colorImage = nullptr;
    
    ~TexturePainterContext()
    {
        delete outcome;
        delete colorImage;
    }
};

class TexturePainter : public QObject
{
    Q_OBJECT
public:
    TexturePainter(const QVector3D &mouseRayNear, const QVector3D &mouseRayFar);
    void setContext(TexturePainterContext *context);
    void setRadius(float radius);
    void setBrushColor(const QColor &color);
    void setBrushMetalness(float value);
    void setBrushRoughness(float value);
    void setPaintMode(PaintMode paintMode);
    void setMaskNodeIds(const std::set<QUuid> &nodeIds);
    
    QImage *takeColorImage();
    
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
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_targetPosition;
    QColor m_brushColor;
    TexturePainterContext *m_context = nullptr;
    QImage *m_colorImage = nullptr;
    float m_brushMetalness = Model::m_defaultMetalness;
    float m_brushRoughness = Model::m_defaultRoughness;
};

#endif
