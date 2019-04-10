#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QImage>
#include "imagecapture.h"

ImageCapture::ImageCapture()
{
    m_cond.reset(new QWaitCondition);
    m_mutex.reset(new QMutex);
}

void ImageCapture::start()
{
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        qDebug() << "Video capture open failed";
        emit stopped();
        return;
    }

    for (;;) {
        if (m_stopped) {
            qDebug() << "Video capture stopped by user";
            cap.release();
            emit stopped();
            return;
        }
    
        m_mutex->lock();
        if (m_cond->wait(m_mutex.data(), 0)) {
            m_mutex->unlock();
            qDebug() << "Video capture stopped by user";
            cap.release();
            emit stopped();
            return;
        }
        m_mutex->unlock();

        QScopedPointer<cv::Mat> frame(new cv::Mat);
        if (!cap.read(*frame)) {
            qDebug() << "Video capture read failed";
            cap.release();
            emit stopped();
            return;
        }
        cv::flip(*frame, *frame, 1);

        auto scaleFrameToWidth = [](cv::Mat *frame, int scaleToWidth) {
            if (frame->size().width > scaleToWidth) {
                int scaleToHeight = (int)(frame->size().height * (float)scaleToWidth / frame->size().width);
                cv::resize(*frame, *frame, cv::Size(scaleToWidth, scaleToHeight));
            }
        };
        scaleFrameToWidth(frame.data(), 640);
        
        cv::cvtColor(*frame, *frame, CV_BGR2RGB);

        auto data = frame->data;
        auto cols = frame->cols;
        auto rows = frame->rows;
        auto step = frame->step;
        const QImage image(data,
            cols,
            rows,
            step,
            QImage::Format_RGB888,
            [](void *mat) {
                delete static_cast<cv::Mat*>(mat);
            },
            frame.take());
        emit imageCaptured(image);
    }
}

void ImageCapture::stop()
{
    m_stopped = true;
    m_cond->wakeAll();
}
