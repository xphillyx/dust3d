#define _USE_MATH_DEFINES
#include <cmath>
#include <LinearMath/btDefaultMotionState.h>
#include <LinearMath/btAlignedAllocator.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletDynamics/ConstraintSolver/btHingeConstraint.h>
#include <BulletDynamics/ConstraintSolver/btConeTwistConstraint.h>
#include <BulletDynamics/ConstraintSolver/btTypedConstraint.h>
#include <QQuaternion>
#include <QtMath>
#include <QMatrix4x4>
#include "ragdoll.h"
#include "poser.h"

RagDoll::RagDoll(const std::vector<RiggerBone> *rigBones) :
    m_jointNodeTree(rigBones),
    m_setpJointNodeTree(rigBones)
{
    if (nullptr == rigBones || rigBones->empty())
        return;
    
    m_bones = *rigBones;
    
    for (const auto &bone: *rigBones) {
        float groundY = bone.headPosition.y() - bone.headRadius;
        if (groundY < m_groundY)
            m_groundY = groundY;
        groundY = bone.tailPosition.y() - bone.tailRadius;
        if (groundY < m_groundY)
            m_groundY = groundY;
    }
    
    createDynamicsWorld();
    
    std::map<QString, std::vector<QString>> chains;
    std::vector<QString> boneNames;
    for (const auto &bone: *rigBones) {
        m_boneNameToIndexMap[bone.name] = bone.index;
        boneNames.push_back(bone.name);
    }
    Poser::fetchChains(boneNames, chains);
    
    //printf("//////////////// bones ///////////////\r\n");
    for (const auto &bone: *rigBones) {
        float radius = (bone.headRadius + bone.tailRadius) * 0.5;
        float height = bone.headPosition.distanceToPoint(bone.tailPosition);
        QVector3D middlePosition = (bone.headPosition + bone.tailPosition) * 0.5;
        m_boneLengthMap[bone.name] = height;
        m_boneRadiusMap[bone.name] = radius;
        m_boneMiddleMap[bone.name] = middlePosition;
        printf("[%s] radius:%f height:%f head:(%f,%f,%f) tail:(%f,%f,%f)\r\n",
            bone.name.toUtf8().constData(),
            radius, height,
            bone.headPosition.x(), bone.headPosition.y(), bone.headPosition.z(),
            bone.tailPosition.x(), bone.tailPosition.y(), bone.tailPosition.z());
        for (const auto &childIndex: bone.children) {
            const auto &child = m_bones[childIndex];
            printf("    child:%s\r\n", child.name.toUtf8().constData());
        }
    }
    //printf("///////////////////////////////////////\r\n");
    
    // Setup some damping on the m_bodies
    //for (const auto &bone: *rigBones) {
    //    m_boneBodies[bone.name]->setDamping(btScalar(0.05), btScalar(0.85));
    //    m_boneBodies[bone.name]->setDeactivationTime(btScalar(0.8));
    //    m_boneBodies[bone.name]->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
    //}
    
    for (const auto &it: chains) {
        if (Rigger::rootBoneName == it.first)
            continue;
        printf("Chain:%s\r\n", it.first.toUtf8().constData());
        float mass = 1.0;
        for (size_t i = 0; i < it.second.size(); ++i) {
            const auto &name = it.second[i];
            printf("    child:%s\r\n", name.toUtf8().constData());
            const auto &bone = m_bones[m_boneNameToIndexMap[name]];
            float height = m_boneLengthMap[bone.name];
            float radius = m_boneRadiusMap[bone.name];

            btCollisionShape *shape = new btCapsuleShape(btScalar(radius), btScalar(height));
            shape->setUserIndex(bone.index);
            
            m_boneShapes[bone.name] = shape;
            
            btTransform transform;
            const auto &middlePosition = m_boneMiddleMap[bone.name];
            transform.setIdentity();
            transform.setOrigin(btVector3(
                btScalar(middlePosition.x()),
                btScalar(middlePosition.y()),
                btScalar(middlePosition.z())
            ));
            QVector3D to = (bone.tailPosition - bone.headPosition).normalized();
            QVector3D from = QVector3D(0, 1, 0);
            QQuaternion rotation = QQuaternion::rotationTo(from, to);
            btQuaternion btRotation(btScalar(rotation.x()), btScalar(rotation.y()), btScalar(rotation.z()),
                btScalar(rotation.scalar()));
            transform.getBasis().setRotation(btRotation);
            
            btRigidBody *body = createRigidBody(btScalar(mass), transform, shape);
            m_boneBodies[bone.name] = body;
        }
    }
    
    for (const auto &it: chains) {
        //if (Rigger::rootBoneName == it.first || it.first.startsWith("Spine"))
        //    continue;
        //if (it.second.empty())
        //    continue;
        //addConstraintWithSpine((*rigBones)[m_boneNameToIndexMap[it.second[0]]]);
        for (size_t i = 1; i < it.second.size(); ++i) {
            const auto &parent = (*rigBones)[m_boneNameToIndexMap[it.second[i - 1]]];
            const auto &child = (*rigBones)[m_boneNameToIndexMap[it.second[i]]];
            addChainConstraint(parent, child);
        }
    }
    
    /*
    std::map<QString, std::pair<int, int>> virtualLinkMap;
    for (const auto &bone: *rigBones) {
        if (bone.name.startsWith("Virtual_")) {
            const auto &parent = bone;
            for (const auto &childIndex: bone.children) {
                virtualLinkMap[parent.name] = std::make_pair(-1, childIndex);
            }
        }
    }
    
    for (const auto &bone: *rigBones) {
        const auto &parent = bone;
        for (const auto &childIndex: bone.children) {
            const auto &child = (*rigBones)[childIndex];
            if (child.name.startsWith("Virtual_")) {
                auto findLink = virtualLinkMap.find(child.name);
                if (findLink == virtualLinkMap.end())
                    continue;
                findLink->second.first = parent.index;
            }
        }
    }
    
    for (const auto &it: virtualLinkMap) {
        if (-1 == it.second.first || -1 == it.second.second)
            continue;
        addVirtualConstraint((*rigBones)[it.second.first], (*rigBones)[it.second.second]);
    }
    */
    
    for (const auto &it: chains) {
        const auto &name = it.second[0];
        const auto &leftBody = m_boneBodies.find(name);
        if (leftBody == m_boneBodies.end())
            continue;
        const QString left = "Left";
        if (name.startsWith(left)) {
            QString pairedName = "Right" + name.mid(left.length());
            const auto &rightBody = m_boneBodies.find(pairedName);
            if (rightBody == m_boneBodies.end())
                continue;
            leftBody->second->setIgnoreCollisionCheck(rightBody->second, true);
            rightBody->second->setIgnoreCollisionCheck(leftBody->second, true);
        }
    }
    
    //printf("//////////// Bullet code /////////////\r\n%s\r\n////////////////////////////////////\r\n", m_bulletCodeList.join("\r\n").toUtf8().constData());
}

