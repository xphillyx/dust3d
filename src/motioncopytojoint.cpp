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

void MotionCopyToJoint::process()
{
    apply();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void MotionCopyToJoint::apply()
{
    if (m_jointNodeTree.joints().empty())
        return;
    
    Q_ASSERT(nullptr == m_resultJointNodeTree);
    
    m_resultJointNodeTree = new JointNodeTree(m_jointNodeTree);
    prepareJointIndexToPositionMap(m_jointIndexToPositionMap);
    
    for (const auto &leg: m_resultJointNodeTree->legs()) {
        std::vector<std::pair<int, QVector3D>> jointIndiciesAndPositions;
        for (size_t i = 0; i < leg.size(); i++) {
            int jointIndex = leg[i];
            const auto &findPositionResult = m_jointIndexToPositionMap.find(jointIndex);
            if (findPositionResult != m_jointIndexToPositionMap.end()) {
                jointIndiciesAndPositions.push_back(std::make_pair(jointIndex, findPositionResult->second));
            }
        }
        for (size_t i = 1; i < jointIndiciesAndPositions.size(); i++) {
            const auto &prev = jointIndiciesAndPositions[i - 1];
            const auto &current = jointIndiciesAndPositions[i];
            QVector3D direction = (current.second - prev.second).normalized();
            auto &prevJoint = m_resultJointNodeTree->joints()[prev.first];
            auto &currentJoint = m_resultJointNodeTree->joints()[current.first];
            QVector3D oldDirection = (currentJoint.position - prevJoint.position).normalized();
            QQuaternion rotation = QQuaternion::rotationTo(oldDirection, direction);
            std::vector<int> children;
            m_resultJointNodeTree->collectChildren(currentJoint.jointIndex, children);
            QVector3D rotateOrigin = prevJoint.position;
            currentJoint.position = rotateOrigin + rotation.rotatedVector(currentJoint.position - rotateOrigin);
            for (const auto &childIndex: children) {
                auto &childJoint = m_resultJointNodeTree->joints()[childIndex];
                childJoint.position = rotateOrigin + rotation.rotatedVector(childJoint.position - rotateOrigin);
            }
        }
    }
    m_resultJointNodeTree->recalculateMatricesAfterPositionUpdated();
}
