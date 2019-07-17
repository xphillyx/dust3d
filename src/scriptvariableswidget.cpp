#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDebug>
#include "scriptvariableswidget.h"
#include "util.h"
#include "theme.h"

ScriptVariablesWidget::ScriptVariablesWidget(const Document *document,
        QWidget *parent) :
    QScrollArea(parent),
    m_document(document)
{
    setWidgetResizable(true);
    
    reload();
}

void ScriptVariablesWidget::reload()
{
    QWidget *widget = new QWidget;
    QFormLayout *formLayout = new QFormLayout;
    for (const auto &variable: m_document->variables()) {
        const auto &name = variable.first;
        auto value = valueOfKeyInMapOrEmpty(variable.second, "value").toFloat();
        qDebug() << "Script variable, name:" << name << "value:" << value;
        QDoubleSpinBox *inputBox = new QDoubleSpinBox;
        inputBox->setValue(value);
        formLayout->addRow(name, inputBox);
    }
    widget->setLayout(formLayout);
    
    setWidget(widget);
}

QSize ScriptVariablesWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}
