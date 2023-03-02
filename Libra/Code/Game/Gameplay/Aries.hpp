#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Math/OBB2.hpp"

class Bullet;

class Aries : public Entity {
public:
	Aries(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor = true);
	~Aries();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void ReactToBullet(Bullet*& bullet) override;

private:

	virtual void RenderDebug() const override;

private:
	Texture* m_texture = nullptr;
	
	float m_shieldSectorHalfAngle = g_gameConfigBlackboard.GetValue("ARIES_SHIELD_SECTOR_HALF_ANGLE", 45.0f);
};