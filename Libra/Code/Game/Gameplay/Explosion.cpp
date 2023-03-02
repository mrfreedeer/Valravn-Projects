#include "Engine/Math/OBB2.hpp"
#include "Game/Gameplay/Explosion.hpp"

Explosion::Explosion(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type) :
	Entity(pointerToGame, startingPosition, orientation, faction, type, false)
{
	GetAndSetExplosionProperties();

	m_duration = m_spriteAnimDef->GetDuration();

	m_canSwim = false;
	m_isActor = true;
	m_isPushedByEntities = false;
	m_pushesEntities = false;
	m_isHitByBullets = false;
	m_isPushedByWalls = false;
}

Explosion::~Explosion()
{
}

void Explosion::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	m_timeAlive += deltaSeconds;
	m_orientationDegrees += m_turnSpeed * deltaSeconds;

	if (m_timeAlive > m_duration) {
		Entity::Die();
	}
}

void Explosion::Render() const
{

	SpriteDefinition const& currentSprite = m_spriteAnimDef->GetSpriteDefAtTime(m_timeAlive);

	float halfSize = m_size * 0.5f;

	OBB2 explosionOBB2(m_position, Vec2(halfSize, halfSize));
	explosionOBB2.RotateAboutCenter(m_orientationDegrees);

	std::vector<Vertex_PCU> explosionVerts;
	AddVertsForOBB2D(explosionVerts, explosionOBB2, Rgba8::WHITE, currentSprite.GetUVs());

	g_theRenderer->BindTexture(&currentSprite.GetTexture());
	g_theRenderer->DrawVertexArray(explosionVerts);

}

void Explosion::GetAndSetExplosionProperties()
{
	static float smallExplosionSize = g_gameConfigBlackboard.GetValue("EXPLOSION_SMALL_SIZE", 0.3f);
	static float mediumExplosionSize = g_gameConfigBlackboard.GetValue("EXPLOSION_MEDIUM_SIZE", 1.0f);
	static float largeExplosionSize = g_gameConfigBlackboard.GetValue("EXPLOSION_LARGE_SIZE", 1.5f);

	static float smallExplosionAngularSpeed = g_gameConfigBlackboard.GetValue("EXPLOSION_ANGULAR_SPEED_SMALL", 500.0f);
	static float mediumExplosionAngularSpeed = g_gameConfigBlackboard.GetValue("EXPLOSION_ANGULAR_SPEED_MEDIUM", 300.0f);
	static float largeExplosionAngularSpeed = g_gameConfigBlackboard.GetValue("EXPLOSION_ANGULAR_SPEED_LARGE", 200.0f);


	float usedExplosionSize = 0.0f;

	switch (m_faction)
	{
	case EntityFaction::GOOD:
		usedExplosionSize = largeExplosionSize;
		m_spriteAnimDef = g_explosionAnimDefinitions[(int)ExplosionScale::LARGE];
		m_turnSpeed = largeExplosionAngularSpeed;
		break;
	case EntityFaction::EVIL:
		usedExplosionSize = mediumExplosionSize;
		m_spriteAnimDef = g_explosionAnimDefinitions[(int)ExplosionScale::MEDIUM];
		m_turnSpeed = mediumExplosionAngularSpeed;
		break;
	default:
		usedExplosionSize = smallExplosionSize;
		m_spriteAnimDef = g_explosionAnimDefinitions[(int)ExplosionScale::SMALL];
		m_turnSpeed = smallExplosionAngularSpeed;
		break;
	}

	m_turnSpeed *= rng.GetRandomIntInRange(-1, 1);

	m_size = usedExplosionSize;
}
