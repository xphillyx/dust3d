#include <QFile>
#include <map>
#include "modelshaderprogram.h"

const QString &ModelShaderProgram::loadShaderSource(const QString &name)
{
    static std::map<QString, QString> s_shaderSources;
    auto findShader = s_shaderSources.find(name);
    if (findShader != s_shaderSources.end()) {
        return findShader->second;
    }
    QFile file(name);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    auto insertResult = s_shaderSources.insert({name, stream.readAll()});
    return insertResult.first->second;
}

bool ModelShaderProgram::isCoreProfile()
{
    return m_isCoreProfile;
}

ModelShaderProgram::ModelShaderProgram(bool isCoreProfile)
{
    if (isCoreProfile) {
        this->addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/default.core.vert"));
        this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/default.core.frag"));
        m_isCoreProfile = true;
    } else {
        this->addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/default.vert"));
        this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/default.frag"));
    }
    this->bindAttributeLocation("vertex", 0);
    this->bindAttributeLocation("normal", 1);
    this->bindAttributeLocation("color", 2);
    this->bindAttributeLocation("texCoord", 3);
    this->bindAttributeLocation("metalness", 4);
    this->bindAttributeLocation("roughness", 5);
    this->bindAttributeLocation("tangent", 6);
    this->bindAttributeLocation("alpha", 7);
    this->link();

    this->bind();
    m_projectionMatrixLoc = this->uniformLocation("projectionMatrix");
    m_modelMatrixLoc = this->uniformLocation("modelMatrix");
    m_normalMatrixLoc = this->uniformLocation("normalMatrix");
    m_viewMatrixLoc = this->uniformLocation("viewMatrix");
    m_eyePosLoc = this->uniformLocation("eyePos");
    m_textureIdLoc = this->uniformLocation("textureId");
    m_textureEnabledLoc = this->uniformLocation("textureEnabled");
    m_normalMapIdLoc = this->uniformLocation("normalMapId");
    m_normalMapEnabledLoc = this->uniformLocation("normalMapEnabled");
    m_metalnessMapEnabledLoc = this->uniformLocation("metalnessMapEnabled");
    m_roughnessMapEnabledLoc = this->uniformLocation("roughnessMapEnabled");
    m_ambientOcclusionMapEnabledLoc = this->uniformLocation("ambientOcclusionMapEnabled");
    m_metalnessRoughnessAmbientOcclusionMapIdLoc = this->uniformLocation("metalnessRoughnessAmbientOcclusionMapId");
    m_mousePickEnabledLoc = this->uniformLocation("mousePickEnabled");
    m_mousePickTargetPositionLoc = this->uniformLocation("mousePickTargetPosition");
    m_mousePickRadiusLoc = this->uniformLocation("mousePickRadius");
    m_toonShadingEnabledLoc = this->uniformLocation("toonShadingEnabled");
    m_renderPurposeLoc = this->uniformLocation("renderPurpose");
    m_toonEdgeEnabledLoc = this->uniformLocation("toonEdgeEnabled");
    m_screenWidthLoc = this->uniformLocation("screenWidth");
    m_screenHeightLoc = this->uniformLocation("screenHeight");
    m_toonNormalMapIdLoc = this->uniformLocation("toonNormalMapId");
    m_toonDepthMapIdLoc = this->uniformLocation("toonDepthMapId");
    if (m_isCoreProfile) {
        m_environmentIrradianceMapIdLoc = this->uniformLocation("environmentIrradianceMapId");
        m_environmentIrradianceMapEnabledLoc = this->uniformLocation("environmentIrradianceMapEnabled");
        m_environmentSpecularMapIdLoc = this->uniformLocation("environmentSpecularMapId");
        m_environmentSpecularMapEnabledLoc = this->uniformLocation("environmentSpecularMapEnabled");
    }
}

int ModelShaderProgram::projectionMatrixLoc()
{
    return m_projectionMatrixLoc;
}

int ModelShaderProgram::modelMatrixLoc()
{
    return m_modelMatrixLoc;
}

int ModelShaderProgram::normalMatrixLoc()
{
    return m_normalMatrixLoc;
}

int ModelShaderProgram::viewMatrixLoc()
{
    return m_viewMatrixLoc;
}

int ModelShaderProgram::eyePosLoc()
{
    return m_eyePosLoc;
}

int ModelShaderProgram::textureEnabledLoc()
{
    return m_textureEnabledLoc;
}

int ModelShaderProgram::textureIdLoc()
{
    return m_textureIdLoc;
}

int ModelShaderProgram::normalMapEnabledLoc()
{
    return m_normalMapEnabledLoc;
}

int ModelShaderProgram::normalMapIdLoc()
{
    return m_normalMapIdLoc;
}

int ModelShaderProgram::metalnessMapEnabledLoc()
{
    return m_metalnessMapEnabledLoc;
}

int ModelShaderProgram::roughnessMapEnabledLoc()
{
    return m_roughnessMapEnabledLoc;
}

