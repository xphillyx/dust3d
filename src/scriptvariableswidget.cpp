#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDebug>
#include "scriptvariableswidget.h"
#include "util.h"
#include "theme.h"

ScriptVariablesWidget::ScriptVariablesWidget(const Document *document,
        QWidget *parent) :
    QTreeWidget(parent),
    m_document(document)
{
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    setAutoScroll(false);
    
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);
    
    setColumnCount(1);
    setHeaderHidden(true);
    
    reload();
}

void ScriptVariablesWidget::reload()
{
    clear();

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
    
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable));
    addTopLevelItem(item);
    setItemWidget(item, 0, widget);
}

QSize ScriptVariablesWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}
