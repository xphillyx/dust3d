#include <QDebug>
#include <QQuaternion>
#include <QRadialGradient>
#include <QBrush>
#include <QPainter>
#include <QGuiApplication>
#include "partdeformmappainter.h"
#include "util.h"
#include "imageforever.h"

PartDeformMapPainter::PartDeformMapPainter(const Outcome &outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

const std::set<QUuid> &PartDeformMapPainter::changedPartIds()
{
    return m_changedPartIds;
}

void PartDeformMapPainter::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void PartDeformMapPainter::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void PartDeformMapPainter::setRadius(float radius)
{
    m_radius = radius;
}

PartDeformMapPainter::~PartDeformMapPainter()
{
}

bool PartDeformMapPainter::calculateMouseModelPosition(QVector3D &mouseModelPosition)
{
    return intersectRayAndPolyhedron(m_mouseRayNear,
        m_mouseRayFar,
        m_outcome.vertices,
        m_outcome.triangles,
        m_outcome.triangleNormals,
        &mouseModelPosition);
}

void PartDeformMapPainter::paint()
{
    if (!calculateMouseModelPosition(m_targetPosition))
        return;
    
    if (PaintMode::None == m_paintMode)
        return;
    
    float distance2 = m_radius * m_radius;
    
    for (const auto &map: m_outcome.paintMaps) {
        for (const auto &node: map.paintNodes) {
            if (!m_mousePickMaskNodeIds.empty() && m_mousePickMaskNodeIds.find(node.originNodeId) == m_mousePickMaskNodeIds.end())
                continue;
            size_t intersectedNum = 0;
            QVector3D sumOfDirection;
            QVector3D referenceDirection = (m_targetPosition - node.origin).normalized();
            float sumOfRadius = 0;
            for (const auto &vertexPosition: node.vertices) {
                // >0.866 = <30 degrees
                auto direction = (vertexPosition - node.origin).normalized();
                if (QVector3D::dotProduct(referenceDirection, direction) > 0.866 &&
                        (vertexPosition - m_targetPosition).lengthSquared() <= distance2) {
                    float distance = vertexPosition.distanceToPoint(m_targetPosition);
                    float radius = (m_radius - distance) / node.radius;
                    sumOfRadius += radius;
                    sumOfDirection += direction * radius;
                    ++intersectedNum;
                }
            }
            if (intersectedNum > 0) {
                float paintRadius = sumOfRadius / intersectedNum;
                QVector3D paintDirection = sumOfDirection.normalized();
                float degrees = angleInRangle360BetweenTwoVectors(node.baseNormal, paintDirection, node.direction);
                float offset = (float)node.order / map.paintNodes.size();
                m_changedPartIds.insert(map.partId);
                paintToImage(map.partId, offset, degrees / 360.0, paintRadius, PaintMode::Push == m_paintMode);
            }
        }
    }
}

void PartDeformMapPainter::process()
{
    paint();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void PartDeformMapPainter::paintToImage(const QUuid &partId, float x, float y, float radius, bool inverted)
{
    QUuid oldImageId;
    QImage image(72, 36, QImage::Format_Grayscale8);
    image.fill(QColor(127, 127, 127));
    const auto &findImageId = m_paintImages.find(partId);
    if (findImageId != m_paintImages.end()) {
        const QImage *oldImage = ImageForever::get(findImageId->second);
        if (nullptr != oldImage) {
            if (oldImage->size() == image.size() &&
                    oldImage->format() == image.format()) {
                image = *oldImage;
            }
        }
    }
    float destX = image.width() * x;
    float destY = image.height() * y;
    float destRadius = image.height() * radius;
    {
        QRadialGradient gradient(destX, destY, destRadius / 2);
        if (inverted) {
            gradient.setColorAt(0, QColor(0, 0, 0, 3));
            gradient.setColorAt(1, Qt::transparent);
        } else {
            gradient.setColorAt(0, QColor(255, 255, 255, 3));
            gradient.setColorAt(1, Qt::transparent);
        }
        QBrush brush(gradient);
        QPainter paint(&image);
        paint.setRenderHint(QPainter::HighQualityAntialiasing);
        paint.setBrush(brush);
        paint.setPen(Qt::NoPen);
        paint.drawEllipse(destX - destRadius / 2, destY - destRadius / 2, destRadius, destRadius);
    }
    QUuid imageId = ImageForever::add(&image);
    m_paintImages[partId] = imageId;
}

const QVector3D &PartDeformMapPainter::targetPosition()
{
    return m_targetPosition;
}

void PartDeformMapPainter::setPaintImages(const std::map<QUuid, QUuid> &paintImages)
{
    m_paintImages = paintImages;
}

const std::map<QUuid, QUuid> &PartDeformMapPainter::resultPaintImages()
{
    return m_paintImages;
}
