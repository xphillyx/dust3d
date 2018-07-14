#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QTimer>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QFileInfo>
#include "motioncopydocumentwidget.h"
#include "version.h"
#include "graphicscontainerwidget.h"

MotionCopyDocumentWidget::MotionCopyDocumentWidget(const SkeletonDocument *document, QWidget *parent) :
    QWidget(parent),
    m_skeletonDocument(document),
    m_motionCopyDocument(new MotionCopyDocument(document))
{
    m_scrollbar = new QScrollBar(Qt::Orientation::Horizontal);
    
    QVBoxLayout *timelineLayout = new QVBoxLayout;
    timelineLayout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 2; i++) {
        QHBoxLayout *rowLayout = new QHBoxLayout;
        buttons[i] = new SpinnableAwesomeButton;
        buttons[i]->setAwesomeIcon(QChar(fa::videocamera));
        connect(buttons[i]->button(), &QPushButton::clicked, [=] {
            if (!buttons[i]->isSpinning())
                chooseSourceVideo(i);
        });
        rowLayout->addWidget(buttons[i]);
        
        m_frameImageListWidgets[i] = new QListWidget;
        m_frameImageListWidgets[i]->setFixedHeight(28);
        m_frameImageListWidgets[i]->setFlow(QListWidget::LeftToRight);
        m_frameImageListWidgets[i]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_frameImageListWidgets[i]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_frameImageListWidgets[i]->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        
        connect(m_frameImageListWidgets[i]->horizontalScrollBar(), &QScrollBar::rangeChanged, m_scrollbar, [=] {
            m_scrollbar->setRange(m_frameImageListWidgets[i]->horizontalScrollBar()->minimum(),
                m_frameImageListWidgets[i]->horizontalScrollBar()->maximum());
            m_scrollbar->setValue(m_frameImageListWidgets[i]->horizontalScrollBar()->value());
        });
        
        connect(m_scrollbar, &QScrollBar::valueChanged, m_frameImageListWidgets[i]->horizontalScrollBar(), &QScrollBar::setValue);
        connect(m_frameImageListWidgets[i]->horizontalScrollBar(), &QScrollBar::valueChanged, m_scrollbar, &QScrollBar::setValue);
        connect(m_frameImageListWidgets[i], &QListWidget::currentRowChanged, [=] (int rowIndex) {
            auto &other = m_frameImageListWidgets[(i + 1) % 2];
            if (other->currentRow() != rowIndex) {
                other->setCurrentRow(rowIndex);
            }
            emit setCurrentFrame(rowIndex);
        });
        rowLayout->addWidget(m_frameImageListWidgets[i]);
        
        timelineLayout->addLayout(rowLayout);
    }
    buttons[0]->button()->setStyleSheet("QPushButton {color: #fc6621}");
    buttons[1]->button()->setStyleSheet("QPushButton {color: #aaebc4}");
    
    timelineLayout->addWidget(m_scrollbar);
    
    QHBoxLayout *graphicsLayout = new QHBoxLayout;
    graphicsLayout->setSpacing(0);
    graphicsLayout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 2; i++) {
        MotionCopyFrameGraphicsView *graphicsView = new MotionCopyFrameGraphicsView(m_motionCopyDocument, i);
        
        connect(m_motionCopyDocument, &MotionCopyDocument::currentFrameChanged, graphicsView, &MotionCopyFrameGraphicsView::reload);
        connect(m_motionCopyDocument, &MotionCopyDocument::trackNodePositionChanged, graphicsView, &MotionCopyFrameGraphicsView::updateTackNodePosition);
        connect(graphicsView, &MotionCopyFrameGraphicsView::moveTrackNodeBy, m_motionCopyDocument, &MotionCopyDocument::moveTrackNodeBy);
        connect(graphicsView, &MotionCopyFrameGraphicsView::hoverTrackNode, m_motionCopyDocument, &MotionCopyDocument::trackNodeHovered);
        connect(m_motionCopyDocument, &MotionCopyDocument::trackNodeHovered, graphicsView, &MotionCopyFrameGraphicsView::trackNodeHovered);
        
        GraphicsContainerWidget *containerWidget = new GraphicsContainerWidget;
        containerWidget->setGraphicsWidget(graphicsView);
        QGridLayout *containerLayout = new QGridLayout;
        containerLayout->setSpacing(0);
        containerLayout->setContentsMargins(1, 0, 0, 0);
        containerLayout->addWidget(graphicsView);
        containerWidget->setLayout(containerLayout);
        containerWidget->setMinimumSize(240, 160);
        
        connect(containerWidget, &GraphicsContainerWidget::containerSizeChanged, graphicsView, &MotionCopyFrameGraphicsView::canvasResized);
        
        graphicsLayout->addWidget(containerWidget);
    }
    
    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addLayout(graphicsLayout);
    rightLayout->addLayout(timelineLayout);
    rightLayout->setStretch(0, 1);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    QScrollArea *trackNodeListFrame = new QScrollArea;
    trackNodeListFrame->setMinimumWidth(200);
    
    QWidget *rightWidget = new QWidget;
    rightWidget->setLayout(rightLayout);

    QSplitter *splitter = new QSplitter(this);
    splitter->addWidget(rightWidget);
    splitter->addWidget(trackNodeListFrame);
    splitter->setContentsMargins(0, 0, 0, 0);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(splitter);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    setLayout(mainLayout);
    setWindowTitle(APP_NAME);
    
    connect(m_motionCopyDocument, &MotionCopyDocument::videoFramesChanged, this, &MotionCopyDocumentWidget::updateVideoFrames);
    connect(m_motionCopyDocument, &MotionCopyDocument::sourceVideoLoadStateChanged, this, &MotionCopyDocumentWidget::sourceVideoLoadStateChanged);
    connect(this, &MotionCopyDocumentWidget::loadSourceVideo, m_motionCopyDocument, &MotionCopyDocument::loadSourceVideo);
    connect(this, &MotionCopyDocumentWidget::setCurrentFrame, m_motionCopyDocument, &MotionCopyDocument::setCurrentFrame);
    
    auto createTrackNodeListWidget = [=] {
        m_trackNodeListWidget = new MotionCopyTrackNodeListWidget(m_skeletonDocument, m_motionCopyDocument);
        connect(m_trackNodeListWidget, &MotionCopyTrackNodeListWidget::checkMarkedNode, m_motionCopyDocument, &MotionCopyDocument::addTrackNodeToAllFrames);
        connect(m_trackNodeListWidget, &MotionCopyTrackNodeListWidget::uncheckMarkedNode, m_motionCopyDocument, &MotionCopyDocument::removeTrackNodeFromAllFrames);
        connect(m_motionCopyDocument, &MotionCopyDocument::trackNodesAttributesChanged, m_trackNodeListWidget, &MotionCopyTrackNodeListWidget::reload);
        connect(m_motionCopyDocument, &MotionCopyDocument::trackNodeHovered, m_trackNodeListWidget, &MotionCopyTrackNodeListWidget::markedNodeHovered);
        trackNodeListFrame->setWidget(m_trackNodeListWidget);
    };
    createTrackNodeListWidget();
    connect(m_skeletonDocument, &SkeletonDocument::postProcessedResultChanged, this, [=] {
        delete m_trackNodeListWidget;
        createTrackNodeListWidget();
    });
}

