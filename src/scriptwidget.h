#ifndef SCRIPT_WIDGET_H
#define SCRIPT_WIDGET_H
#include <QWidget>
#include "document.h"

class ScriptWidget : public QWidget
{
    Q_OBJECT
public:
    ScriptWidget(const Document *document, QWidget *parent=nullptr);
protected:
    QSize sizeHint() const override;
private:
    const Document *m_document = nullptr;
};

#endif
