#include "voxelgrid.h"
#include <QGuiApplication>
#include "meshsculptor.h"
#include "strokemeshbuilder.h"
#include "strokemodifier.h"
#include "cutface.h"
#include "util.h"
#include "model.h"
#include "shadervertex.h"
#include "theme.h"

class MeshSculptorContext
{
public:
	~MeshSculptorContext()
	{
		delete voxelGrid;
	}
    std::vector<QVector3D> meshVertices;
    std::vector<std::vector<size_t>> meshFaces;
    quint64 meshId = 0;
    VoxelGrid *voxelGrid = nullptr;
};

MeshSculptor::MeshSculptor(MeshSculptorContext *context,
		const std::vector<QVector3D> &meshVertices,
        const std::vector<std::vector<size_t>> &meshFaces,
        quint64 meshId,
		const MeshSculptorStroke &stroke) :
    m_context(context),
    m_stroke(stroke)
{
	if (nullptr == m_context) {
        m_context = new MeshSculptorContext;
    }
    if (m_context->meshId != meshId) {
        delete m_context->voxelGrid;
        m_context->voxelGrid = nullptr;

        m_context->meshVertices = meshVertices;
        m_context->meshFaces = meshFaces;
        m_context->meshId = meshId;
    }
}

void MeshSculptor::makeStrokeGrid()
{
	StrokeModifier *strokeModifier = new StrokeModifier;
	std::vector<QVector2D> cutTemplate = CutFaceToPoints(CutFace::Quad);
	std::vector<size_t> nodeIndices;
	for (const auto &it: m_stroke.points) {
		nodeIndices.push_back(strokeModifier->addNode(it.position, m_stroke.radius, cutTemplate, 0.0f));
	}
	for (size_t i = 1; i < nodeIndices.size(); ++i) {
		strokeModifier->addEdge(nodeIndices[i - 1], nodeIndices[i]);
	}
	strokeModifier->subdivide();
	strokeModifier->finalize();
	
	StrokeMeshBuilder *strokeMeshBuilder = new StrokeMeshBuilder;
	for (const auto &node: strokeModifier->nodes()) {
        auto nodeIndex = strokeMeshBuilder->addNode(node.position, node.radius, node.cutTemplate, node.cutRotation);
        strokeMeshBuilder->setNodeOriginInfo(nodeIndex, node.nearOriginNodeIndex, node.farOriginNodeIndex);
    }
    for (const auto &edge: strokeModifier->edges())
        strokeMeshBuilder->addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
	strokeMeshBuilder->enableBaseNormalAverage(true);
	if (strokeMeshBuilder->build()) {
		delete m_strokeGrid;
		m_strokeGrid = new VoxelGrid;
		m_strokeGrid->fromMesh(strokeMeshBuilder->generatedVertices(),
			strokeMeshBuilder->generatedFaces());
	}
	delete strokeMeshBuilder;
	
	delete strokeModifier;
}

void MeshSculptor::releaseContext(MeshSculptorContext *context)
{
	delete context;
}

MeshSculptorContext *MeshSculptor::takeContext()
{
	MeshSculptorContext *context = m_context;
    m_context = nullptr;
    return context;
}

MeshSculptor::~MeshSculptor()
{
	delete m_context;
	delete m_strokeGrid;
	delete m_model;
	delete m_finalGrid;
}

Model *MeshSculptor::takeModel()
{
    Model *model = m_model;
    m_model = nullptr;
    return model;
}

void MeshSculptor::sculpt()
{
	makeStrokeGrid();
	
	if (nullptr == m_context->voxelGrid) {
		m_context->voxelGrid = new VoxelGrid;
		m_context->voxelGrid->fromMesh(m_context->meshVertices, m_context->meshFaces);
	}
	
	m_finalGrid = new VoxelGrid(*m_context->voxelGrid);
	
	m_finalGrid->unionWith(*m_strokeGrid);
	
	makeModel();
	
	if (!m_stroke.isProvisional) {
		delete m_context->voxelGrid;
		m_context->voxelGrid = m_finalGrid;
		m_finalGrid = nullptr;
	}
	
	delete m_strokeGrid;
	m_strokeGrid = nullptr;
	
	delete m_finalGrid;
	m_finalGrid = nullptr;
}

void MeshSculptor::makeModel()
{
	std::vector<QVector3D> voxelVertices;
	std::vector<std::vector<size_t>> voxelTriangles;
	m_finalGrid->toMesh(&voxelVertices, &voxelTriangles);
	
	std::vector<QVector3D> voxelTriangleNormals(voxelTriangles.size());
	for (size_t i = 0; i < voxelTriangles.size(); ++i)
		voxelTriangleNormals[i] = polygonNormal(voxelVertices, voxelTriangles[i]);
    
    int triangleVertexCount = voxelTriangles.size() * 3;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    int destIndex = 0;
    auto clayR = Theme::clayColor.redF();
    auto clayG = Theme::clayColor.greenF();
    auto clayB = Theme::clayColor.blueF();
    for (size_t i = 0; i < voxelTriangles.size(); ++i) {
        const auto &srcNormal = &voxelTriangleNormals[i];
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = voxelTriangles[i][j];
            const QVector3D *srcVert = &voxelVertices[vertexIndex];
            ShaderVertex *dest = &triangleVertices[destIndex];
            dest->colorR = clayR;
            dest->colorG = clayG;
            dest->colorB = clayB;
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
            dest->roughness = Theme::clayRoughness;
            dest->tangentX = 0;
            dest->tangentY = 0;
            dest->tangentZ = 0;
            destIndex++;
        }
    }

    m_model = new Model(triangleVertices, triangleVertexCount, 0, 0);
}

void MeshSculptor::process()
{
	sculpt();
	
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
