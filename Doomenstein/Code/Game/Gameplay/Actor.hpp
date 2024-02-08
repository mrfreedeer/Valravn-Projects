#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/Clock.hpp"
//#include "Engine/Renderer/Camera.hpp"
#include "Game/Gameplay/ActorUID.hpp"
#include <vector>

enum class Faction
{
	NEUTRAL,
	MARINE,
	DEMON,
	NUM_FACTIONS
};


class Map;
class Controller;
class Weapon;
class AI;
class ActorDefinition;
class SpawnInfo;
class SpriteAnimGroupDefinition;
class Camera;

enum class ActorAnimationName {
	WALK,
	ATTACK,
	PAIN,
	DEATH
};

class Actor {
public:
	Actor(Map* pointerToMap, SpawnInfo const& spawnInfo);
	~Actor();

	void Update(float deltaSeconds);
	void UpdatePhysics(float deltaSeconds);
	void Render(Camera const& camera) const;

	Mat44 const GetModelMatrix(Camera const& camera) const;
	Vec3 GetForward() const;

	void HandleDamage(float damage);
	void Die();
	bool IsSimulated();

	void AddForce(const Vec3& force);
	void AddImpulse(const Vec3& impulse);
	void OnCollide(Actor* other);

	void OnPossessed(Controller* controller);
	void OnUnpossessed(Controller* controller);
	void MoveInDirection(Vec3 direction, float speed);

	Weapon* GetEquippedWeapon();
	void EquipWeapon(int weaponIndex);
	void EquipNextWeapon();
	void EquipPreviousWeapon();
	void Attack();

	bool CanBePossessed() const;
	float GetPhysicsRadius() const;
	bool CanCollideWithWorld() const;
	bool CanCollideWithActors() const;

	Map* GetMap() const { return m_map; }
	void PlayAnimation(ActorAnimationName animationName);
	Vec3 const GetHealthAsArbitraryVector() const;
public:
	ActorUID m_uid = ActorUID::INVALID;
	ActorDefinition const* m_definition = nullptr;

	Vec3 m_position = Vec3::ZERO;
	EulerAngles m_orientation = EulerAngles::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_angularVelocity = Vec3::ZERO;
	Vec3 m_acceleration = Vec3::ZERO;

	std::vector<Weapon*> m_weapons;
	int m_equippedWeaponIndex = -1;

	Actor* m_owner = nullptr;
	bool m_isPendingDelete = false;
	bool m_isDead = false;

	float m_health = 200.0f;
	float m_lastDamageOnDeath = 0.0f;
	Stopwatch m_lifetimeStopwatch;
	Stopwatch m_animationStopwatch;
	Clock m_animationClock;

	Controller* m_controller = nullptr;
	Controller* m_prevController = nullptr;

	bool m_isInvincible = false; // Debug purposes
	bool m_renderForward = false;
	Rgba8 m_solidColor = Rgba8(192, 0, 0);
	Rgba8 m_wireframeColor = Rgba8(255, 192, 192);

	float m_physicsHeight = 0.0f;
	float m_physicsRadius = 0.0f;
	bool m_currentAnimScaledBySpeed = false;
	int m_killCount = 0;
	bool m_gotHurt = false;
private:
	void UpdateGravityEffect();
	void UpdateSpawning();

	void RenderCollisionCylinder() const;
	void Render2DSprite(Camera const& camera) const;
	void Render3DModel(Camera const& camera) const;

	void PlayAnimationBillboardSprite(ActorAnimationName animationName);
	void PlayAnimation3DModel(ActorAnimationName animationName);

	void CreateWeapons();

	std::string GetCurrentAnimationAsString() const;
	std::string GetAnimationAsString(ActorAnimationName animationName) const;
	SpriteAnimGroupDefinition const* GetCurrentAnimationGroup() const;
	Stopwatch m_spawnStopwatch;
	Stopwatch m_lastTimePlayedSound;
	bool m_countedAsKill = false;
private:
	Map* m_map = nullptr;
	ActorAnimationName m_currentAnimation = ActorAnimationName::WALK;
};