#ifndef TRACK_FRAME_CONVERTOR_H
#define TRACK_FRAME_CONVERTOR_H
#include <QObject>
#include "jointnodetree.h"
#include "meshresultcontext.h"
#include "meshloader.h"

class TrackFrameConvertor : public QObject
{
    Q_OBJECT
signals:
    void finished();
public slots:
    void process();
public:
    TrackFrameConvertor(const MeshResultContext &meshResultContext, const JointNodeTree &jointNodeTree, const std::vector<std::pair<JointMarkedNode, QVector3D>> &markedNodesPositions, int frame=-1);
    ~TrackFrameConvertor();
    JointNodeTree *takeResultJointNodeTree();
    MeshLoader *takeResultMesh();
    int frame();
    void convert();
private:
    std::vector<std::pair<JointMarkedNode, QVector3D>> m_markedNodesPositions;
    MeshResultContext *m_meshResultContext = nullptr;
    JointNodeTree *m_inputJointNodeTree = nullptr;
    JointNodeTree *m_resultJointNodeTree = nullptr;
    MeshLoader *m_meshLoader = nullptr;
    int m_frame = -1;
};

#endif
