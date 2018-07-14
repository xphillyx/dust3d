#ifndef JOINT_NODE_TREE_H
#define JOINT_NODE_TREE_H
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>
#include <QStringList>
#include "meshresultcontext.h"
#include "skeletonbonemark.h"
#include "rigframe.h"

enum class JointSide
{
    None,
    Left,
    Right
};

const char *JointSideToString(JointSide jointSide);
#define IMPL_JointSideToString                                      \
const char *JointSideToString(JointSide jointSide)                  \
{                                                                   \
    switch (jointSide) {                                            \
        case JointSide::Left:                                       \
            return "Left";                                          \
        case JointSide::Right:                                      \
            return "Right";                                         \
        case JointSide::None:                                       \
            return "None";                                          \
        default:                                                    \
            return "None";                                          \
    }                                                               \
}
JointSide JointSideFromString(const char *jointSideString);
#define IMPL_JointSideFromString                                    \
JointSide JointSideFromString(const char *jointSideString)          \
{                                                                   \
    QString jointSide = jointSideString;                            \
    if (jointSide == "Left")                                        \
        return JointSide::Left;                                     \
    if (jointSide == "Right")                                       \
        return JointSide::Right;                                    \
    if (jointSide == "None")                                        \
        return JointSide::None;                                     \
    return JointSide::None;                                         \
}
const char *JointSideToDispName(JointSide jointSide);
#define IMPL_JointSideToDispName                                    \
const char *JointSideToDispName(JointSide jointSide)                \
{                                                                   \
    switch (jointSide) {                                            \
        case JointSide::Left:                                       \
            return "Left";                                          \
        case JointSide::Right:                                      \
            return "Right";                                         \
        case JointSide::None:                                       \
            return "";                                              \
        default:                                                    \
            return "";                                              \
    }                                                               \
}

struct JointInfo
{
    int jointIndex = 0;
    int parentIndex = -1;
    int partId = 0;
    int nodeId = 0;
    SkeletonBoneMark boneMark = SkeletonBoneMark::None;
    QVector3D position;
    float radius;
    QVector3D translation;
    QVector3D scale;
    QQuaternion rotation;
    QMatrix4x4 localMatrix;
    QMatrix4x4 bindMatrix;
    QMatrix4x4 inverseBindMatrix;
    std::vector<int> children;
};

class JointMarkedNode
{
public:
    SkeletonBoneMark boneMark = SkeletonBoneMark::None;
    JointSide jointSide = JointSide::None;
    int siblingOrder = -1;
    int jointOrder = -1;
    QVector3D position;
public:
    QString toKey() const
    {
        QStringList toStringList;
        toStringList.append(SkeletonBoneMarkToString(boneMark));
        toStringList.append(JointSideToString(jointSide));
        if (-1 != siblingOrder) {
            toStringList.append(QString("%1").arg(QString::number(siblingOrder + 1)));
        }
        if (-1 != jointOrder) {
            toStringList.append(QString("%1").arg(QString::number(jointOrder + 1)));
        }
        return toStringList.join("_");
    }
    
    QString toString() const
    {
        QStringList toStringList;
        toStringList.append(SkeletonBoneMarkToDispName(boneMark));
        toStringList.append(JointSideToDispName(jointSide));
        if (-1 != siblingOrder) {
            toStringList.append(QString("[%1]").arg(QString::number(siblingOrder + 1)));
        }
        if (-1 != jointOrder) {
            toStringList.append(QString("<%1>").arg(QString::number(jointOrder + 1)));
        }
        return toStringList.join(" ");
    }
};

class JointNodeTree
{
public:
    JointNodeTree(MeshResultContext &resultContext);
    const std::vector<JointInfo> &joints() const;
    std::vector<JointInfo> &joints();
    int nodeToJointIndex(int partId, int nodeId) const;
    void recalculateMatricesAfterPositionUpdated();
    void recalculateMatricesAfterTransformUpdated();
    const std::vector<std::pair<int, int>> &legs() const;
    const std::vector<int> &spine() const;
    const std::vector<std::pair<int, int>> &leftLegs() const;
    const std::vector<std::pair<int, int>> &rightLegs() const;
    const std::vector<std::vector<int>> &leftLegJoints() const;
    const std::vector<std::vector<int>> &rightLegJoints() const;
    void diff(const JointNodeTree &another, RigFrame &rigFrame);
    int findHubJoint(int jointIndex, std::vector<int> *tracePath=nullptr) const;
    void collectChildren(int jointIndex, std::vector<int> &children) const;
    void collectTrivialChildren(int jointIndex, std::vector<int> &children) const;
    int findParent(int jointIndex, int parentIndex, std::vector<int> *tracePath=nullptr) const;
    int findSpineFromChild(int jointIndex) const;
    bool isVerticalSpine = false;
    int head = -1;
    int tail = -1;
    void getMarkedNodeList(std::vector<JointMarkedNode> &markedNodes, std::vector<int> *jointIndicies=nullptr) const;
private:
    void calculateMatricesByTransform();
    void calculateMatricesByPosition();
    void calculateMatricesByPositionFrom(int jointIndex, const QVector3D &parentPosition, const QVector3D &parentDirection, const QMatrix4x4 &parentMatrix);
    std::vector<JointInfo> m_tracedJoints;
    std::map<std::pair<int, int>, int> m_tracedNodeToJointIndexMap;
    void traceBoneFromJoint(MeshResultContext &resultContext, std::pair<int, int> node, std::set<std::pair<int, int>> &visitedNodes, std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> &connections, int parentIndex);
    void collectParts();
    void addLeg(int legStart, int legEnd);
    void sortLegs(std::vector<std::pair<int, int>> &legs);
    void sortSpine(std::vector<int> &spine);
    void collectTrivialChildrenStopEarlyOnNoTrivial(int jointIndex, std::vector<int> &children, bool &hasNoTrivialNode) const;
private:
    std::vector<std::pair<int, int>> m_legs;
    std::vector<std::pair<int, int>> m_leftLegs;
    std::vector<std::pair<int, int>> m_rightLegs;
    std::vector<std::vector<int>> m_leftLegJoints;
    std::vector<std::vector<int>> m_rightLegJoints;
    std::vector<int> m_spine;
};

#endif
