#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <QGuiApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <cmath>
#include <QVector2D>
#include <queue>
#include <unordered_map>
#include "riggenerator.h"
#include "util.h"
#include "boundingboxmesh.h"
#include "theme.h"

class GroupEndpointsStitcher
{
public:
    GroupEndpointsStitcher(const std::vector<ObjectNode> *nodes,
            const std::vector<std::unordered_set<size_t>> *groups,
            const std::vector<std::pair<size_t, size_t>> *groupEndpoints,
            std::vector<std::pair<size_t, float>> *stitchResult) :
        m_nodes(nodes),
        m_groups(groups),
        m_groupEndpoints(groupEndpoints),
        m_stitchResult(stitchResult)
    {
    }
    void operator()(const tbb::blocked_range<size_t> &range) const
    {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            std::vector<std::pair<size_t, float>> distance2WithNodes;
            const auto &endpoint = (*m_groupEndpoints)[i];
            const auto &endpointNode = (*m_nodes)[endpoint.second];
            for (size_t groupIndex = 0; groupIndex < m_groups->size(); ++groupIndex) {
                if (endpoint.first == groupIndex)
                    continue;
                for (const auto &j: (*m_groups)[groupIndex]) {
                    const auto &node = (*m_nodes)[j];
                    if (node.partId == endpointNode.partId ||
                            node.mirroredByPartId == endpointNode.partId ||
                            node.mirrorFromPartId == endpointNode.partId) {
                        continue;
                    }
                    distance2WithNodes.push_back({j,
                        (endpointNode.origin - node.origin).lengthSquared()});
                }
            }
            if (distance2WithNodes.empty())
                continue;
            (*m_stitchResult)[i] = *std::min_element(distance2WithNodes.begin(), distance2WithNodes.end(), [](const std::pair<size_t, float> &first, const std::pair<size_t, float> &second) {
                return first.second < second.second;
            });
        }
    }
private:
    const std::vector<ObjectNode> *m_nodes = nullptr;
    const std::vector<std::unordered_set<size_t>> *m_groups = nullptr;
    const std::vector<std::pair<size_t, size_t>> *m_groupEndpoints = nullptr;
    std::vector<std::pair<size_t, float>> *m_stitchResult = nullptr;
};

RigGenerator::RigGenerator(RigType rigType, const Object &object) :
    m_rigType(rigType),
    m_object(new Object(object))
{
}

RigGenerator::~RigGenerator()
{
    delete m_object;
    delete m_resultMesh;
    delete m_resultBones;
    delete m_resultWeights;
}

Object *RigGenerator::takeObject()
{
    Object *object = m_object;
    m_object = nullptr;
    return object;
}

std::vector<RigBone> *RigGenerator::takeResultBones()
{
    std::vector<RigBone> *resultBones = m_resultBones;
    m_resultBones = nullptr;
    return resultBones;
}

std::map<int, RigVertexWeights> *RigGenerator::takeResultWeights()
{
    std::map<int, RigVertexWeights> *resultWeights = m_resultWeights;
    m_resultWeights = nullptr;
    return resultWeights;
}

Model *RigGenerator::takeResultMesh()
{
    Model *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

bool RigGenerator::isSuccessful()
{
    return m_isSuccessful;
}

const std::vector<std::pair<QtMsgType, QString>> &RigGenerator::messages()
{
    return m_messages;
}

void RigGenerator::groupNodeIndices(const std::map<size_t, std::unordered_set<size_t>> &neighborMap,
        std::vector<std::unordered_set<size_t>> *groups)
{
    std::unordered_set<size_t> visited;
    for (const auto &it: neighborMap) {
        if (visited.find(it.first) != visited.end())
            continue;
        std::unordered_set<size_t> group;
        std::queue<size_t> waitQueue;
        visited.insert(it.first);
        group.insert(it.first);
        waitQueue.push(it.first);
        while (!waitQueue.empty()) {
            auto nodeIndex = waitQueue.front();
            waitQueue.pop();
            auto findNeighbor = neighborMap.find(nodeIndex);
            if (findNeighbor != neighborMap.end()) {
                for (const auto &neighbor: findNeighbor->second) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        group.insert(neighbor);
                        waitQueue.push(neighbor);
                    }
                }
            }
        }
        groups->push_back(group);
    }
}

void RigGenerator::buildNeighborMap()
{
    if (nullptr == m_object->triangleSourceNodes())
        return;
    
    std::map<std::pair<QUuid, QUuid>, size_t> nodeIdToIndexMap;
    for (size_t i = 0; i < m_object->nodes.size(); ++i) {
        const auto &node = m_object->nodes[i];
        if (ComponentLayer::Body != node.layer)
            continue;
        nodeIdToIndexMap.insert({{node.partId, node.nodeId}, i});
        m_neighborMap.insert({i, {}});
    }
    
    for (const auto &it: m_object->edges) {
        const auto &findSource = nodeIdToIndexMap.find(it.first);
        if (findSource == nodeIdToIndexMap.end())
            continue;
        const auto &findTarget = nodeIdToIndexMap.find(it.second);
        if (findTarget == nodeIdToIndexMap.end())
            continue;
        m_neighborMap[findSource->second].insert(findTarget->second);
        m_neighborMap[findTarget->second].insert(findSource->second);
    }
    
    //std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> debugBoxes;
    while (true) {
        std::vector<std::unordered_set<size_t>> groups;
        groupNodeIndices(m_neighborMap, &groups);
        if (groups.size() < 2)
            break;

        std::vector<std::pair<size_t, size_t>> groupEndpoints;
        for (size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
            const auto &group = groups[groupIndex];
            for (const auto &nodeIndex: group) {
                if (m_neighborMap[nodeIndex].size() >= 2)
                    continue;
                groupEndpoints.push_back({groupIndex, nodeIndex});
            }
        }
        
        if (groupEndpoints.empty())
            break;
        
        std::vector<std::pair<size_t, float>> stitchResult(groupEndpoints.size(),
            {m_object->nodes.size(), std::numeric_limits<float>::max()});
        tbb::parallel_for(tbb::blocked_range<size_t>(0, groupEndpoints.size()),
            GroupEndpointsStitcher(&m_object->nodes, &groups, &groupEndpoints,
                &stitchResult));
        auto minDistantMatch = std::min_element(stitchResult.begin(), stitchResult.end(), [&](
                const std::pair<size_t, float> &first,
                const std::pair<size_t, float> &second) {
            return first.second < second.second;
        });
        if (minDistantMatch->first == m_object->nodes.size())
            break;
        
        const auto &fromNodeIndex = groupEndpoints[minDistantMatch - stitchResult.begin()].second;
        const auto &toNodeIndex = minDistantMatch->first;
        m_neighborMap[fromNodeIndex].insert(toNodeIndex);
        m_neighborMap[toNodeIndex].insert(fromNodeIndex);
        
        //const auto &fromNode = m_object->bodyNodes[fromNodeIndex];
        //const auto &toNode = m_object->bodyNodes[toNodeIndex];
        //debugBoxes.push_back(std::make_tuple(fromNode.origin, toNode.origin,
        //    fromNode.radius, toNode.radius, Qt::red));
    }
    
    //m_debugEdgeVertices = buildBoundingBoxMeshEdges(debugBoxes, &m_debugEdgeVerticesNum);
}

