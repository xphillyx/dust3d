#if USE_SIMPLE_BOOLEAN

#include <map>
#include <simpleboolean/meshoperator.h>
#include "triangulatefaces.h"
#include "meshcombiner.h"
#include "positionkey.h"

MeshCombiner::Mesh::Mesh(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, bool disableSelfIntersects)
{
	simpleboolean::Mesh *simpleBooleanMesh = nullptr;
	std::vector<std::vector<size_t>> triangles;
	std::vector<QVector3D> triangulatedVertices = vertices;
	triangulateFacesWithoutKeepVertices(triangulatedVertices, faces, triangles);
	if (!triangles.empty()) {
		simpleBooleanMesh = new simpleboolean::Mesh;
		for (const auto &it: triangulatedVertices) {
			simpleboolean::Vertex vertex;
			vertex.xyz[0] = it.x();
			vertex.xyz[1] = it.y();
			vertex.xyz[2] = it.z();
			simpleBooleanMesh->vertices.push_back(vertex);
		}
		for (const auto &it: triangles) {
			simpleboolean::Face face;
			face.indices[0] = it[0];
			face.indices[1] = it[1];
			face.indices[2] = it[2];
			simpleBooleanMesh->faces.push_back(face);
		}
	}
	m_privateData = simpleBooleanMesh;
}

MeshCombiner::Mesh::~Mesh()
{
    simpleboolean::Mesh *simpleBooeanMesh = (simpleboolean::Mesh *)m_privateData;
    delete simpleBooeanMesh;
}

MeshCombiner::Mesh::Mesh(const Mesh &other)
{
    if (other.m_privateData) {
        m_privateData = new simpleboolean::Mesh(*(simpleboolean::Mesh *)other.m_privateData);
    }
}

void MeshCombiner::Mesh::fetch(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces) const
{
    simpleboolean::Mesh *simpleBooleanMesh = (simpleboolean::Mesh *)m_privateData;
    if (nullptr == simpleBooleanMesh)
        return;
    
    for (const auto &it: simpleBooleanMesh->vertices) {
		QVector3D vertex(it.xyz[0], it.xyz[1], it.xyz[2]);
		vertices.push_back(vertex);
	}
	for (const auto &it: simpleBooleanMesh->faces) {
		std::vector<size_t> face = {
			(size_t)it.indices[0],
			(size_t)it.indices[1],
			(size_t)it.indices[2]
		};
		faces.push_back(face);
	}
}

bool MeshCombiner::Mesh::isNull() const
{
    return nullptr == m_privateData;
}

bool MeshCombiner::Mesh::isSelfIntersected() const
{
    return false;
}