void RagDoll::addConstraintWithSpine(const RiggerBone &child)
{
    btRigidBody *parentBoneBody = m_boneBodies["Spine"];
    btRigidBody *childBoneBody = m_boneBodies[child.name];
    float childLength = m_boneLengthMap[child.name];
    bool isLimb = child.name.startsWith("Limb");
    bool isTail = child.name.startsWith("Tail");
    
    if (nullptr == parentBoneBody || nullptr == childBoneBody)
        return;
    
    const btVector3 btPivotA(child.headPosition.x(), child.headPosition.y(), child.headPosition.z());
    const btVector3 btPivotB(0, -childLength * 0.5, 0.0f);
    
    btTransform localA;
    btTransform localB;
    
    btConeTwistConstraint *constraint = nullptr;
    std::tuple<float, float, float> limits = std::make_tuple(0, 0, 0);
    
    localA.setIdentity();
    localB.setIdentity();
    localA.setOrigin(btPivotA);
    localB.setOrigin(btPivotB);
    if (isLimb) {
        if (child.name.startsWith("Left")) {
            localA.getBasis().setEulerZYX(0, 0, 0);
            localB.getBasis().setEulerZYX(0, 0, M_PI_2);
        } else {
            localA.getBasis().setEulerZYX(0, 0, 0);
            localB.getBasis().setEulerZYX(0, 0, -M_PI_2);
        }
    } else if (isTail) {
        localA.getBasis().setEulerZYX(0, 0, 0);
        localB.getBasis().setEulerZYX(0, 0, -M_PI);
    }
    constraint = new btConeTwistConstraint(*parentBoneBody, *childBoneBody, localA, localB);
    constraint->setLimit(btScalar(std::get<0>(limits)), btScalar(std::get<1>(limits)), btScalar(std::get<2>(limits)));
    
    m_world->addConstraint(constraint, true);
    
    m_boneConstraints.push_back(constraint);
}