void RigGenerator::buildBoneNodeChain()
{
    std::vector<std::tuple<size_t, std::unordered_set<size_t>, bool>> segments;
    std::unordered_set<size_t> middle;
    size_t middleStartNodeIndex = m_object->nodes.size();
    for (size_t nodeIndex = 0; nodeIndex < m_object->nodes.size(); ++nodeIndex) {
        const auto &node = m_object->nodes[nodeIndex];
        if (ComponentLayer::Body != node.layer)
            continue;
        if (!BoneMarkIsBranchNode(node.boneMark))
            continue;
        m_branchNodesMapByMark[(int)node.boneMark].push_back(nodeIndex);
        if (BoneMark::Neck == node.boneMark) {
            if (middleStartNodeIndex == m_object->nodes.size())
                middleStartNodeIndex = nodeIndex;
        } else if (BoneMark::Tail == node.boneMark) {
            middleStartNodeIndex = nodeIndex;
        }
        std::unordered_set<size_t> left;
        std::unordered_set<size_t> right;
        splitByNodeIndex(nodeIndex, &left, &right);
        if (left.size() > right.size())
            std::swap(left, right);
        for (const auto &it: right)
            middle.insert(it);
        segments.push_back(std::make_tuple(nodeIndex, left, false));
    }
    for (const auto &it: segments) {
        const auto &nodeIndex = std::get<0>(it);
        const auto &left = std::get<1>(it);
        for (const auto &it: left)
            middle.erase(it);
        middle.erase(nodeIndex);
    }
    middle.erase(middleStartNodeIndex);
    if (middleStartNodeIndex != m_object->nodes.size())
        segments.push_back(std::make_tuple(middleStartNodeIndex, middle, true));
    for (const auto &it: segments) {
        const auto &fromNodeIndex = std::get<0>(it);
        const auto &left = std::get<1>(it);
        const auto &isSpine = std::get<2>(it);
        const auto &fromNode = m_object->nodes[fromNodeIndex];
        std::vector<std::vector<size_t>> boneNodeIndices;
        std::unordered_set<size_t> visited;
        size_t attachNodeIndex = fromNodeIndex;
        collectNodesForBoneRecursively(fromNodeIndex,
            &left,
            &boneNodeIndices,
            0,
            &visited);
        if (BoneMark::Limb == fromNode.boneMark) {
            for (const auto &neighbor: m_neighborMap[fromNodeIndex]) {
                if (left.find(neighbor) == left.end()) {
                    attachNodeIndex = neighbor;
                    break;
                }
            }
        }
        m_boneNodeChain.push_back({fromNodeIndex, boneNodeIndices, isSpine, attachNodeIndex});
    }
    for (size_t i = 0; i < m_boneNodeChain.size(); ++i) {
        const auto &chain = m_boneNodeChain[i];
        const auto &node = m_object->nodes[chain.fromNodeIndex];
        const auto &isSpine = chain.isSpine;
        if (isSpine) {
            m_spineChains.push_back(i);
            continue;
        }
        if (BoneMark::Neck == node.boneMark) {
            m_neckChains.push_back(i);
        } else if (BoneMark::Tail == node.boneMark) {
            m_tailChains.push_back(i);
        } else if (BoneMark::Limb == node.boneMark) {
            if (node.origin.x() > 0) {
                m_leftLimbChains.push_back(i);
            } else if (node.origin.x() < 0) {
                m_rightLimbChains.push_back(i);
            }
        }
    }
}

void RigGenerator::calculateSpineDirection(bool *isVertical)
{
    float left = std::numeric_limits<float>::lowest();
    float right = std::numeric_limits<float>::max();
    float top = std::numeric_limits<float>::lowest();
    float bottom = std::numeric_limits<float>::max();
    auto updateBoundingBox = [&](const std::vector<size_t> &chains) {
        for (const auto &it: chains) {
            const auto &node = m_object->nodes[m_boneNodeChain[it].fromNodeIndex];
            if (node.origin.y() > top)
                top = node.origin.y();
            if (node.origin.y() < bottom)
                bottom = node.origin.y();
            if (node.origin.z() > left)
                left = node.origin.z();
            if (node.origin.z() < right)
                right = node.origin.z();
        }
    };
    updateBoundingBox(m_leftLimbChains);
    updateBoundingBox(m_rightLimbChains);
    auto zLength = left - right;
    auto yLength = top - bottom;
    *isVertical = yLength >= zLength;
}

void RigGenerator::attachLimbsToSpine()
{
    Q_ASSERT(m_leftLimbChains.size() == m_rightLimbChains.size());
    Q_ASSERT(m_spineChains.size() == 1);
    
    m_attachLimbsToSpineNodeIndices.resize(m_leftLimbChains.size());
    for (size_t i = 0; i < m_leftLimbChains.size(); ++i) {
        const auto &leftNode = m_object->nodes[m_boneNodeChain[m_leftLimbChains[i]].attachNodeIndex];
        const auto &rightNode = m_object->nodes[m_boneNodeChain[m_rightLimbChains[i]].attachNodeIndex];
        auto limbMiddle = (leftNode.origin + rightNode.origin) * 0.5;
        std::vector<std::pair<size_t, float>> distance2WithSpine;
        auto boneNodeChainIndex = m_spineChains[0];
        const auto &nodeIndices = m_boneNodeChain[boneNodeChainIndex].nodeIndices;
        for (const auto &it: nodeIndices) {
            for (const auto &nodeIndex: it) {
                distance2WithSpine.push_back({
                    nodeIndex,
                    (m_object->nodes[nodeIndex].origin - limbMiddle).lengthSquared()
                });
            }
        }
        if (distance2WithSpine.empty())
            continue;
        auto nodeIndex = std::min_element(distance2WithSpine.begin(), distance2WithSpine.end(), [](
                const std::pair<size_t, float> &first, const std::pair<size_t, float> &second) {
            return first.second < second.second;
        })->first;
        m_attachLimbsToSpineNodeIndices[i] = nodeIndex;
        m_virtualJoints.insert(nodeIndex);
    }
}

int RigGenerator::attachedBoneIndex(size_t spineJointIndex)
{
    if (spineJointIndex == m_rootSpineJointIndex) {
        return m_boneNameToIndexMap[QString("Body")];
    }
    return m_boneNameToIndexMap[QString("Spine") + QString::number(spineJointIndex - m_rootSpineJointIndex)];
}

