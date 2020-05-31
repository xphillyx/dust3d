#include "voxelgrid.h"
#include <QGuiApplication>
#include "mousepicker.h"

class MousePickerContext
{
public:
	~MousePickerContext()
	{
		delete voxelGrid;
	}
    std::vector<QVector3D> meshVertices;
    std::vector<std::vector<size_t>> meshFaces;
    quint64 meshId = 0;
    VoxelGrid *voxelGrid = nullptr;
};

MousePicker::MousePicker(MousePickerContext *context,
        const std::vector<QVector3D> &meshVertices, 
        const std::vector<std::vector<size_t>> &meshFaces,
        quint64 meshId) :
    m_context(context)
{
    if (nullptr == m_context) {
        m_context = new MousePickerContext;
    }
    if (m_context->meshId != meshId) {
        delete m_context->voxelGrid;
        m_context->voxelGrid = nullptr;

        m_context->meshVertices = meshVertices;
        m_context->meshFaces = meshFaces;
        m_context->meshId = meshId;
    }
}

MousePicker::~MousePicker()
{
	delete m_context;
}

MousePickerContext *MousePicker::takeContext()
{
    MousePickerContext *context = m_context;
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

void MousePicker::pick()
{
	if (nullptr == m_context->voxelGrid) {
		m_context->voxelGrid = new VoxelGrid;
		m_context->voxelGrid->fromMesh(m_context->meshVertices, m_context->meshFaces);
	}
	if (qFuzzyCompare(m_mouseRayNear, m_mouseRayFar))
		return;
    m_picked = m_context->voxelGrid->intersects(m_mouseRayNear, m_mouseRayFar,
		&m_pickedPosition, &m_pickedNormal);
}

void MousePicker::process()
{
    pick();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void MousePicker::releaseContext(MousePickerContext *context)
{
	delete context;
}

void MousePicker::setPaintMode(PaintMode paintMode)
{
	m_paintMode = paintMode;
}

PaintMode MousePicker::takePaintMode()
{
	return m_paintMode;
}
