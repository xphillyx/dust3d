#ifndef MOTION_COPY_TO_JOINT_H
#define MOTION_COPY_TO_JOINT_H
#include <QObject>
#include <QVector3D>
#include "jointnodetree.h"

class MotionCopyToJoint : public QObject
{
    Q_OBJECT
signals:
    void finished();
public slots:
    void process();
public:
    MotionCopyToJoint(const JointNodeTree &jointNodeTree, const std::vector<std::pair<JointMarkedNode, QVector3D>> &markedNodesPositions);
    ~MotionCopyToJoint();
    JointNodeTree *takeResultJointNodeTree();
    void apply();
private:
    void prepareJointIndexToPositionMap(std::map<int, QVector3D> &jointIndexToPositionMap);
    void applyFromJoint(int jointIndex);
    int findPositionMarkedParent(int jointIndex, std::vector<int> *trace=nullptr);
private:
    JointNodeTree m_jointNodeTree;
    JointNodeTree *m_resultJointNodeTree = nullptr;
    std::vector<std::pair<JointMarkedNode, QVector3D>> m_markedNodesPositions;
    std::map<int, QVector3D> m_jointIndexToPositionMap;
};

#endif
