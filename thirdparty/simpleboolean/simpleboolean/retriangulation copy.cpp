#include <simpleboolean/retriangulation.h>
#include <simpleboolean/util.h>
#include <simpleboolean/edgeloop.h>
#include <igl/triangle/triangulate.h>
#include <set>
#include <vector>
#include <map>
#include <cmath>
#include <QDebug>
#include <QVector3D>

namespace simpleboolean
{

ReTriangulation::ReTriangulation(const std::vector<Vertex> &vertices,
        const std::vector<size_t> &triangle,
        const std::vector<std::vector<size_t>> &edgeLoops) :
    m_vertices(vertices),
    m_triangle(triangle),
    m_edgeLoops(edgeLoops)
{
}

const std::vector<Face> &ReTriangulation::getResult()
{
    return m_reTriangulatedTriangles;
}

void ReTriangulation::buildEdgeLoopsFromDirectedEdges(const std::vector<std::pair<size_t, size_t>> &edges,
        std::vector<std::vector<size_t>> *edgeLoops)
{
    EdgeLoop::buildEdgeLoopsFromDirectedEdges(edges, edgeLoops, false, true);
}

void ReTriangulation::recalculateEdgeLoops()
{
    std::vector<std::pair<size_t, size_t>> newEdges;
    
    // Test all heads and tails of open edge loops with the edges of triangle,
    // order by distances per each triangle edge.
    std::set<size_t> endpoints;
    std::set<size_t> collapsedEndpoints;
    std::vector<std::pair<size_t, size_t>> oneSegmentEdges;
    for (const auto &edgeLoop: m_edgeLoops) {
        if (edgeLoop.front() != edgeLoop.back()) {
            //qDebug() << "Open edgeloop:" << edgeLoop;
        
            endpoints.insert(edgeLoop.front());
            endpoints.insert(edgeLoop.back());
            
            if (edgeLoop.size() > 2) {
                for (size_t i = 1; i < edgeLoop.size(); ++i) {
                    newEdges.push_back({edgeLoop[i - 1], edgeLoop[i]});
                    newEdges.push_back({edgeLoop[i], edgeLoop[i - 1]});
                }
            } else {
                oneSegmentEdges.push_back(std::make_pair(edgeLoop[0], edgeLoop[1]));
            }
        } else {
            //qDebug() << "Closed edgeloop:" << edgeLoop;
            std::vector<size_t> newEdgeLoop(edgeLoop); // Remove the head/tail
            newEdgeLoop.pop_back();
            m_closedEdgeLoops.push_back(newEdgeLoop);
        }
    }
    
    std::map<size_t, std::vector<std::pair<size_t, float>>> endpointsAttachedToEdges;
    for (const auto &index: endpoints) {
        std::vector<std::tuple<size_t, float, float>> offsets;
        bool collapsed = false;
        const auto &vertex = m_vertices[index];
        QVector3D endpointPosition(vertex.xyz[0], vertex.xyz[1], vertex.xyz[2]);
        size_t attachToEdge = 0;
        float shortestDistance = std::numeric_limits<float>::max();
        for (size_t i = 0; i < 3; ++i) {
            if (m_triangle[i] == index) {
                collapsed = true;
                break;
            }
            size_t j = (i + 1) % 3;
            const auto &vertexI = m_vertices[m_triangle[i]];
            QVector3D pointI(vertexI.xyz[0], vertexI.xyz[1], vertexI.xyz[2]);
            const auto &vertexJ = m_vertices[m_triangle[j]];
            QVector3D pointJ(vertexJ.xyz[0], vertexJ.xyz[1], vertexJ.xyz[2]);
            float distance = endpointPosition.distanceToLine(pointI, (pointJ - pointI).normalized());
            if (distance < shortestDistance) {
                attachToEdge = i;
                shortestDistance = distance;
            }
        }
        if (collapsed) {
            collapsedEndpoints.insert(index);
            continue;
        }
        const auto &corner = m_vertices[m_triangle[attachToEdge]];
        QVector3D cornerPosition(corner.xyz[0], corner.xyz[1], corner.xyz[2]);
        endpointsAttachedToEdges[attachToEdge].push_back({index, (endpointPosition - cornerPosition).lengthSquared()});
    }
    for (auto &it: endpointsAttachedToEdges) {
        size_t startCorner = m_triangle[it.first];
        size_t stopCorner = m_triangle[(it.first + 1) % 3];
        std::sort(it.second.begin(), it.second.end(), [](const std::pair<size_t, float> &first,
                const std::pair<size_t, float> &second) {
            return first.second < second.second;
        });
        std::vector<size_t> points;
        points.push_back(startCorner);
        for (const auto &it: it.second) {
            points.push_back(it.first);
        }
        points.push_back(stopCorner);
        
        //qDebug() << "Attach" << points << "to" << startCorner << "-" << stopCorner;
        
        for (size_t i = 1; i < points.size(); ++i) {
            newEdges.push_back({points[i - 1], points[i]});
        }
    }
    for (size_t i = 0; i < 3; ++i) {
        if (endpointsAttachedToEdges.find(i) != endpointsAttachedToEdges.end()) {
            //qDebug() << "Full triangle edge:" << m_triangle[i] << m_triangle[(i + 1) % 3];
            continue;
        }
        //qDebug() << "Empty triangle edge:" << m_triangle[i] << m_triangle[(i + 1) % 3];
        newEdges.push_back({m_triangle[i], m_triangle[(i + 1) % 3]});
    }
    
    for (const auto &edge: oneSegmentEdges) {
        if (collapsedEndpoints.find(edge.first) == collapsedEndpoints.end() ||
                collapsedEndpoints.find(edge.second) == collapsedEndpoints.end()) {
            newEdges.push_back(edge);
            newEdges.push_back(std::make_pair(edge.second, edge.first));
        }
    }
    
    buildEdgeLoopsFromDirectedEdges(newEdges, &m_recalculatedEdgeLoops);
}

void ReTriangulation::convertVerticesTo2D()
{
    Vector planeNormal;
    triangleNormal(m_vertices[m_triangle[0]],
        m_vertices[m_triangle[1]],
        m_vertices[m_triangle[2]],
        planeNormal);
    const Vertex &planeOrigin = m_vertices[m_triangle[0]];
    Vector planeX;
    directionBetweenTwoVertices(m_vertices[m_triangle[0]],
        m_vertices[m_triangle[1]], planeX);
    std::vector<Vertex> points;
    std::vector<Vertex> points2D;
    std::vector<size_t> indices;
    std::set<size_t> histories;
    auto addPoints = [&](const std::vector<std::vector<size_t>> &edgeLoops) {
        for (const auto &it: edgeLoops) {
            for (const auto &index: it) {
                if (histories.find(index) != histories.end())
                    continue;
                histories.insert(index);
                indices.push_back(index);
                points.push_back(m_vertices[index]);
            }
        }
    };
    addPoints(m_closedEdgeLoops);
    addPoints(m_recalculatedEdgeLoops);
    projectToPlane(planeNormal, planeOrigin,
        planeX, points, points2D);
    for (size_t i = 0; i < points2D.size(); ++i) {
        m_vertices2D.insert(std::make_pair(indices[i], points2D[i]));
    }
}

void ReTriangulation::convertEdgeLoopsToVertices2D()
{
    auto convertEdgeLoops = [&](const std::vector<std::vector<size_t>> &edgeLoops,
            std::vector<std::vector<Vertex>> &result) {
        for (size_t i = 0; i < edgeLoops.size(); ++i) {
            std::vector<Vertex> points2D;
            for (const auto &it: edgeLoops[i]) {
                points2D.push_back(m_vertices2D[it]);
            }
            result.push_back(points2D);
        }
    };
    convertEdgeLoops(m_closedEdgeLoops, m_closedEdgeLoopsVertices2D);
    convertEdgeLoops(m_recalculatedEdgeLoops, m_recalculatedEdgeLoopsVertices2D);
}

bool ReTriangulation::attachClosedEdgeLoopsToOutter()
{
    if (m_closedEdgeLoops.empty() || m_recalculatedEdgeLoops.empty())
        return true;
    for (size_t inner = 0; inner < m_closedEdgeLoops.size(); ++inner) {
        Vertex center2D;
        averageOfPoints2D(m_closedEdgeLoopsVertices2D[inner], center2D);
        bool attached = false;
        for (size_t outter = 0; outter < m_recalculatedEdgeLoops.size(); ++outter) {
            if (pointInPolygon2D(center2D, m_recalculatedEdgeLoopsVertices2D[outter])) {
                m_innerEdgeLoopsMap[outter].push_back(inner);
                attached = true;
                break;
            }
        }
        //qDebug() << "attached:" << attached;
        if (!attached)
            return false;
    }
    return true;
}

void ReTriangulation::unifyFaceDirections(const std::vector<Face> &existedFaces,
        std::vector<Face> *newFaces)
{
    if (existedFaces.empty())
        return;
    std::set<std::pair<size_t, size_t>> halfEdges;
    for (const auto &face: existedFaces) {
        for (size_t i = 0; i < 3; ++i) {
            size_t j = (i + 1) % 3;
            halfEdges.insert(std::make_pair(face.indices[i], face.indices[j]));
        }
    }
    for (const auto &face: *newFaces) {
        for (size_t i = 0; i < 3; ++i) {
            size_t j = (i + 1) % 3;
            if (halfEdges.find(std::make_pair(face.indices[i], face.indices[j])) != halfEdges.end()) {
                for (auto &newFace: *newFaces)
                    std::reverse(newFace.indices, newFace.indices + 3);
                return;
            }
        }
    }
}

void ReTriangulation::reTriangulate()
{
    recalculateEdgeLoops();
    convertVerticesTo2D();
    convertEdgeLoopsToVertices2D();
    if (!attachClosedEdgeLoopsToOutter()) {
        ++m_errors;
    } else {
        for (size_t outter = 0; outter < m_recalculatedEdgeLoops.size(); ++outter) {
            std::vector<std::pair<double, double>> vertices;
            std::vector<size_t> realIndices;
            std::vector<std::vector<size_t>> polygons;
            const auto &outterPoints2D = m_recalculatedEdgeLoopsVertices2D[outter];
            {
                std::vector<size_t> polygon;
                for (size_t index = 0; index < outterPoints2D.size(); ++index) {
                    const auto &it = outterPoints2D[index];
                    polygon.push_back(vertices.size());
                    vertices.push_back({(double)it.xyz[0], (double)it.xyz[1]});
                    const auto &vertexIndex = m_recalculatedEdgeLoops[outter][index];
                    realIndices.push_back(vertexIndex);
                }
                if (polygon.empty())
                    continue;
                polygons.push_back(polygon);
            }
            const auto &findInners = m_innerEdgeLoopsMap.find(outter);
            if (findInners != m_innerEdgeLoopsMap.end()) {
                for (const auto &inner: findInners->second) {
                    const auto &innerPoints2D = m_closedEdgeLoopsVertices2D[inner];
                    std::vector<size_t> polygon;
                    for (size_t index = 0; index < innerPoints2D.size(); ++index) {
                        const auto &it = innerPoints2D[index];
                        polygon.push_back(vertices.size());
                        vertices.push_back({(double)it.xyz[0], (double)it.xyz[1]});
                        const auto &vertexIndex = m_closedEdgeLoops[inner][index];
                        realIndices.push_back(vertexIndex);
                    }
                    if (polygon.empty())
                        continue;
                    polygons.push_back(polygon);
                }
            }
            
            qDebug() << "================";
            
            Eigen::MatrixXd V;
            Eigen::MatrixXi E;
            Eigen::MatrixXd H;
            H.resize(1,2);
            V.resize(vertices.size(), 2);
            for (size_t i = 0; i < vertices.size(); ++i) {
                const auto &vertex = vertices[i];
                V.row(i) << vertex.first, vertex.second;
            }
            std::vector<std::pair<size_t, size_t>> edges;
            for (const auto &it: polygons) {
                for (size_t i = 0; i < it.size(); ++i) {
                    size_t j = (i + 1) % it.size();
                    edges.push_back({it[i], it[j]});
                }
            }
            E.resize(edges.size(), 2);
            for (size_t i = 0; i < edges.size(); ++i) {
                const auto &edge = edges[i];
                E.row(i) << edge.first, edge.second;
            }
            Eigen::MatrixXd V2;
            Eigen::MatrixXi F2;
            H << 0,0;
            igl::triangle::triangulate(V, E, H, "a0.005q", V2, F2);
            
            qDebug() << "V.size:" << V.size();
            qDebug() << "V2.size:" << V2.size();
            qDebug() << "F2.size:" << F2.size();
            
            /*
            std::vector<CDT::Edge> cdtEdges;
            for (const auto &it: cdtInnerEdgeLoops) {
                for (size_t i = 0; i < it.size(); ++i) {
                    size_t j = (i + 1) % it.size();
                    cdtEdges.push_back(CDT::Edge {realIndices[i], realIndices[j]});
                }
            }
            if (!cdtEdges.empty())
                cdt.insertEdges(cdtEdges);
            
            cdt.eraseSuperTriangle();
            
            for (const auto &it: cdt.triangles) {
                m_reTriangulatedTriangles.push_back(Face {{
                    realIndices[it.vertices[0]],
                    realIndices[it.vertices[1]],
                    realIndices[it.vertices[2]]}});
            }
            */
        }
    }
}

}

