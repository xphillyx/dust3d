#include "skeletonparttreewidget.h"
#include "skeletonpartwidget.h"

SkeletonPartTreeWidget::SkeletonPartTreeWidget(const SkeletonDocument *document, QWidget *parent) :
    QTreeWidget(parent),
    m_document(document)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setContentsMargins(0, 0, 0, 0);
    
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);
    
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);
    
    setColumnCount(1);
    setHeaderHidden(true);
}

void SkeletonPartTreeWidget::partChanged(QUuid partId)
{
}

void SkeletonPartTreeWidget::partTreeChanged()
{
    clear();
    m_itemMap.clear();
    
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(this);
    rootItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    rootItem->setExpanded(true);
    //rootItem->setText(0, tr("<Unnamed>"));
    //rootItem->setFlags(rootItem->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_itemMap[QUuid()] = rootItem;
    insertTopLevelItem(0, rootItem);
    
    for (auto partIdIt = m_document->partIds.begin(); partIdIt != m_document->partIds.end(); partIdIt++) {
        QUuid partId = *partIdIt;
        QTreeWidgetItem *item = new QTreeWidgetItem(rootItem);
        rootItem->addChild(item);
        SkeletonPartWidget *widget = new SkeletonPartWidget(m_document, partId);
        item->setSizeHint(0, widget->size());
        setItemWidget(item, 0, widget);
        widget->reload();
        m_itemMap[partId] = item;
    }
}

void SkeletonPartTreeWidget::groupNameChanged(QTreeWidgetItem *item, int column)
{
    
}

void SkeletonPartTreeWidget::partPreviewChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updatePreview();
}

void SkeletonPartTreeWidget::partLockStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateLockButton();
}

void SkeletonPartTreeWidget::partVisibleStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateVisibleButton();
}

void SkeletonPartTreeWidget::partSubdivStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateSubdivButton();
}

void SkeletonPartTreeWidget::partDisableStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateDisableButton();
}

void SkeletonPartTreeWidget::partXmirrorStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateXmirrorButton();
}

void SkeletonPartTreeWidget::partDeformChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateDeformButton();
}

void SkeletonPartTreeWidget::partRoundStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateRoundButton();
}

void SkeletonPartTreeWidget::partColorStateChanged(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void SkeletonPartTreeWidget::partChecked(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateCheckedState(true);
}

void SkeletonPartTreeWidget::partUnchecked(QUuid partId)
{
    auto item = m_itemMap.find(partId);
    if (item == m_itemMap.end()) {
        //qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateCheckedState(false);
}

QSize SkeletonPartTreeWidget::sizeHint() const
{
    QSize size = SkeletonPartWidget::preferredSize();
    return QSize(size.width() * 1.6, size.height() * 5.5);
}

