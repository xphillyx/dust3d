#ifndef DUST3D_IMAGE_PREVIEW_WIDGET_H
#define DUST3D_IMAGE_PREVIEW_WIDGET_H
#include <QWidget>
#include <QImage>
#include <QVector3D>

class ImagePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    ImagePreviewWidget(QWidget *parent=nullptr);
    
protected:
    void paintEvent(QPaintEvent *event);
    
public slots:
    void setImage(const QImage &image);
    void setKeypoints(const std::map<QString, QVector3D> &keypoints);
    
private:
    QImage m_image;
    std::map<QString, QVector3D> m_keypoints;
};

#endif
