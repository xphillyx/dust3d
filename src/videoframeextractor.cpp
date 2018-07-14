#include <QGuiApplication>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include "videoframeextractor.h"

VideoFrameExtractor::VideoFrameExtractor(const QString &fileName, const QString &realPath, QTemporaryFile *fileHandle, float thumbnailHeight, int maxFrames) :
    m_fileName(fileName),
    m_realPath(realPath),
    m_fileHandle(fileHandle),
    m_maxFrames(maxFrames),
    m_thumbnailHeight(thumbnailHeight)
{
}

const QString &VideoFrameExtractor::fileName()
{
    return m_fileName;
}

const QString &VideoFrameExtractor::realPath()
{
    return m_realPath;
}

QTemporaryFile *VideoFrameExtractor::fileHandle()
{
    return m_fileHandle;
}

VideoFrameExtractor::~VideoFrameExtractor()
{
    delete m_resultFrames;
    release();
}

void VideoFrameExtractor::release()
{
}

void VideoFrameExtractor::process()
{
    extract();
    
    release();
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

std::vector<VideoFrame> *VideoFrameExtractor::takeResultFrames()
{
    std::vector<VideoFrame> *result = m_resultFrames;
    m_resultFrames = nullptr;
    return result;
}

void VideoFrameExtractor::extract()
{
    delete m_resultFrames;
    m_resultFrames = new std::vector<VideoFrame>;
    cv::VideoCapture cap(m_realPath.toUtf8().constData());
    if (!cap.isOpened()) {
        qDebug() << "cap.isOpened() return false";
        return;
    }
    int frameIndex = 0;
    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
            break;
        
        if (3 != frame.channels()) {
            qDebug() << "frame.channels():" << frame.channels() << " not supported";
            break;
        }
        
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, CV_BGR2RGB);
        
        VideoFrame videoFrame;
        QImage image = QImage((const unsigned char*)(rgbFrame.data), rgbFrame.cols, rgbFrame.rows, QImage::Format_RGB888);
        videoFrame.image = image.copy();
        videoFrame.thumbnail = image.scaledToHeight(m_thumbnailHeight);
        m_resultFrames->push_back(videoFrame);
        
        frameIndex++;
        if (frameIndex >= m_maxFrames)
            break;
    }
    cap.release();
}

