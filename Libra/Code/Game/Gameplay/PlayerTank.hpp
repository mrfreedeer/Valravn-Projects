#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Math/OBB2.hpp"

class Texture;

enum class PlayerWeapon {
	REGULAR,
	FLAMETHROWER,
	NUM_WEAPONS
};

class PlayerTank : public Entity {
public:
	PlayerTank(Map* mapPointer, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type);
	~PlayerTank();

	void Update(float deltaSeconds);
	void Render() const;
	void Reset();
	void StartCrossingMapsAnimation();
	virtual void Die() override;

private:
	void UpdateBaseQuadOrientation(float deltaSeconds);
	void UpdateTurretQuadOrientation(float deltaSeconds);
	void UpdateCrossingAnimation(float deltaSeconds);
	void UpdateInput(float deltaSeconds);
	void RenderDebug() const;
	virtual void TakeDamage() override;

private:
	Texture* m_baseTexture = nullptr;
	Texture* m_turretTexture = nullptr;
	float m_turretOrientation = m_orientationDegrees;
	float m_turretTurnSpeed = g_gameConfigBlackboard.GetValue("PLAYER_TANK_TURRET_TURN_SPEED", 360.0f);

	float m_goalOrientation = 0.0f;
	float m_goalTurretOrientation = 0.0f;

	float m_regularFireCoolDown = g_gameConfigBlackboard.GetValue("PLAYER_BULLET_FIRE_COOLDOWN", 0.1f);
	float m_flameThrowerFireCoolDown = 0.1f;
	float m_timeLeftToShoot = 0.0f;
	
	bool m_isMoving = false;
	bool m_isCrossingMaps = false;
	float m_timeToCrossMaps = g_gameConfigBlackboard.GetValue("FADE_TO_BLACK_TOTAL_TIME", 2.0f);
	float m_totalTimeCrossingMaps = 0.0f;
	float m_animationStartingOrientation = 0.0f;
	float m_animationStartingTurrentOrientation = 0.0f;
	float m_crossingAnimationTurnSpeed = g_gameConfigBlackboard.GetValue("CROSSING_ANIMATION_TURN_SPEED", 360.0f);

	PlayerWeapon m_weaponType = PlayerWeapon::REGULAR;
	float m_flameThrowerHalfAngleVariance = g_gameConfigBlackboard.GetValue("FLAMETHROWER_HALF_ANGLE_VARIANCE", 15.0f);
};