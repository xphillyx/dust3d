#ifndef DUST3D_POSE_CAPTURE_WIDGET_H
#define DUST3D_POSE_CAPTURE_WIDGET_H
#include <QDialog>
#include <QWebEngineView>
#include <QVariantMap>
#include <QTimer>
#include <QElapsedTimer>
#include "posecapturepreviewwidget.h"
#include "imagecapture.h"
#include "posecapture.h"

#if USE_MOCAP

class PoseCaptureWidget : public QDialog
{
    Q_OBJECT
public:
    PoseCaptureWidget(QWidget *parent=nullptr);
    ~PoseCaptureWidget();
    
signals:
    void poseKeypointsDetected(const std::map<QString, QVector3D> &keypoints);
    
public slots:
    void changeStateIndicator(PoseCapture::State state);
    void updateTrack(const PoseCapture::Track &track);
    
private slots:
    void startCapture();
    void stopCapture();
    void updateCapturedImage(const QImage &image);
    void checkFramesPerSecond();
    
private:
    PoseCapturePreviewWidget *m_rawCapturePreviewWidget = nullptr;
    ImageCapture *m_imageCapture = nullptr;
    QWebEngineView *m_webView = nullptr;
    bool m_webLoaded = false;
    PoseCapture m_poseCapture;
    uint64_t m_capturedFrames = 0;
    uint64_t m_parsedFrames = 0;
    uint64_t m_tillLastSecondCapturedFrames = 0;
    uint64_t m_tillLastSecondParsedFrames = 0;
    uint64_t m_capturedFramesPerSecond = 0;
    uint64_t m_parsedFramesPerSecond = 0;
    int64_t m_lastFrameTime = 0;
    QTimer *m_framesPerSecondCheckTimer = nullptr;
    QElapsedTimer m_elapsedTimer;
};

#endif

#endif

