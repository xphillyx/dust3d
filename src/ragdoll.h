#ifndef DUST3D_RAGDOLL_H
#define DUST3D_RAGDOLL_H
#include <QObject>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletDynamics/ConstraintSolver/btTypedConstraint.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btScalar.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btStaticPlaneShape.h>
#include <QString>
#include <map>
#include <QStringList>
#include "rigger.h"
#include "jointnodetree.h"

class RagDoll : public QObject
{
    Q_OBJECT
public:
    RagDoll(const std::vector<RiggerBone> *rigBones);
    ~RagDoll();
    bool stepSimulation(float amount);
    const JointNodeTree &getStepJointNodeTree();

private:
    btDefaultCollisionConfiguration *m_collisionConfiguration = nullptr;
    btCollisionDispatcher *m_collisionDispather = nullptr;
    btDbvtBroadphase *m_broadphase = nullptr;
    btSequentialImpulseConstraintSolver *m_constraintSolver = nullptr;
    btDynamicsWorld *m_world = nullptr;
    btCollisionShape *m_groundShape = nullptr;
    btRigidBody *m_groundBody = nullptr;
    float m_groundY = 0;
    
    std::map<QString, btCollisionShape *> m_boneShapes;
    std::map<QString, btRigidBody *> m_boneBodies;
    std::vector<btTypedConstraint *> m_boneConstraints;
    std::map<QString, QVector3D> m_boneLastPositions;
    
    std::map<QString, QVector3D> m_boneMiddleMap;
    std::map<QString, float> m_boneLengthMap;
    std::vector<btTransform> m_boneInitialTransforms;
    
    JointNodeTree m_jointNodeTree;
    JointNodeTree m_setpJointNodeTree;
    std::vector<RiggerBone> m_bones;
    
    QStringList m_bulletCodeList;
 
    btRigidBody *createRigidBody(btScalar mass, const btTransform &startTransform, btCollisionShape *shape);
    void createDynamicsWorld();
    void addConstraint(const RiggerBone &parent, const RiggerBone &child);
};

#endif
