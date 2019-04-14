#include <QVBoxLayout>
#include <QThread>
#include <QBuffer>
#include <QDebug>
#include <string>
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
    m_webView->setPage(new WebEnginePage(m_webView));
    connect(m_webView->page(), &QWebEnginePage::loadFinished, this, [&](bool ok) {
        Q_UNUSED(ok);
        m_webLoaded = true;
    });
    m_webView->setUrl(QUrl("qrc:/scripts/motioncapture.html"));
    
    m_rawCapturePreviewWidget = new ImagePreviewWidget;
    
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

void PoseCaptureWidget::updateCapturedImage(const QImage &image)
{
    if (!m_webLoaded)
        return;
    
    QByteArray bateArray;
    QBuffer buffer(&bateArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "BMP");
    m_webView->page()->runJavaScript(QString("updateImage(\"data:image/bmp;base64,%1\")").arg(QString::fromLatin1(bateArray.toBase64())),
            [](const QVariant &result) {
            //qDebug() << result.toString();
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