MeshCombiner::Mesh *MeshCombiner::combine(const Mesh &firstMesh, const Mesh &secondMesh, Method method,
    std::vector<std::pair<Source, size_t>> *combinedVerticesComeFrom)
{
    simpleboolean::Mesh *resultSimpleBooleanMesh = nullptr;
    simpleboolean::Mesh *firstSimpleBooleanMesh = (simpleboolean::Mesh *)firstMesh.m_privateData;
    simpleboolean::Mesh *secondSimpleBooleanMesh = (simpleboolean::Mesh *)secondMesh.m_privateData;
    std::map<PositionKey, std::pair<Source, size_t>> verticesSourceMap;
    
    auto addToSourceMap = [&](simpleboolean::Mesh *mesh, Source source) {
        size_t vertexIndex = 0;
        for (const auto &vertexIt: mesh->vertices) {
            float x = (float)vertexIt.xyz[0];
            float y = (float)vertexIt.xyz[1];
            float z = (float)vertexIt.xyz[2];
            auto insertResult = verticesSourceMap.insert({{x, y, z}, {source, vertexIndex}});
            //if (!insertResult.second) {
            //    qDebug() << "Position key conflict:" << QVector3D {x, y, z} << "with:" << insertResult.first->first.position();
            //}
            ++vertexIndex;
        }
    };
    if (nullptr != combinedVerticesComeFrom) {
        addToSourceMap(firstSimpleBooleanMesh, Source::First);
        addToSourceMap(secondSimpleBooleanMesh, Source::Second);
    }
    
    if (Method::Union == method) {
		simpleboolean::MeshOperator meshOperator;
		meshOperator.setMeshes(*firstSimpleBooleanMesh, *secondSimpleBooleanMesh);
		if (meshOperator.combine()) {
			resultSimpleBooleanMesh = new simpleboolean::Mesh;
			meshOperator.getResult(simpleboolean::Type::Union, resultSimpleBooleanMesh);
		}
    } else if (Method::Diff == method) {
        simpleboolean::MeshOperator meshOperator;
		meshOperator.setMeshes(*firstSimpleBooleanMesh, *secondSimpleBooleanMesh);
		if (meshOperator.combine()) {
			resultSimpleBooleanMesh = new simpleboolean::Mesh;
			meshOperator.getResult(simpleboolean::Type::Subtraction, resultSimpleBooleanMesh);
		}
    }

    if (nullptr != combinedVerticesComeFrom) {
        combinedVerticesComeFrom->clear();
        if (nullptr != resultSimpleBooleanMesh) {
            for (const auto &vertexIt: resultSimpleBooleanMesh->vertices) {
				float x = (float)vertexIt.xyz[0];
				float y = (float)vertexIt.xyz[1];
				float z = (float)vertexIt.xyz[2];
                auto findSource = verticesSourceMap.find(PositionKey(x, y, z));
                if (findSource == verticesSourceMap.end()) {
                    combinedVerticesComeFrom->push_back({Source::None, 0});
                } else {
                    combinedVerticesComeFrom->push_back(findSource->second);
                }
            }
        }
    }
    
    if (nullptr == resultSimpleBooleanMesh)
        return nullptr;
    
    Mesh *mesh = new Mesh;
    mesh->m_privateData = resultSimpleBooleanMesh;
    return mesh;
}

#else

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <QDebug>
#include <map>
#include "meshcombiner.h"
#include "positionkey.h"
#include "booleanmesh.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel CgalKernel;
typedef CGAL::Surface_mesh<CgalKernel::Point_3> CgalMesh;

MeshCombiner::Mesh::Mesh(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, bool disableSelfIntersects)
{
    CgalMesh *cgalMesh = nullptr;
    if (!faces.empty()) {
        cgalMesh = buildCgalMesh<CgalKernel>(vertices, faces);
        if (!disableSelfIntersects) {
            if (!CGAL::is_valid_polygon_mesh(*cgalMesh)) {
                qDebug() << "Mesh is not valid polygon";
                delete cgalMesh;
                cgalMesh = nullptr;
            } else {
                if (CGAL::Polygon_mesh_processing::triangulate_faces(*cgalMesh)) {
                    if (CGAL::Polygon_mesh_processing::does_self_intersect(*cgalMesh)) {
                        m_isSelfIntersected = true;
                        qDebug() << "Mesh does_self_intersect";
                        delete cgalMesh;
                        cgalMesh = nullptr;
                    }
                } else {
                    qDebug() << "Mesh triangulate failed";
                    delete cgalMesh;
                    cgalMesh = nullptr;
                }
            }
        }
    }
    m_privateData = cgalMesh;
    validate();
}

MeshCombiner::Mesh::Mesh(const Mesh &other)
{
    if (other.m_privateData) {
        m_privateData = new CgalMesh(*(CgalMesh *)other.m_privateData);
        validate();
    }
}

MeshCombiner::Mesh::~Mesh()
{
    CgalMesh *cgalMesh = (CgalMesh *)m_privateData;
    delete cgalMesh;
}

void MeshCombiner::Mesh::fetch(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces) const
{
    CgalMesh *exactMesh = (CgalMesh *)m_privateData;
    if (nullptr == exactMesh)
        return;
    
    fetchFromCgalMesh<CgalKernel>(exactMesh, vertices, faces);
}

