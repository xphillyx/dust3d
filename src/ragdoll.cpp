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
#include "ragdoll.h"
#include "poser.h"

RagDoll::RagDoll(const std::vector<RiggerBone> *rigBones) :
    m_jointNodeTree(rigBones),
    m_setpJointNodeTree(rigBones)
{
    if (nullptr == rigBones)
        return;
    
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
        boneNames.push_back(bone.name);
    }
    Poser::fetchChains(boneNames, chains);
    
    // Setup the geometry
    for (const auto &bone: *rigBones) {
        float radius = (bone.headRadius + bone.tailRadius) * 0.5;
        float height = bone.headPosition.distanceToPoint(bone.tailPosition);
        m_boneShapes[bone.name] = new btCapsuleShape(btScalar(radius), btScalar(height));
        m_boneShapes[bone.name]->setUserIndex(bone.index);
    }
    
    // Setup all the rigid bodies
    for (const auto &bone: *rigBones) {
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(
            btScalar(bone.headPosition.x()),
            btScalar(bone.headPosition.y()),
            btScalar(bone.headPosition.z())
        ));
        QVector3D to = (bone.tailPosition - bone.headPosition).normalized();
        QVector3D from = QVector3D(0, 1, 0);
        QQuaternion rotation = QQuaternion::rotationTo(from, to);
        btQuaternion btRotation(btScalar(rotation.x()), btScalar(rotation.y()), btScalar(rotation.z()),
            btScalar(rotation.scalar()));
        transform.getBasis().setRotation(btRotation);
        m_boneBodies[bone.name] = createRigidBody(btScalar(1.), transform, m_boneShapes[bone.name]);
    }
    
    // Setup some damping on the m_bodies
    for (const auto &bone: *rigBones) {
        m_boneBodies[bone.name]->setDamping(btScalar(0.05), btScalar(0.85));
        m_boneBodies[bone.name]->setDeactivationTime(btScalar(0.8));
        m_boneBodies[bone.name]->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
    }
}

RagDoll::~RagDoll()
{
    // Remove all bodies and shapes
    for (auto &body: m_boneBodies) {
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
    m_world->setGravity(btVector3(0, -100, 0));
    
    m_groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));
    
    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, m_groundY, 0));

    m_groundBody = createRigidBody(btScalar(0.), groundTransform, m_groundShape);
}

bool RagDoll::stepSimulation(float amount)
{
    bool positionChanged = false;
    m_world->stepSimulation(btScalar(amount));
    
    m_setpJointNodeTree = m_jointNodeTree;
    for (const auto &it: m_boneShapes) {
        const auto *body = m_boneBodies[it.first];
        int jointIndex = it.second->getUserIndex();
        btTransform btWorldTransform;
        if (body->getMotionState())
            body->getMotionState()->getWorldTransform(btWorldTransform);
        else
            btWorldTransform = body->getWorldTransform();
        const auto &btOrigin = btWorldTransform.getOrigin();
        QVector3D position = QVector3D(btOrigin.x(), btOrigin.y(), btOrigin.z());
        if (!qFuzzyCompare(position, m_boneLastPositions[it.first])) {
            positionChanged = true;
            m_boneLastPositions[it.first] = position;
        }
        /*
        m_setpJointNodeTree.updateTranslation(jointIndex, position);
        btQuaternion btRotation = btWorldTransform.getRotation();
        QQuaternion rotation;
        rotation.setX(btRotation.getX());
        rotation.setY(btRotation.getY());
        rotation.setZ(btRotation.getZ());
        rotation.setScalar(btRotation.getW());
        m_setpJointNodeTree.updateRotation(jointIndex, rotation);
        */
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

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);

    m_world->addRigidBody(body);

    return body;
}
