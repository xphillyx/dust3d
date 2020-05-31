#include "voxelgrid.h"
#include <QGuiApplication>
#include "mousepicker.h"

MousePicker::MousePicker(MeshVoxelContext *context) :
    m_context(context)
{
}

MousePicker::~MousePicker()
{
	delete m_context;
}

void MousePicker::setIsEndOfStroke(bool isEndOfStroke)
{
	m_isEndOfStroke = isEndOfStroke;
}

bool MousePicker::takeIsEndOfStroke()
{
	return m_isEndOfStroke;
}

MeshVoxelContext *MousePicker::takeContext()
{
    MeshVoxelContext *context = m_context;
    m_context = nullptr;
    return context;
}

bool MousePicker::takePickedPosition(QVector3D *position, QVector3D *normal)
{
    if (!m_picked)
        return false;
    if (nullptr != position)
        *position = m_pickedPosition;
	if (nullptr != normal)
		*normal = m_pickedNormal;
    return true;
}

void MousePicker::setMouseRay(const QVector3D &mouseRayNear, 
    const QVector3D &mouseRayFar)
{
    m_mouseRayNear = mouseRayNear;
    m_mouseRayFar = mouseRayFar;
}

void MousePicker::setMouseRadius(float radius)
{
	m_mouseRadius = radius;
}

float MousePicker::takeMouseRadius()
{
	return m_mouseRadius;
}

void MousePicker::pick()
{
	m_context->voxelize();
	if (qFuzzyCompare(m_mouseRayNear, m_mouseRayFar))
		return;
    m_picked = m_context->voxelGrid()->intersects(m_mouseRayNear, m_mouseRayFar,
		&m_pickedPosition, &m_pickedNormal);
}

void MousePicker::process()
{
    pick();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void MousePicker::setPaintMode(PaintMode paintMode)
{
	m_paintMode = paintMode;
}

PaintMode MousePicker::takePaintMode()
{
	return m_paintMode;
}