void RigGenerator::buildSkeleton()
{
    bool addMarkHelpInfo = false;
    
    if (m_leftLimbChains.size() != m_rightLimbChains.size()) {
        m_messages.push_back({QtInfoMsg, tr("Imbalanced left and right limbs")});
    } else if (m_leftLimbChains.empty()) {
        m_messages.push_back({QtInfoMsg, tr("No limbs found")});
        addMarkHelpInfo = true;
    }
    
    if (m_spineChains.empty()) {
        m_messages.push_back({QtInfoMsg, tr("No body found")});
        addMarkHelpInfo = true;
    }
    
    if (addMarkHelpInfo) {
        m_messages.push_back({QtInfoMsg, tr("Please mark the neck, limbs and joints from the context menu")});
    }
    
    if (!m_messages.empty())
        return;
    
    calculateSpineDirection(&m_isSpineVertical);
    qDebug() << "Spine:" << (m_isSpineVertical ? "Vertical" : "Horizontal");
    
    auto sortLimbChains = [&](std::vector<size_t> &chains) {
        std::sort(chains.begin(), chains.end(), [&](const size_t &first,
                const size_t &second) {
            if (m_isSpineVertical) {
                return m_object->nodes[m_boneNodeChain[first].fromNodeIndex].origin.y() <
                    m_object->nodes[m_boneNodeChain[second].fromNodeIndex].origin.y();
            }
            return m_object->nodes[m_boneNodeChain[first].fromNodeIndex].origin.z() <
                m_object->nodes[m_boneNodeChain[second].fromNodeIndex].origin.z();
        });
    };
    sortLimbChains(m_leftLimbChains);
    sortLimbChains(m_rightLimbChains);
    
    attachLimbsToSpine();
    extractSpineJoints();
    extractBranchJoints();
    
    m_rootSpineJointIndex = m_attachLimbsToSpineJointIndices[0];
    m_lastSpineJointIndex = m_spineJoints.size() - 1;
    
    m_resultBones = new std::vector<RigBone>;
    m_resultWeights = new std::map<int, RigVertexWeights>;
    
    {
        const auto &firstSpineNode = m_object->nodes[m_spineJoints[m_rootSpineJointIndex]];
        RigBone bone;
        bone.headPosition = QVector3D(0.0, 0.0, 0.0);
        bone.tailPosition = firstSpineNode.origin;
        bone.headRadius = 0;
        bone.tailRadius = firstSpineNode.radius;
        bone.color = Theme::white;
        bone.name = QString("Body");
        bone.attributes["spineDirection"] = m_isSpineVertical ? "Vertical" : "Horizontal";
        bone.index = m_resultBones->size();
        bone.parent = -1;
        m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
        m_resultBones->push_back(bone);
    }

    for (size_t spineJointIndex = m_rootSpineJointIndex;
            spineJointIndex + 1 < m_spineJoints.size();
            ++spineJointIndex) {
        const auto &currentNode = m_object->nodes[m_spineJoints[spineJointIndex]];
        const auto &nextNode = m_object->nodes[m_spineJoints[spineJointIndex + 1]];
        RigBone bone;
        bone.headPosition = currentNode.origin;
        bone.tailPosition = nextNode.origin;
        bone.headRadius = currentNode.radius;
        bone.tailRadius = nextNode.radius;
        bone.color = 0 == (spineJointIndex - m_rootSpineJointIndex) % 2 ? Theme::red : Qt::blue; //BoneMarkToColor(BoneMark::Joint);
        bone.name = QString("Spine") + QString::number(spineJointIndex + 1 - m_rootSpineJointIndex);
        bone.index = m_resultBones->size();
        bone.parent = attachedBoneIndex(spineJointIndex);
        m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
        m_resultBones->push_back(bone);
        (*m_resultBones)[bone.parent].children.push_back(bone.index);
    }
    
    auto addSpineLinkBone = [&](size_t limbIndex,
            const std::vector<std::vector<size_t>> &limbJoints,
            const QString &chainPrefix) {
        QString chainName = chainPrefix + QString::number(limbIndex + 1);
        const auto &spineJointIndex = m_attachLimbsToSpineJointIndices[limbIndex];
        const auto &spineNode = m_object->nodes[m_spineJoints[spineJointIndex]];
        const auto &limbFirstNode = m_object->nodes[limbJoints[limbIndex][0]];
        const auto &parentIndex = attachedBoneIndex(spineJointIndex);
        RigBone bone;
        bone.headPosition = spineNode.origin;
        bone.tailPosition = limbFirstNode.origin;
        bone.headRadius = spineNode.radius;
        bone.tailRadius = limbFirstNode.radius;
        bone.color = chainPrefix.startsWith("Left") ? BoneMarkToColor(BoneMark::Tail) : BoneMarkToColor(BoneMark::Neck);
        bone.name = QString("Virtual_") + (*m_resultBones)[parentIndex].name + QString("_") + chainName;
        bone.index = m_resultBones->size();
        bone.parent = parentIndex;
        m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
        m_resultBones->push_back(bone);
        (*m_resultBones)[parentIndex].children.push_back(bone.index);
    };
    
    auto addLimbBone = [&](size_t limbIndex,
            const std::vector<std::vector<size_t>> &limbJoints,
            const QString &chainPrefix) {
        const auto &joints = limbJoints[limbIndex];
        QString chainName = chainPrefix + QString::number(limbIndex + 1);
        for (size_t limbJointIndex = 0;
                limbJointIndex + 1 < joints.size();
                ++limbJointIndex) {
            const auto &currentNode = m_object->nodes[joints[limbJointIndex]];
            const auto &nextNode = m_object->nodes[joints[limbJointIndex + 1]];
            RigBone bone;
            bone.headPosition = currentNode.origin;
            bone.tailPosition = nextNode.origin;
            bone.headRadius = currentNode.radius;
            bone.tailRadius = nextNode.radius;
            bone.color = 0 == limbJointIndex % 2 ? BoneMarkToColor(BoneMark::Limb) : BoneMarkToColor(BoneMark::Joint);
            bone.name = chainName + QString("_Joint") + QString::number(limbJointIndex + 1);
            bone.index = m_resultBones->size();
            if (limbJointIndex > 0) {
                auto parentName = chainName + QString("_Joint") + QString::number(limbJointIndex);
                bone.parent = m_boneNameToIndexMap[parentName];
            } else {
                const auto &spineJointIndex = m_attachLimbsToSpineJointIndices[limbIndex];
                const auto &parentIndex = attachedBoneIndex(spineJointIndex);
                auto parentName = QString("Virtual_") + (*m_resultBones)[parentIndex].name + QString("_") + chainName;
                bone.parent = m_boneNameToIndexMap[parentName];
            }
            m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
            m_resultBones->push_back(bone);
            (*m_resultBones)[bone.parent].children.push_back(bone.index);
        }
    };
    
    for (size_t limbIndex = 0;
            limbIndex < m_attachLimbsToSpineJointIndices.size();
            ++limbIndex) {
        addSpineLinkBone(limbIndex, m_leftLimbJoints, QString("LeftLimb"));
        addSpineLinkBone(limbIndex, m_rightLimbJoints, QString("RightLimb"));
        addLimbBone(limbIndex, m_leftLimbJoints, QString("LeftLimb"));
        addLimbBone(limbIndex, m_rightLimbJoints, QString("RightLimb"));
    }
    
    if (!m_neckJoints.empty()) {
        for (size_t neckJointIndex = 0;
                neckJointIndex + 1 < m_neckJoints.size();
                ++neckJointIndex) {
            const auto &currentNode = m_object->nodes[m_neckJoints[neckJointIndex]];
            const auto &nextNode = m_object->nodes[m_neckJoints[neckJointIndex + 1]];
            RigBone bone;
            bone.headPosition = currentNode.origin;
            bone.tailPosition = nextNode.origin;
            bone.headRadius = currentNode.radius;
            bone.tailRadius = nextNode.radius;
            bone.color = 0 == neckJointIndex % 2 ? BoneMarkToColor(BoneMark::Neck) : BoneMarkToColor(BoneMark::Joint);
            bone.name = QString("Neck_Joint") + QString::number(neckJointIndex + 1);
            bone.index = m_resultBones->size();
            if (neckJointIndex > 0) {
                auto parentName = QString("Neck_Joint") + QString::number(neckJointIndex);
                bone.parent = m_boneNameToIndexMap[parentName];
            } else {
                auto parentName = QString("Spine") + QString::number(m_lastSpineJointIndex - m_rootSpineJointIndex);
                bone.parent = m_boneNameToIndexMap[parentName];
            }
            m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
            m_resultBones->push_back(bone);
            (*m_resultBones)[bone.parent].children.push_back(bone.index);
        }
    }
    
    if (!m_tailJoints.empty()) {
        QString nearestSpine = "Body";
        for (int spineJointIndex = m_rootSpineJointIndex;
                spineJointIndex >= 0;
                --spineJointIndex) {
            if (m_spineJoints[spineJointIndex] == m_tailJoints[0])
                break;
            const auto &currentNode = m_object->nodes[m_spineJoints[spineJointIndex]];
            const auto &nextNode = spineJointIndex > 0 ?
                m_object->nodes[m_spineJoints[spineJointIndex - 1]] :
                m_object->nodes[m_tailJoints[0]];
            RigBone bone;
            bone.headPosition = currentNode.origin;
            bone.tailPosition = nextNode.origin;
            bone.headRadius = currentNode.radius;
            bone.tailRadius = nextNode.radius;
            bone.color = 0 == (m_rootSpineJointIndex - spineJointIndex) % 2 ? BoneMarkToColor(BoneMark::Joint) : Theme::white;
            bone.name = QString("Spine0") + QString::number(m_rootSpineJointIndex - spineJointIndex + 1);
            bone.index = m_resultBones->size();
            if ((int)m_rootSpineJointIndex == spineJointIndex) {
                auto parentName = QString("Body");
                bone.parent = m_boneNameToIndexMap[parentName];
            } else {
                auto parentName = QString("Spine0") + QString::number(m_rootSpineJointIndex - spineJointIndex);
                bone.parent = m_boneNameToIndexMap[parentName];
            }
            m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
            m_resultBones->push_back(bone);
            (*m_resultBones)[bone.parent].children.push_back(bone.index);
            nearestSpine = bone.name;
        }
    
        for (size_t tailJointIndex = 0;
                tailJointIndex + 1 < m_tailJoints.size();
                ++tailJointIndex) {
            const auto &currentNode = m_object->nodes[m_tailJoints[tailJointIndex]];
            const auto &nextNode = m_object->nodes[m_tailJoints[tailJointIndex + 1]];
            RigBone bone;
            bone.headPosition = currentNode.origin;
            bone.tailPosition = nextNode.origin;
            bone.headRadius = currentNode.radius;
            bone.tailRadius = nextNode.radius;
            bone.color = 0 == tailJointIndex % 2 ? BoneMarkToColor(BoneMark::Tail) : BoneMarkToColor(BoneMark::Joint);
            bone.name = QString("Tail_Joint") + QString::number(tailJointIndex + 1);
            bone.index = m_resultBones->size();
            if (tailJointIndex > 0) {
                auto parentName = QString("Tail_Joint") + QString::number(tailJointIndex);
                bone.parent = m_boneNameToIndexMap[parentName];
            } else {
                auto parentName = nearestSpine;
                bone.parent = m_boneNameToIndexMap[parentName];
            }
            m_boneNameToIndexMap.insert({bone.name, (int)bone.index});
            m_resultBones->push_back(bone);
            (*m_resultBones)[bone.parent].children.push_back(bone.index);
        }
    }
    
    m_isSuccessful = true;
    
    if (nullptr != m_resultBones) {
        for (size_t i = 0; i < m_resultBones->size(); ++i) {
            const auto &bone = (*m_resultBones)[i];
            qDebug() << "Bone[" << i << "]: name:" << bone.name << "parent:" << (-1 == bone.parent ? "(null)" : (*m_resultBones)[bone.parent].name);
        }
    }
}

