#ifndef DUST3D_MOUSE_PICKER_H
#define DUST3D_MOUSE_PICKER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include "paintmode.h"
#include "meshvoxelcontext.h"

class MousePicker : public QObject
{
    Q_OBJECT
public:
    MousePicker(MeshVoxelContext *context);
	~MousePicker();
	void setPaintMode(PaintMode paintMode);
	void setIsEndOfStroke(bool isEndOfStroke);
	bool takeIsEndOfStroke();
	void setMouseRadius(float radius);
	float takeMouseRadius();
	PaintMode takePaintMode();
    bool takePickedPosition(QVector3D *position=nullptr, QVector3D *normal=nullptr);
    MeshVoxelContext *takeContext();
    void setMouseRay(const QVector3D &mouseRayNear, 
        const QVector3D &mouseRayFar);
    void pick();
signals:
    void finished();
public slots:
    void process();
private:
    MeshVoxelContext *m_context = nullptr;
    QVector3D m_pickedPosition;
    QVector3D m_pickedNormal;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    PaintMode m_paintMode = PaintMode::None;
    bool m_isEndOfStroke = false;
    bool m_picked = false;
    float m_mouseRadius = 0.0f;
};

#endif
