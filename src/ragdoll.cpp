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

RagDoll::RagDoll(const std::vector<RiggerBone> *rigBones, const JointNodeTree &jointNodeTree)
{
    if (nullptr == rigBones)
        return;
    
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
        auto eulerAngles = rotation.toEulerAngles();
        transform.getBasis().setEulerZYX(
            qDegreesToRadians(eulerAngles.x()),
            qDegreesToRadians(eulerAngles.y()),
            qDegreesToRadians(eulerAngles.z())
        );
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
    // Remove all constraints
    //for (i = 0; i < JOINT_COUNT; ++i)
    //{
    //    m_ownerWorld->removeConstraint(m_joints[i]);
    //    delete m_joints[i];
    //    m_joints[i] = 0;
    //}

    // Remove all bodies and shapes
    for (auto &body: m_boneBodies) {
        m_ownerWorld->removeRigidBody(body.second);
        delete body.second->getMotionState();
        delete body.second;
    }
    m_boneBodies.clear();
    for (auto &shape: m_boneShapes) {
        delete shape.second;
    }
    m_boneShapes.clear();
    
    delete m_ownerWorld;
    
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
    
    m_ownerWorld = new btDiscreteDynamicsWorld(m_collisionDispather, m_broadphase, m_constraintSolver, m_collisionConfiguration);
    m_ownerWorld->setGravity(btVector3(0, -100, 0));
}

