#ifndef DUST3D_MODEL_SHADER_PROGRAM_H
#define DUST3D_MODEL_SHADER_PROGRAM_H
#include <QOpenGLShaderProgram>
#include <QString>

class ModelShaderProgram : public QOpenGLShaderProgram
{
public:
    ModelShaderProgram(bool isCoreProfile);
    int projectionMatrixLoc();
    int modelMatrixLoc();
    int normalMatrixLoc();
    int viewMatrixLoc();
    int eyePosLoc();
    int textureIdLoc();
    int textureEnabledLoc();
    int normalMapEnabledLoc();
    int normalMapIdLoc();
    int metalnessMapEnabledLoc();
    int roughnessMapEnabledLoc();
    int ambientOcclusionMapEnabledLoc();
    int metalnessRoughnessAmbientOcclusionMapIdLoc();
    int mousePickEnabledLoc();
    int mousePickTargetPositionLoc();
    int mousePickRadiusLoc();
    int environmentIrradianceMapIdLoc();
    int environmentIrradianceMapEnabledLoc();
    int environmentSpecularMapIdLoc();
    int environmentSpecularMapEnabledLoc();
    int toonShadingEnabledLoc();
    int renderPurposeLoc();
    int toonEdgeEnabledLoc();
    int screenWidthLoc();
    int screenHeightLoc();
    int toonNormalMapIdLoc();
    int toonDepthMapIdLoc();
    bool isCoreProfile();
    static const QString &loadShaderSource(const QString &name);
    void setProjectionMatrixValue(const QMatrix4x4 &value);
    void setModelMatrixValue(const QMatrix4x4 &value);
	void setNormalMatrixValue(const QMatrix3x3 &value);
	void setViewMatrixValue(const QMatrix4x4 &value);
	void setEyePosValue(const QVector3D &value);
	void setTextureIdValue(int value);
	void setTextureEnabledValue(int value);
	void setNormalMapEnabledValue(int value);
	void setNormalMapIdValue(int value);
	void setMetalnessMapEnabledValue(int value);
	void setRoughnessMapEnabledValue(int value);
	void setAmbientOcclusionMapEnabledValue(int value);
	void setMetalnessRoughnessAmbientOcclusionMapIdValue(int value);
	void setMousePickEnabledValue(int value);
	void setMousePickTargetPositionValue(const QVector3D &value);
	void setMousePickRadiusValue(float value);
	void setEnvironmentIrradianceMapIdValue(int value);
	void setEnvironmentIrradianceMapEnabledValue(int value);
	void setEnvironmentSpecularMapIdValue(int value);
	void setEnvironmentSpecularMapEnabledValue(int value);
	void setToonShadingEnabledValue(int value);
	void setRenderPurposeValue(int value);
	void setToonEdgeEnabledValue(int value);
	void setScreenWidthValue(float value);
	void setScreenHeightValue(float value);
	void setToonNormalMapIdValue(int value);
	void setToonDepthMapIdValue(int value);
private:
    bool m_isCoreProfile = false;
    int m_projectionMatrixLoc = 0;
    int m_modelMatrixLoc = 0;
    int m_normalMatrixLoc = 0;
    int m_viewMatrixLoc = 0;
    int m_eyePosLoc = 0;
    int m_textureIdLoc = 0;
    int m_textureEnabledLoc = 0;
    int m_normalMapEnabledLoc = 0;
    int m_normalMapIdLoc = 0;
    int m_metalnessMapEnabledLoc = 0;
    int m_roughnessMapEnabledLoc = 0;
    int m_ambientOcclusionMapEnabledLoc = 0;
    int m_metalnessRoughnessAmbientOcclusionMapIdLoc = 0;
    int m_mousePickEnabledLoc = 0;
    int m_mousePickTargetPositionLoc = 0;
    int m_mousePickRadiusLoc = 0;
    int m_environmentIrradianceMapIdLoc = 0;
    int m_environmentIrradianceMapEnabledLoc = 0;
    int m_environmentSpecularMapIdLoc = 0;
    int m_environmentSpecularMapEnabledLoc = 0;
    int m_toonShadingEnabledLoc = 0;
    int m_renderPurposeLoc = 0;
    int m_toonEdgeEnabledLoc = 0;
    int m_screenWidthLoc = 0;
    int m_screenHeightLoc = 0;
    int m_toonNormalMapIdLoc = 0;
    int m_toonDepthMapIdLoc = 0;
    ////////////////////////////////////////////////////////////////
    QMatrix4x4 m_projectionMatrixValue;
    QMatrix4x4 m_modelMatrixValue;
    QMatrix3x3 m_normalMatrixValue;
    QMatrix4x4 m_viewMatrixValue;
    QVector3D m_eyePosValue;
    int m_textureIdValue = -1;
    int m_textureEnabledValue = -1;
    int m_normalMapEnabledValue = -1;
    int m_normalMapIdValue = -1;
    int m_metalnessMapEnabledValue = -1;
    int m_roughnessMapEnabledValue = -1;
    int m_ambientOcclusionMapEnabledValue = -1;
    int m_metalnessRoughnessAmbientOcclusionMapIdValue = -1;
	int m_mousePickEnabledValue = -1;
	QVector3D m_mousePickTargetPositionValue;
	float m_mousePickRadiusValue = 0.0;
	int m_environmentIrradianceMapIdValue = -1;
	int m_environmentIrradianceMapEnabledValue = -1;
	int m_environmentSpecularMapIdValue = -1;
	int m_environmentSpecularMapEnabledValue = -1;
    int m_toonShadingEnabledValue = -1;
    int m_renderPurposeValue = -1;
    int m_toonEdgeEnabledValue = -1;
    float m_screenWidthValue = 0.0;
    float m_screenHeightValue = 0.0;
    int m_toonNormalMapIdValue = -1;
    int m_toonDepthMapIdValue = -1;
};

#endif
