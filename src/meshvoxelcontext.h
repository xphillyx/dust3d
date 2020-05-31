#ifndef DUST3D_MESH_VOXEL_CONTEXT
#define DUST3D_MESH_VOXEL_CONTEXT
#include <QVector3D>
#include <vector>

class VoxelGrid;

class MeshVoxelContext
{
public:
    MeshVoxelContext(const MeshVoxelContext &other);
    MeshVoxelContext(const VoxelGrid *voxelGrid, quint64 meshId);
    MeshVoxelContext(const std::vector<QVector3D> &meshVertices,
        const std::vector<std::vector<size_t>> &meshFaces,
        quint64 meshId);
    void voxelize();
    VoxelGrid *voxelGrid();
    quint64 meshId();
    void updateVoxelGrid(VoxelGrid *voxelGrid);
	~MeshVoxelContext();
private:
    std::vector<QVector3D> m_meshVertices;
    std::vector<std::vector<size_t>> m_meshFaces;
    quint64 m_meshId = 0;
    quint64 m_targetMeshId = 0;
    VoxelGrid *m_voxelGrid = nullptr;
};

#endif