void RigGenerator::computeSkinWeights()
{
    if (!m_isSuccessful)
        return;

    auto collectNodeIndices = [&](size_t chainIndex,
            std::unordered_map<size_t, size_t> *nodeIndexToContainerMap,
            size_t containerIndex) {
        const auto &chain = m_boneNodeChain[chainIndex];
        for (const auto &it: chain.nodeIndices) {
            for (const auto &subIt: it)
                nodeIndexToContainerMap->insert({subIt, containerIndex});
        }
        nodeIndexToContainerMap->insert({chain.fromNodeIndex, containerIndex});
    };
    
    const size_t neckIndex = 0;
    const size_t tailIndex = 1;
    const size_t spineIndex = 2;
    const size_t limbStartIndex = 3;
    
    std::unordered_map<size_t, size_t> nodeIndicesToBranchMap;
    
    if (!m_neckChains.empty())
        collectNodeIndices(m_neckChains[0], &nodeIndicesToBranchMap, neckIndex);
    
    if (!m_tailChains.empty())
        collectNodeIndices(m_tailChains[0], &nodeIndicesToBranchMap, tailIndex);

    if (!m_spineChains.empty())
        collectNodeIndices(m_spineChains[0], &nodeIndicesToBranchMap, spineIndex);
    
    for (size_t i = 0; i < m_leftLimbChains.size(); ++i) {
        collectNodeIndices(m_leftLimbChains[i], &nodeIndicesToBranchMap,
            limbStartIndex + i);
    }
    
    for (size_t i = 0; i < m_rightLimbChains.size(); ++i) {
        collectNodeIndices(m_rightLimbChains[i], &nodeIndicesToBranchMap,
            limbStartIndex + m_leftLimbChains.size() + i);
    }
    
    std::vector<std::vector<size_t>> vertexBranches(limbStartIndex +
        m_leftLimbChains.size() +
        m_rightLimbChains.size() +
        1);
    
    std::map<std::pair<QUuid, QUuid>, size_t> nodeIdToIndexMap;
    for (size_t nodeIndex = 0; nodeIndex < m_object->nodes.size(); ++nodeIndex) {
        const auto &node = m_object->nodes[nodeIndex];
        if (ComponentLayer::Body != node.layer)
            continue;
        nodeIdToIndexMap[{node.partId, node.nodeId}] = nodeIndex;
    }
    if (!nodeIdToIndexMap.empty()) {
        for (size_t nonBodyNodeIndex = 0; nonBodyNodeIndex < m_object->nodes.size(); ++nonBodyNodeIndex) {
            const auto &nonBodyNode = m_object->nodes[nonBodyNodeIndex];
            if (ComponentLayer::Body == nonBodyNode.layer)
                continue;
            std::vector<std::pair<size_t, float>> distance2s;
            distance2s.reserve(m_object->nodes.size());
            for (size_t nodeIndex = 0; nodeIndex < m_object->nodes.size(); ++nodeIndex) {
                const auto &node = m_object->nodes[nodeIndex];
                if (ComponentLayer::Body != node.layer)
                    continue;
                distance2s.push_back(std::make_pair(nodeIndex,
                    (nonBodyNode.origin - node.origin).lengthSquared()));
            }
            nodeIdToIndexMap[{nonBodyNode.partId, nonBodyNode.nodeId}] = std::min_element(distance2s.begin(), distance2s.end(), [](const std::pair<size_t, float> &first,
                    const std::pair<size_t, float> &second) {
                return first.second < second.second;
            })->first;
        }
    }
    for (size_t vertexIndex = 0; vertexIndex < m_object->vertices.size(); ++vertexIndex) {
        const auto &vertexSourceId = m_object->vertexSourceNodes[vertexIndex];
        auto findNodeIndex = nodeIdToIndexMap.find(vertexSourceId);
        if (findNodeIndex == nodeIdToIndexMap.end()) {
            vertexBranches[spineIndex].push_back(vertexIndex);
            continue;
        }
        const auto &nodeIndex = findNodeIndex->second;
        auto findBranch = nodeIndicesToBranchMap.find(nodeIndex);
        if (findBranch == nodeIndicesToBranchMap.end()) {
            vertexBranches[spineIndex].push_back(vertexIndex);
            continue;
        }
        vertexBranches[findBranch->second].push_back(vertexIndex);
    }
    
    auto findNeckBoneIndex = m_boneNameToIndexMap.find(QString("Neck_Joint1"));
    if (findNeckBoneIndex != m_boneNameToIndexMap.end()) {
        computeBranchSkinWeights(findNeckBoneIndex->second,
            QString("Neck_"), vertexBranches[neckIndex],
            &vertexBranches[spineIndex]);
    }
    
    auto findTailBoneIndex = m_boneNameToIndexMap.find(QString("Tail_Joint1"));
    if (findTailBoneIndex != m_boneNameToIndexMap.end()) {
        computeBranchSkinWeights(findTailBoneIndex->second,
            QString("Tail_"), vertexBranches[tailIndex],
            &vertexBranches[spineIndex]);
    }
    
    for (size_t i = 0; i < m_leftLimbChains.size(); ++i) {
        auto namePrefix = QString("LeftLimb") + QString::number(i + 1) + "_";
        auto findLimbBoneIndex = m_boneNameToIndexMap.find(namePrefix + "Joint1");
        if (findLimbBoneIndex != m_boneNameToIndexMap.end()) {
            computeBranchSkinWeights(findLimbBoneIndex->second,
                namePrefix, vertexBranches[limbStartIndex + i],
                &vertexBranches[spineIndex]);
        }
    }
    
    for (size_t i = 0; i < m_rightLimbChains.size(); ++i) {
        auto namePrefix = QString("RightLimb") + QString::number(i + 1) + "_";
        auto findLimbBoneIndex = m_boneNameToIndexMap.find(namePrefix + "Joint1");
        if (findLimbBoneIndex != m_boneNameToIndexMap.end()) {
            computeBranchSkinWeights(findLimbBoneIndex->second,
                namePrefix, vertexBranches[limbStartIndex + m_leftLimbChains.size() + i],
                &vertexBranches[spineIndex]);
        }
    }
    
    auto findBackSpineBoneIndex = m_boneNameToIndexMap.find(QString("Spine01"));
    std::vector<size_t> backSpineVertices;
    auto findSpineBoneIndex = m_boneNameToIndexMap.find(QString("Spine1"));
    if (findSpineBoneIndex != m_boneNameToIndexMap.end()) {
        computeBranchSkinWeights(findSpineBoneIndex->second,
            QString("Spine"), vertexBranches[spineIndex],
            findBackSpineBoneIndex != m_boneNameToIndexMap.end() ?
                &backSpineVertices : nullptr);
    }
    
    if (findBackSpineBoneIndex != m_boneNameToIndexMap.end()) {
        computeBranchSkinWeights(findBackSpineBoneIndex->second,
            QString("Spine"), backSpineVertices);
    }
    
    fixVirtualBoneSkinWeights();
    
    for (auto &it: *m_resultWeights)
        it.second.finalizeWeights();
    
    //for (size_t i = 0; i < m_object->vertices.size(); ++i) {
    //    auto findWeights = m_resultWeights->find(i);
    //    if (findWeights == m_resultWeights->end()) {
    //        const auto &sourceNode = m_object->vertexSourceNodes[i];
    //        qDebug() << "NoWeight vertex index:" << i << sourceNode.first << sourceNode.second;
    //    }
    //}
}

