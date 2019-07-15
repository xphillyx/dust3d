#ifndef DUST3D_SCRIPT_VARIABLES_WIDGET_H
#define DUST3D_SCRIPT_VARIABLES_WIDGET_H
#include <QWidget>
#include <QString>
#include <map>

class ScriptVariablesWidget : public QWidget
{
    Q_OBJECT
public:
    ScriptVariablesWidget(const std::map<QString, std::map<QString, QString>> &varaibles,
        QWidget *parent=nullptr);
};

#endif
