#include "voxelgrid.h"
#include <QDebug>
#include <QElapsedTimer>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include "voxelmodelgenerator.h"
#include "model.h"
#include "util.h"
#include "theme.h"

VoxelModelGenerator::VoxelModelGenerator(VoxelGrid *voxelGrid) :
	m_voxelGrid(voxelGrid)
{
}

VoxelModelGenerator::~VoxelModelGenerator()
{
	delete m_model;
	delete m_voxelGrid;
}

void VoxelModelGenerator::process()
{
	generate();
    emit finished();
}

Model *VoxelModelGenerator::takeModel()
{
    Model *model = m_model;
    m_model = nullptr;
    return model;
}

struct MeshToModel
{
    MeshToModel(std::vector<QVector3D> *vertices, std::vector<std::vector<size_t>> *quads, ShaderVertex *target):
        voxelVertices(vertices),
        voxelQuads(quads),
        triangleVertices(target)
    {
    }
    
    float clayR = Theme::clayColor.redF();
    float clayG = Theme::clayColor.greenF();
    float clayB = Theme::clayColor.blueF();
    
    int facePointIndices[2][3] = {
        {0, 1, 2},
        {2, 3, 0}
    };
    
    std::vector<QVector3D> *voxelVertices = nullptr;
    std::vector<std::vector<size_t>> *voxelQuads = nullptr;
    ShaderVertex *triangleVertices = nullptr;
    
    inline void addTriangleVertex(int vertexIndex, const QVector3D &srcNormal, int destTriangleIndex) const
    {
        const QVector3D *srcVert = &(*voxelVertices)[vertexIndex];
        ShaderVertex *dest = &triangleVertices[destTriangleIndex];
        dest->colorR = clayR;
        dest->colorG = clayG;
        dest->colorB = clayB;
        dest->alpha = 1.0;
        dest->posX = srcVert->x();
        dest->posY = srcVert->y();
        dest->posZ = srcVert->z();
        dest->texU = 0;
        dest->texV = 0;
        dest->normX = srcNormal.x();
        dest->normY = srcNormal.y();
        dest->normZ = srcNormal.z();
        dest->metalness = Model::m_defaultMetalness;
        dest->roughness = Theme::clayRoughness;
        dest->tangentX = 0;
        dest->tangentY = 0;
        dest->tangentZ = 0;
    }
    
    void operator()(const tbb::blocked_range<size_t> &range) const 
    {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            const auto &facePoints = (*voxelQuads)[i];
            const auto srcNormal = normalOfThreePointsHighPrecision((*voxelVertices)[facePoints[0]],
                (*voxelVertices)[facePoints[1]],
                (*voxelVertices)[facePoints[2]]);
            int destTriangleIndex = i * 6;
            
            addTriangleVertex(facePoints[0], srcNormal, destTriangleIndex++);
            addTriangleVertex(facePoints[1], srcNormal, destTriangleIndex++);
            addTriangleVertex(facePoints[2], srcNormal, destTriangleIndex++);
            
            addTriangleVertex(facePoints[2], srcNormal, destTriangleIndex++);
            addTriangleVertex(facePoints[3], srcNormal, destTriangleIndex++);
            addTriangleVertex(facePoints[0], srcNormal, destTriangleIndex++);
        }
    }
};

void VoxelModelGenerator::generate()
{
	if (nullptr == m_voxelGrid)
		return;

	QElapsedTimer timer;
	timer.start();

	std::vector<QVector3D> voxelVertices;
	std::vector<std::vector<size_t>> voxelQuads;
    
    std::vector<openvdb::Vec3s> points;
	std::vector<openvdb::Vec3I> triangles;
	std::vector<openvdb::Vec4I> quads;
	
    double isovalue = 0.0;
	double adaptivity = 0.0;
	bool relaxDisorientedTriangles = false;
	openvdb::tools::volumeToMesh<openvdb::FloatGrid>(*m_voxelGrid->m_grid, points, triangles, quads,
		isovalue, adaptivity, relaxDisorientedTriangles);
        
    voxelVertices.resize(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		const auto &src = points[i];
		voxelVertices[i] = QVector3D(src.x(), src.y(), src.z());
	}
    
    voxelQuads.resize(quads.size());
    for (size_t i = 0; i < quads.size(); ++i) {
        const auto &src = quads[i];
		voxelQuads[i] = std::vector<size_t> {src.x(), src.w(), src.z(), src.y()};
	}
    
    auto createMeshStartTime = timer.elapsed();

    int triangleVertexCount = voxelQuads.size() * 3 * 2;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    tbb::parallel_for(tbb::blocked_range<size_t>(0, voxelQuads.size()),
        MeshToModel(&voxelVertices, &voxelQuads, triangleVertices));
    
    m_model = new Model(triangleVertices, triangleVertexCount, 0, 0,
        &voxelVertices, &voxelQuads);
    
    auto createMeshConsumedTime = timer.elapsed() - createMeshStartTime;
	qDebug() << "VOXEL createMesh took milliseconds:" << createMeshConsumedTime;
	
	auto totalConsumedTime = timer.elapsed();
	qDebug() << "VOXEL TOTAL generation took milliseconds:" << totalConsumedTime;
}