int ModelShaderProgram::ambientOcclusionMapEnabledLoc()
{
    return m_ambientOcclusionMapEnabledLoc;
}

int ModelShaderProgram::metalnessRoughnessAmbientOcclusionMapIdLoc()
{
    return m_metalnessRoughnessAmbientOcclusionMapIdLoc;
}

int ModelShaderProgram::mousePickEnabledLoc()
{
    return m_mousePickEnabledLoc;
}

int ModelShaderProgram::mousePickTargetPositionLoc()
{
    return m_mousePickTargetPositionLoc;
}

int ModelShaderProgram::mousePickRadiusLoc()
{
    return m_mousePickRadiusLoc;
}

int ModelShaderProgram::environmentIrradianceMapIdLoc()
{
    return m_environmentIrradianceMapIdLoc;
}

int ModelShaderProgram::environmentIrradianceMapEnabledLoc()
{
    return m_environmentIrradianceMapEnabledLoc;
}

int ModelShaderProgram::environmentSpecularMapIdLoc()
{
    return m_environmentSpecularMapIdLoc;
}

int ModelShaderProgram::environmentSpecularMapEnabledLoc()
{
    return m_environmentSpecularMapEnabledLoc;
}

int ModelShaderProgram::toonShadingEnabledLoc()
{
    return m_toonShadingEnabledLoc;
}

int ModelShaderProgram::renderPurposeLoc()
{
    return m_renderPurposeLoc;
}

int ModelShaderProgram::toonEdgeEnabledLoc()
{
    return m_toonEdgeEnabledLoc;
}

int ModelShaderProgram::screenWidthLoc()
{
    return m_screenWidthLoc;
}

int ModelShaderProgram::screenHeightLoc()
{
    return m_screenHeightLoc;
}

int ModelShaderProgram::toonNormalMapIdLoc()
{
    return m_toonNormalMapIdLoc;
}

int ModelShaderProgram::toonDepthMapIdLoc()
{
    return m_toonDepthMapIdLoc;
}

void ModelShaderProgram::setProjectionMatrixValue(const QMatrix4x4 &value)
{
    if (value == m_projectionMatrixValue)
        return;
    m_projectionMatrixValue = value;
    setUniformValue(m_projectionMatrixLoc, m_projectionMatrixValue);
}

void ModelShaderProgram::setModelMatrixValue(const QMatrix4x4 &value)
{
    if (value == m_modelMatrixValue)
        return;
    m_modelMatrixValue = value;
    setUniformValue(m_modelMatrixLoc, m_modelMatrixValue);
}


void ModelShaderProgram::setNormalMatrixValue(const QMatrix3x3 &value)
{
    if (value == m_normalMatrixValue)
        return;
    m_normalMatrixValue = value;
    setUniformValue(m_normalMatrixLoc, m_normalMatrixValue);
}

void ModelShaderProgram::setViewMatrixValue(const QMatrix4x4 &value)
{
    if (value == m_viewMatrixValue)
        return;
    m_viewMatrixValue = value;
    setUniformValue(m_viewMatrixLoc, m_viewMatrixValue);
}

void ModelShaderProgram::setEyePosValue(const QVector3D &value)
{
    if (qFuzzyCompare(value, m_eyePosValue))
        return;
    m_eyePosValue = value;
    setUniformValue(m_eyePosLoc, m_eyePosValue);
}

void ModelShaderProgram::setTextureIdValue(int value)
{
    if (value == m_textureIdValue)
        return;
    m_textureIdValue = value;
    setUniformValue(m_textureIdLoc, m_textureIdValue);
}

void ModelShaderProgram::setTextureEnabledValue(int value)
{
    if (value == m_textureEnabledValue)
        return;
    m_textureEnabledValue = value;
    setUniformValue(m_textureEnabledLoc, m_textureEnabledValue);
}

void ModelShaderProgram::setNormalMapEnabledValue(int value)
{
    if (value == m_normalMapEnabledValue)
        return;
    m_normalMapEnabledValue = value;
    setUniformValue(m_normalMapEnabledLoc, m_normalMapEnabledValue);
}

void ModelShaderProgram::setNormalMapIdValue(int value)
{
    if (value == m_normalMapIdValue)
        return;
    m_normalMapIdValue = value;
    setUniformValue(m_normalMapIdLoc, m_normalMapIdValue);
}

void ModelShaderProgram::setMetalnessMapEnabledValue(int value)
{
    if (value == m_metalnessMapEnabledValue)
        return;
    m_metalnessMapEnabledValue = value;
    setUniformValue(m_metalnessMapEnabledLoc, m_metalnessMapEnabledValue);
}

void ModelShaderProgram::setRoughnessMapEnabledValue(int value)
{
    if (value == m_roughnessMapEnabledValue)
        return;
    m_roughnessMapEnabledValue = value;
    setUniformValue(m_roughnessMapEnabledLoc, m_roughnessMapEnabledValue);
}

