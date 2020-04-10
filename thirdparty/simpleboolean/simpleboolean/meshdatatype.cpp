#include <QFile>
#include <QTextStream>
#include "meshdatatype.h"

namespace simpleboolean
{

static QString getVertexIndexToken(const QString &tokens)
{
    auto subTokens = tokens.split('/');
    if (subTokens.empty())
        return "";
    return subTokens[0];
}

bool loadTriangulatedObj(Mesh &mesh, const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            auto tokens = line.split(' ');
            if (tokens.empty())
                continue;
            if (tokens[0] == "v") {
                Vertex vertex;
                if (tokens.size() != 4)
                    return false;
                vertex.xyz[0] = tokens[1].toFloat();
                vertex.xyz[1] = -tokens[2].toFloat();
                vertex.xyz[2] = -tokens[3].toFloat();
                mesh.vertices.push_back(vertex);
            } else if (tokens[0] == "f") {
                Face face;
                if (tokens.size() == 4) {
                    face.indices[0] = getVertexIndexToken(tokens[1]).toInt() - 1;
                    face.indices[1] = getVertexIndexToken(tokens[2]).toInt() - 1;
                    face.indices[2] = getVertexIndexToken(tokens[3]).toInt() - 1;
                    mesh.faces.push_back(face);
                } else if (tokens.size() == 5) {
                    face.indices[0] = getVertexIndexToken(tokens[1]).toInt() - 1;
                    face.indices[1] = getVertexIndexToken(tokens[2]).toInt() - 1;
                    face.indices[2] = getVertexIndexToken(tokens[3]).toInt() - 1;
                    mesh.faces.push_back(face);
                    
                    face.indices[0] = getVertexIndexToken(tokens[3]).toInt() - 1;
                    face.indices[1] = getVertexIndexToken(tokens[4]).toInt() - 1;
                    face.indices[2] = getVertexIndexToken(tokens[1]).toInt() - 1;
                    mesh.faces.push_back(face);
                }
            }
        }
       file.close();
       return true;
    }
    
    return false;
}

void exportTriangulatedObj(const Mesh &mesh, const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        for (std::vector<Vertex>::const_iterator it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
            stream << "v " << (*it).xyz[0] << " " << -(*it).xyz[1] << " " << -(*it).xyz[2] << endl;
        }
        for (std::vector<Face>::const_iterator it = mesh.faces.begin(); it != mesh.faces.end(); ++it) {
            stream << "f";
            stream << " " << (1 + (*it).indices[0]) << " " << (1 + (*it).indices[1]) << " " << (1 + (*it).indices[2]);
            stream << endl;
        }
    }
}

void exportEdgeLoopsAsObj(const std::vector<Vertex> &vertices,
        const std::vector<std::vector<size_t>> &edgeLoops,
        const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        for (std::vector<Vertex>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
            stream << "v " << (*it).xyz[0] << " " << -(*it).xyz[1] << " " << -(*it).xyz[2] << endl;
        }
        for (std::vector<std::vector<size_t>>::const_iterator it = edgeLoops.begin(); it != edgeLoops.end(); ++it) {
            stream << "f";
            for (std::vector<size_t>::const_iterator subIt = it->begin(); subIt != it->end(); ++subIt) {
                stream << " " << (1 + *subIt);
            }
            stream << endl;
        }
    }
}

}
