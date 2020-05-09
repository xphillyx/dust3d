#include "voxelgrid.h"
#include <QDebug>
#include <QGuiApplication>
#include <QElapsedTimer>
#include "voxelpainter.h"
#include "util.h"

VoxelPainterContext::~VoxelPainterContext()
{
	delete sourceVoxelGrid;
	delete positiveVoxelGrid;
	delete negativeVoxelGrid;
}

VoxelPainter::VoxelPainter(Outcome *outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

Outcome *VoxelPainter::takeOutcome()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

VoxelGrid *VoxelPainter::takeResultVoxelGrid()
{
	VoxelGrid *voxelGrid = m_resultVoxelGrid;
	m_resultVoxelGrid = nullptr;
	return voxelGrid;
}

void VoxelPainter::setPaintMode(PaintMode paintMode)
{
    m_paintMode = paintMode;
}

void VoxelPainter::setMaskNodeIds(const std::set<QUuid> &nodeIds)
{
    m_mousePickMaskNodeIds = nodeIds;
}

void VoxelPainter::setContext(VoxelPainterContext *context)
{
	delete m_context;
	m_context = context;
}

void VoxelPainter::setRadius(float radius)
{
    m_radius = radius;
}

VoxelPainter::~VoxelPainter()
{
    delete m_outcome;
    delete m_context;
    delete m_resultVoxelGrid;
}

bool VoxelPainter::calculateMouseModelPosition(QVector3D &mouseModelPosition)
{
    return intersectRayAndPolyhedron(m_mouseRayNear,
        m_mouseRayFar,
        m_outcome->vertices,
        m_outcome->triangles,
        m_outcome->triangleNormals,
        &mouseModelPosition);
}

void VoxelPainter::paintToVoxelGrid()
{
	if (nullptr == m_context)
		return;

	QElapsedTimer timer;
	timer.start();

	if (nullptr == m_context->sourceVoxelGrid) {
		auto meshToVoxelsStartTime = timer.elapsed();
		m_context->sourceVoxelGrid = new VoxelGrid;
		m_context->sourceVoxelGrid->fromMesh(m_outcome->vertices, m_outcome->triangleAndQuads);
		auto meshToVoxelsConsumedTime = timer.elapsed() - meshToVoxelsStartTime;
		qDebug() << "VOXEL meshToVoxels took milliseconds:" << meshToVoxelsConsumedTime;
	}
	
	m_resultVoxelGrid = new VoxelGrid(*m_context->sourceVoxelGrid);
	
	if (nullptr == m_context->positiveVoxelGrid) {
		m_context->positiveVoxelGrid = new VoxelGrid;
	}
	if (nullptr == m_context->negativeVoxelGrid) {
		m_context->negativeVoxelGrid = new VoxelGrid;
	}
	
	auto constructSphereStartTime = timer.elapsed();
	
	VoxelGrid sphereMesh;
	sphereMesh.makeSphere(m_targetPosition, m_radius);
	if (PaintMode::Pull == m_paintMode) {
		m_context->positiveVoxelGrid->unionWith(sphereMesh);
	} else if (PaintMode::Push == m_paintMode) {
		m_context->negativeVoxelGrid->unionWith(sphereMesh);
		m_context->positiveVoxelGrid->diffWith(sphereMesh);
	}
	
	auto constructSphereConsumedTime = timer.elapsed() - constructSphereStartTime;
	qDebug() << "VOXEL constructSphere took milliseconds:" << constructSphereConsumedTime;
	
	auto applyToTargetStartTime = timer.elapsed();
	
	m_resultVoxelGrid->diffWith(*m_context->negativeVoxelGrid);
	m_resultVoxelGrid->unionWith(*m_context->positiveVoxelGrid);
	
	auto applyToTargetConsumedTime = timer.elapsed() - applyToTargetStartTime;
	qDebug() << "VOXEL applyToTarget took milliseconds:" << applyToTargetConsumedTime;
}

void VoxelPainter::paint()
{
    if (!calculateMouseModelPosition(m_targetPosition))
        return;
    
    if (PaintMode::None == m_paintMode)
        return;
    
    paintToVoxelGrid();
}

void VoxelPainter::process()
{
    paint();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

const QVector3D &VoxelPainter::targetPosition()
{
    return m_targetPosition;
}

VoxelPainterContext *VoxelPainter::takeContext()
{
	VoxelPainterContext *context = m_context;
	m_context = nullptr;
	return context;
}
