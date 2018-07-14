#include <QGuiApplication>
#include "motioncopytojoint.h"

MotionCopyToJoint::MotionCopyToJoint(const JointNodeTree &jointNodeTree, const std::vector<std::pair<JointMarkedNode, QVector3D>> &markedNodesPositions) :
    m_jointNodeTree(jointNodeTree),
    m_markedNodesPositions(markedNodesPositions)
{
}

MotionCopyToJoint::~MotionCopyToJoint()
{
    delete m_resultJointNodeTree;
}

JointNodeTree *MotionCopyToJoint::takeResultJointNodeTree()
{
    JointNodeTree *result = m_resultJointNodeTree;
    m_resultJointNodeTree = nullptr;
    return result;
}

void MotionCopyToJoint::prepareJointIndexToPositionMap(std::map<int, QVector3D> &jointIndexToPositionMap)
{
    std::vector<JointMarkedNode> markedNodes;
    std::vector<int> jointIndicies;
    m_jointNodeTree.getMarkedNodeList(markedNodes, &jointIndicies);
    Q_ASSERT(markedNodes.size() == jointIndicies.size());
    
    std::map<QString, int> markedKeyToJointIndexMap;
    for (size_t i = 0; i < markedNodes.size(); i++) {
        markedKeyToJointIndexMap[markedNodes[i].toKey()] = jointIndicies[i];
    }
    
    for (const auto &it: m_markedNodesPositions) {
        const auto &findResult = markedKeyToJointIndexMap.find(it.first.toKey());
        if (findResult == markedKeyToJointIndexMap.end()) {
            qDebug() << "markedKeyToJointIndexMap find failed:" << it.first.toKey();
            continue;
        }
        jointIndexToPositionMap[findResult->second] = it.second;
    }
}

void MotionCopyToJoint::apply()
{
    if (m_jointNodeTree.joints().empty())
        return;
    
    Q_ASSERT(nullptr == m_resultJointNodeTree);
    
    m_resultJointNodeTree = new JointNodeTree(m_jointNodeTree);
    prepareJointIndexToPositionMap(m_jointIndexToPositionMap);
    applyFromJoint(0);
}

void MotionCopyToJoint::process()
{
    apply();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

int MotionCopyToJoint::findPositionMarkedParent(int jointIndex, std::vector<int> *trace)
{
    int loopJointIndex = jointIndex;
    while (true) {
        const auto &joint = m_jointNodeTree.joints()[loopJointIndex];
        if (-1 == joint.parentIndex)
            break;
        if (nullptr != trace)
            trace->push_back(joint.parentIndex);
        if (m_jointIndexToPositionMap.find(joint.parentIndex) == m_jointIndexToPositionMap.end()) {
            return jointIndex;
        }
    }
    return -1;
}

void MotionCopyToJoint::applyFromJoint(int jointIndex)
{
    if (m_jointIndexToPositionMap.find(jointIndex) != m_jointIndexToPositionMap.end()) {
        std::vector<int> trace;
        int positionMarkedParent = findPositionMarkedParent(jointIndex, &trace);
        if (-1 != positionMarkedParent) {
            
        }
    }
    for (const auto &child: m_jointNodeTree.joints()[jointIndex].children) {
        applyFromJoint(child);
    }
}
