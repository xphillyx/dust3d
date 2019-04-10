#include <QPainter>
#include "imagepreviewwidget.h"

ImagePreviewWidget::ImagePreviewWidget(QWidget *parent) :
    QWidget(parent)
{
}

void ImagePreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawImage(QRect(0, 0, width(), height()), m_image);
}

void ImagePreviewWidget::setImage(const QImage &image)
{
    m_image = image.copy();
    update();
}
