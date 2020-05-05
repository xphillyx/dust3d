#include <openvdb/tools/RayIntersector.h>
#include <openvdb/tools/RayTracer.h>
#include "voxelmesh.h"
#include <QDebug>
#include <QElapsedTimer>

float VoxelMesh::m_scale = 200;

void VoxelMesh::fromMesh(const std::vector<QVector3D> &vertices,
	const std::vector<std::vector<size_t>> &faces)
{
	std::vector<openvdb::Vec3s> points(vertices.size());
	std::vector<openvdb::Vec3I> triangles;
	std::vector<openvdb::Vec4I> quads;
	
	for (size_t i = 0; i < vertices.size(); ++i) {
		const auto &src = vertices[i];
		points[i] = openvdb::Vec3s(src.x() * m_scale, src.y() * m_scale, src.z() * m_scale);
	}
	
	for (const auto &face: faces) {
		if (3 == face.size()) {
			triangles.push_back(openvdb::Vec3I(face[0], face[1], face[2]));
		} else if (4 == face.size()) {
			quads.push_back(openvdb::Vec4I(face[0], face[1], face[2], face[3]));
		}
	}
	
	openvdb::math::Transform::Ptr xform = openvdb::math::Transform::createLinearTransform();
	
	m_grid = openvdb::tools::meshToLevelSet<openvdb::FloatGrid>(
		*xform, points, triangles, quads, 0.5);
}

void VoxelMesh::toMesh(std::vector<QVector3D> *vertices,
	std::vector<std::vector<size_t>> *faces)
{
	double isovalue = 0.0;
	double adaptivity = 0.0;
	bool relaxDisorientedTriangles = true;
	
	QElapsedTimer timer;
	timer.start();
	
	openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*m_grid);
	filter.laplacian();
	
	std::vector<openvdb::Vec3s> points;
	std::vector<openvdb::Vec3I> triangles;
	std::vector<openvdb::Vec4I> quads;
	
	openvdb::tools::volumeToMesh<openvdb::FloatGrid>(*m_grid, points, triangles, quads,
		isovalue, adaptivity, relaxDisorientedTriangles);
		
	vertices->resize(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		const auto &src = points[i];
		(*vertices)[i] = QVector3D(src.x() / m_scale, src.y() / m_scale, src.z() / m_scale);
	}
	
	for (const auto &it: triangles) {
		faces->push_back({it.z(), it.y(), it.x()});
	}
	
	for (const auto &it: quads) {
		//faces->push_back({it.x(), it.y(), it.z(), it.w()});
		faces->push_back({it.z(), it.y(), it.x()});
		faces->push_back({it.x(), it.w(), it.z()});
	}
	
	auto elapsedMilliseconds = timer.elapsed();
	
	qDebug() << "Voxel to mesh took milliseconds:" << elapsedMilliseconds;
}
