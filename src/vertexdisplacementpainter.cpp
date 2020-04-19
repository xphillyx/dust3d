#include <QDebug>
#include <QQuaternion>
#include <QRadialGradient>
#include <QBrush>
#include <QPainter>
#include <QGuiApplication>
#include "vertexdisplacementpainter.h"
#include "util.h"
#include "imageforever.h"

const int VertexDisplacementPainter::m_gridSize = 127;

VertexDisplacementPainter::VertexDisplacementPainter(Outcome *outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

Model *VertexDisplacementPainter::takePaintedModel()
{
    Model *paintedModel = m_model;
    m_model = nullptr;
    return paintedModel;
}

Outcome *VertexDisplacementPainter::takeOutcome()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

void VertexDisplacementPainter::setVoxelGrid(VoxelGrid<QVector3D> *voxelGrid)
{
    m_voxelGrid = voxelGrid;
}

void VertexDisplacementPainter::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void VertexDisplacementPainter::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void VertexDisplacementPainter::setRadius(float radius)
{
    m_radius = radius;
}

VertexDisplacementPainter::~VertexDisplacementPainter()
{
    delete m_model;
    delete m_outcome;
}

bool VertexDisplacementPainter::calculateMouseModelPosition(QVector3D &mouseModelPosition)
{
    return intersectRayAndPolyhedron(m_mouseRayNear,
        m_mouseRayFar,
        m_outcome->vertices,
        m_outcome->triangles,
        m_outcome->triangleNormals,
        &mouseModelPosition);
}

void VertexDisplacementPainter::paintToVoxelGrid()
{
    int voxelX = toVoxelLength(m_targetPosition.x());
    int voxelY = toVoxelLength(m_targetPosition.y());
    int voxelZ = toVoxelLength(m_targetPosition.z());
    int voxelRadius = toVoxelLength(m_radius);
    int range2 = voxelRadius * voxelRadius;
    
    auto deformNormal = (m_mouseRayNear - m_mouseRayFar).normalized();
    
    m_voxelGrid->add(voxelX, voxelY, voxelZ, deformNormal * m_brushWeight);
    qDebug() << "voxelRadius:" << voxelRadius
        << "voxelX:" << voxelX << "voxelY:" << voxelY << "voxelZ:" << voxelZ;
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
                    m_voxelGrid->add(x, y, z, deformNormal * alpha * m_brushWeight);
                }
            }
        }
    }
}

void VertexDisplacementPainter::createPaintedModel()
{
    const QVector3D defaultNormal = QVector3D(0, 0, 0);
    const QVector2D defaultUv = QVector2D(0, 0);
    const QVector3D defaultTangent = QVector3D(0, 0, 0);
    const auto triangleVertexNormals = m_outcome->triangleVertexNormals();
    const auto triangleVertexUvs = m_outcome->triangleVertexUvs();
    const auto triangleTangents = m_outcome->triangleTangents();
    
    static std::vector<QVector3D> *s_originalVertices = nullptr;
    
    std::unordered_map<size_t, std::pair<QVector3D, size_t>> vertexOffsetMap;
    for (size_t i = 0; i < m_outcome->triangles.size(); ++i) {
        const auto &triangleColor = &m_outcome->triangleColors[i];
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = m_outcome->triangles[i][j];
            const QVector3D *srcVert = &m_outcome->vertices[vertexIndex];
            const QVector3D *srcNormal = &defaultNormal;
            if (triangleVertexNormals)
                srcNormal = &(*triangleVertexNormals)[i][j];
            int voxelX = toVoxelLength(srcVert->x());
            int voxelY = toVoxelLength(srcVert->y());
            int voxelZ = toVoxelLength(srcVert->z());
            QVector3D offsetVector = m_voxelGrid->query(voxelX, voxelY, voxelZ);
            if (offsetVector.isNull())
                continue;
            auto &offset = vertexOffsetMap[vertexIndex];
            offset.first += (*srcNormal + offsetVector.normalized()).normalized() * offsetVector.length();
            offset.second++;
        }
    }
    
    for (size_t i = 0; i < m_outcome->vertices.size(); ++i) {
        auto &offset = vertexOffsetMap[i];
        if (0 == offset.second)
            continue;
        QVector3D averagedOffset = offset.first / offset.second;
        m_outcome->vertices[i] += averagedOffset;
    }
    
    {
        std::vector<std::vector<QVector3D>> triangleVertexNormals;
        std::vector<QVector3D> smoothNormals;
        angleSmooth(m_outcome->vertices,
            m_outcome->triangles,
            m_outcome->triangleNormals,
            60,
            smoothNormals);
        triangleVertexNormals.resize(m_outcome->triangles.size(), {
            QVector3D(), QVector3D(), QVector3D()
        });
        size_t index = 0;
        for (size_t i = 0; i < m_outcome->triangles.size(); ++i) {
            auto &normals = triangleVertexNormals[i];
            for (size_t j = 0; j < 3; ++j) {
                if (index < smoothNormals.size())
                    normals[j] = smoothNormals[index];
                ++index;
            }
        }
        m_outcome->setTriangleVertexNormals(triangleVertexNormals);
    }
    
    int triangleVertexCount = m_outcome->triangles.size() * 3;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    int destIndex = 0;
    for (size_t i = 0; i < m_outcome->triangles.size(); ++i) {
        const auto &triangleColor = &m_outcome->triangleColors[i];
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = m_outcome->triangles[i][j];
            const QVector3D *srcVert = &m_outcome->vertices[vertexIndex];
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
            dest->colorR = triangleColor->redF();
            dest->colorG = triangleColor->greenF();
            dest->colorB = triangleColor->blueF();
            dest->alpha = triangleColor->alphaF();
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
    
    size_t edgeCount = 0;
    for (const auto &face: m_outcome->triangleAndQuads) {
        edgeCount += face.size();
    }
    int edgeVertexCount = edgeCount * 2;
    ShaderVertex *edgeVertices = new ShaderVertex[edgeVertexCount];
    size_t edgeVertexIndex = 0;
    for (size_t faceIndex = 0; faceIndex < m_outcome->triangleAndQuads.size(); ++faceIndex) {
        const auto &face = m_outcome->triangleAndQuads[faceIndex];
        for (size_t i = 0; i < face.size(); ++i) {
            for (size_t x = 0; x < 2; ++x) {
                size_t sourceIndex = face[(i + x) % face.size()];
                const QVector3D *srcVert = &m_outcome->vertices[sourceIndex];
                ShaderVertex *dest = &edgeVertices[edgeVertexIndex];
                memset(dest, 0, sizeof(ShaderVertex));
                dest->colorR = 0.0;
                dest->colorG = 0.0;
                dest->colorB = 0.0;
                dest->alpha = 1.0;
                dest->posX = srcVert->x();
                dest->posY = srcVert->y();
                dest->posZ = srcVert->z();
                dest->metalness = Model::m_defaultMetalness;
                dest->roughness = Model::m_defaultRoughness;
                edgeVertexIndex++;
            }
        }
    }
    
    m_model = new Model(triangleVertices, triangleVertexCount, edgeVertices, edgeVertexCount);
}

int VertexDisplacementPainter::toVoxelLength(float length)
{
    int voxelLength = length * 100;
    if (voxelLength > m_gridSize)
        voxelLength = m_gridSize;
    else if (voxelLength < -m_gridSize)
        voxelLength = -m_gridSize;
    return voxelLength;
}

void VertexDisplacementPainter::paint()
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

void VertexDisplacementPainter::process()
{
    paint();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

const QVector3D &VertexDisplacementPainter::targetPosition()
{
    return m_targetPosition;
}
