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
#include <QString>
#include <map>
#include "rigger.h"
#include "jointnodetree.h"

class RagDoll : public QObject
{
    Q_OBJECT
public:
    /*
    enum
	{
		BODYPART_PELVIS = 0,
		BODYPART_SPINE,
		BODYPART_HEAD,

		BODYPART_LEFT_UPPER_LEG,
		BODYPART_LEFT_LOWER_LEG,

		BODYPART_RIGHT_UPPER_LEG,
		BODYPART_RIGHT_LOWER_LEG,

		BODYPART_LEFT_UPPER_ARM,
		BODYPART_LEFT_LOWER_ARM,

		BODYPART_RIGHT_UPPER_ARM,
		BODYPART_RIGHT_LOWER_ARM,

		BODYPART_COUNT
	};

	enum
	{
		JOINT_PELVIS_SPINE = 0,
		JOINT_SPINE_HEAD,

		JOINT_LEFT_HIP,
		JOINT_LEFT_KNEE,

		JOINT_RIGHT_HIP,
		JOINT_RIGHT_KNEE,

		JOINT_LEFT_SHOULDER,
		JOINT_LEFT_ELBOW,

		JOINT_RIGHT_SHOULDER,
		JOINT_RIGHT_ELBOW,

		JOINT_COUNT
	};
    */
    //RagDoll(btDynamicsWorld* ownerWorld, const btVector3& positionOffset, btScalar scale);
    RagDoll(const std::vector<RiggerBone> *rigBones, const JointNodeTree &jointNodeTree);
    ~RagDoll();

private:
	//btCollisionShape *m_shapes[BODYPART_COUNT] = {nullptr};
	//btRigidBody *m_bodies[BODYPART_COUNT] = {nullptr};
	//btTypedConstraint *m_joints[JOINT_COUNT] = {nullptr};
 
    btDefaultCollisionConfiguration *m_collisionConfiguration = nullptr;
    btCollisionDispatcher *m_collisionDispather = nullptr;
    btDbvtBroadphase *m_broadphase = nullptr;
    btSequentialImpulseConstraintSolver *m_constraintSolver = nullptr;
    btDynamicsWorld *m_ownerWorld = nullptr;
    
    std::map<QString, btCollisionShape *> m_boneShapes;
    std::map<QString, btRigidBody *> m_boneBodies;
 
    btRigidBody *createRigidBody(btScalar mass, const btTransform &startTransform, btCollisionShape *shape);
    void createDynamicsWorld();
};

#endif
