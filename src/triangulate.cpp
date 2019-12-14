#include <QDebug>
#include <thirdparty/poly2tri/poly2tri/poly2tri.h>
#include <QVector2D>
#include <QVector3D>
#include "triangulate.h"
#include "util.h"

static std::vector<QVector2D> convertVerticesTo2D(std::vector<QVector3D> &vertices)
{
    std::vector<QVector2D> vertices2D;
    if (vertices.empty())
        return vertices2D;
    std::vector<size_t> polyline;
    QVector3D planeOrigin;
    for (size_t i = 0; i < vertices.size(); ++i) {
        polyline.push_back(i);
        planeOrigin += vertices[i];
    }
    planeOrigin /= vertices.size();
    auto maxDistance2 = std::numeric_limits<float>::min();
    size_t pickVertexIndex = 0;
    for (size_t i = 0; i < vertices.size(); ++i) {
        auto distance2 = (vertices[i] - planeOrigin).lengthSquared();
        if (distance2 > maxDistance2) {
            maxDistance2 = distance2;
            pickVertexIndex = i;
        }
    }
    QVector3D planeNormal = polygonNormal(vertices, polyline);
    auto planeX = (vertices[pickVertexIndex] - planeOrigin).normalized();
    std::vector<QVector2D> points2D;
    const QVector3D &normal = planeNormal;
    const QVector3D &origin = planeOrigin;
    const QVector3D &xAxis = planeX;
    QVector3D yAxis = QVector3D::crossProduct(normal, xAxis);
    for (const auto &it: vertices) {
        QVector2D point2D;
        QVector3D direction = it - origin;
        vertices2D.push_back({QVector3D::dotProduct(direction, xAxis) * 1000,
            QVector3D::dotProduct(direction, yAxis) * 1000
        });
    }
    return vertices2D;
}

template <class C> void FreeClear( C & cntr )
{
    for ( typename C::iterator it = cntr.begin();
              it != cntr.end(); ++it ) {
        delete * it;
    }
    cntr.clear();
}

bool triangulate(std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, std::vector<std::vector<size_t>> &triangles)
{
    bool succeed = true;
    
    auto fetchTriangulatedResult = [&](const std::vector<p2t::Triangle *> &triangles,
            const std::map<p2t::Point *, size_t> &pointToIndexMap) {
        std::vector<std::vector<size_t>> newFaces;
        bool foundBadTriangle = false;
        for (const auto &it: triangles) {
            std::vector<size_t> face;
            face.reserve(3);
            for (int i = 0; i < 3; ++i) {
                p2t::Point *point = it->GetPoint(i);
                auto findIndex = pointToIndexMap.find(point);
                if (findIndex == pointToIndexMap.end()) {
                    continue;
                }
                face.push_back(findIndex->second);
            }
            if (face.size() != 3) {
                qDebug() << "Ignore triangle";
                foundBadTriangle = true;
                continue;
            }
            newFaces.push_back(face);
        }
        if (foundBadTriangle)
            return std::vector<std::vector<size_t>>();
        return newFaces;
    };
    
    for (const auto &face: faces) {
        if (face.size() > 3) {
            std::vector<QVector3D> vertices3D;
            vertices3D.reserve(face.size());
            std::vector<size_t> oldIndices;
            oldIndices.reserve(face.size());
            for (const auto &i: face) {
                oldIndices.push_back(i);
                vertices3D.push_back(vertices[i]);
            }
            auto vertices2D = convertVerticesTo2D(vertices3D);
            std::vector<p2t::Point*> polyline;
            polyline.reserve(vertices2D.size());
            std::map<p2t::Point *, size_t> pointToIndexMap;
            for (size_t i = 0; i < vertices2D.size(); ++i) {
                const auto &vertex2D = vertices2D[i];
                p2t::Point *point = new p2t::Point(vertex2D.x(), vertex2D.y());
                pointToIndexMap.insert({point, oldIndices[i]});
                polyline.push_back(point);
            }
            p2t::CDT *cdt = new p2t::CDT(polyline);
            cdt->Triangulate();
            std::vector<std::vector<size_t>> holeFaces = fetchTriangulatedResult(cdt->GetTriangles(),
                pointToIndexMap);
            if (holeFaces.empty())
                succeed = false;
            triangles.insert(triangles.end(), holeFaces.begin(), holeFaces.end());
            FreeClear(polyline);
            delete cdt;
        } else {
            triangles.push_back(face);
        }
    }
    
    return succeed;
}