void RagDoll::addChainConstraint(const RiggerBone &parent, const RiggerBone &child)
{
    btRigidBody *parentBoneBody = m_boneBodies[parent.name];
    btRigidBody *childBoneBody = m_boneBodies[child.name];
    
    if (nullptr == parentBoneBody || nullptr == childBoneBody)
        return;
    
    float parentLength = m_boneLengthMap[parent.name];
    float childLength = m_boneLengthMap[child.name];
    const btVector3 btPivotA(0, parentLength * 0.5, 0.0f);
    const btVector3 btPivotB(0, -childLength * 0.5, 0.0f);
    bool isLimb = -1 != parent.name.indexOf("Limb") || -1 != child.name.indexOf("Limb");
    QVector3D axis = isLimb ? QVector3D(0, 0, 1) : QVector3D(1, 0, 0);
    
    QVector3D parentDirection = (parent.tailPosition - parent.headPosition).normalized();
    QVector3D childDirection = (child.tailPosition - child.headPosition).normalized();
    float degrees = angleInRangle360BetweenTwoVectors(parentDirection, childDirection, axis);
    if (degrees > 180)
        degrees = -(360 - degrees);
    float radian = qDegreesToRadians(degrees);
    
    btTransform localA;
    btTransform localB;
    
    btHingeConstraint *constraint = nullptr;
    std::pair<float, float> limits = std::make_pair(radian, radian);
    
    localA.setIdentity();
    localB.setIdentity();
    localA.setOrigin(btPivotA);
    localB.setOrigin(btPivotB);
    if (isLimb) {
        localA.getBasis().setEulerZYX(0, 0, 0);
        localB.getBasis().setEulerZYX(0, 0, 0);
        constraint = new btHingeConstraint(*parentBoneBody, *childBoneBody, localA, localB);
    } else {
        localA.getBasis().setEulerZYX(0, -M_PI_2, 0);
        localB.getBasis().setEulerZYX(0, -M_PI_2, 0);
        constraint = new btHingeConstraint(*parentBoneBody, *childBoneBody, localA, localB);
    }
    
    constraint->setLimit(btScalar(limits.first), btScalar(limits.second));
    
    m_world->addConstraint(constraint, true);
    
    m_boneConstraints.push_back(constraint);
}

RagDoll::~RagDoll()
{
    // Remove all bodies and shapes
    for (auto &constraint: m_boneConstraints) {
        delete constraint;
    }
    m_boneConstraints.clear();
    for (auto &body: m_boneBodies) {
        if (nullptr == body.second)
            continue;
        m_world->removeRigidBody(body.second);
        delete body.second->getMotionState();
        delete body.second;
    }
    m_boneBodies.clear();
    for (auto &shape: m_boneShapes) {
        delete shape.second;
    }
    m_boneShapes.clear();
    
    delete m_groundShape;
    delete m_groundBody;
    
    delete m_world;
    
    delete m_collisionConfiguration;
    delete m_collisionDispather;
    delete m_broadphase;
    delete m_constraintSolver;
}

void RagDoll::createDynamicsWorld()
{
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_collisionDispather = new btCollisionDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_constraintSolver = new btSequentialImpulseConstraintSolver();
    
    m_world = new btDiscreteDynamicsWorld(m_collisionDispather, m_broadphase, m_constraintSolver, m_collisionConfiguration);
    m_world->setGravity(btVector3(0, -10, 0));
    
    m_world->getSolverInfo().m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
    m_world->getSolverInfo().m_numIterations = 5;
    
    m_groundShape = new btBoxShape(btVector3(btScalar(250.), btScalar(0.), btScalar(250.)));

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, m_groundY - 0.5, 0));
    m_groundBody = createRigidBody(0, groundTransform, m_groundShape);
}