void RigGenerator::fixVirtualBoneSkinWeights()
{
    auto calculateSide = [](float x) {
        if (x < 0)
            return -1;
        else if (x > 0)
            return 1;
        return 0;
    };
    
    struct VirtualBone
    {
        int index;
        int side;
        int parentIndex;
        int parentNextIndex;
    };
    
    std::vector<VirtualBone> virtualBones;
    for (size_t limbIndex = 0;
            limbIndex < m_attachLimbsToSpineJointIndices.size();
            ++limbIndex) {
        QString limbChainName = QString("Limb") + QString::number(limbIndex + 1);
        const auto &spineJointIndex = m_attachLimbsToSpineJointIndices[limbIndex];
        const auto &parentIndex = attachedBoneIndex(spineJointIndex);
        if (0 == parentIndex)
            continue;
        int parentNextIndex = -1;
        for (const auto &childIndex: (*m_resultBones)[parentIndex].children) {
            const auto &child = (*m_resultBones)[childIndex];
            if (child.name.startsWith("Spine")) {
                parentNextIndex = childIndex;
                break;
            }
        }
        if (-1 == parentNextIndex)
            continue;
        QString prefixName = QString("Virtual_") + (*m_resultBones)[parentIndex].name;
        QString leftBoneName = prefixName + QString("_Left") + limbChainName;
        QString rightBoneName = prefixName + QString("_Right") + limbChainName;
        auto findLeftIndex = m_boneNameToIndexMap.find(leftBoneName);
        if (findLeftIndex != m_boneNameToIndexMap.end()) {
            virtualBones.push_back({findLeftIndex->second, 
                calculateSide((*m_resultBones)[findLeftIndex->second].tailPosition.x()), 
                parentIndex, 
                parentNextIndex});
            //const auto &leftBone = (*m_resultBones)[findLeftIndex->second];
            //qDebug() << "leftBone:" << leftBone.name << "headRadius:" << leftBone.headRadius << "tailRadius:" << leftBone.tailRadius;
        }
        auto findRightIndex = m_boneNameToIndexMap.find(rightBoneName);
        if (findRightIndex != m_boneNameToIndexMap.end()) {
            virtualBones.push_back({findRightIndex->second, 
                calculateSide((*m_resultBones)[findRightIndex->second].tailPosition.x()), 
                parentIndex, 
                parentNextIndex});
            //const auto &rightBone = (*m_resultBones)[findRightIndex->second];
            //qDebug() << "rightBone:" << rightBone.name << "headRadius:" << rightBone.headRadius << "tailRadius:" << rightBone.tailRadius;
        }
    }
    
    std::unordered_map<int, std::vector<size_t>> boneVerticesMap;
    for (auto &it: *m_resultWeights) {
        for (const auto &weight: it.second.boneRawWeights()) {
            const auto &boneIndex = weight.first;
            if (0 == boneIndex)
                continue;
            boneVerticesMap[boneIndex].push_back(it.first);
        }
    }
    
    for (const auto &it: virtualBones) {
        const auto &bone = (*m_resultBones)[it.index];
        
        double boneLength = (bone.tailPosition - bone.headPosition).length() * 0.75;
        
        QVector3D boundaryLineTailForParentOnZ;
        QVector3D boundaryLineHeadForParentOnZ;
        QVector3D boundaryLineTailForParentNextOnZ;
        QVector3D boundaryLineHeadForParentNextOnZ;
        
        QVector3D boundaryLineTailForParentOnX;
        QVector3D boundaryLineHeadForParentOnX;
        QVector3D boundaryLineTailForParentNextOnX;
        QVector3D boundaryLineHeadForParentNextOnX;
        
        if (m_isSpineVertical) {
            boundaryLineTailForParentOnX = QVector3D(bone.tailPosition.x(),
                bone.tailPosition.y() - bone.tailRadius, 
                bone.tailPosition.z());
            boundaryLineHeadForParentOnX = QVector3D(bone.headPosition.x(),
                bone.headPosition.y() - bone.headRadius, 
                bone.headPosition.z());
                
            boundaryLineTailForParentNextOnX = QVector3D(bone.tailPosition.x(),
                bone.tailPosition.y() + bone.tailRadius, 
                bone.tailPosition.z());
            boundaryLineHeadForParentNextOnX = QVector3D(bone.headPosition.x(),
                bone.headPosition.y() + bone.headRadius, 
                bone.headPosition.z());
        } else {
            boundaryLineTailForParentOnZ = QVector3D(bone.tailPosition.x(),
                bone.tailPosition.y(), 
                bone.tailPosition.z() - bone.tailRadius);
            boundaryLineHeadForParentOnZ = QVector3D(bone.headPosition.x(),
                bone.headPosition.y(), 
                bone.headPosition.z() - bone.headRadius);
                
            boundaryLineTailForParentNextOnZ = QVector3D(bone.tailPosition.x(),
                bone.tailPosition.y(), 
                bone.tailPosition.z() + bone.tailRadius);
            boundaryLineHeadForParentNextOnZ = QVector3D(bone.headPosition.x(),
                bone.headPosition.y(), 
                bone.headPosition.z() + bone.headRadius);
        }
            
        float angleInRangle360BetweenTwoVectors(QVector3D a, QVector3D b, QVector3D planeNormal);
        for (const auto &vertexIndex: boneVerticesMap[it.parentIndex]) {
            if (it.side != calculateSide(m_object->vertices[vertexIndex].x()))
                continue;
            QVector3D projectedPosition = projectPointOnLine(m_object->vertices[vertexIndex], bone.tailPosition, bone.headPosition);
            if ((projectedPosition - bone.tailPosition).length() > boneLength)
                continue;
            if (m_isSpineVertical) {
                double angle = angleInRangle360BetweenTwoVectors((boundaryLineHeadForParentOnX - boundaryLineTailForParentOnX).normalized(),
                    (m_object->vertices[vertexIndex] - boundaryLineTailForParentOnX).normalized(),
                    QVector3D(0.0, 0.0, -it.side));
                if (angle > 180)
                    continue;
            } else {
                double angle = angleInRangle360BetweenTwoVectors((boundaryLineHeadForParentOnZ - boundaryLineTailForParentOnZ).normalized(),
                    (m_object->vertices[vertexIndex] - boundaryLineTailForParentOnZ).normalized(),
                    QVector3D(1.0, 0.0, 0.0));
                if (angle > 180)
                    continue;
            }
            (*m_resultWeights)[vertexIndex].addBone(it.index, 1.0);
        }
        for (const auto &vertexIndex: boneVerticesMap[it.parentNextIndex]) {
            if (it.side != calculateSide(m_object->vertices[vertexIndex].x()))
                continue;
            QVector3D projectedPosition = projectPointOnLine(m_object->vertices[vertexIndex], bone.tailPosition, bone.headPosition);
            if ((projectedPosition - bone.tailPosition).length() > boneLength)
                continue;
            if (m_isSpineVertical) {
                double angle = angleInRangle360BetweenTwoVectors((m_object->vertices[vertexIndex] - boundaryLineTailForParentNextOnX).normalized(),
                    (boundaryLineHeadForParentNextOnX - boundaryLineTailForParentNextOnX).normalized(),
                    QVector3D(0.0, 0.0, -it.side));
                if (angle > 180)
                    continue;
            } else {
                double angle = angleInRangle360BetweenTwoVectors((m_object->vertices[vertexIndex] - boundaryLineTailForParentNextOnZ).normalized(),
                    (boundaryLineHeadForParentNextOnZ - boundaryLineTailForParentNextOnZ).normalized(),
                    QVector3D(1.0, 0.0, 0.0));
                if (angle > 180)
                    continue;
            }
            (*m_resultWeights)[vertexIndex].addBone(it.index, 1.0);
        }
    }
}

