#ifndef DUST3D_POSE_CAPTURE_WIDGET_H
#define DUST3D_POSE_CAPTURE_WIDGET_H
#include <QDialog>
#include <QWebEngineView>
#include <QVariantMap>
#include <QTimer>
#include <QElapsedTimer>
#include <QCloseEvent>
#include <QLineEdit>
#include <QSizeF>
#include "posecapturepreviewwidget.h"
#include "imagecapture.h"
#include "posecapture.h"
#include "modelwidget.h"
#include "motionsgenerator.h"
#include "animationclipplayer.h"
#include "document.h"

#if USE_MOCAP

class PoseCaptureWidget : public QDialog
{
    Q_OBJECT
public:
    PoseCaptureWidget(const Document *document, QWidget *parent=nullptr);
    ~PoseCaptureWidget();
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
    
signals:
    void poseKeypointsDetected(const std::map<QString, QVector3D> &keypoints,
        const QSizeF &imageSize);
    void addPose(QUuid poseId, QString name, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames, QUuid turnaroundImageId);
    void setPoseFrames(QUuid poseId, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames);
    void renamePose(QUuid poseId, QString name);
    
public slots:
    void changeStateIndicator(PoseCapture::State state);
    void updateTrack(const PoseCapture::Track &track, const std::vector<qint64> timeline);
    
private slots:
    void startCapture();
    void stopCapture();
    void updateCapturedImage(const QImage &image);
    void checkFramesPerSecond();
    void generatePreviews();
    void previewsReady();
    void clearUnsaveState();
    void setUnsaveState();
    void updateTitle();
    void save();
    
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
    bool m_isPreviewsObsolete = false;
    ModelWidget *m_previewWidget = nullptr;
    MotionsGenerator *m_previewsGenerator = nullptr;
    AnimationClipPlayer *m_clipPlayer = nullptr;
    const Document *m_document = nullptr;
    bool m_closed = false;
    bool m_unsaved = false;
    QUuid m_poseId;
    bool m_captureEnabled = true;
    QLineEdit *m_nameEdit = nullptr;
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> m_poseFrames;
    QWidget *m_tposeWidget = nullptr;
    QWidget *m_sevenPoseWidget = nullptr;
    QWidget *m_flippedSevenPoseWidget = nullptr;
    QWidget *m_sliderContainerWidget = nullptr;
};

#endif

#endif

