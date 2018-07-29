#ifndef MOTION_COPY_LAYER_LIST_WIDGET_H
#define MOTION_COPY_LAYER_LIST_WIDGET_H
#include <QWidget>
#include <QStackedWidget>
#include <QListWidget>
#include "skeletondocument.h"
#include "motioncopydocumentwidget.h"

struct MotionCopyLayer
{
    QString name;
    MotionCopyDocumentWidget *widget;
};

class MotionCopyLayerListWidget : public QWidget
{
    Q_OBJECT
signals:
    void currentLayerChanged();
    void someDocumentChanged();
    void currentFrameConvertedMeshChanged();
public:
    MotionCopyLayerListWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    const std::vector<MotionCopyLayer> &layers();
    MotionCopyLayer *addLayer(const QString &name);
    int findLayer(const QString &name);
    void removeLayerByName(const QString &name);
    MotionCopyDocument *currentMotionCopyDocument();
public slots:
    void addNewLayer();
    void deleteCurrentLayer();
    void setCurrentLayer(int layer);
    void playCurrentLayer();
    void pauseCurrentLayer();
    void stopCurrentLayer();
    void layerNameItemChanged(QListWidgetItem *item);
private:
    void removeLayer(int layer);
private:
    std::vector<MotionCopyLayer> m_layers;
    int m_currentLayer = -1;
    int m_unnamedCount = 0;
    QStackedWidget *m_stackedWidget = nullptr;
    const SkeletonDocument *m_skeletonDocument = nullptr;
    QListWidget *m_layerListWidget = nullptr;
};

#endif