void RigGenerator::computeBranchSkinWeights(size_t fromBoneIndex,
        const QString &boneNamePrefix,
        const std::vector<size_t> &vertexIndices,
        std::vector<size_t> *discardedVertexIndices)
{
    //qDebug() << "computeBranchSkinWeights boneNamePrefix:" << boneNamePrefix;
    std::vector<size_t> remainVertexIndices = vertexIndices;
    size_t currentBoneIndex = fromBoneIndex;
    while (true) {
        const auto &currentBone = (*m_resultBones)[currentBoneIndex];
        //qDebug() << "  bone:" << currentBone.name;
        std::vector<size_t> newRemainVertexIndices;
        const auto &parentBone = (*m_resultBones)[currentBone.parent];
        auto currentDirection = (currentBone.tailPosition - currentBone.headPosition).normalized();
        auto parentDirection = currentBone.parent <= 0 ?
            currentDirection :
            (parentBone.tailPosition - parentBone.headPosition).normalized();
        auto cutNormal = ((parentDirection + currentDirection) * 0.5f).normalized();
        auto beginGradientLength = parentBone.headRadius * 0.5f;
        auto endGradientLength = parentBone.tailRadius * 0.5f;
        auto parentLength = (parentBone.tailPosition - parentBone.headPosition).length();
        auto previousBoneIndex = /*currentBone.name.startsWith("Virtual") ? parentBone.parent : */currentBone.parent;
        for (const auto &vertexIndex: remainVertexIndices) {
            const auto &position = m_object->vertices[vertexIndex];
            auto direction = (position - currentBone.headPosition).normalized();
            if (QVector3D::dotProduct(direction, cutNormal) > 0) {
                float angle = radianBetweenVectors(direction, currentDirection);
                auto projectedLength = std::cos(angle) * (position - currentBone.headPosition).length();
                if (projectedLength < 0)
					projectedLength = 0;
                if (projectedLength <= endGradientLength) {
                    auto factor = 0.1 + 0.4 * (1.0 - projectedLength / endGradientLength);
                    (*m_resultWeights)[vertexIndex].addBone(previousBoneIndex, factor);
                }
                newRemainVertexIndices.push_back(vertexIndex);
                continue;
            }
            if (fromBoneIndex == currentBoneIndex) {
                if (nullptr != discardedVertexIndices)
                    discardedVertexIndices->push_back(vertexIndex);
                else
                    (*m_resultWeights)[vertexIndex].addBone(currentBoneIndex, 1.0);
                continue;
            }
            float angle = radianBetweenVectors(direction, -parentDirection);
            auto projectedLength = std::cos(angle) * (position - currentBone.headPosition).length();
            if (projectedLength < 0)
				projectedLength = 0;
            if (projectedLength <= endGradientLength) {
                (*m_resultWeights)[vertexIndex].addBone(previousBoneIndex, 0.5 + 0.5 * projectedLength / endGradientLength);
                (*m_resultWeights)[vertexIndex].addBone(currentBoneIndex, 0.5 * (1.0 - projectedLength / endGradientLength));
                continue;
            }
            if (projectedLength <= parentLength - beginGradientLength) {
                (*m_resultWeights)[vertexIndex].addBone(previousBoneIndex, 1.0);
                continue;
            }
            if (projectedLength <= parentLength) {
                auto factor = 0.5 + 0.5 * (parentLength - projectedLength) / beginGradientLength;
                (*m_resultWeights)[vertexIndex].addBone(previousBoneIndex, factor);
                continue;
            }
            auto factor = 0.1 + 0.4 * (1.0 - (projectedLength - parentLength) / beginGradientLength);
            (*m_resultWeights)[vertexIndex].addBone(previousBoneIndex, factor);
            continue;
        }
        remainVertexIndices = newRemainVertexIndices;
        if (currentBone.children.empty() || !currentBone.name.startsWith(boneNamePrefix)) {
            for (const auto &vertexIndex: remainVertexIndices) {
                (*m_resultWeights)[vertexIndex].addBone(currentBoneIndex, 0.5);
            }
            break;
        }
        currentBoneIndex = currentBone.children[0];
    }
}

