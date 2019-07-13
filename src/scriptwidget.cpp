#include <QVBoxLayout>
#include "scriptwidget.h"
#include "scripteditwidget.h"
#include "theme.h"

ScriptWidget::ScriptWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    ScriptEditWidget *scriptEditWidget = new ScriptEditWidget;
    
    connect(scriptEditWidget, &ScriptEditWidget::scriptChanged, m_document, &Document::updateScript);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(scriptEditWidget);
    
    setLayout(mainLayout);
}

QSize ScriptWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}

