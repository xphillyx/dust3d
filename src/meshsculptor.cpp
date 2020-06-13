#include "voxelgrid.h"
#include <QDebug>
#include <QElapsedTimer>
#include "meshsculptor.h"
#include "strokemeshbuilder.h"
#include "strokemodifier.h"
#include "cutface.h"
#include "util.h"
#include "model.h"
#include "shadervertex.h"
#include "theme.h"

MeshSculptor::MeshSculptor(MeshVoxelContext *context,
		const MeshSculptorStroke &stroke) :
    m_context(context),
    m_stroke(stroke)
{
}

void MeshSculptor::makeStrokeGrid()
{
	StrokeModifier *strokeModifier = new StrokeModifier;
	std::vector<QVector2D> cutTemplate = CutFaceToPoints(CutFace::Quad);
	std::vector<size_t> nodeIndices;
    /*
	if (!m_stroke.points.empty()) {
		float endRadius = VoxelGrid::m_defaultVoxelSize * 0.5f;
		float beginRadius = std::max(m_stroke.points[0].radius, endRadius);
		for (size_t i = 0; i < m_stroke.points.size(); ++i) {
			const auto &it = m_stroke.points[i];
			float rate = (float)(i + 1) / m_stroke.points.size();
			float radius = (1.0 - rate) * beginRadius + rate * endRadius;
			nodeIndices.push_back(strokeModifier->addNode(it.position, radius, cutTemplate, 0.0f));
		}
	}
    */
	for (const auto &it: m_stroke.points) {
        auto nodeIndex = strokeModifier->addNode(it.position, std::max(it.radius, VoxelGrid::m_defaultVoxelSize * 0.5f), cutTemplate, 0.0f);
        strokeModifier->setNodeBaseNormal(nodeIndex, it.normal);
		nodeIndices.push_back(nodeIndex);
	}
	for (size_t i = 1; i < nodeIndices.size(); ++i) {
		strokeModifier->addEdge(nodeIndices[i - 1], nodeIndices[i]);
	}
	//strokeModifier->subdivide();
	//strokeModifier->subdivide();
	strokeModifier->finalize();
	
	StrokeMeshBuilder *strokeMeshBuilder = new StrokeMeshBuilder;
	for (const auto &node: strokeModifier->nodes()) {
        auto nodeIndex = strokeMeshBuilder->addNode(node.position, node.radius, node.cutTemplate, node.cutRotation);
        strokeMeshBuilder->setNodeOriginInfo(nodeIndex, node.nearOriginNodeIndex, node.farOriginNodeIndex);
        strokeMeshBuilder->setNodeBaseNormal(nodeIndex, node.baseNormal);
    }
    for (const auto &edge: strokeModifier->edges())
        strokeMeshBuilder->addEdge(edge.firstNodeIndex, edge.secondNodeIndex);
	//strokeMeshBuilder->enableBaseNormalAverage(true);
    strokeMeshBuilder->setUseInputBaseNormals(true);
    strokeMeshBuilder->setDeformThickness(0.25);
	if (strokeMeshBuilder->build()) {
		delete m_strokeGrid;
		m_strokeGrid = new VoxelGrid;
		m_strokeGrid->fromMesh(strokeMeshBuilder->generatedVertices(),
			strokeMeshBuilder->generatedFaces());
	}
	delete strokeMeshBuilder;
	
	delete strokeModifier;
}

MeshVoxelContext *MeshSculptor::takeContext()
{
	MeshVoxelContext *context = m_context;
    m_context = nullptr;
    return context;
}

MeshVoxelContext *MeshSculptor::takeMousePickContext()
{
	MeshVoxelContext *context = m_mousePickContext;
    m_mousePickContext = nullptr;
    return context;
}

MeshSculptor::~MeshSculptor()
{
	delete m_context;
	delete m_mousePickContext;
	delete m_strokeGrid;
	delete m_finalGrid;
}

void MeshSculptor::sculpt()
{
    QElapsedTimer timer;
    timer.start();
    
    auto makeStrokeBeginTime = timer.elapsed();
	makeStrokeGrid();
    auto makeStrokeEndTime = timer.elapsed();
	
	m_context->voxelize();
	
	m_finalGrid = new VoxelGrid(*m_context->voxelGrid());
    
    auto booleanBeginTime = timer.elapsed();
    if (nullptr != m_strokeGrid) {
        if (PaintMode::Pull == m_stroke.paintMode) {
            m_finalGrid->unionWith(*m_strokeGrid);
        } else if (PaintMode::Push == m_stroke.paintMode) {
            m_finalGrid->diffWith(*m_strokeGrid);
        } else if (PaintMode::Smooth == m_stroke.paintMode) {
            openvdb::tools::sdfToFogVolume(*m_strokeGrid->m_grid);
            openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*m_finalGrid->m_grid);
            filter.gaussian(3, &(*m_strokeGrid->m_grid));
            
            // If half width of level set is 1, 
            // the direct masked gaussian will cause some artifacts to the voxels which are not included in the mask
            // The following section of code is a workaround
            
            //auto intersectedGrid = new VoxelGrid(*m_finalGrid);
            //{
            //	openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*intersectedGrid->m_grid);
            //	filter.gaussian();
            //}
            //m_finalGrid->diffWith(*m_strokeGrid);
            //{
            //	openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*m_strokeGrid->m_grid);
            //	filter.offset(-m_strokeGrid->m_voxelSize * 1.5);
            //}
            //intersectedGrid->intersectWith(*m_strokeGrid);
            //m_finalGrid->unionWith(*intersectedGrid);
            //delete intersectedGrid;
        }
    }
    auto booleanEndTime = timer.elapsed();
	
	if (m_stroke.isProvisional)
		m_mousePickContext = new MeshVoxelContext(m_context->voxelGrid(), m_context->meshId());
	else
		m_mousePickContext = new MeshVoxelContext(m_finalGrid, m_context->meshId());
	
	if (!m_stroke.isProvisional) {
		m_context->updateVoxelGrid(new VoxelGrid(*m_finalGrid));
	}
	
	delete m_strokeGrid;
	m_strokeGrid = nullptr;
    
    qDebug() << "Sculptor took" << timer.elapsed() << "milliseconds(makeStrokeGrid:" << (makeStrokeEndTime - makeStrokeBeginTime) << " boolean:" << (booleanEndTime - booleanBeginTime) << ")";
}

VoxelGrid *MeshSculptor::takeFinalVoxelGrid()
{
	VoxelGrid *finalVoxelGrid = m_finalGrid;
	m_finalGrid = nullptr;
	return finalVoxelGrid;
}

const MeshSculptorStroke &MeshSculptor::stroke()
{
    return m_stroke;
}

void MeshSculptor::process()
{
	sculpt();
    emit finished();
}
