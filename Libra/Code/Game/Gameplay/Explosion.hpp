#pragma once
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Gameplay/Entity.hpp"

class Explosion : public Entity {
public:
	Explosion(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type);
	~Explosion();

	virtual void Update(float deltaSeconds);
	virtual void Render() const;
	virtual void RenderHealthBar() const override {}; // Do not render health bar for Explosions

private:
	void GetAndSetExplosionProperties();

protected:
	SpriteAnimDefinition const* m_spriteAnimDef;
	float m_timeAlive = 0.0f;
	float m_size = 0.0f;
	float m_duration = 0.0f;
};