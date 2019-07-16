#ifndef DUST3D_SCRIPT_VARIABLES_WIDGET_H
#define DUST3D_SCRIPT_VARIABLES_WIDGET_H
#include <QWidget>
#include <QString>
#include <map>
#include <QTreeWidget>
#include "document.h"

class ScriptVariablesWidget : public QTreeWidget
{
    Q_OBJECT
public:
    ScriptVariablesWidget(const Document *document,
        QWidget *parent=nullptr);
public slots:
    void reload();
protected:
    QSize sizeHint() const override;
private:
    const Document *m_document = nullptr;
};

#endif
