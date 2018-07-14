#ifndef TRACK_NODE_GRAPHICS_ITEM_H
#define TRACK_NODE_GRAPHICS_ITEM_H
#include <QGraphicsItem>
#include <QVector2D>
#include <QObject>
#include "jointnodetree.h"

class TrackNodeGraphicsItem : public QGraphicsItem
{
public:
    TrackNodeGraphicsItem(const QString &id);
    QRectF boundingRect() const;
    void paint(QPainter *painter,
        const QStyleOptionGraphicsItem *option,
        QWidget *widget);
    void setOrigin(float x, float y);
    void setOrigin(QVector2D origin);
    void setFillColor(QColor color);
    void setBorderColor(QColor color);
    void setJointSide(JointSide jointSide);
    const QString &id();
    const QVector2D &origin();
    void setHovered(bool hovered);
private:
    QVector2D m_origin;
    float m_radius = 4;
    QColor m_fillColor;
    QColor m_borderColor;
    QString m_id;
    JointSide m_jointSide = JointSide::None;
    bool m_hovered = false;
};

#endif
