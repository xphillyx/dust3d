#include <QThread>
#include <QFileInfo>
#include "motioncopydocument.h"
#include "dust3dutil.h"

MotionCopyDocument::MotionCopyDocument(const SkeletonDocument *skeletonDocument) :
    m_skeletonDocument(skeletonDocument)
{
    for (int i = 0; i < MOTION_COPY_CHANNEL_NUM; i++) {
        m_videoFrames[i] = new std::vector<VideoFrame>;
    }
}

MotionCopyDocument::~MotionCopyDocument()
{
    for (int i = 0; i < MOTION_COPY_CHANNEL_NUM; i++) {
        delete m_videoFrameExtractors[i];
        delete m_videoFrames[i];
    }
}

bool MotionCopyDocument::isLoadingSourceVideo(int channel) const
{
    if (nullptr != m_videoFrameExtractors[channel])
        return true;
    return false;
}

void MotionCopyDocument::markAsDeleted()
{
    m_markedAsDeleted = true;
    checkDelete();
}

void MotionCopyDocument::checkDelete()
{
    if (!m_markedAsDeleted)
        return;
    
    for (size_t i = 0; i < MOTION_COPY_CHANNEL_NUM; i++) {
        if (nullptr != m_videoFrameExtractors[i])
            return;
    }
    
    delete this;
}

void MotionCopyDocument::loadSourceVideoData(int channel, int thumbnailHeight, QString fileName, QByteArray fileContent)
{
    QTemporaryFile *fileHandle = new QTemporaryFile;
    if (fileHandle->open()) {
        fileHandle->write(fileContent);
        loadSourceVideo(channel, thumbnailHeight, fileName, fileHandle->fileName(), fileHandle);
    } else {
        delete fileHandle;
    }
}

