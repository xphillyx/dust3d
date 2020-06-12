#include "voxelgrid.h"
#include <QDebug>
#include <QElapsedTimer>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include "voxelmodelgenerator.h"
#include "model.h"
#include "util.h"
#include "theme.h"
#include "meshsimplify.h"

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
    MeshToModel(std::vector<QVector3D> *vertices, std::vector<std::vector<size_t>> *quads, ShaderVertex *target, bool isFromQuads=true):
        m_voxelVertices(vertices),
        m_voxelQuads(quads),
        m_triangleVertices(target),
        m_isFromQuads(isFromQuads)
    {
    }
    
    float m_clayR = Theme::clayColor.redF();
    float m_clayG = Theme::clayColor.greenF();
    float m_clayB = Theme::clayColor.blueF();
    
    bool m_isFromQuads = true;
    
    std::vector<QVector3D> *m_voxelVertices = nullptr;
    std::vector<std::vector<size_t>> *m_voxelQuads = nullptr;
    ShaderVertex *m_triangleVertices = nullptr;
    
    inline void addTriangleVertex(int vertexIndex, const QVector3D &srcNormal, int destTriangleIndex) const
    {
        const QVector3D *srcVert = &(*m_voxelVertices)[vertexIndex];
        ShaderVertex *dest = &m_triangleVertices[destTriangleIndex];
        dest->colorR = m_clayR;
        dest->colorG = m_clayG;
        dest->colorB = m_clayB;
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
        if (m_isFromQuads) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                const auto &facePoints = (*m_voxelQuads)[i];
                const auto srcNormal = normalOfThreePointsHighPrecision((*m_voxelVertices)[facePoints[0]],
                    (*m_voxelVertices)[facePoints[1]],
                    (*m_voxelVertices)[facePoints[2]]);
                int destTriangleIndex = i * 6;
                
                addTriangleVertex(facePoints[0], srcNormal, destTriangleIndex++);
                addTriangleVertex(facePoints[1], srcNormal, destTriangleIndex++);
                addTriangleVertex(facePoints[2], srcNormal, destTriangleIndex++);
                
                addTriangleVertex(facePoints[2], srcNormal, destTriangleIndex++);
                addTriangleVertex(facePoints[3], srcNormal, destTriangleIndex++);
                addTriangleVertex(facePoints[0], srcNormal, destTriangleIndex++);
            }
        } else {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                const auto &facePoints = (*m_voxelQuads)[i];
                const auto srcNormal = normalOfThreePointsHighPrecision((*m_voxelVertices)[facePoints[0]],
                    (*m_voxelVertices)[facePoints[1]],
                    (*m_voxelVertices)[facePoints[2]]);
                int destTriangleIndex = i * 3;
                
                addTriangleVertex(facePoints[0], srcNormal, destTriangleIndex++);
                addTriangleVertex(facePoints[1], srcNormal, destTriangleIndex++);
                addTriangleVertex(facePoints[2], srcNormal, destTriangleIndex++);
            }
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
    //const openvdb::math::NonlinearFrustumMap frustum{
    //    /*position=*/openvdb::Vec3d{0.0, 0.0, -1.0},
    //    /*direction=*/openvdb::Vec3d{0.0, 0.0, 1.0},
    //    /*up=*/openvdb::Vec3d{0.0, 1.0, 0.0},
    //    /*aspect=*/1.0,
    //    /*near=*/0.01,
    //    /*depth=*/100,
    //    /*x_count=*/100,
    //    /*z_count=*/100};
    //auto clipped = openvdb::tools::clip(*m_voxelGrid->m_grid, frustum);
    //{
    //    openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*clipped);
    //    filter.laplacian();
    //}
    //openvdb::math::Transform::Ptr toTransform = openvdb::math::Transform::createLinearTransform(VoxelGrid::m_defaultVoxelSize * 4);
    //auto rebuiltGrid = openvdb::tools::levelSetRebuild<openvdb::FloatGrid>(*m_voxelGrid->m_grid, 0.0f, 1.0f, toTransform.get());
    openvdb::tools::MultiResGrid<openvdb::FloatTree> multiResGrid(2, *m_voxelGrid->m_grid);
    auto lo2Grid = multiResGrid.grid(1);
    //{
    //    openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*lo2Grid);
    //    filter.laplacian();
    //}
	openvdb::tools::volumeToMesh<openvdb::FloatGrid>(*lo2Grid, points, triangles, quads,
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
    
    //meshSimplifyFromQuads(voxelVertices, voxelQuads,
    //    &voxelVertices, &voxelQuads);
    
    auto createMeshStartTime = timer.elapsed();

    int triangleVertexCount = voxelQuads.size() * 3 * 2;
    bool isQuads = true;
    //int triangleVertexCount = voxelQuads.size() * 3;
    //bool isQuads = false;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    tbb::parallel_for(tbb::blocked_range<size_t>(0, voxelQuads.size()),
        MeshToModel(&voxelVertices, &voxelQuads, triangleVertices, isQuads));
    
    m_model = new Model(triangleVertices, triangleVertexCount, 0, 0,
        &voxelVertices, &voxelQuads);
    
    auto createMeshConsumedTime = timer.elapsed() - createMeshStartTime;
	qDebug() << "VOXEL createMesh took milliseconds:" << createMeshConsumedTime;
	
	auto totalConsumedTime = timer.elapsed();
	qDebug() << "VOXEL TOTAL generation took milliseconds:" << totalConsumedTime <<
		"vertices:" << voxelVertices.size() << "quads:" << voxelQuads.size() << ")";
}
