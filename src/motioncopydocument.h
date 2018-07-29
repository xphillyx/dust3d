#ifndef MOTION_COPY_DOCUMENT_H
#define MOTION_COPY_DOCUMENT_H
#include <QUuid>
#include <QTemporaryFile>
#include "videoframeextractor.h"
#include "skeletondocument.h"
#include "jointnodetree.h"
#include "motioncopysnapshot.h"
#include "motioncopytojoint.h"
#include "trackframeconvertor.h"

#define MOTION_COPY_CHANNEL_NUM 2

class TrackNodeInstance
{
public:
    QString id;
    QVector3D position;
};

class TrackNode
{
public:
    QString id;
    JointMarkedNode markedNode;
    bool visible = true;
};

class MotionCopyFrameCache
{
public:
    MotionCopyFrameCache()
    {
    }
    ~MotionCopyFrameCache()
    {
        delete jointNodeTree;
        delete meshResultContext;
        delete mesh;
    }
public:
    JointNodeTree *jointNodeTree = nullptr;
    MeshResultContext *meshResultContext = nullptr;
    MeshLoader *mesh = nullptr;
private:
    Q_DISABLE_COPY(MotionCopyFrameCache);
};

class MotionCopyDocument : public QObject
{
    Q_OBJECT
signals:
    void videoFramesChanged();
    void sourceVideoLoadStateChanged(int channel);
    void currentFrameChanged();
    void documentChanged();
    void trackNodesAttributesChanged();
    void trackNodeHovered(QString id);
    void trackNodePositionChanged(QString id, QVector3D position);
    void currentFrameCacheReady();
    void markedNodesChanged();
public:
    MotionCopyDocument(const SkeletonDocument *skeletonDocument);
    ~MotionCopyDocument();
    bool isLoadingSourceVideo(int channel) const;
    const std::vector<VideoFrame> &videoFrames(int channel) const;
    const TrackNodeInstance *findTrackNodeInstance(int frame, const QString &id) const;
    const std::map<QString, TrackNodeInstance> *getTrackNodeInstances(int frame) const;
    const TrackNode *findTrackNode(const QString &id) const;
    int currentFrame() const;
    void toSnapshot(MotionCopySnapshot *snapshot) const;
    void fromSnapshot(const MotionCopySnapshot &snapshot);
    const QString &videoFileName(int channel) const;
    const QString &videoRealPath(int channel) const;
    void markAsDeleted();
    MeshLoader *takeCurrentFrameMesh();
    const std::vector<JointMarkedNode> &markedNodes() const;
public slots:
    void loadSourceVideo(int channel, int thumbnailHeight, QString fileName, QString realPath, QTemporaryFile *fileHandle);
    void loadSourceVideoData(int channel, int thumbnailHeight, QString fileName, QByteArray fileContent);
    void videoFrameExtractionDone(int channel);
    void setCurrentFrame(int frameIndex);
    void addTrackNodeToAllFrames(JointMarkedNode markedNode);
    void removeTrackNodeFromAllFrames(JointMarkedNode markedNode);
    void setTrackNodeVisibleStateToAllFrames(JointMarkedNode markedNode, bool visible);
    void moveTrackNodeBy(int frame, QString id, float x, float y, float z);
    void reset();
    void frameConvertDone();
    void frameChanged(int frame);
    void reloadSourceMarkedNodes();
private:
    void checkDelete();
    void checkFrameCaches();
private:
    QString m_loadedVideoFileNames[MOTION_COPY_CHANNEL_NUM];
    QString m_loadedVideoRealPaths[MOTION_COPY_CHANNEL_NUM];
    QTemporaryFile *m_loadedVideoTemporaryFiles[MOTION_COPY_CHANNEL_NUM] = {nullptr};
    QString m_deferredVideoFileNames[MOTION_COPY_CHANNEL_NUM];
    QString m_deferredVideoRealPaths[MOTION_COPY_CHANNEL_NUM];
    QTemporaryFile *m_deferredVideoTemporaryFiles[MOTION_COPY_CHANNEL_NUM] = {nullptr};
    int m_thumbnailHeight = 0;
    VideoFrameExtractor *m_videoFrameExtractors[MOTION_COPY_CHANNEL_NUM] = {nullptr};
    std::vector<VideoFrame> *m_videoFrames[MOTION_COPY_CHANNEL_NUM] = {nullptr};
    std::vector<std::map<QString, TrackNodeInstance>> m_trackNodeInstances;
    std::map<QString, TrackNode> m_trackNodes;
    std::map<int, MotionCopyFrameCache> m_frameCaches;
    std::set<int> m_dirtyFrames;
    int m_currentFrame = -1;
    const SkeletonDocument *m_skeletonDocument = nullptr;
    bool m_markedAsDeleted = false;
    TrackFrameConvertor *m_trackFrameConvertor = nullptr;
    std::vector<JointMarkedNode> m_markedNodes;
    std::set<QString> m_markedNodeIds;
};

#endif

