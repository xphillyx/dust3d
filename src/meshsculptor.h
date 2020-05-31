#ifndef DUST3D_MESH_SCULPTOR_H
#define DUST3D_MESH_SCULPTOR_H
#include <QObject>
#include <QVector3D>
#include "paintmode.h"
#include "meshvoxelcontext.h"

class VoxelGrid;
class Model;

struct MeshSculptorStrokePoint
{
	QVector3D position;
	QVector3D normal;
	float radius = 0.0f;
};

struct MeshSculptorStroke
{
	std::vector<MeshSculptorStrokePoint> points;
	PaintMode paintMode = PaintMode::Push;
	bool isProvisional = true;
};

class MeshSculptor : public QObject
{
    Q_OBJECT
public:
	MeshSculptor(MeshVoxelContext *context,
		const MeshSculptorStroke &stroke);
	~MeshSculptor();
	void sculpt();
	Model *takeModel();
	MeshVoxelContext *takeContext();
	MeshVoxelContext *takeMousePickContext();
signals:
	void finished();
public slots:
	void process();
private:
	void makeStrokeGrid();
	void makeModel();
	MeshVoxelContext *m_context = nullptr;
	MeshVoxelContext *m_mousePickContext = nullptr;
	MeshSculptorStroke m_stroke;
	VoxelGrid *m_strokeGrid = nullptr;
	Model *m_model = nullptr;
	VoxelGrid *m_finalGrid = nullptr;
};

#endif
