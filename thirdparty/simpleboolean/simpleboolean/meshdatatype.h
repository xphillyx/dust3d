#ifndef SIMPLEBOOLEAN_MESH_DATA_TYPE_H
#define SIMPLEBOOLEAN_MESH_DATA_TYPE_H
#include <vector>
#include <cstdlib>
#include <QString>
#include <QDebug>

namespace simpleboolean
{

class Vector
{
public:
    double xyz[3] = {0.0f, 0.0f, 0.0f};
    
    Vector()
    {
    }
    
    Vector(float x, float y, float z)
    {
        xyz[0] = x;
        xyz[1] = y;
        xyz[2] = z;
    }
    
    Vector &operator+=(const Vector &other)
    {
        for (size_t i = 0; i < 3; ++i)
            xyz[i] += other.xyz[i];
        return *this;
    }
    
    Vector &operator*=(float factor)
    {
        for (size_t i = 0; i < 3; ++i)
            xyz[i] *= factor;
        return *this;
    }
    
    Vector &operator/=(float factor)
    {
        for (size_t i = 0; i < 3; ++i)
            xyz[i] /= factor;
        return *this;
    }
};

typedef Vector Vertex;

struct Face
{
    size_t indices[3];
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

bool loadTriangulatedObj(Mesh &mesh, const QString &filename);
void exportTriangulatedObj(const Mesh &mesh, const QString &filename);
void exportEdgeLoopsAsObj(const std::vector<Vertex> &vertices,
        const std::vector<std::vector<size_t>> &edgeLoops,
        const QString &filename);
  
}

#endif

