#include <vector>
#include <QGuiApplication>
#include "meshgenerator.h"
#include "dust3dutil.h"
#include "skeletondocument.h"
#include "meshlite.h"
#include "modelofflinerender.h"
#include "meshutil.h"
#include "theme.h"
#include "positionmap.h"

bool MeshGenerator::m_enableDebug = false;

MeshGenerator::MeshGenerator(SkeletonSnapshot *snapshot, QThread *thread) :
    m_snapshot(snapshot),
    m_mesh(nullptr),
    m_preview(nullptr),
    m_requirePreview(false),
    m_previewRender(nullptr),
    m_thread(thread),
    m_meshResultContext(nullptr),
    m_sharedContextWidget(nullptr),
    m_cacheContext(nullptr)
{
}

MeshGenerator::~MeshGenerator()
{
    delete m_snapshot;
    delete m_mesh;
    delete m_preview;
    for (const auto &partPreviewIt: m_partPreviewMap) {
        delete partPreviewIt.second;
    }
    for (const auto &render: m_partPreviewRenderMap) {
        delete render.second;
    }
    delete m_previewRender;
    delete m_meshResultContext;
}

void MeshGenerator::setGeneratedCacheContext(GeneratedCacheContext *cacheContext)
{
    m_cacheContext = cacheContext;
}

void MeshGenerator::addPreviewRequirement()
{
    m_requirePreview = true;
    if (nullptr == m_previewRender) {
        m_previewRender = new ModelOfflineRender(m_sharedContextWidget);
        m_previewRender->setRenderThread(m_thread);
    }
}

void MeshGenerator::addPartPreviewRequirement(const QString &partId)
{
    //qDebug() << "addPartPreviewRequirement:" << partId;
    m_requirePartPreviewMap.insert(partId);
    if (m_partPreviewRenderMap.find(partId) == m_partPreviewRenderMap.end()) {
        ModelOfflineRender *render = new ModelOfflineRender(m_sharedContextWidget);
        render->setRenderThread(m_thread);
        m_partPreviewRenderMap[partId] = render;
    }
}

void MeshGenerator::setSharedContextWidget(QOpenGLWidget *widget)
{
    m_sharedContextWidget = widget;
}

MeshLoader *MeshGenerator::takeResultMesh()
{
    MeshLoader *resultMesh = m_mesh;
    m_mesh = nullptr;
    return resultMesh;
}

QImage *MeshGenerator::takePreview()
{
    QImage *resultPreview = m_preview;
    m_preview = nullptr;
    return resultPreview;
}

QImage *MeshGenerator::takePartPreview(const QString &partId)
{
    QImage *resultImage = m_partPreviewMap[partId];
    m_partPreviewMap[partId] = nullptr;
    return resultImage;
}

MeshResultContext *MeshGenerator::takeMeshResultContext()
{
    MeshResultContext *meshResultContext = m_meshResultContext;
    m_meshResultContext = nullptr;
    return meshResultContext;
}

void MeshGenerator::loadVertexSourcesToMeshResultContext(void *meshliteContext, int meshId, QUuid partId, const std::map<int, QUuid> &bmeshToNodeIdMap)
{
    int vertexCount = meshlite_get_vertex_count(meshliteContext, meshId);
    int positionBufferLen = vertexCount * 3;
    float *positionBuffer = new float[positionBufferLen];
    int positionCount = meshlite_get_vertex_position_array(meshliteContext, meshId, positionBuffer, positionBufferLen) / 3;
    int *sourceBuffer = new int[positionBufferLen];
    int sourceCount = meshlite_get_vertex_source_array(meshliteContext, meshId, sourceBuffer, positionBufferLen);
    Q_ASSERT(positionCount == sourceCount);
    for (int i = 0, positionIndex = 0; i < positionCount; i++, positionIndex+=3) {
        BmeshVertex vertex;
        vertex.partId = partId;
        auto findNodeId = bmeshToNodeIdMap.find(sourceBuffer[i]);
        if (findNodeId != bmeshToNodeIdMap.end())
            vertex.nodeId = findNodeId->second;
        vertex.position = QVector3D(positionBuffer[positionIndex + 0], positionBuffer[positionIndex + 1], positionBuffer[positionIndex + 2]);
        m_meshResultContext->bmeshVertices.push_back(vertex);
    }
    delete[] positionBuffer;
    delete[] sourceBuffer;
}

