#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Game/Framework/GameCommon.hpp"

class Game;
struct SimpleMinerRaycast;

enum class MovementMode {
	WALKING,
	FLYING,
	NOCLIP,
	NUM_MOVEMENT_MODES
};

class Entity {
public:
	Entity(Game* pointerToGame, Vec3 const& startingPosition, Vec3 const& startingVelocity, EulerAngles const& startingOrientation);
	~Entity();

	virtual void MoveInDirection(Vec3 const& direction, float speed);
	virtual void AddForce(Vec3 const& force);
	virtual void AddImpulse(Vec3 const& impulse);
	virtual void Jump();
	virtual void Update(float deltaSeconds);
	virtual void PreventativePhysics(float deltaSeconds);
	virtual void Render();
	virtual void SetPhysicsMode(MovementMode newMode);
	virtual void SetNextPhysicsMode();
	virtual MovementMode GetPhysicsMode() const { return m_movementMode; }
	virtual std::string GetPhysicsModeAsString() const;
	virtual AABB3 const GetBounds() const;
	virtual SimpleMinerRaycast GetClosestRaycastHit(float deltaSeconds) const;

public:
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_acceleration = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	EulerAngles m_angularVelocity = EulerAngles::ZERO;

	Game* m_game = nullptr;

	float m_height = g_gameConfigBlackboard.GetValue("ENTITY_HEIGHT", 1.8f);
	float m_width = g_gameConfigBlackboard.GetValue("ENTITY_WIDTH", 0.6f);
	float m_groundDrag = g_gameConfigBlackboard.GetValue("ENTITY_GROUND_DRAG", 9.0f);
	float m_airDrag = g_gameConfigBlackboard.GetValue("ENTITY_AIR_DRAG", 9.0f);
	float m_gravity = g_gameConfigBlackboard.GetValue("ENTITY_GRAVITY", 9.0f);
	float m_usedDrag = 0.0f;

	bool m_renderMesh = false;
	bool m_pushedFromBottom = false;
	bool m_isPreventativeEnabled = true;
	MovementMode m_movementMode = MovementMode::WALKING;
};