#include "voxelgrid.h"
#include <QDebug>
#include <QElapsedTimer>
#include "voxelpainter.h"
#include "util.h"

VoxelPainterContext::~VoxelPainterContext()
{
	delete sourceVoxelGrid;
	delete positiveVoxelGrid;
	delete negativeVoxelGrid;
	delete lastResultVoxelGrid;
}

VoxelPainter::VoxelPainter(Outcome *outcome, const QVector3D &mouseRayNear, const QVector3D &mouseRayFar) :
    m_outcome(outcome),
    m_mouseRayNear(mouseRayNear),
    m_mouseRayFar(mouseRayFar)
{
}

void VoxelPainter::setStrokeId(quint64 strokeId)
{
	m_strokeId = strokeId;
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

void VoxelPainter::setLastPaintPosition(const QVector3D &lastPaintPosition)
{
	m_lastPaintPosition = lastPaintPosition;
}

VoxelPainter::~VoxelPainter()
{
    delete m_outcome;
    delete m_context;
    delete m_resultVoxelGrid;
}

bool VoxelPainter::calculateMouseModelPosition(QVector3D &mouseModelPosition)
{
	return m_context->sourceVoxelGrid->intersects(m_mouseRayNear, m_mouseRayFar,
		&mouseModelPosition);
}

void VoxelPainter::paintToVoxelGrid()
{
	if (nullptr == m_context || m_radius <= 0)
		return;

	QElapsedTimer timer;
	timer.start();
	
	m_resultVoxelGrid = new VoxelGrid(*m_context->sourceVoxelGrid);
	
	if (nullptr == m_context->positiveVoxelGrid) {
		m_context->positiveVoxelGrid = new VoxelGrid;
	}
	if (nullptr == m_context->negativeVoxelGrid) {
		m_context->negativeVoxelGrid = new VoxelGrid;
	}
	
	auto constructSphereStartTime = timer.elapsed();
	
	QVector3D beginPosition = m_targetPosition;
	QVector3D endPosition = m_lastPaintPosition.isNull() ? m_targetPosition : m_lastPaintPosition;
	QVector3D moveDirection = (endPosition - beginPosition).normalized();
	float moveDistance = (endPosition - beginPosition).length();
	float currentDistance = 0;
	
	VoxelGrid *strokeGrid = new VoxelGrid;
	do
	{
		auto center = beginPosition + moveDirection * currentDistance;
		VoxelGrid sphereUnit;
		sphereUnit.makeSphere(center, m_radius);
		strokeGrid->unionWith(sphereUnit);
		currentDistance += m_resultVoxelGrid->m_voxelSize * 0.25;
	} while (currentDistance <= moveDistance);
	if (PaintMode::Pull == m_paintMode) {
		strokeGrid->intersectWith(*m_context->sourceVoxelGrid);
		openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*strokeGrid->m_grid);
		filter.offset(-VoxelGrid::m_defaultVoxelSize * 0.25);
		filter.laplacian();
		m_context->positiveVoxelGrid->unionWith(*strokeGrid);
	} else if (PaintMode::Push == m_paintMode) {
		strokeGrid->intersectWith(*m_context->sourceVoxelGrid);
		m_context->negativeVoxelGrid->unionWith(*strokeGrid);
		m_context->positiveVoxelGrid->diffWith(*strokeGrid);
		openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*strokeGrid->m_grid);
		filter.offset(VoxelGrid::m_defaultVoxelSize * 0.25);
		filter.laplacian();
		m_context->positiveVoxelGrid->unionWith(*strokeGrid);
	}
	delete strokeGrid;
	
	auto constructSphereConsumedTime = timer.elapsed() - constructSphereStartTime;
	qDebug() << "VOXEL constructSphere took milliseconds:" << constructSphereConsumedTime;
	
	auto applyToTargetStartTime = timer.elapsed();
	
	m_resultVoxelGrid->diffWith(*m_context->negativeVoxelGrid);
	m_resultVoxelGrid->unionWith(*m_context->positiveVoxelGrid);
	
	auto applyToTargetConsumedTime = timer.elapsed() - applyToTargetStartTime;
	qDebug() << "VOXEL applyToTarget took milliseconds:" << applyToTargetConsumedTime;
	
	delete m_context->lastResultVoxelGrid;
	m_context->lastResultVoxelGrid = new VoxelGrid(*m_resultVoxelGrid);
	
	qDebug() << "VOXEL create grid took milliseconds:" << timer.elapsed();
	qDebug() << "111111111111111111111111111111111111111111111111111111111111";
}

void VoxelPainter::paint()
{
	if (nullptr == m_context)
		return;
		
	if (nullptr == m_context->sourceVoxelGrid ||
			m_context->meshId != m_outcome->meshId) {
		delete m_context->sourceVoxelGrid;
		m_context->sourceVoxelGrid = nullptr;

		m_context->sourceVoxelGrid = new VoxelGrid;
		m_context->sourceVoxelGrid->fromMesh(m_outcome->vertices, m_outcome->triangleAndQuads);
		m_context->meshId = m_outcome->meshId;
		m_context->strokeId = m_strokeId;
		
		//openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*m_context->sourceVoxelGrid->m_grid);
		//filter.laplacian();
	}
	
	if (m_context->strokeId != m_strokeId) {
		if (nullptr != m_context->lastResultVoxelGrid) {
			delete m_context->sourceVoxelGrid;
			m_context->sourceVoxelGrid = new VoxelGrid(*m_context->lastResultVoxelGrid);
			m_context->strokeId = m_strokeId;
		}
	}
	
    if (!calculateMouseModelPosition(m_targetPosition))
        return;
    
    if (PaintMode::None == m_paintMode)
        return;
    
    paintToVoxelGrid();
}

void VoxelPainter::process()
{
    paint();
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
