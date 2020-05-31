#ifndef DUST3D_MESH_SCULPTOR_H
#define DUST3D_MESH_SCULPTOR_H
#include <QObject>
#include <QVector3D>

class MeshSculptorContext;
class VoxelGrid;
class Model;

struct MeshSculptorStrokePoint
{
	QVector3D position;
	QVector3D normal;
};

struct MeshSculptorStroke
{
	std::vector<MeshSculptorStrokePoint> points;
	float radius = 0.0f;
	bool isProvisional = true;
};

class MeshSculptor : public QObject
{
    Q_OBJECT
public:
	MeshSculptor(MeshSculptorContext *context,
		const std::vector<QVector3D> &meshVertices,
        const std::vector<std::vector<size_t>> &meshFaces,
        quint64 meshId,
		const MeshSculptorStroke &stroke);
	~MeshSculptor();
	void sculpt();
	Model *takeModel();
	MeshSculptorContext *takeContext();
	static void releaseContext(MeshSculptorContext *context);
signals:
	void finished();
public slots:
	void process();
private:
	void makeStrokeGrid();
	void makeModel();
	MeshSculptorContext *m_context = nullptr;
	MeshSculptorStroke m_stroke;
	VoxelGrid *m_strokeGrid = nullptr;
	Model *m_model = nullptr;
	VoxelGrid *m_finalGrid = nullptr;
};

#endif
