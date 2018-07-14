#include <QTimer>
#include "motioncopyframegraphicsview.h"

MotionCopyFrameGraphicsView::MotionCopyFrameGraphicsView(const MotionCopyDocument *motionCopyDocument, int channel) :
    m_motionCopyDocument(motionCopyDocument),
    m_channel(channel)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    setRenderHint(QPainter::Antialiasing, false);
    setBackgroundBrush(QBrush(QWidget::palette().color(QWidget::backgroundRole()), Qt::SolidPattern));
    setContentsMargins(0, 0, 0, 0);
    setFrameStyle(QFrame::NoFrame);
    
    setAlignment(Qt::AlignCenter);
    
    setScene(new QGraphicsScene());
    
    m_backgroundItem = new QGraphicsPixmapItem();
    enableBackgroundBlur();
    scene()->addItem(m_backgroundItem);
    
    setMouseTracking(true);
}

void MotionCopyFrameGraphicsView::enableBackgroundBlur()
{
    m_backgroundItem->setOpacity(0.25);
}

void MotionCopyFrameGraphicsView::reload()
{
    m_frame = m_motionCopyDocument->currentFrame();
    if (-1 == m_frame) {
        return;
    }
    const auto &frames = m_motionCopyDocument->videoFrames(m_channel);
    if (m_frame >= (int)frames.size()) {
        return;
    }
    const auto &image = frames[m_frame].image;
    if (image.isNull())
        return;
    QImage resizedImage = image.scaled(m_canvasSize, Qt::KeepAspectRatio);
    m_backgroundItem->setPixmap(QPixmap::fromImage(resizedImage));
    setFixedSize(resizedImage.size());
    scene()->setSceneRect(rect());
    
    removeAllTackNodes();
    addAllTrackNodes();
}

void MotionCopyFrameGraphicsView::canvasResized(QSize size)
{
    m_canvasSize = size;
    reload();
}

void MotionCopyFrameGraphicsView::addTrackNode(QString id, QVector3D position, QColor color, JointSide jointSide)
{
    const auto &findResult = m_trackNodeItemMap.find(id);
    if (findResult != m_trackNodeItemMap.end()) {
        qDebug() << "Track node already exists:" << id;
    } else {
        TrackNodeGraphicsItem *item = new TrackNodeGraphicsItem(id);
        item->setFillColor(color);
        item->setBorderColor(color);
        item->setJointSide(jointSide);
        scene()->addItem(item);
        m_trackNodeItemMap[id] = item;
    }
    updateTackNodePosition(id, position);
}

QVector2D MotionCopyFrameGraphicsView::nodeToScenePosition(QVector2D position)
{
    QVector2D scenePosition;
    scenePosition.setX(scene()->sceneRect().center().x() + scene()->sceneRect().width() * 0.7 * position.x());
    scenePosition.setY(scene()->sceneRect().center().y() + scene()->sceneRect().height() * 0.7 * position.y());
    return scenePosition;
}

QVector2D MotionCopyFrameGraphicsView::sceneToNodePosition(QVector2D position)
{
    QVector2D nodePosition;
    nodePosition.setX((position.x() - scene()->sceneRect().center().x()) / (scene()->sceneRect().width() * 0.7));
    nodePosition.setY((position.y() - scene()->sceneRect().center().y()) / (scene()->sceneRect().height() * 0.7));
    return nodePosition;
}

void MotionCopyFrameGraphicsView::removeAllTackNodes()
{
    std::vector<TrackNodeGraphicsItem *> contentItems;
    for (const auto &it: items()) {
        QGraphicsItem *item = it;
        if (item->data(0) == "trackNode") {
            auto trackNodeGraphicsItem = ((TrackNodeGraphicsItem *)item);
            m_trackNodeItemMap.erase(trackNodeGraphicsItem->id());
            contentItems.push_back(trackNodeGraphicsItem);
        }
    }
    for (size_t i = 0; i < contentItems.size(); i++) {
        TrackNodeGraphicsItem *item = contentItems[i];
        qDebug() << "channel:" << m_channel << "frame:" << m_frame << " removeItem:" << item->id();
        //scene()->removeItem(item);
        delete item;
    }
    Q_ASSERT(m_trackNodeItemMap.size() == 0);
    m_lastHoveredItem = nullptr;
    update();
}

void MotionCopyFrameGraphicsView::removeTrackNode(QString id)
{
    const auto &findResult = m_trackNodeItemMap.find(id);
    if (findResult == m_trackNodeItemMap.end()) {
        qDebug() << "Find track node item failed:" << id;
        return;
    }
    if (nullptr != m_lastHoveredItem && m_lastHoveredItem->id() == id)
        m_lastHoveredItem = nullptr;
    scene()->removeItem(findResult->second);
    delete findResult->second;
    m_trackNodeItemMap.erase(findResult);
}

