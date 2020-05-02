#ifndef DUST3D_VOXEL_MESH_H
#define DUST3D_VOXEL_MESH_H
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <vector>
#include <QVector3D>

class VoxelMesh
{
public:
	openvdb::FloatGrid::Ptr grid;
	
	void fromMesh(const std::vector<QVector3D> &vertices,
		const std::vector<std::vector<size_t>> &faces);
	void toMesh(std::vector<QVector3D> *vertices,
		std::vector<std::vector<size_t>> *faces);
};

#endif
