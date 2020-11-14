#include <QDebug>
#include "texturetransfer.h"
#include "util.h"

void TextureTransfer::transferTriangle(size_t destinationTriangleIndex)
{
    const std::vector<QVector2D> &uv = (*m_destinationUvs)[destinationTriangleIndex];
    int uvPixels[3][2] = {
        (int)(uv[0][0] * m_destinationImage->height()), (int)(uv[0][1] * m_destinationImage->height()),
        (int)(uv[1][0] * m_destinationImage->height()), (int)(uv[1][1] * m_destinationImage->height()),
        (int)(uv[2][0] * m_destinationImage->height()), (int)(uv[2][1] * m_destinationImage->height())
    };
    int minX = std::min(uvPixels[0][0], std::min(uvPixels[1][0], uvPixels[2][0]));
    int maxX = std::max(uvPixels[0][0], std::max(uvPixels[1][0], uvPixels[2][0]));
    int minY = std::min(uvPixels[0][1], std::min(uvPixels[1][1], uvPixels[2][1]));
    int maxY = std::max(uvPixels[0][1], std::max(uvPixels[1][1], uvPixels[2][1]));
    QVector3D a(uvPixels[0][0], uvPixels[0][1], 0);
    QVector3D b(uvPixels[1][0], uvPixels[1][1], 0);
    QVector3D c(uvPixels[2][0], uvPixels[2][1], 0);
    const auto &destinationTriangle = (*m_destinationTriangles)[destinationTriangleIndex];
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            QVector3D point(x, y, 0);
            QVector3D barycentric = barycentricCoordinates(a, b, c, point);
            if (barycentric[0] < 0 || barycentric[1] < 0 || barycentric[2] < 0)
                continue;
            QVector3D sharedPosition = (*m_destinationVertices)[destinationTriangle[0]] * barycentric[0] +
                (*m_destinationVertices)[destinationTriangle[1]] * barycentric[1] +
                (*m_destinationVertices)[destinationTriangle[2]] * barycentric[2];
            QVector2D uvInSource = findUvInSource(sharedPosition, (*m_sourceTriangleNormals)[destinationTriangleIndex]);
            m_destinationImage->setPixelColor(x, y,
                m_sourceImage->pixelColor(uvInSource[0] * m_sourceImage->height(),
                    uvInSource[1] * m_sourceImage->height()));
        }
    }
}

QVector2D TextureTransfer::findUvInSource(const QVector3D &position, const QVector3D &normal)
{
    size_t targetTriangleIndex = 0;
    QVector3D mouseRayNear = position + normal * 0.01;
    QVector3D mouseRayFar = position - normal * 0.01;
    QVector3D targetPosition;
    if (!intersectRayAndPolyhedron(mouseRayNear,
            mouseRayFar,
            *m_sourceVertices,
            *m_sourceTriangles,
            *m_sourceTriangleNormals,
            &targetPosition,
            &targetTriangleIndex)) {
        return QVector2D();
    }
    const auto &triangle = (*m_sourceTriangles)[targetTriangleIndex];
    QVector3D coordinates = barycentricCoordinates((*m_sourceVertices)[triangle[0]],
        (*m_sourceVertices)[triangle[1]],
        (*m_sourceVertices)[triangle[2]],
        targetPosition);
    const auto &uvCoords = (*m_sourceUvs)[targetTriangleIndex];
    return uvCoords[0] * coordinates[0] +
            uvCoords[1] * coordinates[1] +
            uvCoords[2] * coordinates[2];
}

void TextureTransfer::transfer()
{
    for (size_t destinationTriangleIndex = 0; 
            destinationTriangleIndex < m_destinationUvs->size(); 
            ++destinationTriangleIndex) {
        qDebug() << "transfer:" << destinationTriangleIndex << m_destinationUvs->size();
        transferTriangle(destinationTriangleIndex);        
    }
}

