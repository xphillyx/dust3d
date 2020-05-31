#include "voxelgrid.h"
#include "meshvoxelcontext.h"

MeshVoxelContext::~MeshVoxelContext()
{
    delete m_voxelGrid;
}

MeshVoxelContext::MeshVoxelContext(const MeshVoxelContext &other)
{
    m_meshId = other.m_meshId;
    m_targetMeshId = other.m_targetMeshId;
    if (nullptr != other.m_voxelGrid)
        m_voxelGrid = new VoxelGrid(*other.m_voxelGrid);
}

MeshVoxelContext::MeshVoxelContext(const VoxelGrid *voxelGrid, quint64 meshId)
{
    m_meshId = meshId;
    m_targetMeshId = meshId;
    m_voxelGrid = new VoxelGrid(*voxelGrid);
}

MeshVoxelContext::MeshVoxelContext(const std::vector<QVector3D> &meshVertices,
        const std::vector<std::vector<size_t>> &meshFaces,
        quint64 meshId) :
    m_targetMeshId(meshId)
{
    if (m_meshId != m_targetMeshId) {
        m_meshVertices = meshVertices;
        m_meshFaces = meshFaces;
    }
}

void MeshVoxelContext::voxelize()
{
    if (m_meshId != m_targetMeshId) {
        delete m_voxelGrid;
        m_voxelGrid = nullptr;
    }
    if (nullptr == m_voxelGrid &&
            !m_meshFaces.empty()) {
		m_voxelGrid = new VoxelGrid;
		m_voxelGrid->fromMesh(m_meshVertices, m_meshFaces);
		m_meshId = m_targetMeshId;
	}
}

VoxelGrid *MeshVoxelContext::voxelGrid()
{
    return m_voxelGrid;
}

quint64 MeshVoxelContext::meshId()
{
    return m_meshId;
}

void MeshVoxelContext::updateVoxelGrid(VoxelGrid *voxelGrid)
{
	delete m_voxelGrid;
	m_voxelGrid = voxelGrid;
}
