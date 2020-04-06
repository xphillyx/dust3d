#include "meshstroketifier.h"

MeshStroketifier::MeshStroketifier(Outcome *mesh) :
    m_mesh(mesh)
{
}

void MeshStroketifier::MeshStroketifier::stroketify()
{
    float boundingBoxMinX = 0.0;
    float boundingBoxMaxX = 0.0;
    float boundingBoxMinY = 0.0;
    float boundingBoxMaxY = 0.0;
    float boundingBoxMinZ = 0.0;
    float boundingBoxMaxZ = 0.0;
    calculateBoundingBox(&boundingBoxMinX, &boundingBoxMaxX,
        &boundingBoxMinY, &boundingBoxMaxY,
        &boundingBoxMinZ, &boundingBoxMaxZ);
    float xLength = boundingBoxMaxX - boundingBoxMinX;
    float yLength = boundingBoxMaxY - boundingBoxMinY;
    float zLength = boundingBoxMaxZ - boundingBoxMinZ;
    QVector3D origin;
    QVector3D direction;
    float length = 0;
    if (yLength >= xLength && yLength >= zLength) {
        // Y-axis
        origin = QVector3D((boundingBoxMinX + boundingBoxMaxX) * 0.5, 
            boundingBoxMinY, 
            (boundingBoxMinZ + boundingBoxMaxZ) * 0.5);
        direction = QVector3D(0.0, 1.0, 0.0);
        length = yLength;
    } else if (zLength >= xLength && zLength >= yLength) {
        // Z-axis
        origin = QVector3D((boundingBoxMinX + boundingBoxMaxX) * 0.5, 
            (boundingBoxMinY + boundingBoxMaxY) * 0.5, 
            boundingBoxMinZ);
        direction = QVector3D(0.0, 0.0, 1.0);
        length = zLength;
    } else {
        // X-axis
        origin = QVector3D(boundingBoxMinX, 
            (boundingBoxMinY + boundingBoxMaxY) * 0.5, 
            (boundingBoxMinZ + boundingBoxMaxZ) * 0.5);
        direction = QVector3D(1.0, 0.0, 0.0);
        length = xLength;
    }
    
    std::vector<float> strokeSegmentLengths;
    float strokeLength = calculateStrokeLengths(&strokeSegmentLengths) + std::numeric_limits<float>::epsilon();
    
    translate(-origin);
    if (strokeLength > 0)
        scale(strokeLength / (length + std::numeric_limits<float>::epsilon()));
    
    if (!strokeSegmentLengths.empty()) {
        // TODO: rotate to Y (facing front:Z+)
        // ... ...
        
        std::vector<QVector3D> jointPositions;
        float strokeOffset = 0;
        for (size_t i = 0; i < strokeSegmentLengths.size(); ++i) {
            jointPositions.push_back(QVector3D(0.0, strokeOffset, 0.0));
            strokeOffset += strokeSegmentLengths[i];
        }
    }
    // TODO:
}

void MeshStroketifier::translate(const QVector3D &offset)
{
    // TODO:
}

void MeshStroketifier::scale(float amount)
{
    // TODO:
}

float MeshStroketifier::calculateStrokeLengths(std::vector<float> *lengths)
{
    float total = 0.0;
    for (size_t i = 1; i < m_strokeNodes.size(); ++i) {
        size_t h = i - 1;
        const auto &strokeNodeH = m_strokeNodes[h];
        const auto &strokeNodeI = m_strokeNodes[i];
        float distance = (strokeNodeH.position - strokeNodeI.position).length();
        total += distance;
        if (nullptr != lengths)
            lengths->push_back(distance);
    }
    return total;
}

void MeshStroketifier::calculateBoundingBox(float *minX, float *maxX,
        float *minY, float *maxY,
        float *minZ, float *maxZ)
{
    // TODO:
}