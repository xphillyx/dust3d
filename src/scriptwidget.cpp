#include <QVBoxLayout>
#include <QSplitter>
#include "scriptwidget.h"
#include "scripteditwidget.h"
#include "theme.h"

ScriptWidget::ScriptWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    m_scriptErrorLabel = new InfoLabel;
    m_scriptErrorLabel->hide();
    
    ScriptVariablesWidget *scriptVariablesWidget = new ScriptVariablesWidget(m_document);
    
    ScriptEditWidget *scriptEditWidget = new ScriptEditWidget;

    connect(m_document, &Document::cleanup, scriptEditWidget, &ScriptEditWidget::clear);
    connect(m_document, &Document::scriptModifiedFromExternal, this, [=]() {
        scriptEditWidget->setPlainText(document->script());
    });
    connect(m_document, &Document::scriptErrorChanged, this, &ScriptWidget::updateScriptError);

    connect(scriptEditWidget, &ScriptEditWidget::scriptChanged, m_document, &Document::updateScript);
    
    connect(m_document, &Document::mergedVaraiblesChanged, scriptVariablesWidget, &ScriptVariablesWidget::reload);
    
    QSplitter *splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(scriptVariablesWidget);
    splitter->addWidget(scriptEditWidget);
    splitter->addWidget(m_scriptErrorLabel);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);
    
    /*
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(scriptVariablesWidget);
    mainLayout->addWidget(scriptEditWidget);
    mainLayout->addWidget(m_scriptErrorLabel);
    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 1);
    
    setLayout(mainLayout);
    */
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

