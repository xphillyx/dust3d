#include <simpleboolean/util.h>
#include <QVector3D>
#include <QtGlobal>
#include <QPolygon>

namespace simpleboolean
{

static QVector3D vertexToQVector3D(const Vertex &first)
{
    return QVector3D(first.xyz[0], first.xyz[1], first.xyz[2]);
}

static Vertex QVector3DToVertex(const QVector3D &v)
{
    Vertex vertex;
    vertex.xyz[0] = v.x();
    vertex.xyz[1] = v.y();
    vertex.xyz[2] = v.z();
    return vertex;
}

float distanceSquaredOfVertices(const Vertex &first, const Vertex &second)
{
    return (vertexToQVector3D(first) - vertexToQVector3D(second)).lengthSquared();
}

float distanceOfVertices(const Vertex &first, const Vertex &second)
{
    return (vertexToQVector3D(first) - vertexToQVector3D(second)).length();
}

float isNull(float number)
{
    return qFuzzyIsNull(number);
}

void projectToPlane(const Vector &planeNormal, const Vertex &planeOrigin,
    const Vector &planeX, const std::vector<Vertex> &points,
    std::vector<Vertex> &result)
{
    QVector3D normal = vertexToQVector3D(planeNormal);
    QVector3D origin = vertexToQVector3D(planeOrigin);
    QVector3D xAxis = vertexToQVector3D(planeX);
    QVector3D yAxis = QVector3D::crossProduct(normal, xAxis);
    for (const auto &it: points) {
        Vertex point2D;
        QVector3D direction = vertexToQVector3D(it) - origin;
        point2D.xyz[0] = QVector3D::dotProduct(direction, xAxis) * 1000;
        point2D.xyz[1] = QVector3D::dotProduct(direction, yAxis) * 1000;
        result.push_back(point2D);
    }
}

bool pointInPolygon2D(const Vertex &vertex, const std::vector<Vertex> &ring)
{
    QVector<QPoint> points;
    for (const auto &it: ring) {
        QPoint point(it.xyz[0], it.xyz[1]);
        points.push_back(point);
    }
    QPolygon polygon(points);
    return polygon.containsPoint(QPoint(vertex.xyz[0], vertex.xyz[1]), Qt::WindingFill);
}

void averageOfPoints2D(const std::vector<Vertex> &points, Vertex &result)
{
    Vertex sum;
    sum.xyz[0] = 0;
    sum.xyz[1] = 0;
    for (const auto &it: points) {
        sum.xyz[0] += it.xyz[0];
        sum.xyz[1] += it.xyz[1];
    }
    result.xyz[0] = 0;
    result.xyz[1] = 0;
    if (points.empty())
        return;
    result.xyz[0] = sum.xyz[0] / points.size();
    result.xyz[1] = sum.xyz[1] / points.size();
}

void triangleNormal(const Vertex &first, const Vertex &second, const Vertex &third, Vertex &normal)
{
    QVector3D v1 = vertexToQVector3D(first);
    QVector3D v2 = vertexToQVector3D(second);
    QVector3D v3 = vertexToQVector3D(third);
    normal = QVector3DToVertex(QVector3D::normal(v1, v2, v3));
}

void directionBetweenTwoVertices(const Vertex &from, const Vertex &to, Vector &direction)
{
    QVector3D v1 = vertexToQVector3D(from);
    QVector3D v2 = vertexToQVector3D(to);
    direction = QVector3DToVertex((v1 - v2).normalized());
}

}
