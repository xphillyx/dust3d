#ifndef DUST3D_MODEL_OFFLINE_RENDER_H
#define DUST3D_MODEL_OFFLINE_RENDER_H
#include <QOffscreenSurface>
#include <QScreen>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QImage>
#include <QThread>
#include "modelshaderprogram.h"
#include "modelmeshbinder.h"
#include "meshloader.h"

class ModelOfflineRender : QOffscreenSurface
{
public:
    ModelOfflineRender(const QSurfaceFormat &format, QScreen *targetScreen = Q_NULLPTR);
    ~ModelOfflineRender();
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setRenderThread(QThread *thread);
    void updateMesh(MeshLoader *mesh);
    QImage toImage(const QSize &size);
private:
    int m_xRot = 0;
    int m_yRot = 0;
    int m_zRot = 0;
    QOpenGLContext *m_context = nullptr;
    MeshLoader *m_mesh = nullptr;
};

#endif
