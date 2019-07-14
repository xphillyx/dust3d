#ifndef SCRIPT_WIDGET_H
#define SCRIPT_WIDGET_H
#include <QWidget>
#include "document.h"
#include "infolabel.h"

class ScriptWidget : public QWidget
{
    Q_OBJECT
public:
    ScriptWidget(const Document *document, QWidget *parent=nullptr);
public slots:
    void updateScriptError();
protected:
    QSize sizeHint() const override;
private:
    const Document *m_document = nullptr;
    InfoLabel *m_scriptErrorLabel = nullptr;
};

#endif
