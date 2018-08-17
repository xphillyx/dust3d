#include <QMenu>
#include <vector>
#include "skeletonparttreewidget.h"
#include "skeletonpartwidget.h"

SkeletonPartTreeWidget::SkeletonPartTreeWidget(const SkeletonDocument *document, QWidget *parent) :
    QTreeWidget(parent),
    m_document(document)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);
    
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);
    
    setColumnCount(1);
    setHeaderHidden(true);
    
    m_componentItemMap[QUuid()] = invisibleRootItem();
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    setStyleSheet(QString("QTreeView {qproperty-indentation: 10; margin-left: 5px;}"));
    
    connect(this, &QTreeWidget::customContextMenuRequested, this, &SkeletonPartTreeWidget::showContextMenu);
    connect(this, &QTreeWidget::itemChanged, this, &SkeletonPartTreeWidget::groupChanged);
    connect(this, &QTreeWidget::itemExpanded, this, &SkeletonPartTreeWidget::groupExpanded);
    connect(this, &QTreeWidget::itemCollapsed, this, &SkeletonPartTreeWidget::groupCollapsed);
}

void SkeletonPartTreeWidget::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = itemAt(pos);
    if (nullptr == item)
        return;
    
    auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    QMenu contextMenu(this);
    
    if (!component->name.isEmpty())
        contextMenu.addSection(component->name);
    
    if (component->linkToPartId.isNull()) {
        
    } else {
        emit checkPart(component->linkToPartId);
    }
    
    QAction moveUpAction(tr("Move Up"), this);
    connect(&moveUpAction, &QAction::triggered, [=]() {
        emit moveComponentUp(componentId);
    });
    contextMenu.addAction(&moveUpAction);
    
    QAction moveDownAction(tr("Move Down"), this);
    connect(&moveDownAction, &QAction::triggered, [=]() {
        emit moveComponentDown(componentId);
    });
    contextMenu.addAction(&moveDownAction);
    
    QAction moveToTopAction(tr("Move To Top"), this);
    connect(&moveToTopAction, &QAction::triggered, [=]() {
        emit moveComponentToTop(componentId);
    });
    contextMenu.addAction(&moveToTopAction);
    
    QAction moveToBottomAction(tr("Move To Bottom"), this);
    connect(&moveToBottomAction, &QAction::triggered, [=]() {
        emit moveComponentToBottom(componentId);
    });
    contextMenu.addAction(&moveToBottomAction);
    
    QMenu *moveToMenu = contextMenu.addMenu(tr("Move To"));
    
    QAction moveToNewGroupAction(tr("New Group"), this);
    moveToMenu->addAction(&moveToNewGroupAction);
    connect(&moveToNewGroupAction, &QAction::triggered, [=]() {
        emit createNewComponentAndMoveThisIn(componentId);
    });
    
    moveToMenu->addSeparator();

    QAction moveToRootGroupAction(tr("Root"), this);
    moveToMenu->addAction(&moveToRootGroupAction);
    connect(&moveToRootGroupAction, &QAction::triggered, [=]() {
        emit moveComponent(componentId, QUuid());
    });
    
    std::vector<QAction *> groupsActions;
    std::function<void(QUuid, int)> addChildGroupsFunc;
    addChildGroupsFunc = [this, &groupsActions, &addChildGroupsFunc, &moveToMenu, &componentId](QUuid currentId, int tabs) -> void {
        const SkeletonComponent *current = m_document->findComponent(currentId);
        if (nullptr == current)
            return;
        if (!current->id.isNull() && current->linkDataType().isEmpty()) {
            QAction *action = new QAction(QString(" ").repeated(tabs * 4) + current->name, this);
            connect(action, &QAction::triggered, [=]() {
                emit moveComponent(componentId, current->id);
            });
            groupsActions.push_back(action);
            moveToMenu->addAction(action);
        }
        for (const auto &childId: current->childrenIds) {
            addChildGroupsFunc(childId, tabs + 1);
        }
    };
    addChildGroupsFunc(QUuid(), 0);
    
    contextMenu.addSeparator();
    
    QAction deleteAction(tr("Delete"), this);
    connect(&deleteAction, &QAction::triggered, [=]() {
        emit removeComponent(componentId);
    });
    contextMenu.addAction(&deleteAction);
    
    contextMenu.exec(mapToGlobal(pos));
    
    for (const auto &action: groupsActions) {
        delete action;
    }
}

QTreeWidgetItem *SkeletonPartTreeWidget::findComponentItem(QUuid componentId)
{
    auto findResult = m_componentItemMap.find(componentId);
    if (findResult == m_componentItemMap.end())
        return nullptr;
    
    return findResult->second;
}

void SkeletonPartTreeWidget::componentNameChanged(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end()) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    componentItem->second->setText(0, component->name);
}

void SkeletonPartTreeWidget::componentExpandStateChanged(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end()) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    componentItem->second->setExpanded(component->expanded);
}

