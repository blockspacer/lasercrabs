#pragma once

#include "data/entity.h"
#include "lmath.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"

namespace VI
{

struct RigidBody;

#define WALKER_SUPPORT_HEIGHT 0.45f
#define WALKER_HEIGHT 0.85f
#define WALKER_PARKOUR_RADIUS 0.45f
#define WALKER_MINION_RADIUS 0.35f
#define WALKER_TRACTION_DOT 0.7f

struct Walker : public ComponentType<Walker>
{
	static Vec3 get_support_velocity(const Vec3&, const btCollisionObject*);

	Vec2 dir;
	r32 speed,
		max_speed,
		rotation,
		target_rotation,
		net_speed,
		net_speed_timer;
	Ref<RigidBody> support;
	LinkArg<r32> land;
	b8 auto_rotate;
	b8 enabled;

	Walker(r32 = 0.0f);
	void awake();
	b8 slide(Vec2*, const Vec3&);
	btCollisionWorld::ClosestRayResultCallback check_support(r32 = 0.0f) const;
	RigidBody* get_support(r32 = 0.0f) const;

	Vec3 base_pos() const;
	void absolute_pos(const Vec3&);
	Vec3 forward() const;
	Vec3 right() const;
	r32 radius() const;
	r32 capsule_height() const;
	r32 default_capsule_height() const;
	void crouch(b8);

	void update_server(const Update&);
};

}
