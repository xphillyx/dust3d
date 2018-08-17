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

class MeshGenerator : public QObject
{
    Q_OBJECT
public:
    MeshGenerator(SkeletonSnapshot *snapshot, QThread *thread);
    ~MeshGenerator();
    void setSharedContextWidget(QOpenGLWidget *widget);
    void addPreviewRequirement();
    void addPartPreviewRequirement(const QString &partId);
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
    std::map<QString, int> m_partBmeshMap;
    std::map<QString, QColor> m_partColorMap;
private:
    static bool m_enableDebug;
private:
    void resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId=QString());
    void loadVertexSourcesToMeshResultContext(void *meshliteContext, int meshId, int bmeshId);
    void loadGeneratedPositionsToMeshResultContext(void *meshliteContext, int triangulatedMeshId);
    int generateComponent(QUuid componentId);
};

#endif