void MeshGenerator::loadGeneratedPositionsToMeshResultContext(void *meshliteContext, int triangulatedMeshId)
{
    int vertexCount = meshlite_get_vertex_count(meshliteContext, triangulatedMeshId);
    int positionBufferLen = vertexCount * 3;
    float *positionBuffer = new float[positionBufferLen];
    int positionCount = meshlite_get_vertex_position_array(meshliteContext, triangulatedMeshId, positionBuffer, positionBufferLen) / 3;
    std::map<int, int> verticesMap;
    for (int i = 0, positionIndex = 0; i < positionCount; i++, positionIndex+=3) {
        ResultVertex vertex;
        vertex.position = QVector3D(positionBuffer[positionIndex + 0], positionBuffer[positionIndex + 1], positionBuffer[positionIndex + 2]);
        verticesMap[i] = m_meshResultContext->vertices.size();
        m_meshResultContext->vertices.push_back(vertex);
    }
    int faceCount = meshlite_get_face_count(meshliteContext, triangulatedMeshId);
    int triangleIndexBufferLen = faceCount * 3;
    int *triangleIndexBuffer = new int[triangleIndexBufferLen];
    int triangleCount = meshlite_get_triangle_index_array(meshliteContext, triangulatedMeshId, triangleIndexBuffer, triangleIndexBufferLen) / 3;
    int triangleNormalBufferLen = faceCount * 3;
    float *normalBuffer = new float[triangleNormalBufferLen];
    int normalCount = meshlite_get_triangle_normal_array(meshliteContext, triangulatedMeshId, normalBuffer, triangleNormalBufferLen) / 3;
    Q_ASSERT(triangleCount == normalCount);
    for (int i = 0, triangleVertIndex = 0, normalIndex=0; i < triangleCount; i++, triangleVertIndex+=3, normalIndex += 3) {
        ResultTriangle triangle;
        triangle.indicies[0] = verticesMap[triangleIndexBuffer[triangleVertIndex + 0]];
        triangle.indicies[1] = verticesMap[triangleIndexBuffer[triangleVertIndex + 1]];
        triangle.indicies[2] = verticesMap[triangleIndexBuffer[triangleVertIndex + 2]];
        triangle.normal = QVector3D(normalBuffer[normalIndex + 0], normalBuffer[normalIndex + 1], normalBuffer[normalIndex + 2]);
        m_meshResultContext->triangles.push_back(triangle);
    }
    delete[] positionBuffer;
    delete[] triangleIndexBuffer;
    delete[] normalBuffer;
}

void MeshGenerator::collectParts()
{
    for (const auto &node: m_snapshot->nodes) {
        QString partId = valueOfKeyInMapOrEmpty(node.second, "partId");
        if (partId.isEmpty())
            continue;
        m_partNodeIds[partId].insert(node.first);
    }
    for (const auto &edge: m_snapshot->edges) {
        QString partId = valueOfKeyInMapOrEmpty(edge.second, "partId");
        if (partId.isEmpty())
            continue;
        m_partEdgeIds[partId].insert(edge.first);
    }
}

