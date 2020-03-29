#include <QOpenGLFramebufferObjectFormat>
#include <QThread>
#include <QDebug>
#include "modelofflinerender.h"

ModelOfflineRender::ModelOfflineRender(QScreen *targetScreen) :
    QOffscreenSurface(targetScreen),
    m_context(nullptr),
    m_mesh(nullptr)
{
    m_context = new QOpenGLContext();
    
    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(4);
    setFormat(fmt);
    
    create();
    if (!isValid())
        qDebug() << "ModelOfflineRender is invalid";
    
    m_context->setFormat(fmt);
    if (!m_context->create())
        qDebug() << "QOpenGLContext create failed";
}

ModelOfflineRender::~ModelOfflineRender()
{
    delete m_context;
    m_context = nullptr;
    destroy();
    delete m_mesh;
}

void ModelOfflineRender::updateMesh(MeshLoader *mesh)
{
    delete m_mesh;
    m_mesh = mesh;
}

void ModelOfflineRender::setRenderThread(QThread *thread)
{
    m_context->moveToThread(thread);
}

QImage ModelOfflineRender::toImage(const QSize &size)
{
    QImage image;
    
    if (!m_context->makeCurrent(this)) {
        qDebug() << "QOpenGLContext makeCurrent failed";
        return image;
    }
    
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    QOpenGLFramebufferObject *renderFbo = new QOpenGLFramebufferObject(size, format);
    renderFbo->bind();
    m_context->functions()->glViewport(0, 0, size.width(), size.height());
    
    if (nullptr != m_mesh) {
        int xRot = 0;
        int yRot = 0;
        int zRot = 0;
        QMatrix4x4 projection;
        QMatrix4x4 world;
        
        bool isCoreProfile = false;
        const char *versionString = (const char *)m_context->functions()->glGetString(GL_VERSION);
        if (nullptr != versionString &&
                '\0' != versionString[0] &&
                0 == strstr(versionString, "Mesa")) {
            isCoreProfile = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
        }

        ModelShaderProgram *program = new ModelShaderProgram(isCoreProfile);
        ModelMeshBinder meshBinder;
        meshBinder.initialize();
        meshBinder.hideWireframes();

        program->setUniformValue(program->lightPosLoc(), QVector3D(0, 0, 70));
		
        m_context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_context->functions()->glEnable(GL_BLEND);
        m_context->functions()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_context->functions()->glEnable(GL_DEPTH_TEST);
        m_context->functions()->glEnable(GL_CULL_FACE);
#ifdef GL_LINE_SMOOTH
        m_context->functions()->glEnable(GL_LINE_SMOOTH);
#endif

        world.setToIdentity();
        world.rotate(xRot / 16.0f, 1, 0, 0);
        world.rotate(yRot / 16.0f, 0, 1, 0);
        world.rotate(zRot / 16.0f, 0, 0, 1);

        projection.setToIdentity();
        projection.perspective(45.0f, GLfloat(size.width()) / size.height(), 0.01f, 100.0f);

        program->bind();
        program->setUniformValue(program->projectionMatrixLoc(), projection);
        program->setUniformValue(program->modelMatrixLoc(), world);
        QMatrix3x3 normalMatrix = world.normalMatrix();
        program->setUniformValue(program->normalMatrixLoc(), normalMatrix);
        program->setUniformValue(program->textureEnabledLoc(), 0);
        program->setUniformValue(program->normalMapEnabledLoc(), 0);
        
        program->setUniformValue(program->mousePickEnabledLoc(), 0);

        meshBinder.updateMesh(m_mesh);
        meshBinder.paint(program);

        meshBinder.cleanup();

        program->release();
        delete program;

        m_mesh = nullptr;
    }

    m_context->functions()->glFlush();

    image = renderFbo->toImage();

    renderFbo->bindDefault();
    delete renderFbo;

    m_context->doneCurrent();

    return image;
}