void ModelShaderProgram::setAmbientOcclusionMapEnabledValue(int value)
{
    if (value == m_ambientOcclusionMapEnabledValue)
        return;
    m_ambientOcclusionMapEnabledValue = value;
    setUniformValue(m_ambientOcclusionMapEnabledLoc, m_ambientOcclusionMapEnabledValue);
}

void ModelShaderProgram::setMetalnessRoughnessAmbientOcclusionMapIdValue(int value)
{
    if (value == m_metalnessRoughnessAmbientOcclusionMapIdValue)
        return;
    m_metalnessRoughnessAmbientOcclusionMapIdValue = value;
    setUniformValue(m_metalnessRoughnessAmbientOcclusionMapIdLoc, m_metalnessRoughnessAmbientOcclusionMapIdValue);
}

void ModelShaderProgram::setMousePickEnabledValue(int value)
{
    if (value == m_mousePickEnabledValue)
        return;
    m_mousePickEnabledValue = value;
    setUniformValue(m_mousePickEnabledLoc, m_mousePickEnabledValue);
}

void ModelShaderProgram::setMousePickTargetPositionValue(const QVector3D &value)
{
    if (qFuzzyCompare(value, m_mousePickTargetPositionValue))
        return;
    m_mousePickTargetPositionValue = value;
    setUniformValue(m_mousePickTargetPositionLoc, m_mousePickTargetPositionValue);
}

void ModelShaderProgram::setMousePickRadiusValue(float value)
{
    if (qFuzzyCompare(value, m_mousePickRadiusValue))
        return;
    m_mousePickRadiusValue = value;
    setUniformValue(m_mousePickRadiusLoc, m_mousePickRadiusValue);
}

void ModelShaderProgram::setEnvironmentIrradianceMapIdValue(int value)
{
    if (value == m_environmentIrradianceMapIdValue)
        return;
    m_environmentIrradianceMapIdValue = value;
    setUniformValue(m_environmentIrradianceMapIdLoc, m_environmentIrradianceMapIdValue);
}

void ModelShaderProgram::setEnvironmentIrradianceMapEnabledValue(int value)
{
    if (value == m_environmentIrradianceMapEnabledValue)
        return;
    m_environmentIrradianceMapEnabledValue = value;
    setUniformValue(m_environmentIrradianceMapEnabledLoc, m_environmentIrradianceMapEnabledValue);
}

void ModelShaderProgram::setEnvironmentSpecularMapIdValue(int value)
{
    if (value == m_environmentSpecularMapIdValue)
        return;
    m_environmentSpecularMapIdValue = value;
    setUniformValue(m_environmentSpecularMapIdLoc, m_environmentSpecularMapIdValue);
}

void ModelShaderProgram::setEnvironmentSpecularMapEnabledValue(int value)
{
    if (value == m_environmentSpecularMapEnabledValue)
        return;
    m_environmentSpecularMapEnabledValue = value;
    setUniformValue(m_environmentSpecularMapEnabledLoc, m_environmentSpecularMapEnabledValue);
}

void ModelShaderProgram::setToonShadingEnabledValue(int value)
{
    if (value == m_toonShadingEnabledValue)
        return;
    m_toonShadingEnabledValue = value;
    setUniformValue(m_toonShadingEnabledLoc, m_toonShadingEnabledValue);
}

void ModelShaderProgram::setRenderPurposeValue(int value)
{
    if (value == m_renderPurposeValue)
        return;
    m_renderPurposeValue = value;
    setUniformValue(m_renderPurposeLoc, m_renderPurposeValue);
}

void ModelShaderProgram::setToonEdgeEnabledValue(int value)
{
    if (value == m_toonEdgeEnabledValue)
        return;
    m_toonEdgeEnabledValue = value;
    setUniformValue(m_toonEdgeEnabledLoc, m_toonEdgeEnabledValue);
}

void ModelShaderProgram::setScreenWidthValue(float value)
{
    if (qFuzzyCompare(value, m_screenWidthValue))
        return;
    m_screenWidthValue = value;
    setUniformValue(m_screenWidthLoc, m_screenWidthValue);
}

void ModelShaderProgram::setScreenHeightValue(float value)
{
    if (qFuzzyCompare(value, m_screenHeightValue))
        return;
    m_screenHeightValue = value;
    setUniformValue(m_screenHeightLoc, m_screenHeightValue);
}

void ModelShaderProgram::setToonNormalMapIdValue(int value)
{
    if (value == m_toonNormalMapIdValue)
        return;
    m_toonNormalMapIdValue = value;
    setUniformValue(m_toonNormalMapIdLoc, m_toonNormalMapIdValue);
}

void ModelShaderProgram::setToonDepthMapIdValue(int value)
{
    if (value == m_toonDepthMapIdValue)
        return;
    m_toonDepthMapIdValue = value;
    setUniformValue(m_toonDepthMapIdLoc, m_toonDepthMapIdValue);
}
