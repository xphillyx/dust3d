#ifndef DUST3D_POSE_CAPTURE_PREVIEW_WIDGET_H
#define DUST3D_POSE_CAPTURE_PREVIEW_WIDGET_H
#include <QWidget>
#include <QImage>
#include <QVector3D>
#include <QElapsedTimer>
#include "posecapture.h"

class PoseCapturePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    PoseCapturePreviewWidget(QWidget *parent=nullptr);
    
protected:
    void paintEvent(QPaintEvent *event);
    
public slots:
    void setImage(const QImage &image);
    void setKeypoints(const std::map<QString, QVector3D> &keypoints);
    void setState(PoseCapture::State state);
    
private:
    QImage m_image;
    std::map<QString, QVector3D> m_keypoints;
    PoseCapture::State m_currentState = PoseCapture::State::Idle;
    QElapsedTimer m_stateProgressTimer;
};

#endif
