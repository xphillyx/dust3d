#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QBuffer>
#include <QDebug>
#include <string>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector3D>
#include <QApplication>
#include "posecapturewidget.h"

#define POSE_CAPTURE_FRAMES_PER_SECOND_MEASURE_INTVAL_SECONDS    3
#define MAX_POSE_PARSE_FRAMES_PER_SECOND                         30
#define MIN_POSE_CONFIDENCE                                      0.7
#define MIN_POSE_FRAME_DURATION                                  (1000 / MAX_POSE_PARSE_FRAMES_PER_SECOND)

#if USE_MOCAP

PoseCaptureWidget::PoseCaptureWidget(QWidget *parent) :
    QDialog(parent)
{
    class WebEnginePage : public QWebEnginePage
    {
    public:
        WebEnginePage(QObject *parent = 0) : QWebEnginePage(parent) {}
        virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID)
        {
            qDebug() << "Page console" << "level:" << level << "message:" << message << "line:" << lineNumber << "source:" << sourceID;
        }
    };
    
    m_elapsedTimer.start();

    QVBoxLayout *previewLayout = new QVBoxLayout;
    
    m_webView = new QWebEngineView;
    m_webView->hide();
    m_webView->setPage(new WebEnginePage(m_webView));
    connect(m_webView->page(), &QWebEnginePage::loadFinished, this, [&](bool ok) {
        Q_UNUSED(ok);
        m_webLoaded = true;
    });
    m_webView->setUrl(QUrl("qrc:/scripts/motioncapture.html"));
    
    m_rawCapturePreviewWidget = new PoseCapturePreviewWidget;
    
    QObject::connect(this, &PoseCaptureWidget::poseKeypointsDetected, m_rawCapturePreviewWidget, &PoseCapturePreviewWidget::setKeypoints);
    QObject::connect(this, &PoseCaptureWidget::poseKeypointsDetected, &m_poseCapture, &PoseCapture::updateKeypoints);
    connect(&m_poseCapture, &PoseCapture::stateChanged, this, &PoseCaptureWidget::changeStateIndicator);
    connect(&m_poseCapture, &PoseCapture::trackMerged, this, &PoseCaptureWidget::updateTrack);
    connect(&m_poseCapture, &PoseCapture::stateChanged, this, [&](PoseCapture::State state) {
        m_rawCapturePreviewWidget->setProfile(m_poseCapture.profile());
        m_rawCapturePreviewWidget->setState(state);
    });
    
    previewLayout->addWidget(m_rawCapturePreviewWidget);
    previewLayout->addWidget(m_webView);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addLayout(previewLayout);
    
    setLayout(mainLayout);
    
    setMinimumSize(640, 480);
    
    startCapture();
    
    m_framesPerSecondCheckTimer = new QTimer;
    connect(m_framesPerSecondCheckTimer, &QTimer::timeout, this, &PoseCaptureWidget::checkFramesPerSecond);
    m_framesPerSecondCheckTimer->start(POSE_CAPTURE_FRAMES_PER_SECOND_MEASURE_INTVAL_SECONDS * 1000);
}

void PoseCaptureWidget::checkFramesPerSecond()
{
    m_capturedFramesPerSecond = (m_capturedFrames - m_tillLastSecondCapturedFrames) / POSE_CAPTURE_FRAMES_PER_SECOND_MEASURE_INTVAL_SECONDS;
    m_parsedFramesPerSecond = (m_parsedFrames - m_tillLastSecondParsedFrames) / POSE_CAPTURE_FRAMES_PER_SECOND_MEASURE_INTVAL_SECONDS;
    qDebug() << "Capture fps:" << m_capturedFramesPerSecond << "parse fps:" << m_parsedFramesPerSecond;
    m_tillLastSecondCapturedFrames = m_capturedFrames;
    m_tillLastSecondParsedFrames = m_parsedFrames;
}

PoseCaptureWidget::~PoseCaptureWidget()
{
    stopCapture();
    delete m_framesPerSecondCheckTimer;
}

void PoseCaptureWidget::changeStateIndicator(PoseCapture::State state)
{
    qDebug() << "State changed:" << (int)state;
}

void PoseCaptureWidget::updateTrack(const PoseCapture::Track &track)
{
    qDebug() << "Track merged, size:" << track.size();
}

void PoseCaptureWidget::startCapture()
{
    if (nullptr != m_imageCapture)
        return;
    
    m_imageCapture = new ImageCapture;
    
    QThread *thread = new QThread;
    m_imageCapture->moveToThread(thread);
    
    QObject::connect(m_imageCapture, &ImageCapture::imageCaptured, m_rawCapturePreviewWidget, &PoseCapturePreviewWidget::setImage);
    QObject::connect(m_imageCapture, &ImageCapture::imageCaptured, this, &PoseCaptureWidget::updateCapturedImage);
    QObject::connect(thread, &QThread::started, m_imageCapture, &ImageCapture::start);
    QObject::connect(m_imageCapture, &ImageCapture::stopped, thread, &QThread::quit);
    QObject::connect(m_imageCapture, &ImageCapture::stopped, m_imageCapture, &QObject::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void PoseCaptureWidget::updateCapturedImage(const QImage &image)
{
    ++m_capturedFrames;
    
    if (!m_webLoaded)
        return;
    
    if (m_lastFrameTime + MIN_POSE_FRAME_DURATION > m_elapsedTimer.elapsed()) {
        return;
    }
    m_lastFrameTime = m_elapsedTimer.elapsed();
    
    QByteArray bateArray;
    QBuffer buffer(&bateArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "BMP");
    m_webView->page()->runJavaScript(QString("updateImage(\"data:image/bmp;base64,%1\")").arg(QString::fromLatin1(bateArray.toBase64())),
            [&](const QVariant &result) {
        auto json = QJsonDocument::fromJson(result.toString().toUtf8());
        if (!json.isArray())
            return;
        QJsonArray array = json.array();
        if (array.isEmpty())
            return;
        for (int i = 0; i < array.size(); ++i) {
            ++m_parsedFrames;
            QJsonObject object = array[i].toObject();
            auto map = object.toVariantMap();
            const auto &keypoints = map["keypoints"];
            if (QMetaType::QVariantList != keypoints.userType())
                continue;
            std::map<QString, QVector3D> keypointPositions;
            for (const auto &keypoint: keypoints.value<QSequentialIterable>()) {
                QVariantMap properties = qvariant_cast<QVariantMap>(keypoint);
                const auto &score = properties.value("score");
                if (score.isNull() || score.toFloat() < MIN_POSE_CONFIDENCE)
                    continue;
                const auto &part = properties.value("part");
                if (part.isNull())
                    continue;
                const auto &position = properties.value("position");
                if (position.isNull())
                    continue;
                QVariantMap positionComponents = qvariant_cast<QVariantMap>(position);
                keypointPositions[part.toString()] = QVector3D(positionComponents.value("x").toFloat(),
                    positionComponents.value("y").toFloat(),
                    0);
            }
            if (keypointPositions.empty())
                continue;
            emit poseKeypointsDetected(keypointPositions);
        }
    });
}

void PoseCaptureWidget::stopCapture()
{
    if (nullptr == m_imageCapture)
        return;
    
    auto imageCapture = m_imageCapture;
    m_imageCapture = nullptr;
    imageCapture->stop();
}

#endif
