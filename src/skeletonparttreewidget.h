#ifndef SKELETON_PART_TREE_WIDGET_H
#define SKELETON_PART_TREE_WIDGET_H
#include <QTreeWidget>
#include <QUuid>
#include "skeletondocument.h"

class SkeletonPartTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    SkeletonPartTreeWidget(const SkeletonDocument *document, QWidget *parent);
public slots:
    void partChanged(QUuid partId);
    void partTreeChanged();
    void partPreviewChanged(QUuid partid);
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partSubdivStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void partXmirrorStateChanged(QUuid partId);
    void partDeformChanged(QUuid partId);
    void partRoundStateChanged(QUuid partId);
    void partColorStateChanged(QUuid partId);
    void partChecked(QUuid partId);
    void partUnchecked(QUuid partId);
    void groupNameChanged(QTreeWidgetItem *item, int column);
protected:
    virtual QSize sizeHint() const;
private:
    const SkeletonDocument *m_document = nullptr;
    std::map<QUuid, QTreeWidgetItem *> m_itemMap;
};

#endif
