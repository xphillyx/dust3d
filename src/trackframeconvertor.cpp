#include <QGuiApplication>
#include "trackframeconvertor.h"
#include "motioncopytojoint.h"
#include "skinnedmesh.h"

TrackFrameConvertor::TrackFrameConvertor(const MeshResultContext &meshResultContext, const JointNodeTree &jointNodeTree, const std::vector<std::pair<JointMarkedNode, QVector3D>> &markedNodesPositions, int frame) :
    m_markedNodesPositions(markedNodesPositions),
    m_meshResultContext(new MeshResultContext(meshResultContext)),
    m_inputJointNodeTree(new JointNodeTree(jointNodeTree)),
    m_frame(frame)
{
}

TrackFrameConvertor::~TrackFrameConvertor()
{
    delete m_meshResultContext;
    delete m_inputJointNodeTree;
    delete m_resultJointNodeTree;
    delete m_meshLoader;
}

int TrackFrameConvertor::frame()
{
    return m_frame;
}

JointNodeTree *TrackFrameConvertor::takeResultJointNodeTree()
{
    JointNodeTree *resultJointNodeTree = m_resultJointNodeTree;
    m_resultJointNodeTree = nullptr;
    return resultJointNodeTree;
}

MeshLoader *TrackFrameConvertor::takeResultMesh()
{
    MeshLoader *meshLoader = m_meshLoader;
    m_meshLoader = nullptr;
    return meshLoader;
}

void TrackFrameConvertor::process()
{
    convert();

    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void TrackFrameConvertor::convert()
{
    MotionCopyToJoint toJoint(*m_inputJointNodeTree, m_markedNodesPositions);
    toJoint.apply();
    m_resultJointNodeTree = toJoint.takeResultJointNodeTree();
    SkinnedMesh skinned(*m_meshResultContext, *m_inputJointNodeTree);
    RigFrame rigFrame;
    m_inputJointNodeTree->diff(*m_resultJointNodeTree, rigFrame);
    skinned.applyRigFrameToMesh(rigFrame);
    m_meshLoader = skinned.toMeshLoader();
}
