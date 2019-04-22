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
#include <QSlider>
#include <QHBoxLayout>
#include <QMessageBox>
#include "version.h"
#include "posecapturewidget.h"

#define POSE_CAPTURE_FRAMES_PER_SECOND_MEASURE_INTVAL_SECONDS    3
#define MAX_POSE_PARSE_FRAMES_PER_SECOND                         30
#define MIN_POSE_CONFIDENCE                                      0.7
#define MIN_POSE_FRAME_DURATION                                  (1000 / MAX_POSE_PARSE_FRAMES_PER_SECOND)

#if USE_MOCAP

PoseCaptureWidget::PoseCaptureWidget(const Document *document, QWidget *parent) :
    QDialog(parent),
    m_document(document)
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
    
    m_clipPlayer = new AnimationClipPlayer;
    
    m_elapsedTimer.start();
    
    m_previewWidget = new ModelWidget(this);
    m_previewWidget->setMinimumSize(128, 128);
    m_previewWidget->resize(384, 384);
    m_previewWidget->move(-64, 0);
    
    connect(m_clipPlayer, &AnimationClipPlayer::frameReadyToShow, this, [=]() {
        m_previewWidget->updateMesh(m_clipPlayer->takeFrameMesh());
    });
    
    QSlider *speedModeSlider = new QSlider(Qt::Horizontal);
    speedModeSlider->setFixedWidth(100);
    speedModeSlider->setMaximum(2);
    speedModeSlider->setMinimum(0);
    speedModeSlider->setValue(1);
    
    connect(speedModeSlider, &QSlider::valueChanged, this, [=](int value) {
        m_clipPlayer->setSpeedMode((AnimationClipPlayer::SpeedMode)value);
    });
    
    QHBoxLayout *sliderLayout = new QHBoxLayout;
    sliderLayout->addStretch();
    sliderLayout->addSpacing(50);
    sliderLayout->addWidget(new QLabel(tr("Slow")));
    sliderLayout->addWidget(speedModeSlider);
    sliderLayout->addWidget(new QLabel(tr("Fast")));
    sliderLayout->addSpacing(50);
    sliderLayout->addStretch();
    
    QVBoxLayout *resultPreviewLayout = new QVBoxLayout;
    resultPreviewLayout->addStretch();
    resultPreviewLayout->addLayout(sliderLayout);
    resultPreviewLayout->addSpacing(20);

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
    
    QPushButton *startCaptureButton = new QPushButton(tr("Start Capture"));
    startCaptureButton->hide();
    connect(startCaptureButton, &QPushButton::clicked, this, [&, startCaptureButton]() {
        startCaptureButton->hide();
        m_rawCapturePreviewWidget->show();
        m_captureEnabled = true;
        startCapture();
    });
    
    QHBoxLayout *startCaptureButtonLayout = new QHBoxLayout;
    startCaptureButtonLayout->addStretch();
    startCaptureButtonLayout->addWidget(startCaptureButton);
    startCaptureButtonLayout->addStretch();
    
    QObject::connect(this, &PoseCaptureWidget::poseKeypointsDetected, m_rawCapturePreviewWidget, &PoseCapturePreviewWidget::setKeypoints);
    QObject::connect(this, &PoseCaptureWidget::poseKeypointsDetected, &m_poseCapture, &PoseCapture::updateKeypoints);
    connect(&m_poseCapture, &PoseCapture::stateChanged, this, &PoseCaptureWidget::changeStateIndicator);
    connect(&m_poseCapture, &PoseCapture::trackChanged, this, &PoseCaptureWidget::updateTrack);
    connect(&m_poseCapture, &PoseCapture::stateChanged, this, [&, startCaptureButton](PoseCapture::State state) {
        m_rawCapturePreviewWidget->setProfile(m_poseCapture.profile());
        m_rawCapturePreviewWidget->setState(state);
        if (PoseCapture::State::Idle == state) {
            startCaptureButton->setText(tr("Continue Capture"));
            startCaptureButton->show();
            m_rawCapturePreviewWidget->hide();
            m_captureEnabled = false;
            stopCapture();
        }
    });
    
    previewLayout->addWidget(m_rawCapturePreviewWidget);
    previewLayout->addWidget(m_webView);
    previewLayout->addLayout(startCaptureButtonLayout);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addLayout(resultPreviewLayout);
    topLayout->addLayout(previewLayout);
    topLayout->setStretch(1, 1);
    
    QPushButton *saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked, this, &PoseCaptureWidget::save);
    saveButton->setDefault(true);
    
    m_nameEdit = new QLineEdit;
    m_nameEdit->setFixedWidth(200);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [=]() {
        setUnsaveState();
    });
    
    QHBoxLayout *baseInfoLayout = new QHBoxLayout;
    baseInfoLayout->addWidget(new QLabel(tr("Name")));
    baseInfoLayout->addWidget(m_nameEdit);
    baseInfoLayout->addSpacing(10);
    baseInfoLayout->addStretch();
    baseInfoLayout->addWidget(saveButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(Theme::createHorizontalLineWidget());
    mainLayout->addLayout(baseInfoLayout);
    
    setLayout(mainLayout);
    
    setMinimumSize(940, 480);
    
    startCapture();
    
    m_framesPerSecondCheckTimer = new QTimer;
    connect(m_framesPerSecondCheckTimer, &QTimer::timeout, this, &PoseCaptureWidget::checkFramesPerSecond);
    m_framesPerSecondCheckTimer->start(POSE_CAPTURE_FRAMES_PER_SECOND_MEASURE_INTVAL_SECONDS * 1000);
    
    connect(this, &PoseCaptureWidget::addPose, m_document, &Document::addPose);
    connect(this, &PoseCaptureWidget::renamePose, m_document, &Document::renamePose);
    connect(this, &PoseCaptureWidget::setPoseFrames, m_document, &Document::setPoseFrames);
    
    updateTitle();
}