MotionCopyDocument *MotionCopyDocumentWidget::motionCopyDocument()
{
    return m_motionCopyDocument;
}

MotionCopyDocumentWidget::~MotionCopyDocumentWidget()
{
    delete m_trackNodeListWidget;
    m_motionCopyDocument->markAsDeleted();
}

void MotionCopyDocumentWidget::sourceVideoLoadStateChanged(int index)
{
    //setEnabled(!m_motionCopyDocument->isLoadingSourceVideo(index));
    
    buttons[index]->showSpinner(m_motionCopyDocument->isLoadingSourceVideo(index));
}

void MotionCopyDocumentWidget::chooseSourceVideo(int index)
{
    if (m_motionCopyDocument->isLoadingSourceVideo(index))
        return;
    
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(),
        tr("Video Files (*.*)")).trimmed();
    if (fileName.isEmpty())
        return;
    
    emit loadSourceVideo(index, m_frameImageListWidgets[index]->height(), QFileInfo(fileName).fileName(), fileName, nullptr);
}

int MotionCopyDocumentWidget::thumbnailHeight(int index)
{
    return m_frameImageListWidgets[index]->height();
}

void MotionCopyDocumentWidget::updateVideoFrames()
{
    int maxImageNum = 0;
    int pickIndex = 0;
    for (int index = 0; index < 2; index++) {
        const auto &videoFrames = m_motionCopyDocument->videoFrames(index);
        if (maxImageNum < (int)videoFrames.size()) {
            maxImageNum = videoFrames.size();
            pickIndex = index;
        }
    }
    
    for (int index = 0; index < 2; index++) {
        m_frameImageListWidgets[index]->clear();
        int i = 0;
        QSize thumbnailSize;
        const auto &videoFrames = m_motionCopyDocument->videoFrames(index);
        for (i = 0; i < (int)videoFrames.size(); i++) {
            QListWidgetItem *item = new QListWidgetItem;
            const QImage &thumbnail = videoFrames[i].thumbnail;
            //qDebug() << "video frame:" << i << " thumbnail size:" << thumbnail.size();
            item->setData(Qt::DecorationRole, QPixmap::fromImage(thumbnail));
            thumbnailSize = thumbnail.size();
            m_frameImageListWidgets[index]->addItem(item);
        }
        if (i < maxImageNum && !thumbnailSize.isNull()) {
            QPixmap empty(thumbnailSize);
            empty.fill(Qt::white);
            for (; i < maxImageNum; i++) {
                QListWidgetItem *item = new QListWidgetItem;
                item->setData(Qt::DecorationRole, empty);
                m_frameImageListWidgets[index]->addItem(item);
            }
        }
    }
}


