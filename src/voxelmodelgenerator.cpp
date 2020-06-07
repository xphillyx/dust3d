#include "voxelgrid.h"
#include <QDebug>
#include <QElapsedTimer>
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

void VoxelModelGenerator::generate()
{
	if (nullptr == m_voxelGrid)
		return;

	QElapsedTimer timer;
	timer.start();

	std::vector<QVector3D> voxelVertices;
	std::vector<std::vector<size_t>> voxelTriangles;
	m_voxelGrid->toMesh(&voxelVertices, &voxelTriangles);
	
	//auto calculateNormalStartTime = timer.elapsed();
	
	std::vector<QVector3D> voxelTriangleNormals(voxelTriangles.size());
	for (size_t i = 0; i < voxelTriangles.size(); ++i) {
	    const auto &facePoints = voxelTriangles[i];
	    voxelTriangleNormals[i] = normalOfThreePointsHighPrecision(voxelVertices[facePoints[0]],
			voxelVertices[facePoints[1]],
			voxelVertices[facePoints[2]]);
	}
	//auto calculateNormalConsumedTime = timer.elapsed() - calculateNormalStartTime;
	//qDebug() << "VOXEL calculateNormal took milliseconds:" << calculateNormalConsumedTime;
    
    //auto createMeshStartTime = timer.elapsed();
    
    int triangleVertexCount = voxelTriangles.size() * 3;
    ShaderVertex *triangleVertices = new ShaderVertex[triangleVertexCount];
    int destIndex = 0;
    auto clayR = Theme::clayColor.redF();
    auto clayG = Theme::clayColor.greenF();
    auto clayB = Theme::clayColor.blueF();
    for (size_t i = 0; i < voxelTriangles.size(); ++i) {
        const auto &srcNormal = &voxelTriangleNormals[i];
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = voxelTriangles[i][j];
            const QVector3D *srcVert = &voxelVertices[vertexIndex];
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
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            dest->metalness = Model::m_defaultMetalness;
            dest->roughness = Theme::clayRoughness;
            dest->tangentX = 0;
            dest->tangentY = 0;
            dest->tangentZ = 0;
            destIndex++;
        }
    }

    m_model = new Model(triangleVertices, triangleVertexCount, 0, 0,
        &voxelVertices, &voxelTriangles);
    
    //auto createMeshConsumedTime = timer.elapsed() - createMeshStartTime;
	//qDebug() << "VOXEL createMesh took milliseconds:" << createMeshConsumedTime;
	
	//auto totalConsumedTime = timer.elapsed();
	//qDebug() << "VOXEL total took milliseconds:" << totalConsumedTime;
}