void RigGenerator::extractJointsFromBoneNodeChain(const BoneNodeChain &boneNodeChain,
        std::vector<size_t> *joints)
{
    const size_t &fromNodeIndex = boneNodeChain.fromNodeIndex;
    const std::vector<std::vector<size_t>> &nodeIndices = boneNodeChain.nodeIndices;
    extractJoints(fromNodeIndex, nodeIndices, joints);
}

void RigGenerator::extractJoints(const size_t &fromNodeIndex,
    const std::vector<std::vector<size_t>> &nodeIndices,
    std::vector<size_t> *joints,
    bool checkLastNoneMarkedNode)
{
    if (joints->empty() ||
            (*joints)[joints->size() - 1] != fromNodeIndex) {
        joints->push_back(fromNodeIndex);
    }
    const auto &fromNode = m_object->nodes[fromNodeIndex];
    std::vector<std::pair<size_t, float>> nodeIndicesAndDistance2Array;
    for (const auto &it: nodeIndices) {
        for (const auto &nodeIndex: it) {
            const auto &node = m_object->nodes[nodeIndex];
            nodeIndicesAndDistance2Array.push_back({
                nodeIndex,
                (fromNode.origin - node.origin).lengthSquared()
            });
        }
    }
    if (nodeIndicesAndDistance2Array.empty())
        return;
    std::sort(nodeIndicesAndDistance2Array.begin(), nodeIndicesAndDistance2Array.end(),
            [](const decltype(nodeIndicesAndDistance2Array)::value_type &first,
                const decltype(nodeIndicesAndDistance2Array)::value_type &second) {
        return first.second < second.second;
    });
    std::vector<size_t> jointIndices;
    for (size_t i = 0; i < nodeIndicesAndDistance2Array.size(); ++i) {
        const auto &item = nodeIndicesAndDistance2Array[i];
        const auto &node = m_object->nodes[item.first];
        if (BoneMark::None != node.boneMark ||
                m_virtualJoints.find(item.first) != m_virtualJoints.end()) {
            jointIndices.push_back(i);
            continue;
        }
    }
    bool appendLastJoint = false;
    if (checkLastNoneMarkedNode &&
            !jointIndices.empty() &&
            jointIndices[jointIndices.size() - 1] + 1 != nodeIndicesAndDistance2Array.size()) {
        if (jointIndices.empty())
            appendLastJoint = true;
    }
    if (jointIndices.empty() || appendLastJoint) {
        jointIndices.push_back(nodeIndicesAndDistance2Array.size() - 1);
    }
    for (const auto &itemIndex: jointIndices) {
        const auto &item = nodeIndicesAndDistance2Array[itemIndex];
        if (!joints->empty()) {
            if ((*joints)[joints->size() - 1] == item.first)
                continue;
        }
        joints->push_back(item.first);
    }
}

void RigGenerator::extractBranchJoints()
{
    if (!m_neckChains.empty())
        extractJointsFromBoneNodeChain(m_boneNodeChain[m_neckChains[0]], &m_neckJoints);
    if (!m_tailChains.empty())
        extractJointsFromBoneNodeChain(m_boneNodeChain[m_tailChains[0]], &m_tailJoints);
    m_leftLimbJoints.resize(m_leftLimbChains.size());
    for (size_t i = 0; i < m_leftLimbChains.size(); ++i) {
        extractJointsFromBoneNodeChain(m_boneNodeChain[m_leftLimbChains[i]], &m_leftLimbJoints[i]);
    }
    m_rightLimbJoints.resize(m_rightLimbChains.size());
    for (size_t i = 0; i < m_rightLimbChains.size(); ++i) {
        extractJointsFromBoneNodeChain(m_boneNodeChain[m_rightLimbChains[i]], &m_rightLimbJoints[i]);
    }
}

void RigGenerator::extractSpineJoints()
{
    auto findTail = m_branchNodesMapByMark.find((int)BoneMark::Tail);
    if (findTail != m_branchNodesMapByMark.end()) {
        m_spineJoints.push_back(findTail->second[0]);
    }
    
    if (!m_attachLimbsToSpineNodeIndices.empty() &&
            !m_spineChains.empty()) {
        const auto &fromNodeIndex = findTail == m_branchNodesMapByMark.end() ?
            m_attachLimbsToSpineNodeIndices[0] : findTail->second[0];
        std::vector<size_t> joints;
        extractJoints(fromNodeIndex,
            m_boneNodeChain[m_spineChains[0]].nodeIndices,
            &joints,
            false);
        m_attachLimbsToSpineJointIndices.resize(m_attachLimbsToSpineNodeIndices.size());
        for (const auto &nodeIndex: joints) {
            for (size_t i = 0; i < m_attachLimbsToSpineNodeIndices.size(); ++i) {
                if (m_attachLimbsToSpineNodeIndices[i] == nodeIndex) {
                    m_attachLimbsToSpineJointIndices[i] = m_spineJoints.size();
                }
            }
            m_spineJoints.push_back(nodeIndex);
        }
    }
    
    auto findNeck = m_branchNodesMapByMark.find((int)BoneMark::Neck);
    if (findNeck != m_branchNodesMapByMark.end()) {
        m_spineJoints.push_back(findNeck->second[0]);
    }
}

