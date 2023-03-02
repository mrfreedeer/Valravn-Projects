#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Math/OBB2.hpp"

class Leo : public Entity {
public:
	Leo(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor = true);
	~Leo();

	virtual void Render() const override;
	virtual void RenderDebug() const override;
	virtual void Update(float deltaSeconds) override;
	virtual void UpdateActionWithHeatMap(float deltaSeconds) override;

private:
	Texture* m_texture = nullptr;
	float m_timeSinceLastShot = g_gameConfigBlackboard.GetValue("LEO_BULLET_FIRE_COOLDOWN", 1.0f);
	float m_fireCoolDown = g_gameConfigBlackboard.GetValue("LEO_BULLET_FIRE_COOLDOWN", 1.0f);

	bool m_move = false;

};