void *MeshGenerator::combinePartMesh(QString partId)
{
    auto findPart = m_snapshot->parts.find(partId);
    if (findPart == m_snapshot->parts.end()) {
        qDebug() << "Find part failed:" << partId;
        return nullptr;
    }
    auto &part = findPart->second;
    bool isDisabled = isTrueValueString(valueOfKeyInMapOrEmpty(part, "disabled"));
    bool xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(part, "xMirrored"));
    bool subdived = isTrueValueString(valueOfKeyInMapOrEmpty(part, "subdived"));
    int bmeshId = meshlite_bmesh_create(m_meshliteContext);
    if (subdived)
        meshlite_bmesh_set_cut_subdiv_count(m_meshliteContext, bmeshId, 1);
    if (isTrueValueString(valueOfKeyInMapOrEmpty(part, "rounded")))
        meshlite_bmesh_set_round_way(m_meshliteContext, bmeshId, 1);

    QString colorString = valueOfKeyInMapOrEmpty(part, "color");
    QColor partColor = colorString.isEmpty() ? Theme::white : QColor(colorString);

    QString thicknessString = valueOfKeyInMapOrEmpty(part, "deformThickness");
    if (!thicknessString.isEmpty())
        meshlite_bmesh_set_deform_thickness(m_meshliteContext, bmeshId, thicknessString.toFloat());
    QString widthString = valueOfKeyInMapOrEmpty(part, "deformWidth");
    if (!widthString.isEmpty())
        meshlite_bmesh_set_deform_width(m_meshliteContext, bmeshId, widthString.toFloat());
    if (MeshGenerator::m_enableDebug)
        meshlite_bmesh_enable_debug(m_meshliteContext, bmeshId, 1);
    
    QString mirroredPartId;
    if (xMirrored)
        mirroredPartId = QUuid().createUuid().toString();
    
    std::map<QString, int> nodeToBmeshIdMap;
    std::map<int, QUuid> bmeshToNodeIdMap;
    for (const auto &nodeId: m_partNodeIds[partId]) {
        auto findNode = m_snapshot->nodes.find(nodeId);
        if (findNode == m_snapshot->nodes.end()) {
            qDebug() << "Find node failed:" << nodeId;
            continue;
        }
        auto &node = findNode->second;
        
        float radius = valueOfKeyInMapOrEmpty(node, "radius").toFloat();
        float x = (valueOfKeyInMapOrEmpty(node, "x").toFloat() - m_mainProfileMiddleX);
        float y = (m_mainProfileMiddleY - valueOfKeyInMapOrEmpty(node, "y").toFloat());
        float z = (m_sideProfileMiddleX - valueOfKeyInMapOrEmpty(node, "z").toFloat());
        int bmeshNodeId = meshlite_bmesh_add_node(m_meshliteContext, bmeshId, x, y, z, radius);

        nodeToBmeshIdMap[nodeId] = bmeshNodeId;
        bmeshToNodeIdMap[bmeshNodeId] = nodeId;
        
        BmeshNode bmeshNode;
        bmeshNode.partId = QUuid(partId);
        bmeshNode.origin = QVector3D(x, y, z);
        bmeshNode.radius = radius;
        bmeshNode.nodeId = QUuid(nodeId);
        bmeshNode.color = partColor;
        m_meshResultContext->bmeshNodes.push_back(bmeshNode);
        
        if (xMirrored) {
            bmeshNode.partId = mirroredPartId;
            bmeshNode.origin.setX(-x);
            m_meshResultContext->bmeshNodes.push_back(bmeshNode);
        }
    }
    
    for (const auto &edgeId: m_partEdgeIds[partId]) {
        auto findEdge = m_snapshot->edges.find(edgeId);
        if (findEdge == m_snapshot->edges.end()) {
            qDebug() << "Find edge failed:" << edgeId;
            continue;
        }
        auto &edge = findEdge->second;
        
        QString fromNodeId = valueOfKeyInMapOrEmpty(edge, "from");
        QString toNodeId = valueOfKeyInMapOrEmpty(edge, "to");
        
        auto findFromBmeshId = nodeToBmeshIdMap.find(fromNodeId);
        if (findFromBmeshId == nodeToBmeshIdMap.end()) {
            qDebug() << "Find from-node bmesh failed:" << fromNodeId;
            continue;
        }
        
        auto findToBmeshId = nodeToBmeshIdMap.find(toNodeId);
        if (findToBmeshId == nodeToBmeshIdMap.end()) {
            qDebug() << "Find to-node bmesh failed:" << toNodeId;
            continue;
        }
        
        meshlite_bmesh_add_edge(m_meshliteContext, bmeshId, findFromBmeshId->second, findToBmeshId->second);
    }
    
    int meshId = 0;
    void *resultMesh = nullptr;
    if (!bmeshToNodeIdMap.empty()) {
        meshId = meshlite_bmesh_generate_mesh(m_meshliteContext, bmeshId);
        loadVertexSourcesToMeshResultContext(m_meshliteContext, meshId, QUuid(partId), bmeshToNodeIdMap);
        resultMesh = convertToCombinableMesh(m_meshliteContext, meshlite_triangulate(m_meshliteContext, meshId));
    }
    
    if (nullptr != resultMesh) {
        if (xMirrored) {
            int xMirroredMeshId = meshlite_mirror_in_x(m_meshliteContext, meshId, 0);
            loadVertexSourcesToMeshResultContext(m_meshliteContext, xMirroredMeshId, QUuid(mirroredPartId), bmeshToNodeIdMap);
            void *mirroredMesh = convertToCombinableMesh(m_meshliteContext, meshlite_triangulate(m_meshliteContext, xMirroredMeshId));
            if (nullptr != mirroredMesh) {
                void *newResultMesh = unionCombinableMeshs(resultMesh, mirroredMesh);
                deleteCombinableMesh(mirroredMesh);
                if (nullptr != newResultMesh) {
                    deleteCombinableMesh(resultMesh);
                    resultMesh = newResultMesh;
                }
            }
        }
    }
    
    if (m_requirePartPreviewMap.find(partId) != m_requirePartPreviewMap.end()) {
        ModelOfflineRender *render = m_partPreviewRenderMap[partId];
        int trimedMeshId = meshlite_trim(m_meshliteContext, meshId, 1);
        render->updateMesh(new MeshLoader(m_meshliteContext, trimedMeshId, -1, partColor));
        QImage *image = new QImage(render->toImage(QSize(Theme::previewImageRenderSize, Theme::previewImageRenderSize)));
        if (Theme::previewImageSize != Theme::previewImageRenderSize) {
            int cropOffset = (Theme::previewImageRenderSize - Theme::previewImageSize) / 2;
            QImage *crop = new QImage(image->copy(cropOffset, cropOffset, Theme::previewImageSize, Theme::previewImageSize));
            delete image;
            image = crop;
        }
        m_partPreviewMap[partId] = image;
    }
    
    if (isDisabled) {
        if (nullptr != resultMesh) {
            deleteCombinableMesh(resultMesh);
            resultMesh = nullptr;
        }
    }
    
    return resultMesh;
}

