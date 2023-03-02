#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Math/OBB2.hpp"

class Capricorn : public Entity {
public:
	Capricorn(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor = true);
	~Capricorn();

	virtual void Render() const override;
	virtual void RenderDebug() const override;
	virtual void Update(float deltaSeconds) override;
	virtual void UpdateActionWithHeatMap(float deltaSeconds) override;

private:
	Texture* m_texture = nullptr;
	float m_timeSinceLastShot = g_gameConfigBlackboard.GetValue("CAPRICORN_BULLET_FIRE_COOLDOWN", 1.0f);
	float m_fireCoolDown = g_gameConfigBlackboard.GetValue("CAPRICORN_BULLET_FIRE_COOLDOWN", 1.0f);

	bool m_move = false;

};