bool MeshCombiner::Mesh::isNull() const
{
    return nullptr == m_privateData;
}

bool MeshCombiner::Mesh::isSelfIntersected() const
{
    return m_isSelfIntersected;
}

MeshCombiner::Mesh *MeshCombiner::combine(const Mesh &firstMesh, const Mesh &secondMesh, Method method,
    std::vector<std::pair<Source, size_t>> *combinedVerticesComeFrom)
{
    CgalMesh *resultCgalMesh = nullptr;
    CgalMesh *firstCgalMesh = (CgalMesh *)firstMesh.m_privateData;
    CgalMesh *secondCgalMesh = (CgalMesh *)secondMesh.m_privateData;
    std::map<PositionKey, std::pair<Source, size_t>> verticesSourceMap;
    
    auto addToSourceMap = [&](CgalMesh *mesh, Source source) {
        size_t vertexIndex = 0;
        for (auto vertexIt = mesh->vertices_begin(); vertexIt != mesh->vertices_end(); vertexIt++) {
            auto point = mesh->point(*vertexIt);
            float x = (float)CGAL::to_double(point.x());
            float y = (float)CGAL::to_double(point.y());
            float z = (float)CGAL::to_double(point.z());
            auto insertResult = verticesSourceMap.insert({{x, y, z}, {source, vertexIndex}});
            //if (!insertResult.second) {
            //    qDebug() << "Position key conflict:" << QVector3D {x, y, z} << "with:" << insertResult.first->first.position();
            //}
            ++vertexIndex;
        }
    };
    if (nullptr != combinedVerticesComeFrom) {
        addToSourceMap(firstCgalMesh, Source::First);
        addToSourceMap(secondCgalMesh, Source::Second);
    }
    
    if (Method::Union == method) {
        resultCgalMesh = new CgalMesh;
        try {
            if (!CGAL::Polygon_mesh_processing::corefine_and_compute_union(*firstCgalMesh, *secondCgalMesh, *resultCgalMesh)) {
                delete resultCgalMesh;
                resultCgalMesh = nullptr;
            }
        } catch (...) {
            delete resultCgalMesh;
            resultCgalMesh = nullptr;
        }
    } else if (Method::Diff == method) {
        resultCgalMesh = new CgalMesh;
        try {
            if (!CGAL::Polygon_mesh_processing::corefine_and_compute_difference(*firstCgalMesh, *secondCgalMesh, *resultCgalMesh)) {
                delete resultCgalMesh;
                resultCgalMesh = nullptr;
            }
        } catch (...) {
            delete resultCgalMesh;
            resultCgalMesh = nullptr;
        }
    }

    if (nullptr != combinedVerticesComeFrom) {
        combinedVerticesComeFrom->clear();
        if (nullptr != resultCgalMesh) {
            for (auto vertexIt = resultCgalMesh->vertices_begin(); vertexIt != resultCgalMesh->vertices_end(); vertexIt++) {
                auto point = resultCgalMesh->point(*vertexIt);
                float x = (float)CGAL::to_double(point.x());
                float y = (float)CGAL::to_double(point.y());
                float z = (float)CGAL::to_double(point.z());
                auto findSource = verticesSourceMap.find(PositionKey(x, y, z));
                if (findSource == verticesSourceMap.end()) {
                    combinedVerticesComeFrom->push_back({Source::None, 0});
                } else {
                    combinedVerticesComeFrom->push_back(findSource->second);
                }
            }
        }
    }
    
    if (nullptr == resultCgalMesh)
        return nullptr;
    
    Mesh *mesh = new Mesh;
    mesh->m_privateData = resultCgalMesh;
    return mesh;
}

void MeshCombiner::Mesh::validate()
{
    if (nullptr == m_privateData)
        return;
    
    CgalMesh *exactMesh = (CgalMesh *)m_privateData;
    if (isNullCgalMesh<CgalKernel>(exactMesh)) {
        delete exactMesh;
        m_privateData = nullptr;
    }
}

#endif
