#ifndef SCRIPT_EDIT_WIDGET_H
#define SCRIPT_EDIT_WIDGET_H
#include <QWidget>
#include <QTextEdit>

class ScriptEditWidget : public QTextEdit
{
    Q_OBJECT
signals:
    void scriptChanged(const QString &script);
public:
    ScriptEditWidget(QWidget *parent=nullptr);
};

#endif
