#include <QPen>
#include <QPainter>
#include "tracknodegraphicsitem.h"
#include "theme.h"

TrackNodeGraphicsItem::TrackNodeGraphicsItem(const QString &id) :
    m_id(id)
{
    setData(0, "trackNode");
}

void TrackNodeGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QRectF rect = boundingRect();
    
    QColor fillColor = m_fillColor;
    fillColor.setAlphaF(m_hovered ? Theme::checkedAlpha : Theme::fillAlpha);
    
    QBrush brush(fillColor);
    painter->setBrush(brush);
    
    QPen pen(m_borderColor, 0);
    painter->setPen(pen);
    painter->drawRect(rect);
    
    switch (m_jointSide) {
        case JointSide::None:
            break;
        case JointSide::Right:
            painter->drawLine(rect.right() - 1, rect.top() + 1, rect.left(), rect.bottom());
            break;
        case JointSide::Left:
            painter->drawLine(rect.right(), rect.bottom(), rect.left() + 1, rect.top() + 1);
            break;
    }
}

void TrackNodeGraphicsItem::setFillColor(QColor color)
{
    m_fillColor = color;
    update();
}

void TrackNodeGraphicsItem::setBorderColor(QColor color)
{
    m_borderColor = color;
    update();
}

QRectF TrackNodeGraphicsItem::boundingRect() const
{
    return QRectF(m_origin.x() - m_radius,
        m_origin.y() - m_radius,
        m_radius + m_radius,
        m_radius + m_radius);
}

void TrackNodeGraphicsItem::setOrigin(float x, float y)
{
    setOrigin(QVector2D(x, y));
}

void TrackNodeGraphicsItem::setOrigin(QVector2D origin)
{
    m_origin = origin;
    prepareGeometryChange();
    update();
}

const QString &TrackNodeGraphicsItem::id()
{
    return m_id;
}

void TrackNodeGraphicsItem::setJointSide(JointSide jointSide)
{
    m_jointSide = jointSide;
}

const QVector2D &TrackNodeGraphicsItem::origin()
{
    return m_origin;
}

void TrackNodeGraphicsItem::setHovered(bool hovered)
{
    m_hovered = hovered;
    update();
}
