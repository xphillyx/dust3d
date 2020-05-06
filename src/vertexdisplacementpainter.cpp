#include "voxelmesh.h"
#include <QDebug>
#include <QGuiApplication>
#include "vertexdisplacementpainter.h"
#include "util.h"

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
    // TODO:
}

void VertexDisplacementPainter::createPaintedModel()
{
	VoxelMesh voxelMesh;
	voxelMesh.fromMesh(m_outcome->vertices, m_outcome->triangleAndQuads);
	
	std::vector<QVector3D> voxelVertices;
	std::vector<std::vector<size_t>> voxelTriangles;
	voxelMesh.toMesh(&voxelVertices, &voxelTriangles);
	std::vector<QVector3D> voxelTriangleNormals(voxelTriangles.size());
	for (size_t i = 0; i < voxelTriangles.size(); ++i)
		voxelTriangleNormals[i] = polygonNormal(voxelVertices, voxelTriangles[i]);
    
    int triangleVertexCount = voxelTriangles.size() * 3;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    int destIndex = 0;
    for (size_t i = 0; i < voxelTriangles.size(); ++i) {
        const auto &srcNormal = &voxelTriangleNormals[i];
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = voxelTriangles[i][j];
            const QVector3D *srcVert = &voxelVertices[vertexIndex];
            ShaderVertex *dest = &triangleVertices[destIndex];
            dest->colorR = 1.0;
            dest->colorG = 1.0;
            dest->colorB = 1.0;
            dest->alpha = 1.0;
            dest->posX = srcVert->x();
            dest->posY = srcVert->y();
            dest->posZ = srcVert->z();
            dest->texU = 0;
            dest->texV = 0;
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            dest->metalness = Model::m_defaultMetalness;
            dest->roughness = Model::m_defaultRoughness;
            dest->tangentX = 0;
            dest->tangentY = 0;
            dest->tangentZ = 0;
            destIndex++;
        }
    }

    m_model = new Model(triangleVertices, triangleVertexCount, 0, 0);
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
