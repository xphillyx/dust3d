#ifndef RIG_WIDGET_H
#define RIG_WIDGET_H
#include <QWidget>
#include <QComboBox>
#include "skeletondocument.h"
#include "rigtype.h"
#include "modelwidget.h"

class RigWidget : public QWidget
{
    Q_OBJECT
signals:
    void setRigType(RigType rigType);
public slots:
    void rigTypeChanged();
public:
    RigWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    ModelWidget *rigWeightRenderWidget();
private:
    const SkeletonDocument *m_document = nullptr;
    QComboBox *m_rigTypeBox = nullptr;
    ModelWidget *m_rigWeightRenderWidget = nullptr;
};

#endif
