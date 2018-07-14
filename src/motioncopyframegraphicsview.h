#ifndef MOTION_COPY_FRAME_GRAPHICS_VIEW_H
#define MOTION_COPY_FRAME_GRAPHICS_VIEW_H
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QMouseEvent>
#include "motioncopydocument.h"
#include "tracknodegraphicsitem.h"

class MotionCopyFrameGraphicsView : public QGraphicsView
{
    Q_OBJECT
signals:
    void moveTrackNodeBy(int frame, QString id, float x, float y, float z);
    void hoverTrackNode(QString id);
public:
    MotionCopyFrameGraphicsView(const MotionCopyDocument *motionCopyDocument, int channel);
public slots:
    void reload();
    void canvasResized(QSize size);
    void updateTackNodePosition(QString id, QVector3D position);
    void removeTrackNode(QString id);
    void trackNodeHovered(QString id);
private:
    void enableBackgroundBlur();
    QVector2D nodeToScenePosition(QVector2D position);
    QVector2D sceneToNodePosition(QVector2D position);
    void addTrackNode(QString id, QVector3D position, QColor color, JointSide jointSide);
    void removeAllTackNodes();
    void addAllTrackNodes();
    QPointF mouseEventScenePos(QMouseEvent *event);
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
private:
    const MotionCopyDocument *m_motionCopyDocument = nullptr;
    int m_channel = 0;
    int m_frame = -1;
    QGraphicsPixmapItem *m_backgroundItem = nullptr;
    QSize m_canvasSize;
    std::map<QString, TrackNodeGraphicsItem *> m_trackNodeItemMap;
    TrackNodeGraphicsItem *m_lastHoveredItem = nullptr;
    bool m_moveStarted = false;
    QPointF m_lastScenePos;
};

#endif
