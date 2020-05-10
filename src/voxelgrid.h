#ifndef DUST3D_VOXEL_GRID_H
#define DUST3D_VOXEL_GRID_H
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tools/LevelSetFilter.h>
#include <vector>
#include <QVector3D>

class VoxelGrid
{
public:
	openvdb::FloatGrid::Ptr m_grid;
	openvdb::math::Transform::Ptr m_transform;
	VoxelGrid(float voxelSize=m_defaultVoxelSize);
	VoxelGrid(const VoxelGrid &other);
	bool intersects(const QVector3D &near, const QVector3D &far,
		QVector3D *intersection);
	void makeSphere(const QVector3D &center, float radius);
	void fromMesh(const std::vector<QVector3D> &vertices,
		const std::vector<std::vector<size_t>> &faces);
	void unionWith(const VoxelGrid &other);
	void diffWith(const VoxelGrid &other);
	void toMesh(std::vector<QVector3D> *vertices,
		std::vector<std::vector<size_t>> *faces);
public:
	static float m_defaultVoxelSize;
	float m_voxelSize = m_defaultVoxelSize;
};

#endif
