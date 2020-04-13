#include <tin.h>
#include <QDebug>
#include <QElapsedTimer>
#include "fixmesh.h"

using namespace T_MESH;

static void toTmesh(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles, Basic_TMesh &tin)
{
    for (const auto &it: vertices)
        tin.V.appendTail(new Vertex(it.x(), it.y(), it.z()));
    
    Vertex *v = nullptr;
    Node *n = nullptr;
    int nv = tin.V.numels();
    ExtVertex **var = (ExtVertex **)malloc(sizeof(ExtVertex *) * nv);
    int i = 0;
    for (n = tin.V.head(), v = (n)?((Vertex *)n->data):NULL; n != NULL; n=n->next(), v = (n)?((Vertex *)n->data):NULL) {
        var[i++] = new ExtVertex(v);
    }
    for (const auto &it: triangles) {
        if (3 != it.size())
            continue;
        tin.CreateIndexedTriangle(var, (int)it[0], (int)it[1], (int)it[2]);
    }
    
    for (int i= 0; i < nv; ++i)
        delete var[i];
    free(var);
}

#define TVI1(a) (TMESH_TO_INT(((Triangle *)a->data)->v1()->x))
#define TVI2(a) (TMESH_TO_INT(((Triangle *)a->data)->v2()->x))
#define TVI3(a) (TMESH_TO_INT(((Triangle *)a->data)->v3()->x))

static void fromTmesh(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &triangles, const Basic_TMesh &tin)
{
    Vertex *v = nullptr;
    Node *n = nullptr;
    size_t startIndex = vertices.size();
    for (n = tin.V.head(), v = (n)?((Vertex *)n->data):NULL; n != NULL; n=n->next(), v = (n)?((Vertex *)n->data):NULL) {
        vertices.push_back(QVector3D(TMESH_TO_FLOAT(v->x), TMESH_TO_FLOAT(v->y), TMESH_TO_FLOAT(v->z)));
    }
    for ((n) = (tin.T).head(); (n) != NULL; (n)=(n)->next()) {
        triangles.push_back(std::vector<size_t> {startIndex + (size_t)TVI1(n),
            startIndex + (size_t)TVI2(n),
            startIndex + (size_t)TVI3(n)
        });
    }
}

bool fixMesh(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &triangles)
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    Basic_TMesh tin;
    bool fixed = true;
    
    toTmesh(vertices, triangles, tin);
    
    // Keep only the largest component (i.e. with most triangles)
    int smallComponents = tin.removeSmallestComponents();
    if (smallComponents) {
        qDebug() << "Removed small components:" << smallComponents;
    }
    
    // Fill holes
    if (tin.boundaries()) {
        tin.fillSmallBoundaries(0, true);
    }

    if (tin.boundaries() || !tin.meshclean()) {
        qDebug() << "MeshFix could not fix everything";
        fixed = false;
    }
    
    if (fixed) {
        vertices.clear();
        triangles.clear();
        fromTmesh(vertices, triangles, tin);
    }
    
    qDebug() << "Mesh fixing took" << countTimeConsumed.elapsed() << "milliseconds";
    
    return fixed;
}
