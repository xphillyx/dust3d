#include <QPainter>
#include "posecapturepreviewwidget.h"
#include "theme.h"

PoseCapturePreviewWidget::PoseCapturePreviewWidget(QWidget *parent) :
    QWidget(parent)
{
}

void PoseCapturePreviewWidget::paintEvent(QPaintEvent *event)
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
    float radius = std::min(halfWidth / 2, halfHeight / 2);
    float arcBorderWidth = radius / 8;
    QRectF countdownRect(halfWidth - radius, halfHeight - radius, radius + radius, radius + radius);
    
    auto drawArc = [&](int degrees, const QColor &color) {
        QPainterPath arcPath;
        arcPath.moveTo(halfWidth + radius, halfHeight);
        arcPath.arcTo(countdownRect, 0, degrees);
        
        QPen arcPen;
        arcPen.setCapStyle(Qt::FlatCap);
        arcPen.setColor(color);
        arcPen.setWidth(arcBorderWidth);
        
        painter.strokePath(arcPath, arcPen);
    };
    
    switch (m_currentState) {
    case PoseCapture::State::PreEnter:
        {
            int degrees = 360 * m_stateProgressTimer.elapsed() / PoseCapture::PreEnterDuration;
            if (degrees > 360)
                degrees = 360;
            drawArc(360, Theme::white);
            drawArc(degrees, Theme::red);
        }
        break;
    case PoseCapture::State::Capturing:
        {
            int degrees = 360 * m_stateProgressTimer.elapsed() / PoseCapture::CapturingDuration;
            if (degrees > 360)
                degrees = 360;
            degrees = degrees - 360;
            drawArc(degrees, Theme::red);
        }
        break;
    default:
        break;
    }
}

void PoseCapturePreviewWidget::setImage(const QImage &image)
{
    m_image = image.copy();
    update();
}

void PoseCapturePreviewWidget::setKeypoints(const std::map<QString, QVector3D> &keypoints)
{
    m_keypoints = keypoints;
    update();
}

void PoseCapturePreviewWidget::setState(PoseCapture::State state)
{
    if (m_currentState == state)
        return;
    m_currentState = state;
    if (!m_stateProgressTimer.isValid())
        m_stateProgressTimer.start();
    else
        m_stateProgressTimer.restart();
}
