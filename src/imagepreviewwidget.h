#ifndef DUST3D_IMAGE_PREVIEW_WIDGET_H
#define DUST3D_IMAGE_PREVIEW_WIDGET_H
#include <QWidget>
#include <QImage>

class ImagePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    ImagePreviewWidget(QWidget *parent=nullptr);
    
protected:
    void paintEvent(QPaintEvent *event);
    
public slots:
    void setImage(const QImage &image);
    
private:
    QImage m_image;
};

#endif