#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Game/Framework/GameCommon.hpp"

class Texture;

class GuidedMissile : public Entity {
public:
	GuidedMissile(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type);
	~GuidedMissile();

	void Render() const override;
	void Update(float deltaSeconds) override;
	void SetGoalEntity(Entity const* newGoalEntity) { m_goalEntity = newGoalEntity; }
	virtual void RenderHealthBar() const override {}; // Do not render health bar for GuidedMissile

private:

	void RenderDebug() const override;

	Vec2 m_bulletHalfDim = Vec2::ZERO;
	Texture* m_texture = nullptr;
	Entity const* m_goalEntity = nullptr;
};