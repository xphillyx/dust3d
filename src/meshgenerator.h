#ifndef MESH_GENERATOR_H
#define MESH_GENERATOR_H
#include <QObject>
#include <QString>
#include <QImage>
#include <map>
#include <set>
#include <QThread>
#include <QOpenGLWidget>
#include "skeletonsnapshot.h"
#include "meshloader.h"
#include "modelofflinerender.h"
#include "meshresultcontext.h"

struct GeneratedPartCache
{
    std::vector<BmeshVertex> bmeshVertices;
    void *triangulatedMesh = nullptr;
};

struct GeneratedComponentCache
{
    void *triangulatedMesh = nullptr;
};

struct GeneratedCacheContext
{
    std::map<QUuid, GeneratedPartCache> partCaches;
    std::map<QUuid, GeneratedComponentCache> componentCaches;
};

class MeshGenerator : public QObject
{
    Q_OBJECT
public:
    MeshGenerator(SkeletonSnapshot *snapshot, QThread *thread);
    ~MeshGenerator();
    void setSharedContextWidget(QOpenGLWidget *widget);
    void addPreviewRequirement();
    void addPartPreviewRequirement(const QString &partId);
    void setGeneratedCacheContext(GeneratedCacheContext *cacheContext);
    MeshLoader *takeResultMesh();
    QImage *takePreview();
    QImage *takePartPreview(const QString &partId);
    MeshResultContext *takeMeshResultContext();
signals:
    void finished();
public slots:
    void process();
private:
    SkeletonSnapshot *m_snapshot;
    MeshLoader *m_mesh;
    QImage *m_preview;
    std::map<QString, QImage *> m_partPreviewMap;
    bool m_requirePreview;
    std::set<QString> m_requirePartPreviewMap;
    ModelOfflineRender *m_previewRender;
    std::map<QString, ModelOfflineRender *> m_partPreviewRenderMap;
    QThread *m_thread;
    MeshResultContext *m_meshResultContext;
    QOpenGLWidget *m_sharedContextWidget;
    void *m_meshliteContext;
    GeneratedCacheContext *m_cacheContext;
    float m_mainProfileMiddleX;
    float m_sideProfileMiddleX;
    float m_mainProfileMiddleY;
    std::map<QString, std::set<QString>> m_partNodeIds;
    std::map<QString, std::set<QString>> m_partEdgeIds;
private:
    static bool m_enableDebug;
private:
    void loadVertexSourcesToMeshResultContext(void *meshliteContext, int meshId, QUuid partId, const std::map<int, QUuid> &bmeshToNodeIdMap);
    void loadGeneratedPositionsToMeshResultContext(void *meshliteContext, int triangulatedMeshId);
    void collectParts();
    void *combineComponentMesh(QString componentId, bool *inverse);
    void *combinePartMesh(QString partId);
};

#endif
