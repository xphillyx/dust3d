#include "skinnedmesh.h"

SkinnedMesh::SkinnedMesh(const MeshResultContext &resultContext, const JointNodeTree &jointNodeTree) :
    m_resultContext(resultContext),
    m_jointNodeTree(new JointNodeTree(jointNodeTree))
{
    fromMeshResultContext(m_resultContext);
}

SkinnedMesh::~SkinnedMesh()
{
    delete m_jointNodeTree;
}

void SkinnedMesh::fromMeshResultContext(MeshResultContext &resultContext)
{
    m_vertices.clear();
    m_triangles.clear();
    for (const auto &part: resultContext.parts()) {
        std::map<int, int> oldToNewIndexMap;
        for (int vertexIndex = 0; vertexIndex < (int)part.second.vertices.size(); vertexIndex++) {
            const ResultVertex *srcVert = &part.second.vertices[vertexIndex];
            const QVector3D *srcNormal = &part.second.interpolatedVertexNormals[vertexIndex];
            SkinnedMeshVertex vertex;
            vertex.position = vertex.posPosition = srcVert->position;
            vertex.normal = vertex.posNormal = *srcNormal;
            const auto &weights = part.second.weights[vertexIndex];
            for (int i = 0; i < (int)weights.size() && i < MAX_WEIGHT_NUM; i++) {
                vertex.weights[i].amount = weights[i].weight;
                vertex.weights[i].jointIndex = m_jointNodeTree->nodeToJointIndex(weights[i].sourceNode.first, weights[i].sourceNode.second);
            }
            for (int i = weights.size(); i < MAX_WEIGHT_NUM; i++) {
                vertex.weights[i].amount = 0;
                vertex.weights[i].jointIndex = 0;
            }
            oldToNewIndexMap[vertexIndex] = m_vertices.size();
            m_vertices.push_back(vertex);
        }
        for (const auto &it: part.second.triangles) {
            SkinnedMeshTriangle triangle;
            for (auto i = 0; i < 3; i++) {
                int vertexIndex = it.indicies[i];
                triangle.indicies[i] = oldToNewIndexMap[vertexIndex];
            }
            triangle.color = part.second.color;
            m_triangles.push_back(triangle);
        }
    }
}

void SkinnedMesh::frameToMatrices(const RigFrame &frame, std::vector<QMatrix4x4> &matrices)
{
    if (m_jointNodeTree->joints().empty())
        return;
    matrices.clear();
    matrices.resize(m_jointNodeTree->joints().size());

    frameToMatricesAtJoint(frame, matrices, 0, QMatrix4x4());
}

void SkinnedMesh::frameToMatricesAtJoint(const RigFrame &frame, std::vector<QMatrix4x4> &matrices, int jointIndex, const QMatrix4x4 &parentWorldMatrix)
{
    const auto &joint = m_jointNodeTree->joints()[jointIndex];
    
    QMatrix4x4 translateMatrix;
    if (frame.translatedIndicies.find(jointIndex) != frame.translatedIndicies.end())
        translateMatrix.translate(frame.translations[jointIndex]);
    else
        translateMatrix.translate(joint.translation);
    
    QMatrix4x4 rotateMatrix;
    if (frame.rotatedIndicies.find(jointIndex) != frame.rotatedIndicies.end())
        rotateMatrix.rotate(frame.rotations[jointIndex]);
    else
        rotateMatrix.rotate(joint.rotation);
    
    QMatrix4x4 scaleMatrix;
    if (frame.scaledIndicies.find(jointIndex) != frame.scaledIndicies.end())
        scaleMatrix.scale(frame.scales[jointIndex]);
    else
        scaleMatrix.scale(joint.scale);
    
    QMatrix4x4 worldMatrix = parentWorldMatrix * translateMatrix * rotateMatrix * scaleMatrix;
    matrices[jointIndex] = worldMatrix * joint.inverseBindMatrix;
    
    for (const auto &child: joint.children) {
        frameToMatricesAtJoint(frame, matrices, child, worldMatrix);
    }
}

void SkinnedMesh::applyRigFrameToMesh(const RigFrame &frame)
{
    std::vector<QMatrix4x4> matrices;
    frameToMatrices(frame, matrices);
    for (auto &vert: m_vertices) {
        QMatrix4x4 matrix;
        for (int i = 0; i < MAX_WEIGHT_NUM; i++) {
            matrix += matrices[vert.weights[i].jointIndex] * vert.weights[i].amount;
        }
        vert.position = matrix * vert.posPosition;
        vert.normal = (matrix * vert.posNormal).normalized();
    }
}

MeshLoader *SkinnedMesh::toMeshLoader()
{
    int vertexNum = m_triangles.size() * 3;
    Vertex *triangleVertices = new Vertex[vertexNum];
    int destIndex = 0;
    for (const auto &triangle: m_triangles) {
        for (auto i = 0; i < 3; i++) {
            int srcVertexIndex = triangle.indicies[i];
            const SkinnedMeshVertex *srcVertex = &m_vertices[srcVertexIndex];
            Vertex *dest = &triangleVertices[destIndex];
            dest->colorR = triangle.color.redF();
            dest->colorG = triangle.color.greenF();
            dest->colorB = triangle.color.blueF();
            dest->texU = 0;
            dest->texV = 0;
            dest->normX = srcVertex->normal.x();
            dest->normY = srcVertex->normal.y();
            dest->normZ = srcVertex->normal.z();
            dest->posX = srcVertex->position.x();
            dest->posY = srcVertex->position.y();
            dest->posZ = srcVertex->position.z();
            destIndex++;
        }
    }
    return new MeshLoader(triangleVertices, vertexNum);
}

