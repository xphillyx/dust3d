#include <QDebug>
#include <QQuaternion>
#include <QRadialGradient>
#include <QBrush>
#include <QPainter>
#include <QGuiApplication>
#include "vertexcolorpainter.h"
#include "util.h"
#include "imageforever.h"

QColor operator+(const QColor &first, const QColor &second)
{
    float total = first.alphaF() + second.alphaF();
    if (qFuzzyIsNull(total))
        return QColor(0, 0, 0, 0);
    float rate = first.alphaF() / total;
    float remaining = 1.0 - rate;
    return QColor(first.red() * rate + second.red() * remaining,
        first.green() * rate + second.green() * remaining,
        first.blue() * rate + second.blue() * remaining);
}

QColor operator-(const QColor &first, const QColor &second)
{
    QColor color = first;
    color.setAlphaF(std::max(color.alphaF() - second.alphaF(), 0.0));
    return color;
}

VertexColorPainter::VertexColorPainter(const Outcome &m_outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(m_outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

Model *VertexColorPainter::takePaintedModel()
{
    Model *paintedModel = m_model;
    m_model = nullptr;
    return paintedModel;
}

void VertexColorPainter::setVoxelGrid(VoxelGrid<QColor> *voxelGrid)
{
    m_voxelGrid = voxelGrid;
}

void VertexColorPainter::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void VertexColorPainter::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void VertexColorPainter::setRadius(float radius)
{
    m_radius = radius;
}

void VertexColorPainter::setBrushColor(const QColor &color)
{
    m_brushColor = color;
}

VertexColorPainter::~VertexColorPainter()
{
    delete m_model;
}

bool VertexColorPainter::calculateMouseModelPosition(QVector3D &mouseModelPosition)
{
    return intersectRayAndPolyhedron(m_mouseRayNear,
        m_mouseRayFar,
        m_outcome.vertices,
        m_outcome.triangles,
        m_outcome.triangleNormals,
        &mouseModelPosition);
}

void VertexColorPainter::paintToVoxelGrid()
{
    int voxelX = toVoxelLength(m_targetPosition.x());
    int voxelY = toVoxelLength(m_targetPosition.y());
    int voxelZ = toVoxelLength(m_targetPosition.z());
    int voxelRadius = toVoxelLength(m_radius);
    int range2 = voxelRadius * voxelRadius;
    m_voxelGrid->add(voxelX, voxelY, voxelZ, m_brushColor);
    for (int i = -voxelRadius; i <= voxelRadius; ++i) {
        qint8 x = voxelX + i;
        int i2 = i * i;
        for (int j = -voxelRadius; j <= voxelRadius; ++j) {
            qint8 y = voxelY + j;
            int j2 = j * j;
            for (int k = -voxelRadius; k <= voxelRadius; ++k) {
                qint8 z = voxelZ + k;
                int k2 = k * k;
                int dist2 = i2 + j2 + k2;
                if (dist2 <= range2) {
                    int dist = std::sqrt(dist2);
                    float alpha = 1.0 - (float)dist / voxelRadius;
                    QColor color = m_brushColor;
                    color.setAlphaF(alpha);
                    m_voxelGrid->add(x, y, z, color);
                }
            }
        }
    }
}

void VertexColorPainter::createPaintedModel()
{
    std::vector<QColor> vertexColors(m_outcome.vertices.size());
    for (size_t i = 0; i < m_outcome.vertices.size(); ++i) {
        const auto &position = m_outcome.vertices[i];
        int voxelX = toVoxelLength(position.x());
        int voxelY = toVoxelLength(position.y());
        int voxelZ = toVoxelLength(position.z());
        vertexColors[i] = m_voxelGrid->query(voxelX, voxelY, voxelZ);
    }
    
    int triangleVertexCount = m_outcome.triangles.size() * 3;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    int destIndex = 0;
    const auto triangleVertexNormals = m_outcome.triangleVertexNormals();
    const auto triangleVertexUvs = m_outcome.triangleVertexUvs();
    const auto triangleTangents = m_outcome.triangleTangents();
    const QVector3D defaultNormal = QVector3D(0, 0, 0);
    const QVector2D defaultUv = QVector2D(0, 0);
    const QVector3D defaultTangent = QVector3D(0, 0, 0);
    for (size_t i = 0; i < m_outcome.triangles.size(); ++i) {
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = m_outcome.triangles[i][j];
            const auto &vertexColor = &vertexColors[vertexIndex];
            const QVector3D *srcVert = &m_outcome.vertices[vertexIndex];
            const QVector3D *srcNormal = &defaultNormal;
            if (triangleVertexNormals)
                srcNormal = &(*triangleVertexNormals)[i][j];
            const QVector2D *srcUv = &defaultUv;
            if (triangleVertexUvs)
                srcUv = &(*triangleVertexUvs)[i][j];
            const QVector3D *srcTangent = &defaultTangent;
            if (triangleTangents)
                srcTangent = &(*triangleTangents)[i];
            ShaderVertex *dest = &triangleVertices[destIndex];
            dest->colorR = vertexColor->redF();
            dest->colorG = vertexColor->greenF();
            dest->colorB = vertexColor->blueF();
            dest->alpha = vertexColor->alphaF();
            dest->posX = srcVert->x();
            dest->posY = srcVert->y();
            dest->posZ = srcVert->z();
            dest->texU = srcUv->x();
            dest->texV = srcUv->y();
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            dest->metalness = Model::m_defaultMetalness;
            dest->roughness = Model::m_defaultRoughness;
            dest->tangentX = srcTangent->x();
            dest->tangentY = srcTangent->y();
            dest->tangentZ = srcTangent->z();
            destIndex++;
        }
    }
    m_model = new Model(triangleVertices, triangleVertexCount);
}

int VertexColorPainter::toVoxelLength(float length)
{
    int voxelLength = length * 100;
    if (voxelLength > 127)
        voxelLength = 127;
    else if (voxelLength < -127)
        voxelLength = -127;
    return voxelLength;
}

void VertexColorPainter::paint()
{
    if (!calculateMouseModelPosition(m_targetPosition))
        return;
    
    if (PaintMode::None == m_paintMode)
        return;
    
    if (nullptr == m_voxelGrid)
        return;
    
    paintToVoxelGrid();
    createPaintedModel();
}

void VertexColorPainter::process()
{
    paint();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

const QVector3D &VertexColorPainter::targetPosition()
{
    return m_targetPosition;
}
