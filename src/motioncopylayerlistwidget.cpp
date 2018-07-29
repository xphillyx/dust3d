#include <QSplitter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QSplitter>
#include <QMessageBox>
#include "motioncopylayerlistwidget.h"
#include "version.h"
#include "theme.h"

MotionCopyLayerListWidget::MotionCopyLayerListWidget(const SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_skeletonDocument(document)
{
    m_stackedWidget = new QStackedWidget;
    
    QHBoxLayout *toolButtonLayout = new QHBoxLayout;
    toolButtonLayout->setSpacing(0);
    
    QPushButton *newButton = new QPushButton(QChar(fa::file));
    Theme::initAwesomeSmallButton(newButton);
    toolButtonLayout->addWidget(newButton);
    
    toolButtonLayout->addSpacing(10);
    
    QPushButton *trashButton = new QPushButton(QChar(fa::trash));
    Theme::initAwesomeSmallButton(trashButton);
    toolButtonLayout->addWidget(trashButton);
    
    toolButtonLayout->addStretch();
    
    QPushButton *playButton = new QPushButton(QChar(fa::play));
    Theme::initAwesomeSmallButton(playButton);
    toolButtonLayout->addWidget(playButton);
    
    QPushButton *pauseButton = new QPushButton(QChar(fa::pause));
    Theme::initAwesomeSmallButton(pauseButton);
    toolButtonLayout->addWidget(pauseButton);
    
    QPushButton *stopButton = new QPushButton(QChar(fa::stop));
    Theme::initAwesomeSmallButton(stopButton);
    toolButtonLayout->addWidget(stopButton);

    m_layerListWidget = new QListWidget;
    
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addLayout(toolButtonLayout);
    leftLayout->addWidget(m_layerListWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    QWidget *leftWidget = new QWidget;
    leftWidget->setLayout(leftLayout);
    leftWidget->setMinimumWidth(150);
    
    QSplitter *splitter = new QSplitter;
    splitter->addWidget(leftWidget);
    splitter->addWidget(m_stackedWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(splitter);
    mainLayout->setSpacing(0);
    
    connect(newButton, &QPushButton::clicked, this, &MotionCopyLayerListWidget::addNewLayer);
    connect(trashButton, &QPushButton::clicked, this, [=] {
        if (-1 == m_currentLayer || m_layers.empty())
            return;
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            APP_NAME,
            QString(tr("Do you really want to delete motion - %1?")).arg(m_layers[m_currentLayer].name),
            QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::Yes)
            emit deleteCurrentLayer();
    });
    connect(m_layerListWidget, &QListWidget::currentRowChanged, this, &MotionCopyLayerListWidget::setCurrentLayer);
    connect(m_layerListWidget, &QListWidget::itemChanged, this, &MotionCopyLayerListWidget::layerNameItemChanged);
    
    setLayout(mainLayout);
    setWindowTitle(APP_NAME);
}

void MotionCopyLayerListWidget::layerNameItemChanged(QListWidgetItem *item)
{
    int row = m_layerListWidget->row(item);
    if (row >= (int)m_layers.size())
        return;
    if (item->text() != m_layers[row].name) {
        QString newNameBase = item->text();
        QString newName = newNameBase;
        int i = 0;
        while (true) {
            int findResult = findLayer(newName);
            if (-1 == findResult || row == findResult)
                break;
            newName = newNameBase + QString::number(++i);
        }
        m_layers[row].name = newName;
        item->setText(newName);
    }
    
    emit someDocumentChanged();
}

MotionCopyDocument *MotionCopyLayerListWidget::currentMotionCopyDocument()
{
    if (-1 == m_currentLayer || m_currentLayer >= (int)m_layers.size())
        return nullptr;
    return m_layers[m_currentLayer].widget->motionCopyDocument();
}

MotionCopyLayer *MotionCopyLayerListWidget::addLayer(const QString &name)
{
    MotionCopyLayer layer;
    layer.name = name;
    layer.widget = new MotionCopyDocumentWidget(m_skeletonDocument);
    
    connect(layer.widget->motionCopyDocument(), &MotionCopyDocument::documentChanged, this, &MotionCopyLayerListWidget::someDocumentChanged);
    
    MotionCopyDocument *motionCopyDocument = layer.widget->motionCopyDocument();
    const auto &that = this;
    connect(motionCopyDocument, &MotionCopyDocument::currentFrameCacheReady, [=] {
        if (that->currentMotionCopyDocument() == motionCopyDocument)
            emit currentFrameConvertedMeshChanged();
    });
    
    m_stackedWidget->addWidget(layer.widget);
    m_layerListWidget->addItem(layer.name);
    
    QListWidgetItem *item = m_layerListWidget->item(m_layerListWidget->count() - 1);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    
    m_layers.push_back(layer);
    
    MotionCopyLayer *returnLayer = &m_layers[m_layers.size() - 1];
    
    setCurrentLayer(m_layers.size() - 1);
    
    emit someDocumentChanged();
    
    return returnLayer;
}

int MotionCopyLayerListWidget::findLayer(const QString &name)
{
    for (size_t i = 0; i < layers().size(); i++) {
        if (layers()[i].name == name) {
            return i;
        }
    }
    return -1;
}

void MotionCopyLayerListWidget::addNewLayer()
{
    QString layerName;
    do {
        m_unnamedCount++;
        layerName = QString(tr("Unnamed%1")).arg(QString::number(m_unnamedCount));
    } while (-1 != findLayer(layerName));
    addLayer(layerName);
}

void MotionCopyLayerListWidget::setCurrentLayer(int layer)
{
    m_currentLayer = layer;
    
    if (-1 != m_currentLayer) {
        m_stackedWidget->setCurrentIndex(m_currentLayer);
        m_layerListWidget->setCurrentRow(m_currentLayer);
    }
    
    emit currentLayerChanged();
}

void MotionCopyLayerListWidget::removeLayerByName(const QString &name)
{
    int layer = findLayer(name);
    if (-1 == layer) {
        qDebug() << "remove a none existed layer:" << name;
        return;
    }
    removeLayer(layer);
}

void MotionCopyLayerListWidget::removeLayer(int layer)
{
    Q_ASSERT(layer < (int)m_layers.size());
    m_stackedWidget->removeWidget(m_layers[layer].widget);
    delete m_layers[layer].widget;
    m_layers.erase(m_layers.begin() + layer);
    
    int currentLayer = m_currentLayer;
    
    delete m_layerListWidget->takeItem(layer);
    
    if (currentLayer == layer) {
        m_currentLayer = currentLayer;
        
        if (m_currentLayer >= (int)m_layers.size())
            m_currentLayer--;
        
        setCurrentLayer(m_currentLayer);
    }
    
    emit someDocumentChanged();
}

void MotionCopyLayerListWidget::deleteCurrentLayer()
{
    if (-1 == m_currentLayer)
        return;

    removeLayer(m_currentLayer);
}

void MotionCopyLayerListWidget::playCurrentLayer()
{
    
}

void MotionCopyLayerListWidget::pauseCurrentLayer()
{
    
}

void MotionCopyLayerListWidget::stopCurrentLayer()
{
    
}

const std::vector<MotionCopyLayer> &MotionCopyLayerListWidget::layers()
{
    return m_layers;
}