void RigGenerator::splitByNodeIndex(size_t nodeIndex,
        std::unordered_set<size_t> *left,
        std::unordered_set<size_t> *right)
{
    const auto &neighbors = m_neighborMap[nodeIndex];
    if (2 != neighbors.size()) {
        return;
    }
    {
        std::unordered_set<size_t> visited;
        visited.insert(*neighbors.begin());
        collectNodes(nodeIndex, left, &visited);
        left->erase(nodeIndex);
    }
    {
        std::unordered_set<size_t> visited;
        visited.insert(*(++neighbors.begin()));
        collectNodes(nodeIndex, right, &visited);
        right->erase(nodeIndex);
    }
}

void RigGenerator::collectNodes(size_t fromNodeIndex,
        std::unordered_set<size_t> *container,
        std::unordered_set<size_t> *visited)
{
    std::queue<size_t> waitQueue;
    waitQueue.push(fromNodeIndex);
    while (!waitQueue.empty()) {
        auto nodeIndex = waitQueue.front();
        waitQueue.pop();
        if (visited->find(nodeIndex) != visited->end())
            continue;
        visited->insert(nodeIndex);
        container->insert(nodeIndex);
        for (const auto &neighborNodeIndex: m_neighborMap[nodeIndex]) {
            if (visited->find(neighborNodeIndex) != visited->end())
                continue;
            waitQueue.push(neighborNodeIndex);
        }
    }
}

void RigGenerator::collectNodesForBoneRecursively(size_t fromNodeIndex,
        const std::unordered_set<size_t> *limitedNodeIndices,
        std::vector<std::vector<size_t>> *boneNodeIndices,
        size_t depth,
        std::unordered_set<size_t> *visited)
{
    std::vector<size_t> nodeIndices;
    for (const auto &nodeIndex: m_neighborMap[fromNodeIndex]) {
        if (limitedNodeIndices->find(nodeIndex) == limitedNodeIndices->end())
            continue;
        if (visited->find(nodeIndex) != visited->end())
            continue;
        visited->insert(nodeIndex);
        if (depth >= boneNodeIndices->size())
            boneNodeIndices->resize(depth + 1);
        (*boneNodeIndices)[depth].push_back(nodeIndex);
        nodeIndices.push_back(nodeIndex);
    }
    
    for (const auto &nodeIndex: nodeIndices) {
        collectNodesForBoneRecursively(nodeIndex,
            limitedNodeIndices,
            boneNodeIndices,
            depth + 1,
            visited);
    }
}

void RigGenerator::buildDemoMesh()
{
    // Blend vertices colors according to bone weights
    
    std::vector<QColor> inputVerticesColors(m_object->vertices.size(), Qt::black);
    if (m_isSuccessful) {
        const auto &resultWeights = *m_resultWeights;
        const auto &resultBones = *m_resultBones;
        
        m_resultWeights = new std::map<int, RigVertexWeights>;
        *m_resultWeights = resultWeights;
        
        m_resultBones = new std::vector<RigBone>;
        *m_resultBones = resultBones;
        
        for (const auto &weightItem: resultWeights) {
            size_t vertexIndex = weightItem.first;
            const auto &weight = weightItem.second;
            int blendR = 0, blendG = 0, blendB = 0;
            for (int i = 0; i < MAX_WEIGHT_NUM; i++) {
                int boneIndex = weight.boneIndices[i];
				const auto &bone = resultBones[boneIndex];
				blendR += bone.color.red() * weight.boneWeights[i];
				blendG += bone.color.green() * weight.boneWeights[i];
				blendB += bone.color.blue() * weight.boneWeights[i];
            }
            QColor blendColor = QColor(blendR, blendG, blendB, 255);
            inputVerticesColors[vertexIndex] = blendColor;
        }
    }
    
    // Create mesh for demo
    
    const std::vector<QVector3D> *triangleTangents = m_object->triangleTangents();
    const auto &inputVerticesPositions = m_object->vertices;
    const std::vector<std::vector<QVector3D>> *triangleVertexNormals = m_object->triangleVertexNormals();
    
    ShaderVertex *triangleVertices = nullptr;
    int triangleVerticesNum = 0;
    if (m_isSuccessful) {
        triangleVertices = new ShaderVertex[m_object->triangles.size() * 3];
        const QVector3D defaultUv = QVector3D(0, 0, 0);
        const QVector3D defaultTangents = QVector3D(0, 0, 0);
        for (size_t triangleIndex = 0; triangleIndex < m_object->triangles.size(); triangleIndex++) {
            const auto &sourceTriangle = m_object->triangles[triangleIndex];
            const auto *sourceTangent = &defaultTangents;
            if (nullptr != triangleTangents)
                sourceTangent = &(*triangleTangents)[triangleIndex];
            for (int i = 0; i < 3; i++) {
                ShaderVertex &currentVertex = triangleVertices[triangleVerticesNum++];
                const auto &sourcePosition = inputVerticesPositions[sourceTriangle[i]];
                const auto &sourceColor = inputVerticesColors[sourceTriangle[i]];
                const auto *sourceNormal = &defaultUv;
                if (nullptr != triangleVertexNormals)
                    sourceNormal = &(*triangleVertexNormals)[triangleIndex][i];
                currentVertex.posX = sourcePosition.x();
                currentVertex.posY = sourcePosition.y();
                currentVertex.posZ = sourcePosition.z();
                currentVertex.texU = 0;
                currentVertex.texV = 0;
                currentVertex.colorR = sourceColor.redF();
                currentVertex.colorG = sourceColor.greenF();
                currentVertex.colorB = sourceColor.blueF();
                currentVertex.normX = sourceNormal->x();
                currentVertex.normY = sourceNormal->y();
                currentVertex.normZ = sourceNormal->z();
                currentVertex.metalness = Model::m_defaultMetalness;
                currentVertex.roughness = Model::m_defaultRoughness;
                currentVertex.tangentX = sourceTangent->x();
                currentVertex.tangentY = sourceTangent->y();
                currentVertex.tangentZ = sourceTangent->z();
            }
        }
    }
    
    // Create bone bounding box for demo
    
    ShaderVertex *edgeVertices = nullptr;
    int edgeVerticesNum = 0;
    
    if (m_isSuccessful) {
        const auto &resultBones = *m_resultBones;
        std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> boxes;
        for (const auto &bone: resultBones) {
            if (/*bone.name.startsWith("Virtual") || */bone.name.startsWith("Body"))
                continue;
            boxes.push_back(std::make_tuple(bone.headPosition, bone.tailPosition,
                bone.headRadius, bone.tailRadius, bone.color));
        }
        edgeVertices = buildBoundingBoxMeshEdges(boxes, &edgeVerticesNum);
    }
    
    if (nullptr != m_debugEdgeVertices) {
        delete[] edgeVertices;
        edgeVertices = m_debugEdgeVertices;
        m_debugEdgeVertices = nullptr;
        
        edgeVerticesNum = m_debugEdgeVerticesNum;
        m_debugEdgeVerticesNum = 0;
    }
    
    m_resultMesh = new Model(triangleVertices, triangleVerticesNum,
        edgeVertices, edgeVerticesNum);
}

void RigGenerator::generate()
{
    buildNeighborMap();
    buildBoneNodeChain();
    buildSkeleton();
    computeSkinWeights();
    buildDemoMesh();
}

void RigGenerator::process()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    generate();
    
    qDebug() << "The rig generation took" << countTimeConsumed.elapsed() << "milliseconds";
    
    emit finished();
}
