#ifndef DUST3D_MOUSE_PICKER_H
#define DUST3D_MOUSE_PICKER_H
#include <QObject>
#include <QVector3D>
#include <vector>
#include "paintmode.h"

class MousePickerContext;

class MousePicker : public QObject
{
    Q_OBJECT
public:
    MousePicker(MousePickerContext *context,
        const std::vector<QVector3D> &meshVertices, 
        const std::vector<std::vector<size_t>> &meshFaces,
        quint64 meshId);
	~MousePicker();
	void setPaintMode(PaintMode paintMode);
	PaintMode takePaintMode();
    bool takePickedPosition(QVector3D *position=nullptr, QVector3D *normal=nullptr);
    MousePickerContext *takeContext();
    void setMouseRay(const QVector3D &mouseRayNear, 
        const QVector3D &mouseRayFar);
    void pick();
    static void releaseContext(MousePickerContext *context);
signals:
    void finished();
public slots:
    void process();
private:
    MousePickerContext *m_context = nullptr;
    QVector3D m_pickedPosition;
    QVector3D m_pickedNormal;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    PaintMode m_paintMode = PaintMode::None;
    bool m_picked = false;
};

#endif
