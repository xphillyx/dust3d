#ifndef DUST3D_POSE_CAPTURE_WIDGET_H
#define DUST3D_POSE_CAPTURE_WIDGET_H
#include <QDialog>
#include <QWebEngineView>
#include "imagepreviewwidget.h"
#include "imagecapture.h"

#if USE_MOCAP

class PoseCaptureWidget : public QDialog
{
    Q_OBJECT
public:
    PoseCaptureWidget(QWidget *parent=nullptr);
    ~PoseCaptureWidget();
    
private slots:
    void startCapture();
    void stopCapture();
    void updateCapturedImage(const QImage &image);
    
private:
    ImagePreviewWidget *m_rawCapturePreviewWidget = nullptr;
    ImageCapture *m_imageCapture = nullptr;
    QWebEngineView *m_webView = nullptr;
};

#endif

#endif

