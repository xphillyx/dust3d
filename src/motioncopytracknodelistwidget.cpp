#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "motioncopytracknodelistwidget.h"
#include "version.h"
#include "infolabel.h"

MotionCopyTrackNodeListWidget::MotionCopyTrackNodeListWidget(const MotionCopyDocument *motionCopyDocument, QWidget *parent) :
    QWidget(parent),
    m_motionCopyDocument(motionCopyDocument)
{
    const auto &markedNodes = m_motionCopyDocument->markedNodes();
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    if (markedNodes.empty()) {
        InfoLabel *infoLabel = new InfoLabel;
        infoLabel->setText(tr("There is no marked node"));
        mainLayout->addWidget(infoLabel);
    } else {
        QGridLayout *gridLayout = new QGridLayout;
        for (size_t row = 0; row < markedNodes.size(); row++) {
            const auto &markedNode = markedNodes[row];
            QCheckBox *check = new QCheckBox();
            check->setStyleSheet(QString("QCheckBox {color: %1}").arg(SkeletonBoneMarkToColor(markedNode.boneMark).name()));
            QLabel *nameLabel = new QLabel(markedNode.toString());
            m_ctrlMap[markedNode.toKey()] = std::make_tuple(check, nameLabel);
            connect(check, &QCheckBox::stateChanged, [=] (int newState) {
                if (Qt::Checked == newState) {
                    emit checkMarkedNode(markedNode);
                } else {
                    emit uncheckMarkedNode(markedNode);
                }
            });
            gridLayout->addWidget(check, row, 0);
            gridLayout->addWidget(nameLabel, row, 1);
        }
        mainLayout->addLayout(gridLayout);
    }
    
    setLayout(mainLayout);
    
    setWindowTitle(APP_NAME);
    
    reload();
}

void MotionCopyTrackNodeListWidget::reload()
{
    for (const auto &it: m_ctrlMap) {
        const auto &trackNode = m_motionCopyDocument->findTrackNode(it.first);
        if (nullptr != trackNode && trackNode->visible) {
            std::get<0>(it.second)->setChecked(true);
        } else {
            std::get<0>(it.second)->setChecked(false);
        }
    }
}

void MotionCopyTrackNodeListWidget::markedNodeHovered(QString key)
{
    QLabel *hoveredLabel = nullptr;
    
    if (!key.isEmpty()) {
        const auto &findResult = m_ctrlMap.find(key);
        if (findResult != m_ctrlMap.end()) {
            hoveredLabel = std::get<1>(findResult->second);
        }
    }
    
    if (hoveredLabel != m_lastHoveredLabel) {
        if (nullptr != m_lastHoveredLabel) {
            m_lastHoveredLabel->setStyleSheet(QString());
        }
        if (nullptr != hoveredLabel) {
            const auto &trackNode = m_motionCopyDocument->findTrackNode(key);
            if (nullptr != trackNode) {
                hoveredLabel->setStyleSheet(QString("QLabel {color: %1}").arg(SkeletonBoneMarkToColor(trackNode->markedNode.boneMark).name()));
            }
        }
        m_lastHoveredLabel = hoveredLabel;
    }
}