void PoseCaptureWidget::generatePreviews()
{
    if (nullptr != m_previewsGenerator) {
        m_isPreviewsObsolete = true;
        return;
    }
    
    m_isPreviewsObsolete = false;
    
    const std::vector<RiggerBone> *rigBones = m_document->resultRigBones();
    const std::map<int, RiggerVertexWeights> *rigWeights = m_document->resultRigWeights();
    
    if (nullptr == rigBones || nullptr == rigWeights) {
        return;
    }
    
    m_previewWidget->hide();
    
    QUuid onePoseId = QUuid::createUuid();
    m_previewsGenerator = new MotionsGenerator(m_document->rigType, rigBones, rigWeights,
        m_document->currentRiggedOutcome());
    m_previewsGenerator->addPoseToLibrary(onePoseId, m_poseFrames);
    MotionClip oneClip(onePoseId.toString(), "poseId");
    m_previewsGenerator->addMotionToLibrary(QUuid(), {oneClip});
    m_previewsGenerator->addRequirement(QUuid());
    QThread *thread = new QThread;
    m_previewsGenerator->moveToThread(thread);
    connect(thread, &QThread::started, m_previewsGenerator, &MotionsGenerator::process);
    connect(m_previewsGenerator, &MotionsGenerator::finished, this, &PoseCaptureWidget::previewsReady);
    connect(m_previewsGenerator, &MotionsGenerator::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void PoseCaptureWidget::reject()
{
    close();
}

void PoseCaptureWidget::save()
{
    if (m_poseId.isNull()) {
        m_poseId = QUuid::createUuid();
        emit addPose(m_poseId, m_nameEdit->text(), m_poseFrames, QUuid());
    } else if (m_unsaved) {
        emit renamePose(m_poseId, m_nameEdit->text());
        emit setPoseFrames(m_poseId, m_poseFrames);
    }
    clearUnsaveState();
}

void PoseCaptureWidget::updateTitle()
{
    if (m_poseId.isNull()) {
        setWindowTitle(unifiedWindowTitle(tr("New") + (m_unsaved ? "*" : "")));
        return;
    }
    const Pose *pose = m_document->findPose(m_poseId);
    if (nullptr == pose) {
        qDebug() << "Find pose failed:" << m_poseId;
        return;
    }
    setWindowTitle(unifiedWindowTitle(pose->name + (m_unsaved ? "*" : "")));
}

void PoseCaptureWidget::clearUnsaveState()
{
    m_unsaved = false;
    updateTitle();
}

void PoseCaptureWidget::setUnsaveState()
{
    m_unsaved = true;
    updateTitle();
}

void PoseCaptureWidget::closeEvent(QCloseEvent *event)
{
    if (m_unsaved && !m_closed) {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            tr("Do you really want to close while there are unsaved changes?"),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    m_closed = true;
    hide();
    if (nullptr != m_previewsGenerator) {
        event->ignore();
        return;
    }
    event->accept();
}

void PoseCaptureWidget::previewsReady()
{
    auto resultPreviewMeshs = m_previewsGenerator->takeResultPreviewMeshs(QUuid());
    m_clipPlayer->updateFrameMeshes(resultPreviewMeshs);
    m_previewWidget->show();

    delete m_previewsGenerator;
    m_previewsGenerator = nullptr;
    
    if (m_closed) {
        close();
        return;
    }
    
    if (m_isPreviewsObsolete)
        generatePreviews();
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
    delete m_clipPlayer;
}

void PoseCaptureWidget::changeStateIndicator(PoseCapture::State state)
{
    qDebug() << "State changed:" << (int)state;
}

void PoseCaptureWidget::updateTrack(const PoseCapture::Track &track, const std::vector<qint64> timeline)
{
    qDebug() << "Track changed, size:" << track.size();
    if (track.size() != timeline.size()) {
        qDebug() << "Track size:" << track.size() << "not qual to timeline length:" << timeline.size();
        return;
    }
    
    if (track.empty() || timeline.empty())
        return;
    
    qDebug() << track;
    
    m_poseFrames.clear();
    
    //float frameDuration = (float)((timeline[timeline.size() - 1] - timeline[0]) / timeline.size()) / 1000.0;
    float frameDuration = (float)PoseCapture::TargetSeconds / timeline.size();
    qDebug() << "Track frame duration:" << QString::number(frameDuration);
    for (const auto &it: track) {
        std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>> frame;
        frame.first["duration"] = QString::number(frameDuration);
        frame.second = it;
        m_poseFrames.push_back(frame);
    }
    
    generatePreviews();
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
    QObject::connect(m_imageCapture, &ImageCapture::stopped, this, [&]() {
        m_imageCapture->deleteLater();
        m_imageCapture = nullptr;
    });
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
            emit poseKeypointsDetected(keypointPositions, QSizeF(map["width"].toFloat(), map["height"].toFloat()));
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
