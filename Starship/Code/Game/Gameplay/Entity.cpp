#include "Entity.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"

Entity::~Entity()
{
	m_game = nullptr;
}

void Entity::Update(float deltaSeconds)
{
	this->m_position += m_velocity * deltaSeconds;
	this->m_orientationDegrees += m_angularVelocity * deltaSeconds;

	if (m_renderShieldHit) {
		m_shieldHitEffectTimer += deltaSeconds;
		if (m_shieldHitEffectTimer >= SHIELD_TIME_HIT_EFFECT) {
			m_renderShieldHit = false;
			m_shieldHitEffectTimer = 0.0f;
		}
	}
}

void Entity::RenderShield() const
{
	if (m_shieldHealth > 0) {
		Rgba8 fadedBlue(50, 133, 250, 95);
		DrawCircle(m_position, m_shieldRadius, fadedBlue);
	}

	if (m_renderShieldHit) {
		DrawCircleShieldWithHitEffect(m_position, m_shieldRadius, Rgba8(255, 0, 0, 0), m_hitPosition, m_shieldHitEffectTimer);
	}

}

void Entity::Die()
{
	m_isDead = true;
	m_isGarbage = true;
	if (m_spawnsEffects) {
		SpawnDeathDebrisCluster();
		g_theAudio->StartSound(entityDeathSound);
	}

}

void Entity::SpawnDeathDebrisCluster() const {
	float minDebrisSpeed = m_velocity.GetLength() * 4.0f;
	float maxDebrisSpeed = minDebrisSpeed * 2.0f;

	float randomVelocityDegSpread = rng.GetRandomFloatInRange(-1.0f, 1.0f) * 180.0f;
	Vec2 randomBaseVelocity = m_velocity.GetRotatedDegrees(randomVelocityDegSpread);

	m_game->SpawnDebrisCluster(DEBRIS_AMOUNT_DEATH, m_position, m_color, randomBaseVelocity, DEBRIS_MIN_SCALE_DEATH, DEBRIS_MAX_SCALE_DEATH, minDebrisSpeed, maxDebrisSpeed);
}

float Entity::GetCollisionRadius()
{
	if (m_shieldHealth > 0) {
		return m_shieldRadius;
	}
	return m_physicsRadius;
}

const Vec2 Entity::GetForwardNormal() const
{
	Vec2 directionVec(1.f, 0.f);

	TransformPosition2D(directionVec, 1.f, m_orientationDegrees, Vec2());

	return directionVec.GetNormalized();
}

const bool Entity::IsOffScreen()
{
	if (m_position.x > WORLD_SIZE_X + m_cosmeticRadius) {
		return true;
	}
	if (m_position.x < -m_cosmeticRadius) {
		return true;
	}

	if (m_position.y > WORLD_SIZE_Y + m_cosmeticRadius) {
		return true;
	}

	if (m_position.y < -m_cosmeticRadius) {
		return true;
	}

	return false;
}

void Entity::DrawDebug() const
{
	Rgba8 magenta(255, 0, 255, 255);
	Rgba8 cyan(0, 255, 255, 255);
	Rgba8 red(255, 0, 0, 255);
	Rgba8 green(0, 255, 0, 255);
	Rgba8 yellow(255, 255, 0, 255);

	float thickness = .15f;

	DebugDrawRing(m_position, m_cosmeticRadius, thickness, magenta);
	DebugDrawRing(m_position, m_physicsRadius, thickness, cyan);

	Vec2 fwd = GetForwardNormal();
	Vec2 left = fwd.GetRotated90Degrees();

	fwd.SetLength(m_cosmeticRadius);
	left.SetLength(m_cosmeticRadius);

	fwd += m_position;
	left += m_position;

	DebugDrawLine(m_position, fwd, thickness, red);
	DebugDrawLine(m_position, left, thickness, green);

	Vec2 endVelocity = m_position + m_velocity;

	DebugDrawLine(m_position, endVelocity, thickness, yellow);
}

void Entity::TakeAHit(Vec2 const& newHitPosition = Vec2(0.0f, 0.0f))
{
	m_hitPosition = newHitPosition;
	g_theAudio->StartSound(entityDamageSound);
	if (m_shieldHealth <= 0) {
		m_health--;
	}
	else {
		m_renderShieldHit = true;
		m_shieldHealth--;
	}

	if (m_health <= 0) {
		Die();
		m_game->ShakeScreenDeath();
	}
	else {
		m_game->ShakeScreenCollision();
		if (m_spawnsEffects) {

			float minDebrisSpeed = m_velocity.GetLength() * 2.0f;
			float maxDebrisSpeed = minDebrisSpeed * 2.0f;

			float randomVelocityDegSpread = rng.GetRandomFloatInRange(-1.0f, 1.0f) * 180.0f;
			Vec2 randomBaseVelocity = m_velocity.GetRotatedDegrees(randomVelocityDegSpread);

			m_game->SpawnDebrisCluster(DEBRIS_AMOUNT_COLLISION, m_position, m_color, randomBaseVelocity, DEBRIS_MIN_SCALE, DEBRIS_MAX_SCALE, minDebrisSpeed, maxDebrisSpeed);
		}
	}
}
