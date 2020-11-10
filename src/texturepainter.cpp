#include <QDebug>
#include <QRadialGradient>
#include <QBrush>
#include <QPainter>
#include <QGuiApplication>
#include <QPolygon>
#include "texturepainter.h"
#include "util.h"

TexturePainter::TexturePainter(const Outcome &m_outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(m_outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

TexturePainter::~TexturePainter()
{
    delete m_colorImage;
}

void TexturePainter::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void TexturePainter::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void TexturePainter::setRadius(float radius)
{
    m_radius = radius;
}

void TexturePainter::setBrushColor(const QColor &color)
{
    m_brushColor = color;
}

void TexturePainter::setColorImage(QImage *colorImage)
{
    delete m_colorImage;
    m_colorImage = colorImage;
}

QImage *TexturePainter::takeColorImage(void)
{
    QImage *colorImage = m_colorImage;
    m_colorImage = nullptr;
    return colorImage;
}

void TexturePainter::setBrushMetalness(float value)
{
    m_brushMetalness = value;
}

void TexturePainter::setBrushRoughness(float value)
{
    m_brushRoughness = value;
}

void TexturePainter::paint()
{
    size_t targetTriangleIndex = 0;
    if (!intersectRayAndPolyhedron(m_mouseRayNear,
            m_mouseRayFar,
            m_outcome.vertices,
            m_outcome.triangles,
            m_outcome.triangleNormals,
            &m_targetPosition,
            &targetTriangleIndex)) {
        return; 
    }
    
    if (PaintMode::None == m_paintMode)
        return;
    
    if (nullptr == m_colorImage) {
        qDebug() << "TexturePainter paint not happened, no input color image";
        return;
    }
    
    const std::vector<std::vector<QVector2D>> *uvs = m_outcome.triangleVertexUvs();
    if (nullptr == uvs) {
        qDebug() << "TexturePainter paint not happened, no input uv";
        return;
    }
    
    const auto &triangle = m_outcome.triangles[targetTriangleIndex];
    QVector3D coordinates = barycentricCoordinates(m_outcome.vertices[triangle[0]],
        m_outcome.vertices[triangle[1]],
        m_outcome.vertices[triangle[2]],
        m_targetPosition);
    
    auto &uvCoords = (*uvs)[targetTriangleIndex];
    QVector2D target2d = uvCoords[0] * coordinates[0] +
            uvCoords[1] * coordinates[1] +
            uvCoords[2] * coordinates[2];
            
    QPolygon polygon;
    polygon << QPoint(uvCoords[0].x() * m_colorImage->height(), uvCoords[0].y() * m_colorImage->height()) << 
        QPoint(uvCoords[1].x() * m_colorImage->height(), uvCoords[1].y() * m_colorImage->height()) <<
        QPoint(uvCoords[2].x() * m_colorImage->height(), uvCoords[2].y() * m_colorImage->height());
    QRegion clipRegion(polygon);
    
    QPainter painter(m_colorImage);
    QBrush brush(m_brushColor);
    painter.setClipRegion(clipRegion);
    
    double radius = m_radius * m_colorImage->height();
    QVector2D middlePoint = QVector2D(target2d.x() * m_colorImage->height(), 
        target2d.y() * m_colorImage->height());
    
    QRadialGradient gradient(QPointF(middlePoint.x(), middlePoint.y()), radius);
    gradient.setColorAt(0.0, m_brushColor);
    gradient.setColorAt(1.0, Qt::transparent);
                    
    painter.fillRect(middlePoint.x() - radius, 
        middlePoint.y() - radius, 
        radius + radius, 
        radius + radius,
        gradient);
}

void TexturePainter::process()
{
    paint();

    emit finished();
}

const QVector3D &TexturePainter::targetPosition()
{
    return m_targetPosition;
}