void MotionCopyDocument::loadSourceVideo(int channel, int thumbnailHeight, QString fileName, QString realPath, QTemporaryFile *fileHandle)
{
    m_thumbnailHeight = thumbnailHeight;
    
    if (nullptr != m_videoFrameExtractors[channel]) {
        m_deferredVideoFileNames[channel] = fileName;
        m_deferredVideoRealPaths[channel] = realPath;
        delete m_deferredVideoTemporaryFiles[channel];
        m_deferredVideoTemporaryFiles[channel] = fileHandle;
        return;
    }
    
    qDebug() << "Load source video channel:" << channel;
    
    QThread *thread = new QThread;
    m_videoFrameExtractors[channel] = new VideoFrameExtractor(fileName, realPath, fileHandle, thumbnailHeight);
    emit sourceVideoLoadStateChanged(channel);
    m_videoFrameExtractors[channel]->moveToThread(thread);
    connect(m_videoFrameExtractors[channel], &VideoFrameExtractor::finished, this, [=] {
        videoFrameExtractionDone(channel);
    }, Qt::QueuedConnection);
    connect(thread, &QThread::started, m_videoFrameExtractors[channel], &VideoFrameExtractor::process);
    connect(m_videoFrameExtractors[channel], &VideoFrameExtractor::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void MotionCopyDocument::videoFrameExtractionDone(int channel)
{
    delete m_videoFrames[channel];
    m_videoFrames[channel] = nullptr;
    
    m_loadedVideoFileNames[channel] = m_videoFrameExtractors[channel]->fileName();
    m_loadedVideoRealPaths[channel] = m_videoFrameExtractors[channel]->realPath();
    delete m_loadedVideoTemporaryFiles[channel];
    m_loadedVideoTemporaryFiles[channel] = m_videoFrameExtractors[channel]->fileHandle();
    
    m_videoFrames[channel] = m_videoFrameExtractors[channel]->takeResultFrames();
    if (nullptr == m_videoFrames[channel])
        m_videoFrames[channel] = new std::vector<VideoFrame>;
    
    delete m_videoFrameExtractors[channel];
    m_videoFrameExtractors[channel] = nullptr;
    
    if ((*m_videoFrames[channel]).size() > m_trackNodeInstances.size())
        m_trackNodeInstances.resize((*m_videoFrames[channel]).size());
    
    qDebug() << "Source video frame extraction done channel:" << channel;
    
    emit sourceVideoLoadStateChanged(channel);
    emit videoFramesChanged();
    
    if (!m_deferredVideoFileNames[channel].isEmpty()) {
        QString fileName = m_deferredVideoFileNames[channel];
        QString realPath = m_deferredVideoRealPaths[channel];
        QTemporaryFile *fileHandle = m_deferredVideoTemporaryFiles[channel];
        m_deferredVideoFileNames[channel].clear();
        m_deferredVideoRealPaths[channel].clear();
        m_deferredVideoTemporaryFiles[channel] = nullptr;
        loadSourceVideo(channel, m_thumbnailHeight, fileName, realPath, fileHandle);
    }
    
    checkDelete();
}

const std::vector<VideoFrame> &MotionCopyDocument::videoFrames(int channel) const
{
    return (*m_videoFrames[channel]);
}

void MotionCopyDocument::setCurrentFrame(int frameIndex)
{
    if (m_currentFrame == frameIndex)
        return;
    
    m_currentFrame = frameIndex;
    emit currentFrameChanged();
}

int MotionCopyDocument::currentFrame() const
{
    return m_currentFrame;
}

const TrackNodeInstance *MotionCopyDocument::findTrackNodeInstance(int frame, const QString &id) const
{
    if (frame >= (int)m_trackNodeInstances.size()) {
        qDebug() << "Find track frame failed id:" << id << "frame:" << frame;
        return nullptr;
    }
    const auto &findResult = m_trackNodeInstances[frame].find(id);
    if (findResult == m_trackNodeInstances[frame].end()) {
        qDebug() << "Find track node failed id:" << id << "frame:" << frame;
        return nullptr;
    }
    return &findResult->second;
}

const std::map<QString, TrackNodeInstance> *MotionCopyDocument::getTrackNodeInstances(int frame) const
{
    if (frame >= (int)m_trackNodeInstances.size()) {
        qDebug() << "Find track frame failed frame:" << frame;
        return nullptr;
    }
    return &m_trackNodeInstances[frame];
}

void MotionCopyDocument::moveTrackNodeBy(int frame, QString id, float x, float y, float z)
{
    if (frame >= (int)m_trackNodeInstances.size()) {
        qDebug() << "moveTrackNode frame:" << frame << " out of range:" << m_trackNodeInstances.size();
        return;
    }
    auto &trackNodeInstanceMap = m_trackNodeInstances[frame];
    if (trackNodeInstanceMap.find(id) == trackNodeInstanceMap.end()) {
        qDebug() << "moveTrackNode frame:" << frame << " id:" << id << " not found";
        return;
    }
    auto &position = trackNodeInstanceMap[id].position;
    position.setX(position.x() + x);
    position.setY(position.y() + y);
    position.setZ(position.z() + z);
    if (frame == m_currentFrame) {
        emit trackNodePositionChanged(id, position);
    }
    emit documentChanged();
}

void MotionCopyDocument::addTrackNodeToAllFrames(JointMarkedNode markedNode)
{
    QString id = markedNode.toKey();
    QVector3D position = markedNode.position;
    if (m_trackNodes.find(id) == m_trackNodes.end()) {
        TrackNode trackNode;
        trackNode.id = id;
        trackNode.markedNode = markedNode;
        m_trackNodes[id] = trackNode;
    }
    m_trackNodes[id].visible = true;
    for (size_t frame = 0; frame < m_trackNodeInstances.size(); frame++) {
        auto &trackNodeInstanceMap = m_trackNodeInstances[frame];
        if (trackNodeInstanceMap.find(id) == trackNodeInstanceMap.end()) {
            TrackNodeInstance trackNodeInstance;
            trackNodeInstance.position = position;
            trackNodeInstance.id = id;
            trackNodeInstanceMap[id] = trackNodeInstance;
        }
    }
    
    emit currentFrameChanged();
    emit documentChanged();
}

void MotionCopyDocument::removeTrackNodeFromAllFrames(JointMarkedNode markedNode)
{
    const auto &id = markedNode.toKey();
    auto findAttributeResult = m_trackNodes.find(id);
    if (findAttributeResult != m_trackNodes.end()) {
        findAttributeResult->second.visible = false;
    }
    
    /*
    for (size_t frame = 0; frame < m_trackNodeInstances.size(); frame++) {
        auto &trackNodeInstanceMap = m_trackNodeInstances[frame];
        const auto &findInstance = trackNodeInstanceMap.find(id);
        if (findInstance != trackNodeInstanceMap.end()) {
            trackNodeInstanceMap.erase(findInstance);
        }
    }*/
    
    emit currentFrameChanged();
    emit documentChanged();
}

void MotionCopyDocument::setTrackNodeVisibleStateToAllFrames(JointMarkedNode markedNode, bool visible)
{
    const auto &id = markedNode.toKey();
    auto findAttribute = m_trackNodes.find(id);
    if (findAttribute == m_trackNodes.end()) {
        qDebug() << "Find track node visible attribute failed:" << id;
        return;
    }
    if (findAttribute->second.visible != visible) {
        findAttribute->second.visible = visible;
        emit currentFrameChanged();
        emit documentChanged();
    }
}

const TrackNode *MotionCopyDocument::findTrackNode(const QString &id) const
{
    const auto &findResult = m_trackNodes.find(id);
    if (findResult == m_trackNodes.end())
        return nullptr;
    return &findResult->second;
}

void MotionCopyDocument::toSnapshot(MotionCopySnapshot *snapshot) const
{
    for (const auto &it: m_trackNodes) {
        std::map<QString, QString> trackNode;
        trackNode["id"] = it.second.id;
        trackNode["visible"] = it.second.visible ? "true" : "false";
        trackNode["boneMark"] = SkeletonBoneMarkToString(it.second.markedNode.boneMark);
        trackNode["jointSide"] = JointSideToString(it.second.markedNode.jointSide);
        trackNode["siblingOrder"] = QString::number(it.second.markedNode.siblingOrder);
        trackNode["jointOrder"] = QString::number(it.second.markedNode.jointOrder);
        snapshot->trackNodes[trackNode["id"]] = trackNode;
    }
    
    for (size_t frame = 0; frame < m_trackNodeInstances.size(); frame++) {
        const auto &instances = m_trackNodeInstances[frame];
        std::map<QString, std::map<QString, QString>> tracknodeInstances;
        for (const auto &it: instances) {
            std::map<QString, QString> trackNodeInstance;
            trackNodeInstance["id"] = it.second.id;
            trackNodeInstance["x"] = QString::number(it.second.position.x());
            trackNodeInstance["y"] = QString::number(it.second.position.y());
            trackNodeInstance["z"] = QString::number(it.second.position.z());
            tracknodeInstances[trackNodeInstance["id"]] = trackNodeInstance;
        }
        snapshot->framesTrackNodeInstances.push_back(tracknodeInstances);
    }
    
    std::map<QString, QString> motion;
    if (!m_loadedVideoFileNames[0].isEmpty())
        motion["frontSourceFile"] = m_loadedVideoFileNames[0];
    if (!m_loadedVideoFileNames[1].isEmpty())
        motion["sideSourceFile"] = m_loadedVideoFileNames[1];
    snapshot->motion = motion;
}

void MotionCopyDocument::fromSnapshot(const MotionCopySnapshot &snapshot)
{
    reset();
    for (const auto &it: snapshot.trackNodes) {
        TrackNode trackNode;
        if (it.second.find("id") == it.second.end())
            continue;
        trackNode.id = valueOfKeyInMapOrEmpty(it.second, "id");
        trackNode.visible = isTrueValueString(valueOfKeyInMapOrEmpty(it.second, "visible"));
        trackNode.markedNode.boneMark = SkeletonBoneMarkFromString(valueOfKeyInMapOrEmpty(it.second, "boneMark").toUtf8().constData());
        trackNode.markedNode.jointSide = JointSideFromString(valueOfKeyInMapOrEmpty(it.second, "jointSide").toUtf8().constData());
        trackNode.markedNode.siblingOrder = valueOfKeyInMapOrEmpty(it.second, "siblingOrder").toInt();
        trackNode.markedNode.jointOrder = valueOfKeyInMapOrEmpty(it.second, "jointOrder").toInt();
        m_trackNodes[trackNode.id] = trackNode;
    }
    for (size_t frame = 0; frame < snapshot.framesTrackNodeInstances.size(); frame++) {
        std::map<QString, TrackNodeInstance> trackNodeInstances;
        for (const auto &it: snapshot.framesTrackNodeInstances[frame]) {
            TrackNodeInstance trackNodeInstance;
            if (it.second.find("id") == it.second.end() ||
                    it.second.find("x") == it.second.end() ||
                    it.second.find("y") == it.second.end() ||
                    it.second.find("z") == it.second.end())
                continue;
            trackNodeInstance.id = valueOfKeyInMapOrEmpty(it.second, "id");
            trackNodeInstance.position = QVector3D(valueOfKeyInMapOrEmpty(it.second, "x").toFloat(),
                valueOfKeyInMapOrEmpty(it.second, "y").toFloat(),
                valueOfKeyInMapOrEmpty(it.second, "z").toFloat());
            trackNodeInstances[trackNodeInstance.id] = trackNodeInstance;
        }
        m_trackNodeInstances.push_back(trackNodeInstances);
    }
    
    emit currentFrameChanged();
    emit trackNodesAttributesChanged();
}

void MotionCopyDocument::reset()
{
    for (size_t i = 0; i < MOTION_COPY_CHANNEL_NUM; i++) {
        m_loadedVideoFileNames[i].clear();
        m_loadedVideoRealPaths[i].clear();
        delete m_loadedVideoTemporaryFiles[i];
        m_loadedVideoTemporaryFiles[i] = nullptr;
        delete m_videoFrames[i];
        m_videoFrames[i] = new std::vector<VideoFrame>;
    }
    m_trackNodeInstances.clear();
    m_trackNodes.clear();
    m_currentFrame = -1;
    
    emit currentFrameChanged();
    emit documentChanged();
}

const QString &MotionCopyDocument::videoFileName(int channel) const
{
    return m_loadedVideoFileNames[channel];
}

const QString &MotionCopyDocument::videoRealPath(int channel) const
{
    return m_loadedVideoRealPaths[channel];
}