void SkeletonPartTreeWidget::addComponentChildrenToItem(QUuid componentId, QTreeWidgetItem *parentItem)
{
    const SkeletonComponent *parentComponent = m_document->findComponent(componentId);
    if (nullptr == parentComponent)
        return;
    
    for (const auto &childId: parentComponent->childrenIds) {
        const SkeletonComponent *component = m_document->findComponent(childId);
        if (nullptr == component)
            continue;
        if (!component->linkToPartId.isNull()) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            parentItem->addChild(item);
            item->setData(0, Qt::UserRole, QVariant(component->id.toString()));
            QUuid partId = component->linkToPartId;
            SkeletonPartWidget *widget = new SkeletonPartWidget(m_document, partId);
            item->setSizeHint(0, SkeletonPartWidget::preferredSize());
            setItemWidget(item, 0, widget);
            widget->reload();
            m_partItemMap[partId] = item;
            addComponentChildrenToItem(childId, item);
        } else {
            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(component->name));
            parentItem->addChild(item);
            item->setData(0, Qt::UserRole, QVariant(component->id.toString()));
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            item->setExpanded(component->expanded);
            item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_componentItemMap[childId] = item;
            addComponentChildrenToItem(childId, item);
        }
    }
}

void SkeletonPartTreeWidget::componentChildrenChanged(QUuid componentId)
{
    QTreeWidgetItem *parentItem = findComponentItem(componentId);
    if (nullptr == parentItem) {
        qDebug() << "Find component item failed:" << componentId;
        return;
    }
    qDeleteAll(parentItem->takeChildren());
    addComponentChildrenToItem(componentId, parentItem);
    
    // Fix part widget show improperly even the parent item is collapsed.
    bool isExpanded = parentItem->isExpanded();
    parentItem->setExpanded(!isExpanded);
    parentItem->setExpanded(isExpanded);
}

void SkeletonPartTreeWidget::removeAllContent()
{
    qDeleteAll(invisibleRootItem()->takeChildren());
    m_partItemMap.clear();
    m_componentItemMap.clear();
    m_componentItemMap[QUuid()] = invisibleRootItem();
}

void SkeletonPartTreeWidget::componentRemoved(QUuid componentId)
{
    auto componentItem = m_componentItemMap.find(componentId);
    if (componentItem == m_componentItemMap.end())
        return;
    
    m_componentItemMap.erase(componentId);
}

void SkeletonPartTreeWidget::componentAdded(QUuid componentId)
{
    // ignore
}

void SkeletonPartTreeWidget::partRemoved(QUuid partId)
{
    auto partItem = m_partItemMap.find(partId);
    if (partItem == m_partItemMap.end())
        return;
    
    m_partItemMap.erase(partItem);
}

void SkeletonPartTreeWidget::groupChanged(QTreeWidgetItem *item, int column)
{
    if (0 != column)
        return;
    
    auto componentId = QUuid(item->data(0, Qt::UserRole).toString());
    
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        qDebug() << "Find component failed:" << componentId;
        return;
    }
    
    if (item->text(0) != component->name)
        emit renameComponent(componentId, item->text(0));
}

void SkeletonPartTreeWidget::groupExpanded(QTreeWidgetItem *item)
{
    QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
    emit setComponentExpandState(componentId, true);
}

void SkeletonPartTreeWidget::groupCollapsed(QTreeWidgetItem *item)
{
    QUuid componentId = QUuid(item->data(0, Qt::UserRole).toString());
    emit setComponentExpandState(componentId, false);
}

void SkeletonPartTreeWidget::partPreviewChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updatePreview();
}

void SkeletonPartTreeWidget::partLockStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateLockButton();
}

void SkeletonPartTreeWidget::partVisibleStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateVisibleButton();
}

void SkeletonPartTreeWidget::partSubdivStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateSubdivButton();
}

void SkeletonPartTreeWidget::partDisableStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateDisableButton();
}

void SkeletonPartTreeWidget::partXmirrorStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateXmirrorButton();
}

void SkeletonPartTreeWidget::partDeformChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateDeformButton();
}

void SkeletonPartTreeWidget::partRoundStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateRoundButton();
}

void SkeletonPartTreeWidget::partColorStateChanged(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateColorButton();
}

void SkeletonPartTreeWidget::partChecked(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateCheckedState(true);
}

void SkeletonPartTreeWidget::partUnchecked(QUuid partId)
{
    auto item = m_partItemMap.find(partId);
    if (item == m_partItemMap.end()) {
        //qDebug() << "Part item not found:" << partId;
        return;
    }
    SkeletonPartWidget *widget = (SkeletonPartWidget *)itemWidget(item->second, 0);
    widget->updateCheckedState(false);
}

QSize SkeletonPartTreeWidget::sizeHint() const
{
    QSize size = SkeletonPartWidget::preferredSize();
    return QSize(size.width() * 1.3, size.height() * 5.5);
}

