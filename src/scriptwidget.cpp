#include <QVBoxLayout>
#include "scriptwidget.h"
#include "scripteditwidget.h"
#include "theme.h"

ScriptWidget::ScriptWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    m_scriptErrorLabel = new InfoLabel;
    m_scriptErrorLabel->hide();
    
    ScriptEditWidget *scriptEditWidget = new ScriptEditWidget;
    
    connect(m_document, &Document::cleanup, scriptEditWidget, &ScriptEditWidget::clear);
    connect(m_document, &Document::scriptModifiedFromExternal, this, [=]() {
        scriptEditWidget->setPlainText(document->script());
    });
    connect(m_document, &Document::scriptErrorChanged, this, &ScriptWidget::updateScriptError);

    connect(scriptEditWidget, &ScriptEditWidget::scriptChanged, m_document, &Document::updateScript);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(scriptEditWidget);
    mainLayout->addWidget(m_scriptErrorLabel);
    mainLayout->setStretch(0, 1);
    
    setLayout(mainLayout);
}

QSize ScriptWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}

void ScriptWidget::updateScriptError()
{
    const auto &scriptError = m_document->scriptError();
    if (scriptError.isEmpty()) {
        m_scriptErrorLabel->hide();
    } else {
        m_scriptErrorLabel->setText(scriptError);
        m_scriptErrorLabel->setMaximumWidth(width() * 0.90);
        m_scriptErrorLabel->show();
    }
}

