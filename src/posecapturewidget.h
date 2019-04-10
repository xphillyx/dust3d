#ifndef DUST3D_POSE_CAPTURE_WIDGET_H
#define DUST3D_POSE_CAPTURE_WIDGET_H
#include <QDialog>
#include "imagepreviewwidget.h"
#include "imagecapture.h"

class PoseCaptureWidget : public QDialog
{
    Q_OBJECT
public:
    PoseCaptureWidget(QWidget *parent=nullptr);
    ~PoseCaptureWidget();
    
private slots:
    void startCapture();
    void stopCapture();
    
private:
    ImagePreviewWidget *m_rawCapturePreviewWidget = nullptr;
    ImageCapture *m_imageCapture = nullptr;
};

#endif

