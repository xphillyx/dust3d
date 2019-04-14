#ifndef DUST3D_IMAGE_CAPTURE_H
#define DUST3D_IMAGE_CAPTURE_H
#include <QObject>
#include <QWaitCondition>
#include <QMutex>
#include <QImage>

class ImageCapture : public QObject
{
    Q_OBJECT
public:
    ImageCapture(int scaleToWidth=0);
    
signals:
    void stopped();
    void imageCaptured(const QImage &image);
    
public slots:
    void start();
    void stop();
    
private:
    QScopedPointer<QWaitCondition> m_cond;
    QScopedPointer<QMutex> m_mutex;
    bool m_stopped = false;
    int m_scaleToWidth = 600;
};

#endif
