#include <QFormLayout>
#include <QComboBox>
#include "rigwidget.h"
#include "rigtype.h"

RigWidget::RigWidget(const SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    QFormLayout *formLayout = new QFormLayout;
    m_rigTypeBox = new QComboBox;
    m_rigTypeBox->setEditable(false);
    
    for (int i = 0; i < (int)RigType::Count; i++) {
        RigType rigType = (RigType)(i);
        m_rigTypeBox->addItem(RigTypeToDispName(rigType));
    }
    
    formLayout->addRow(tr("Type"), m_rigTypeBox);
    
    m_rigTypeBox->setCurrentIndex((int)m_document->rigType);
    
    connect(m_rigTypeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
        RigType currentRigType = (RigType)index;
        emit setRigType(currentRigType);
    });
    
    m_rigWeightRenderWidget = new ModelWidget(this);
    m_rigWeightRenderWidget->setMinimumSize(128, 128);
    m_rigWeightRenderWidget->setXRotation(0);
    m_rigWeightRenderWidget->setYRotation(0);
    m_rigWeightRenderWidget->setZRotation(0);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(formLayout);
    layout->addWidget(m_rigWeightRenderWidget);
    layout->addStretch();
    
    setLayout(layout);
}

void RigWidget::rigTypeChanged()
{
    m_rigTypeBox->setCurrentIndex((int)m_document->rigType);
}

ModelWidget *RigWidget::rigWeightRenderWidget()
{
    return m_rigWeightRenderWidget;
}
