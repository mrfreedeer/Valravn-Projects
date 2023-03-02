#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/RaycastUtils.hpp"

class Scorpio : public Entity {
public:
	Scorpio(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor = true);
	~Scorpio();

	virtual void Render() const override;
	virtual void Update(float deltaSeconds) override;
	void UpdateOrientation(float deltaSeconds, Entity* player);
	void UpdateShooting(float deltaSeconds, Entity* player);
	void RenderFadingLaser() const;
	
	virtual void Die() override;

private:

	Texture* m_baseTexture = nullptr;
	Texture* m_topTexture = nullptr;
	RaycastResult2D m_lastHit;
	float m_timeSinceLastTimeFired = g_gameConfigBlackboard.GetValue("SCORPIO_BULLET_FIRE_COOLDOWN", 1.0f);
	float m_fireCooldown = g_gameConfigBlackboard.GetValue("SCORPIO_BULLET_FIRE_COOLDOWN", 1.0f);
	float m_rayCastLength = g_gameConfigBlackboard.GetValue("MAX_RAYCAST_LENGTH", 10.0f);

	bool m_canShoot = true;
	bool m_hasSightOfPlayer = false;
};