void MotionCopyFrameGraphicsView::updateTackNodePosition(QString id, QVector3D position)
{
    const auto &findResult = m_trackNodeItemMap.find(id);
    if (findResult == m_trackNodeItemMap.end()) {
        qDebug() << "Track node item not found:" << id;
        return;
    }
    if (0 == m_channel) {
        findResult->second->setOrigin(nodeToScenePosition(QVector2D(position.x(), -position.y())));
    } else {
        findResult->second->setOrigin(nodeToScenePosition(QVector2D(-position.z(), -position.y())));
    }
}

void MotionCopyFrameGraphicsView::addAllTrackNodes()
{
    if (-1 == m_frame)
        return;
    
    const auto &trackNodeInstances = m_motionCopyDocument->getTrackNodeInstances(m_frame);
    if (nullptr == trackNodeInstances)
        return;
    
    for (const auto &it: *trackNodeInstances) {
        const auto &trackNode = m_motionCopyDocument->findTrackNode(it.first);
        if (nullptr == trackNode) {
            qDebug() << "Track node not found:" << it.first;
            continue;
        }
        if (!trackNode->visible)
            continue;
        qDebug() << "channel:" << m_channel << "frame:" << m_frame << " addItem:" << it.first;
        addTrackNode(it.first, it.second.position, SkeletonBoneMarkToColor(trackNode->markedNode.boneMark), trackNode->markedNode.jointSide);
    }
}

QPointF MotionCopyFrameGraphicsView::mouseEventScenePos(QMouseEvent *event)
{
    return mapToScene(mapFromGlobal(event->globalPos()));
}

void MotionCopyFrameGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    
    if (!m_moveStarted) {
        QPointF scenePos = mouseEventScenePos(event);
        
        TrackNodeGraphicsItem *hoveredItem = nullptr;
        QList<QGraphicsItem *> items = scene()->items(scenePos);
        for (auto it = items.begin(); it != items.end(); it++) {
            QGraphicsItem *item = *it;
            if (item->data(0) == "trackNode") {
                hoveredItem = (TrackNodeGraphicsItem *)item;
                break;
            }
        }
        
        if (hoveredItem != m_lastHoveredItem) {
            if (nullptr != m_lastHoveredItem) {
                m_lastHoveredItem->setHovered(false);
            }
            if (nullptr != hoveredItem) {
                hoveredItem->setHovered(true);
            }
            m_lastHoveredItem = hoveredItem;
            emit hoverTrackNode(nullptr == m_lastHoveredItem ? QString() : m_lastHoveredItem->id());
        }
    }
    
    if (m_moveStarted && nullptr != m_lastHoveredItem) {
        QPointF mouseScenePos = mouseEventScenePos(event);
        float byX = mouseScenePos.x() - m_lastScenePos.x();
        float byY = mouseScenePos.y() - m_lastScenePos.y();
        auto currentOriginInScene = nodeToScenePosition(m_lastHoveredItem->origin());
        auto newOriginInScene = QVector2D(currentOriginInScene.x() + byX, currentOriginInScene.y() + byY);
        auto newOrigin = sceneToNodePosition(newOriginInScene);
        if (0 == m_channel) {
            emit moveTrackNodeBy(m_frame, m_lastHoveredItem->id(),
                (newOrigin.x() - m_lastHoveredItem->origin().x()),
                -(newOrigin.y() - m_lastHoveredItem->origin().y()),
                0);
        } else {
            emit moveTrackNodeBy(m_frame, m_lastHoveredItem->id(),
                0,
                -(newOrigin.y() - m_lastHoveredItem->origin().y()),
                -(newOrigin.x() - m_lastHoveredItem->origin().x()));
        }
        m_lastScenePos = mouseScenePos;
    }
}

void MotionCopyFrameGraphicsView::trackNodeHovered(QString id)
{
    TrackNodeGraphicsItem *hoveredItem = nullptr;
    
    if (m_moveStarted)
        return;
    
    if (!id.isEmpty()) {
        const auto &findResult = m_trackNodeItemMap.find(id);
        if (findResult != m_trackNodeItemMap.end())
            hoveredItem = findResult->second;
    }
    
    if (hoveredItem != m_lastHoveredItem) {
        if (nullptr != m_lastHoveredItem) {
            m_lastHoveredItem->setHovered(false);
        }
        if (nullptr != hoveredItem) {
            hoveredItem->setHovered(true);
        }
        m_lastHoveredItem = hoveredItem;
    }
}

void MotionCopyFrameGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton) {
        if (m_moveStarted) {
            m_moveStarted = false;
        }
    }
}

void MotionCopyFrameGraphicsView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        if (!m_moveStarted && nullptr != m_lastHoveredItem) {
            m_moveStarted = true;
            m_lastScenePos = mouseEventScenePos(event);
        }
    }
}
