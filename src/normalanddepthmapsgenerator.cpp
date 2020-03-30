#include "normalanddepthmapsgenerator.h"

NormalAndDepthMapsGenerator::NormalAndDepthMapsGenerator(ModelWidget *modelWidget)
{
    m_viewPortSize = modelWidget->size();
    m_normalMapRender = createOfflineRender(modelWidget, 1);
    m_depthMapRender = createOfflineRender(modelWidget, 2);
}

void NormalAndDepthMapsGenerator::updateMesh(MeshLoader *mesh)
{
    if (nullptr == mesh) {
        m_normalMapRender->updateMesh(nullptr);
        m_depthMapRender->updateMesh(nullptr);
        return;
    }
    m_normalMapRender->updateMesh(new MeshLoader(*mesh));
    m_depthMapRender->updateMesh(mesh);
}

void NormalAndDepthMapsGenerator::setRenderThread(QThread *thread)
{
    m_normalMapRender->setRenderThread(thread);
    m_depthMapRender->setRenderThread(thread);
}

ModelOfflineRender *NormalAndDepthMapsGenerator::createOfflineRender(ModelWidget *modelWidget, int purpose)
{
    ModelOfflineRender *offlineRender = new ModelOfflineRender(modelWidget->format());
    offlineRender->setXRotation(modelWidget->xRot());
    offlineRender->setYRotation(modelWidget->yRot());
    offlineRender->setZRotation(modelWidget->zRot());
    offlineRender->setRenderPurpose(purpose);
    return offlineRender;
}

NormalAndDepthMapsGenerator::~NormalAndDepthMapsGenerator()
{
    delete m_normalMapRender;
    delete m_depthMapRender;
    delete m_normalMap;
    delete m_depthMap;
}

void NormalAndDepthMapsGenerator::generate()
{
    QImage normalMapImage = m_normalMapRender->toImage(m_viewPortSize);
    m_normalMap = new QOpenGLTexture(normalMapImage);
    
    QImage depthMapImage = m_depthMapRender->toImage(m_viewPortSize);
    m_depthMap = new QOpenGLTexture(depthMapImage);
}

void NormalAndDepthMapsGenerator::process()
{
    generate();
    emit finished();
}

QOpenGLTexture *NormalAndDepthMapsGenerator::takeNormalMap()
{
    QOpenGLTexture *normalMap = m_normalMap;
    m_normalMap = nullptr;
    return normalMap;
}

QOpenGLTexture *NormalAndDepthMapsGenerator::takeDepthMap()
{
    QOpenGLTexture *depthMap = m_depthMap;
    m_depthMap = nullptr;
    return depthMap;
}