bool RagDoll::stepSimulation(float amount)
{
    bool positionChanged = false;
    m_world->stepSimulation(btScalar(amount));
    
    if (m_boneBodies.empty())
        return positionChanged;
    
    std::vector<std::pair<QVector3D, QVector3D>> newBonePositions(m_bones.size());
    for (size_t i = 0; i < newBonePositions.size(); ++i) {
        const auto &bone = m_bones[i];
        newBonePositions[i] = std::make_pair(bone.headPosition, bone.tailPosition);
    }
    m_setpJointNodeTree = m_jointNodeTree;
    
    auto updateToNewPosition = [&](const QString &boneName, const btTransform &btWorldTransform, int jointIndex) {
        float halfHeight = m_boneLengthMap[boneName] * 0.5;
        btVector3 oldBoneHead(btScalar(0.0f), btScalar(-halfHeight), btScalar(0.0f));
        btVector3 oldBoneTail(btScalar(0.0f), btScalar(halfHeight), btScalar(0.0f));
        btVector3 newBoneHead = btWorldTransform * oldBoneHead;
        btVector3 newBoneTail = btWorldTransform * oldBoneTail;
        //const auto &bone = m_bones[jointIndex];
        //printf("bone[%s] oldHead:(%f,%f,%f)\r\n", bone.name.toUtf8().constData(), bone.headPosition.x(), bone.headPosition.y(), bone.headPosition.z());
        //printf("bone[%s] newHead:(%f,%f,%f)\r\n", bone.name.toUtf8().constData(), newBoneHead.x(), newBoneHead.y(), newBoneHead.z());
        //printf("bone[%s] oldTail:(%f,%f,%f)\r\n", bone.name.toUtf8().constData(), bone.tailPosition.x(), bone.tailPosition.y(), bone.tailPosition.z());
        //printf("bone[%s] newTail:(%f,%f,%f)\r\n", bone.name.toUtf8().constData(), newBoneTail.x(), newBoneTail.y(), newBoneTail.z());
        newBonePositions[jointIndex] = std::make_pair(
            QVector3D(newBoneHead.x(), newBoneHead.y(), newBoneHead.z()),
            QVector3D(newBoneTail.x(), newBoneTail.y(), newBoneTail.z())
        );
    };
    for (const auto &it: m_boneShapes) {
        btTransform btWorldTransform;
        const auto *body = m_boneBodies[it.first];
        if (nullptr == body)
            continue;
        if (body->isActive()) {
            positionChanged = true;
        }
        if (body->getMotionState()) {
            body->getMotionState()->getWorldTransform(btWorldTransform);
        } else {
            btWorldTransform = body->getWorldTransform();
        }
        int jointIndex = it.second->getUserIndex();
        if (-1 == jointIndex)
            continue;
        printf("Update \"%s\" to new position\r\n", m_bones[jointIndex].name.toUtf8().constData());
        updateToNewPosition(it.first, btWorldTransform, jointIndex);
    }
    
    //QVector3D newRootBoneMiddle = (newBonePositions[0].first + newBonePositions[0].second) * 0.5;
    //QVector3D modelTranslation = newRootBoneMiddle - m_boneMiddleMap[Rigger::rootBoneName];
    //qDebug() << "modelTranslation:" << modelTranslation << "newRootBoneMiddle:" << newRootBoneMiddle << "oldRootBoneMiddle:" << m_boneMiddleMap[Rigger::rootBoneName];
    //m_setpJointNodeTree.addTranslation(0,
    //    QVector3D(0, modelTranslation.y(), 0));
    
    std::vector<QVector3D> directions(newBonePositions.size());
    for (size_t index = 0; index < newBonePositions.size(); ++index) {
        const auto &bone = m_bones[index];
        directions[index] = (bone.tailPosition - bone.headPosition).normalized();
    }
    std::function<void(size_t index, const QQuaternion &rotation)> rotateChildren;
    rotateChildren = [&](size_t index, const QQuaternion &rotation) {
        const auto &bone = m_bones[index];
        for (const auto &childIndex: bone.children) {
            directions[childIndex] = rotation.rotatedVector(directions[childIndex]);
            rotateChildren(childIndex, rotation);
        }
    };
    for (size_t index = 0; index < newBonePositions.size(); ++index) {
        QQuaternion rotation;
        const auto &oldDirection = directions[index];
        QVector3D newDirection = (newBonePositions[index].second - newBonePositions[index].first).normalized();
        rotation = QQuaternion::rotationTo(oldDirection, newDirection);
        //if (m_bones[index].name.startsWith("Spine") ||
        //        m_bones[index].name.startsWith("Tail") ||
        //        m_bones[index].name.startsWith("Neck"))
        m_setpJointNodeTree.updateRotation(index, rotation);
        rotateChildren(index, rotation);
    }
    
    m_setpJointNodeTree.recalculateTransformMatrices();
    return positionChanged;
}

const JointNodeTree &RagDoll::getStepJointNodeTree()
{
    return m_setpJointNodeTree;
}

btRigidBody *RagDoll::createRigidBody(btScalar mass, const btTransform &startTransform, btCollisionShape *shape)
{
    bool isDynamic = !qFuzzyIsNull(mass);

    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        shape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    btRigidBody *body = new btRigidBody(rbInfo);
    body->setFriction(1.0f);
    body->setRollingFriction(1.0f);
    body->setSpinningFriction(1.0f);

    m_world->addRigidBody(body);

    return body;
}
