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
    VoxelGrid *takeVoxelGrid();
    void setTargetTriangleCount(size_t triangleCount);
    void markAsProvisional();
    int takeTargetLevel();
    bool isProvisional();
signals:
	void finished();
public slots:
	void process();
	void generate();
private:
	VoxelGrid *m_voxelGrid = nullptr;
	Model *m_model = nullptr;
    int m_targetLevel = 0;
    bool m_isProvisional = false;
    size_t m_targetTriangleCount = 0;
};

#endif
