#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Renderer.hpp"

class Game;
class Entity {

public:
	Vec2 m_position = Vec2();
	Vec2 m_velocity = Vec2();
	float m_orientationDegrees = 0.0f;
	float m_angularVelocity = 0.0f;
	float m_physicsRadius = 0.0f;
	float m_cosmeticRadius = 0.0f;
	int m_health = 0;
	bool m_isDead = false;
	bool m_isGarbage = false;
	bool m_spawnsEffects = true;

	int m_shieldHealth = 0;
	bool m_renderShieldHit = false;
	float m_shieldHitEffectTimer = 0.0f;
	float m_shieldRadius = 0.0f;

	Rgba8 m_color;
	Game* m_game = nullptr;

public:
	Entity(Game* gamePointer, const Vec2& startingPosition) :
		m_game(gamePointer),
		m_position(startingPosition)
	{
	}

	virtual ~Entity();
	virtual void Update(float deltaSeconds);
	virtual void Render() const = 0;
	virtual void RenderShield() const;
	virtual void Die();
	virtual bool IsAlive() const { return !m_isDead; }
	virtual const Vec2 GetForwardNormal() const;
	virtual const bool IsOffScreen();
	virtual void DrawDebug() const;
	virtual void TakeAHit(Vec2 const& hitPosition);
	virtual void SpawnDeathDebrisCluster() const;
	virtual void InitializeLocalVerts() = 0;
	virtual float GetCollisionRadius();

protected:
	Vec2 m_hitPosition;
};