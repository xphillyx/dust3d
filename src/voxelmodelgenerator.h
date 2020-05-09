#ifndef DUST3D_VOXEL_MODEL_GENERATOR_H
#define DUST3D_VOXEL_MODEL_GENERATOR_H
#include <QObject>

class VoxelGrid;
class Model;

class VoxelModelGenerator : public QObject
{
	Q_OBJECT
public:
	VoxelModelGenerator(VoxelGrid *voxelGrid);
	~VoxelModelGenerator();
	Model *takeModel();
signals:
	void finished();
public slots:
	void process();
	void generate();
private:
	VoxelGrid *m_voxelGrid = nullptr;
	Model *m_model = nullptr;
};

#endif
