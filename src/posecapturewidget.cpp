#include <QVBoxLayout>
#include <QThread>
#include "posecapturewidget.h"

PoseCaptureWidget::PoseCaptureWidget(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    m_rawCapturePreviewWidget = new ImagePreviewWidget;
    mainLayout->addWidget(m_rawCapturePreviewWidget);
    
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
    QObject::connect(thread, &QThread::started, m_imageCapture, &ImageCapture::start);
    QObject::connect(m_imageCapture, &ImageCapture::stopped, thread, &QThread::quit);
    QObject::connect(m_imageCapture, &ImageCapture::stopped, m_imageCapture, &QObject::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void PoseCaptureWidget::stopCapture()
{
    if (nullptr == m_imageCapture)
        return;
    
    auto imageCapture = m_imageCapture;
    m_imageCapture = nullptr;
    imageCapture->stop();
}