/*
RagDoll::RagDoll(btDynamicsWorld* ownerWorld, const btVector3& positionOffset, btScalar scale)
    : m_ownerWorld(ownerWorld)
{
    // Setup the geometry
    m_shapes[BODYPART_PELVIS] = new btCapsuleShape(btScalar(0.15) * scale, btScalar(0.20) * scale);
    m_shapes[BODYPART_SPINE] = new btCapsuleShape(btScalar(0.15) * scale, btScalar(0.28) * scale);
    m_shapes[BODYPART_HEAD] = new btCapsuleShape(btScalar(0.10) * scale, btScalar(0.05) * scale);
    m_shapes[BODYPART_LEFT_UPPER_LEG] = new btCapsuleShape(btScalar(0.07) * scale, btScalar(0.45) * scale);
    m_shapes[BODYPART_LEFT_LOWER_LEG] = new btCapsuleShape(btScalar(0.05) * scale, btScalar(0.37) * scale);
    m_shapes[BODYPART_RIGHT_UPPER_LEG] = new btCapsuleShape(btScalar(0.07) * scale, btScalar(0.45) * scale);
    m_shapes[BODYPART_RIGHT_LOWER_LEG] = new btCapsuleShape(btScalar(0.05) * scale, btScalar(0.37) * scale);
    m_shapes[BODYPART_LEFT_UPPER_ARM] = new btCapsuleShape(btScalar(0.05) * scale, btScalar(0.33) * scale);
    m_shapes[BODYPART_LEFT_LOWER_ARM] = new btCapsuleShape(btScalar(0.04) * scale, btScalar(0.25) * scale);
    m_shapes[BODYPART_RIGHT_UPPER_ARM] = new btCapsuleShape(btScalar(0.05) * scale, btScalar(0.33) * scale);
    m_shapes[BODYPART_RIGHT_LOWER_ARM] = new btCapsuleShape(btScalar(0.04) * scale, btScalar(0.25) * scale);

    // Setup all the rigid bodies
    btTransform offset;
    offset.setIdentity();
    offset.setOrigin(positionOffset);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.), btScalar(1.), btScalar(0.)));
    m_bodies[BODYPART_PELVIS] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_PELVIS]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.), btScalar(1.2), btScalar(0.)));
    m_bodies[BODYPART_SPINE] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_SPINE]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.), btScalar(1.6), btScalar(0.)));
    m_bodies[BODYPART_HEAD] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_HEAD]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(-0.18), btScalar(0.65), btScalar(0.)));
    m_bodies[BODYPART_LEFT_UPPER_LEG] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_LEFT_UPPER_LEG]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(-0.18), btScalar(0.2), btScalar(0.)));
    m_bodies[BODYPART_LEFT_LOWER_LEG] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_LEFT_LOWER_LEG]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.18), btScalar(0.65), btScalar(0.)));
    m_bodies[BODYPART_RIGHT_UPPER_LEG] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_RIGHT_UPPER_LEG]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.18), btScalar(0.2), btScalar(0.)));
    m_bodies[BODYPART_RIGHT_LOWER_LEG] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_RIGHT_LOWER_LEG]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(-0.35), btScalar(1.45), btScalar(0.)));
    transform.getBasis().setEulerZYX(0, 0, M_PI_2);
    m_bodies[BODYPART_LEFT_UPPER_ARM] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_LEFT_UPPER_ARM]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(-0.7), btScalar(1.45), btScalar(0.)));
    transform.getBasis().setEulerZYX(0, 0, M_PI_2);
    m_bodies[BODYPART_LEFT_LOWER_ARM] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_LEFT_LOWER_ARM]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.35), btScalar(1.45), btScalar(0.)));
    transform.getBasis().setEulerZYX(0, 0, -M_PI_2);
    m_bodies[BODYPART_RIGHT_UPPER_ARM] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_RIGHT_UPPER_ARM]);

    transform.setIdentity();
    transform.setOrigin(scale * btVector3(btScalar(0.7), btScalar(1.45), btScalar(0.)));
    transform.getBasis().setEulerZYX(0, 0, -M_PI_2);
    m_bodies[BODYPART_RIGHT_LOWER_ARM] = createRigidBody(btScalar(1.), offset * transform, m_shapes[BODYPART_RIGHT_LOWER_ARM]);

    // Setup some damping on the m_bodies
    for (int i = 0; i < BODYPART_COUNT; ++i)
    {
        m_bodies[i]->setDamping(btScalar(0.05), btScalar(0.85));
        m_bodies[i]->setDeactivationTime(btScalar(0.8));
        m_bodies[i]->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
    }

    // Now setup the constraints
    btHingeConstraint *hingeC;
    btConeTwistConstraint *coneC;

    btTransform localA, localB;

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, M_PI_2, 0);
    localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.15), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, M_PI_2, 0);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.15), btScalar(0.)));
    hingeC = new btHingeConstraint(*m_bodies[BODYPART_PELVIS], *m_bodies[BODYPART_SPINE], localA, localB);
    hingeC->setLimit(btScalar(-M_PI_4), btScalar(M_PI_2));
    m_joints[JOINT_PELVIS_SPINE] = hingeC;
    m_ownerWorld->addConstraint(m_joints[JOINT_PELVIS_SPINE], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, 0, M_PI_2);
    localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.30), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, 0, M_PI_2);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.14), btScalar(0.)));
    coneC = new btConeTwistConstraint(*m_bodies[BODYPART_SPINE], *m_bodies[BODYPART_HEAD], localA, localB);
    coneC->setLimit(M_PI_4, M_PI_4, M_PI_2);
    m_joints[JOINT_SPINE_HEAD] = coneC;
    m_ownerWorld->addConstraint(m_joints[JOINT_SPINE_HEAD], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, 0, -M_PI_4 * 5);
    localA.setOrigin(scale * btVector3(btScalar(-0.18), btScalar(-0.10), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, 0, -M_PI_4 * 5);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.225), btScalar(0.)));
    coneC = new btConeTwistConstraint(*m_bodies[BODYPART_PELVIS], *m_bodies[BODYPART_LEFT_UPPER_LEG], localA, localB);
    coneC->setLimit(M_PI_4, M_PI_4, 0);
    m_joints[JOINT_LEFT_HIP] = coneC;
    m_ownerWorld->addConstraint(m_joints[JOINT_LEFT_HIP], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, M_PI_2, 0);
    localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.225), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, M_PI_2, 0);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.185), btScalar(0.)));
    hingeC = new btHingeConstraint(*m_bodies[BODYPART_LEFT_UPPER_LEG], *m_bodies[BODYPART_LEFT_LOWER_LEG], localA, localB);
    hingeC->setLimit(btScalar(0), btScalar(M_PI_2));
    m_joints[JOINT_LEFT_KNEE] = hingeC;
    m_ownerWorld->addConstraint(m_joints[JOINT_LEFT_KNEE], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, 0, M_PI_4);
    localA.setOrigin(scale * btVector3(btScalar(0.18), btScalar(-0.10), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, 0, M_PI_4);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.225), btScalar(0.)));
    coneC = new btConeTwistConstraint(*m_bodies[BODYPART_PELVIS], *m_bodies[BODYPART_RIGHT_UPPER_LEG], localA, localB);
    coneC->setLimit(M_PI_4, M_PI_4, 0);
    m_joints[JOINT_RIGHT_HIP] = coneC;
    m_ownerWorld->addConstraint(m_joints[JOINT_RIGHT_HIP], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, M_PI_2, 0);
    localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.225), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, M_PI_2, 0);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.185), btScalar(0.)));
    hingeC = new btHingeConstraint(*m_bodies[BODYPART_RIGHT_UPPER_LEG], *m_bodies[BODYPART_RIGHT_LOWER_LEG], localA, localB);
    hingeC->setLimit(btScalar(0), btScalar(M_PI_2));
    m_joints[JOINT_RIGHT_KNEE] = hingeC;
    m_ownerWorld->addConstraint(m_joints[JOINT_RIGHT_KNEE], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, 0, M_PI);
    localA.setOrigin(scale * btVector3(btScalar(-0.2), btScalar(0.15), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, 0, M_PI_2);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.18), btScalar(0.)));
    coneC = new btConeTwistConstraint(*m_bodies[BODYPART_SPINE], *m_bodies[BODYPART_LEFT_UPPER_ARM], localA, localB);
    coneC->setLimit(M_PI_2, M_PI_2, 0);
    m_joints[JOINT_LEFT_SHOULDER] = coneC;
    m_ownerWorld->addConstraint(m_joints[JOINT_LEFT_SHOULDER], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, M_PI_2, 0);
    localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.18), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, M_PI_2, 0);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.14), btScalar(0.)));
    hingeC = new btHingeConstraint(*m_bodies[BODYPART_LEFT_UPPER_ARM], *m_bodies[BODYPART_LEFT_LOWER_ARM], localA, localB);
    hingeC->setLimit(btScalar(-M_PI_2), btScalar(0));
    m_joints[JOINT_LEFT_ELBOW] = hingeC;
    m_ownerWorld->addConstraint(m_joints[JOINT_LEFT_ELBOW], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, 0, 0);
    localA.setOrigin(scale * btVector3(btScalar(0.2), btScalar(0.15), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, 0, M_PI_2);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.18), btScalar(0.)));
    coneC = new btConeTwistConstraint(*m_bodies[BODYPART_SPINE], *m_bodies[BODYPART_RIGHT_UPPER_ARM], localA, localB);
    coneC->setLimit(M_PI_2, M_PI_2, 0);
    m_joints[JOINT_RIGHT_SHOULDER] = coneC;
    m_ownerWorld->addConstraint(m_joints[JOINT_RIGHT_SHOULDER], true);

    localA.setIdentity();
    localB.setIdentity();
    localA.getBasis().setEulerZYX(0, M_PI_2, 0);
    localA.setOrigin(scale * btVector3(btScalar(0.), btScalar(0.18), btScalar(0.)));
    localB.getBasis().setEulerZYX(0, M_PI_2, 0);
    localB.setOrigin(scale * btVector3(btScalar(0.), btScalar(-0.14), btScalar(0.)));
    hingeC = new btHingeConstraint(*m_bodies[BODYPART_RIGHT_UPPER_ARM], *m_bodies[BODYPART_RIGHT_LOWER_ARM], localA, localB);
    hingeC->setLimit(btScalar(-M_PI_2), btScalar(0));
    m_joints[JOINT_RIGHT_ELBOW] = hingeC;
    m_ownerWorld->addConstraint(m_joints[JOINT_RIGHT_ELBOW], true);
}

RagDoll::~RagDoll()
{
    int i;

    // Remove all constraints
    for (i = 0; i < JOINT_COUNT; ++i)
    {
        m_ownerWorld->removeConstraint(m_joints[i]);
        delete m_joints[i];
        m_joints[i] = 0;
    }

    // Remove all bodies and shapes
    for (i = 0; i < BODYPART_COUNT; ++i)
    {
        m_ownerWorld->removeRigidBody(m_bodies[i]);

        delete m_bodies[i]->getMotionState();

        delete m_bodies[i];
        m_bodies[i] = 0;
        delete m_shapes[i];
        m_shapes[i] = 0;
    }
}
*/

btRigidBody *RagDoll::createRigidBody(btScalar mass, const btTransform &startTransform, btCollisionShape *shape)
{
    bool isDynamic = (mass != 0.f);

    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        shape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);

    m_ownerWorld->addRigidBody(body);

    return body;
}
