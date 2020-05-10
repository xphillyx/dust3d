#include <openvdb/tools/RayIntersector.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/LevelSetSphere.h>
#include "voxelgrid.h"
#include <QDebug>
#include <QElapsedTimer>
#include "util.h"

float VoxelGrid::m_defaultVoxelSize = 0.0025;
bool VoxelGrid::m_openvdbInitialized = false;

VoxelGrid::VoxelGrid(float voxelSize) :
	m_voxelSize(voxelSize)
{
	if (!m_openvdbInitialized) {
		m_openvdbInitialized = true;
		openvdb::initialize();
	}
	m_transform = openvdb::math::Transform::createLinearTransform(m_voxelSize);
	m_grid = openvdb::FloatGrid::create(m_voxelSize);
	m_grid->setTransform(m_transform);
}

VoxelGrid::VoxelGrid(const VoxelGrid &other)
{
	m_transform = openvdb::math::Transform::Ptr(new openvdb::math::Transform(*other.m_transform));
	m_grid = other.m_grid->deepCopy();
}

bool VoxelGrid::intersects(const QVector3D &near, const QVector3D &far,
	QVector3D *intersection)
{
	QVector3D direction = (far - near).normalized();
	openvdb::math::Ray<double> ray(openvdb::math::Vec3<double>(near.x(), near.y(), near.z()),
		openvdb::math::Vec3<double>(direction.x(), direction.y(), direction.z()));
	openvdb::tools::LevelSetRayIntersector<openvdb::FloatGrid> rayIntersector(*m_grid);
	openvdb::math::Vec3<double> intersectsAt;
	openvdb::math::Vec3<double> normal;
	if (!rayIntersector.intersectsWS(ray, intersectsAt, normal))
		return false;
	if (QVector3D::dotProduct(QVector3D(normal.x(), normal.y(), normal.z()), direction) >= 0) {
		return false;
	}
	if (nullptr != intersection) {
		intersection->setX(intersectsAt.x());
		intersection->setY(intersectsAt.y());
		intersection->setZ(intersectsAt.z());
	}
	return true;
}

void VoxelGrid::makeSphere(const QVector3D &center, float radius)
{
	m_grid = openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(radius, openvdb::Vec3f(center.x(), center.y(), center.z()), m_voxelSize);
}

void VoxelGrid::fromMesh(const std::vector<QVector3D> &vertices,
	const std::vector<std::vector<size_t>> &faces)
{
	std::vector<openvdb::Vec3s> points(vertices.size());
	std::vector<openvdb::Vec3I> triangles;
	std::vector<openvdb::Vec4I> quads;
	
	for (size_t i = 0; i < vertices.size(); ++i) {
		const auto &src = vertices[i];
		points[i] = openvdb::Vec3s(src.x(), src.y(), src.z());
	}
	
	for (const auto &face: faces) {
		if (3 == face.size()) {
			triangles.push_back(openvdb::Vec3I(face[0], face[1], face[2]));
		} else if (4 == face.size()) {
			quads.push_back(openvdb::Vec4I(face[0], face[1], face[2], face[3]));
		}
	}
	
	m_grid = openvdb::tools::meshToLevelSet<openvdb::FloatGrid>(
		*m_transform, points, triangles, quads, 1.0);
}

void VoxelGrid::unionWith(const VoxelGrid &other)
{
	openvdb::tools::csgUnion(*m_grid, *other.m_grid);
}

void VoxelGrid::diffWith(const VoxelGrid &other)
{
	openvdb::tools::csgDifference(*m_grid, *other.m_grid);
}

void VoxelGrid::toMesh(std::vector<QVector3D> *vertices,
	std::vector<std::vector<size_t>> *faces)
{
	double isovalue = 0.0;
	double adaptivity = 0.0;
	bool relaxDisorientedTriangles = true;
	
	QElapsedTimer timer;
	timer.start();
	
	//openvdb::tools::LevelSetFilter<openvdb::FloatGrid> filter(*m_grid);
	//filter.laplacian();
	
	std::vector<openvdb::Vec3s> points;
	std::vector<openvdb::Vec3I> triangles;
	std::vector<openvdb::Vec4I> quads;
	
	openvdb::tools::volumeToMesh<openvdb::FloatGrid>(*m_grid, points, triangles, quads,
		isovalue, adaptivity, relaxDisorientedTriangles);
		
	vertices->resize(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		const auto &src = points[i];
		(*vertices)[i] = QVector3D(src.x(), src.y(), src.z());
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
	
	qDebug() << "Voxel to mesh took milliseconds:" << elapsedMilliseconds <<
		"vertices:" << vertices->size() << "faces:" << faces->size() <<
		"(triangles:" << triangles.size() << "quads:" << quads.size() << ")";
		
	//saveAsObj("/Users/jeremy/Desktop/test.obj", *vertices, *faces);
}
