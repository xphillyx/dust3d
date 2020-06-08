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
    MeshToModel(std::vector<QVector3D> *vertices, std::vector<std::vector<size_t>> *triangles, ShaderVertex *target):
        voxelVertices(vertices),
        voxelTriangles(triangles),
        triangleVertices(target)
    {
    }
    
    float clayR = Theme::clayColor.redF();
    float clayG = Theme::clayColor.greenF();
    float clayB = Theme::clayColor.blueF();
    
    std::vector<QVector3D> *voxelVertices = nullptr;
    std::vector<std::vector<size_t>> *voxelTriangles = nullptr;
    ShaderVertex *triangleVertices = nullptr;
    
    void operator()(const tbb::blocked_range<size_t> &range) const {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            const auto &facePoints = (*voxelTriangles)[i];
            const auto srcNormal = normalOfThreePointsHighPrecision((*voxelVertices)[facePoints[0]],
                (*voxelVertices)[facePoints[1]],
                (*voxelVertices)[facePoints[2]]);
            int destIndex = i * 3;
            for (auto j = 0; j < 3; j++) {
                int vertexIndex = (*voxelTriangles)[i][j];
                const QVector3D *srcVert = &(*voxelVertices)[vertexIndex];
                ShaderVertex *dest = &triangleVertices[destIndex];
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
                destIndex++;
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
	std::vector<std::vector<size_t>> voxelTriangles;
	m_voxelGrid->toMesh(&voxelVertices, &voxelTriangles);
    
    auto createMeshStartTime = timer.elapsed();

    int triangleVertexCount = voxelTriangles.size() * 3;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    tbb::parallel_for(tbb::blocked_range<size_t>(0, voxelTriangles.size()),
        MeshToModel(&voxelVertices, &voxelTriangles, triangleVertices));
    
    m_model = new Model(triangleVertices, triangleVertexCount, 0, 0,
        &voxelVertices, &voxelTriangles);
    
    auto createMeshConsumedTime = timer.elapsed() - createMeshStartTime;
	qDebug() << "VOXEL createMesh took milliseconds:" << createMeshConsumedTime;
	
	auto totalConsumedTime = timer.elapsed();
	qDebug() << "VOXEL TOTAL generation took milliseconds:" << totalConsumedTime;
}
