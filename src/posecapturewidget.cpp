#include <QVBoxLayout>
#include <QThread>
#include <QBuffer>
#include <QDebug>
#include <string>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector3D>
#include "posecapturewidget.h"

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

    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    m_webView = new QWebEngineView;
    m_webView->hide();
    m_webView->setPage(new WebEnginePage(m_webView));
    connect(m_webView->page(), &QWebEnginePage::loadFinished, this, [&](bool ok) {
        Q_UNUSED(ok);
        m_webLoaded = true;
    });
    m_webView->setUrl(QUrl("qrc:/scripts/motioncapture.html"));
    
    m_rawCapturePreviewWidget = new ImagePreviewWidget;
    
    QObject::connect(this, &PoseCaptureWidget::poseKeypointsDetected, m_rawCapturePreviewWidget, &ImagePreviewWidget::setKeypoints);
    
    mainLayout->addWidget(m_rawCapturePreviewWidget);
    mainLayout->addWidget(m_webView);
    
    setLayout(mainLayout);
    
    setMinimumSize(640, 480);
    
    startCapture();
}

PoseCaptureWidget::~PoseCaptureWidget()
{
    stopCapture();
}

void PoseCaptureWidget::startCapture()
{
    if (nullptr != m_imageCapture)
        return;
    
    m_imageCapture = new ImageCapture;
    
    QThread *thread = new QThread;
    m_imageCapture->moveToThread(thread);
    
    QObject::connect(m_imageCapture, &ImageCapture::imageCaptured, m_rawCapturePreviewWidget, &ImagePreviewWidget::setImage);
    QObject::connect(m_imageCapture, &ImageCapture::imageCaptured, this, &PoseCaptureWidget::updateCapturedImage);
    QObject::connect(thread, &QThread::started, m_imageCapture, &ImageCapture::start);
    QObject::connect(m_imageCapture, &ImageCapture::stopped, thread, &QThread::quit);
    QObject::connect(m_imageCapture, &ImageCapture::stopped, m_imageCapture, &QObject::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

#define MIN_POSE_CONFIDENCE 0.7

void PoseCaptureWidget::updateCapturedImage(const QImage &image)
{
    if (!m_webLoaded)
        return;
    
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
