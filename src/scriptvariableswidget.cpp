#include <QDoubleSpinBox>
#include <QFormLayout>
#include "scriptvariableswidget.h"
#include "util.h"

ScriptVariablesWidget::ScriptVariablesWidget(const std::map<QString, std::map<QString, QString>> &varaibles,
        QWidget *parent) :
    QWidget(parent)
{
    QFormLayout *formLayout = new QFormLayout;
    for (const auto &variable: varaibles) {
        const auto &name = variable.first;
        auto value = valueOfKeyInMapOrEmpty(variable.second, "value").toFloat();
        QDoubleSpinBox *inputBox = new QDoubleSpinBox;
        inputBox->setValue(value);
        formLayout->addRow(name, inputBox);
    }
    
    setLayout(formLayout);
}
