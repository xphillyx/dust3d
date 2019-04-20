#include <QPainter>
#include "imagepreviewwidget.h"
#include "theme.h"

ImagePreviewWidget::ImagePreviewWidget(QWidget *parent) :
    QWidget(parent)
{
}

void ImagePreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_image.width() <=0 || m_image.height() <= 0)
        return;
    QPainter painter(this);
    painter.drawImage(QRect(0, 0, width(), height()), m_image);
    float xRatio = (float)width() / m_image.width();
    float yRatio = (float)height() / m_image.height();
    
    painter.setPen(QPen(QColor(0xaa, 0xeb, 0xc4), 4));
    
    auto drawLine = [&](const QVector3D &fromPosition, const QVector3D &toPosition) {
        float fromX = width() - xRatio * fromPosition.x();
        float fromY = yRatio * fromPosition.y();
        float toX = width() - xRatio * toPosition.x();
        float toY = yRatio * toPosition.y();
        painter.drawLine(fromX, fromY, toX, toY);
    };
    
    auto drawPair = [&, drawLine](const QString &fromName, const QString &toName) {
        auto findFrom = m_keypoints.find(fromName);
        auto findTo = m_keypoints.find(toName);
        if (findFrom == m_keypoints.end())
            return;
        if (findTo == m_keypoints.end())
            return;
        drawLine(findFrom->second, findTo->second);
    };
    
    drawPair("leftShoulder", "leftElbow");
    drawPair("leftElbow", "leftWrist");
    drawPair("rightShoulder", "rightElbow");
    drawPair("rightElbow", "rightWrist");
    
    drawPair("leftHip", "leftKnee");
    drawPair("leftKnee", "leftAnkle");
    drawPair("rightHip", "rightKnee");
    drawPair("rightKnee", "rightAnkle");
    
    float fullWidth = (float)width();
    float fullHeight = (float)height();
    float halfWidth = fullWidth / 2;
    float halfHeight = fullHeight / 2;
    float radius = std::min(halfWidth / 7, halfHeight / 7);
    float margin = radius / 2;
    float arcBorderWidth = radius / 8;
    QRectF countdownRect(margin, margin, radius + radius, radius + radius);
    
    auto drawArc = [&](int degrees, const QColor &color) {
        QPainterPath arcPath;
        arcPath.moveTo(margin + radius + radius, margin + radius);
        arcPath.arcTo(countdownRect, 0, degrees);
        
        QPen arcPen;
        arcPen.setCapStyle(Qt::FlatCap);
        arcPen.setColor(color);
        arcPen.setWidth(arcBorderWidth);
        
        painter.strokePath(arcPath, arcPen);
    };
    
    drawArc(360, Theme::white);
    drawArc(37, Theme::red);
    
    painter.setPen(QPen());
    painter.setBrush(Qt::red);
    painter.drawEllipse(margin, margin, radius + radius, radius + radius);
}

void ImagePreviewWidget::setImage(const QImage &image)
{
    m_image = image.copy();
    update();
}

void ImagePreviewWidget::setKeypoints(const std::map<QString, QVector3D> &keypoints)
{
    m_keypoints = keypoints;
    update();
}
