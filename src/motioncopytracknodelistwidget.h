#ifndef MOTION_COPY_TRACK_NODE_LIST_WIDGET_H
#define MOTION_COPY_TRACK_NODE_LIST_WIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QLabel>
#include "motioncopydocument.h"
#include "skeletondocument.h"

class MotionCopyTrackNodeListWidget : public QWidget
{
    Q_OBJECT
signals:
    void checkMarkedNode(JointMarkedNode markedNode);
    void uncheckMarkedNode(JointMarkedNode markedNode);
public:
    MotionCopyTrackNodeListWidget(const MotionCopyDocument *motionCopyDocument, QWidget *parent=nullptr);
public slots:
    void reload();
    void markedNodeHovered(QString key);
private:
    const SkeletonDocument *m_skeletonDocument = nullptr;
    const MotionCopyDocument *m_motionCopyDocument = nullptr;
    std::map<QString, std::tuple<QCheckBox *, QLabel *>> m_ctrlMap;
    QLabel *m_lastHoveredLabel = nullptr;
};

#endif
