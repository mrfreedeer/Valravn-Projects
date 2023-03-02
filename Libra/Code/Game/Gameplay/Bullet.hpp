#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Game/Framework/GameCommon.hpp"

enum class BulletType {
	REGULAR,
	FLAMETHROWER
};

class Texture;

class Bullet : public Entity {
public:
	Bullet(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, BulletType bulletType);
	~Bullet();

	void Render() const override;
	void Update(float deltaSeconds) override;
	void BounceOffNormal(Vec2 const& surfaceNormal);
	virtual void RenderHealthBar() const override {}; // Do not render health bar for Bullets

private:
	void RenderRegularBullet() const;
	void RenderFlameThrowerBullet() const;
	void RenderDebug() const override;

	void UpdateRegularBullet(float deltaSeconds);
	void UpdateFlameThrowerBullet(float deltaSeconds);

	Vec2 m_bulletHalfDim = Vec2::ZERO;
	BulletType m_bulletType = BulletType::REGULAR;
	Texture* m_texture = nullptr;
	float m_timeAlive = 0.0f;
	float m_lifeTimeSeconds = 0.0f;
	SpriteAnimDefinition const* m_flameThrowerAnimDef;
};