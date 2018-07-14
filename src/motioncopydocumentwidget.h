#ifndef MOTION_COPY_DOCUMENT_WIDGET_H
#define MOTION_COPY_DOCUMENT_WIDGET_H
#include <QWidget>
#include <QPushButton>
#include <QListWidget>
#include <QScrollBar>
#include <QTemporaryFile>
#include "skeletondocument.h"
#include "spinnableawesomebutton.h"
#include "motioncopydocument.h"
#include "motioncopyframegraphicsview.h"
#include "motioncopytracknodelistwidget.h"

class MotionCopyDocumentWidget : public QWidget
{
    Q_OBJECT
signals:
    void loadSourceVideo(int index, int thumbnailHeight, QString fileName, QString realPath, QTemporaryFile *fileHandle);
    void setCurrentFrame(int frameIndex);
public:
    MotionCopyDocumentWidget(const SkeletonDocument *document, QWidget *parent=nullptr);
    ~MotionCopyDocumentWidget();
    int thumbnailHeight(int index);
    MotionCopyDocument *motionCopyDocument();
public slots:
    void chooseSourceVideo(int index);
    void sourceVideoLoadStateChanged(int index);
    void updateVideoFrames();
private:
    const SkeletonDocument *m_skeletonDocument = nullptr;
    SpinnableAwesomeButton *buttons[2] = {nullptr};
    QListWidget *m_frameImageListWidgets[2] = {nullptr};
    QScrollBar *m_scrollbar = nullptr;
    MotionCopyDocument *m_motionCopyDocument = nullptr;
    MotionCopyFrameGraphicsView *m_graphicsViews[2] = {nullptr};
    MotionCopyTrackNodeListWidget *m_trackNodeListWidget = nullptr;
};

#endif