void *MeshGenerator::combineComponentMesh(QString componentId, bool *inverse)
{
    *inverse = false;
    
    const std::map<QString, QString> *component = &m_snapshot->rootComponent;
    if (componentId != QUuid().toString()) {
        auto findComponent = m_snapshot->components.find(componentId);
        if (findComponent == m_snapshot->components.end()) {
            qDebug() << "Component not found:" << componentId;
            return nullptr;
        }
        component = &findComponent->second;
    }
    
    if (isTrueValueString(valueOfKeyInMapOrEmpty(*component, "inverse")))
        *inverse = true;
    
    QString linkDataType = valueOfKeyInMapOrEmpty(*component, "linkDataType");
    if ("partId" == linkDataType) {
        QString partId = valueOfKeyInMapOrEmpty(*component, "linkData");
        return combinePartMesh(partId);
    }
    
    void *resultMesh = nullptr;
    
    for (const auto &childId: valueOfKeyInMapOrEmpty(*component, "children").split(",")) {
        bool childInverse = false;
        void *childCombinedMesh = combineComponentMesh(childId, &childInverse);
        if (nullptr == childCombinedMesh)
            continue;
        if (nullptr == resultMesh) {
            if (childInverse) {
                deleteCombinableMesh(childCombinedMesh);
            } else {
                resultMesh = childCombinedMesh;
            }
        } else {
            void *newResultMesh = childInverse ? diffCombinableMeshs(resultMesh, childCombinedMesh) : unionCombinableMeshs(resultMesh, childCombinedMesh);
            deleteCombinableMesh(childCombinedMesh);
            if (nullptr != newResultMesh) {
                deleteCombinableMesh(resultMesh);
                resultMesh = newResultMesh;
            }
        }
    }
    
    return resultMesh;
}

void MeshGenerator::process()
{
    if (nullptr == m_snapshot)
        return;
    
    m_meshliteContext = meshlite_create_context();
    
    initMeshUtils();
    m_meshResultContext = new MeshResultContext;
    
    collectParts();
    
    m_mainProfileMiddleX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originX").toFloat();
    m_mainProfileMiddleY = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originY").toFloat();
    m_sideProfileMiddleX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originZ").toFloat();
    
    int resultMeshId = 0;
    
    bool inverse = false;
    void *combinedMesh = combineComponentMesh(QUuid().toString(), &inverse);
    if (nullptr != combinedMesh) {
        resultMeshId = convertFromCombinableMesh(m_meshliteContext, combinedMesh);
        deleteCombinableMesh(combinedMesh);
    }
    
    if (resultMeshId > 0) {
        resultMeshId = meshlite_combine_coplanar_faces(m_meshliteContext, resultMeshId);
        if (resultMeshId > 0)
            resultMeshId = meshlite_fix_hole(m_meshliteContext, resultMeshId);
    }
    
    if (resultMeshId > 0) {
        int triangulatedFinalMeshId = meshlite_triangulate(m_meshliteContext, resultMeshId);
        loadGeneratedPositionsToMeshResultContext(m_meshliteContext, triangulatedFinalMeshId);
        m_mesh = new MeshLoader(m_meshliteContext, resultMeshId, triangulatedFinalMeshId, Theme::white, &m_meshResultContext->triangleColors());
    }
    
    meshlite_destroy_context(m_meshliteContext);
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}


