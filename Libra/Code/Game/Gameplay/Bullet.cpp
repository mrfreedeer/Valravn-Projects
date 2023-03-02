#include "Engine/Math/OBB2.hpp"
#include "Game/Gameplay/Bullet.hpp"
#include "Game/Gameplay/Map.hpp"

Bullet::Bullet(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, BulletType bulletType) :
	Entity(pointerToGame, startingPosition, orientation, faction, type, false),
	m_bulletType(bulletType)
{
	m_isHitByBullets = false;
	m_isPushedByWalls = false;
	m_isPushedByEntities = false;
	m_pushesEntities = false;

	m_physicsRadius = g_gameConfigBlackboard.GetValue("BULLET_PHYSICS_RADIUS", 0.1f);
	m_cosmeticsRadius = g_gameConfigBlackboard.GetValue("BULLET_COSMETIC_RADIUS", 0.12f);


	if (m_faction == EntityFaction::EVIL) {
		m_texture = g_textures[GAME_TEXTURE::ENEMY_BULLET];
	}
	else {
		m_texture = g_textures[GAME_TEXTURE::FRIENDLY_BULLET];
	}


	m_health = 3;
	m_speed = g_gameConfigBlackboard.GetValue("BULLET_SPEED", 7.5f);
	m_bulletHalfDim = Vec2(g_gameConfigBlackboard.GetValue("BULLET_HALF_DIMESION_X", 0.08f), g_gameConfigBlackboard.GetValue("BULLET_HALF_DIMESION_Y", 0.04f));

	if (m_bulletType == BulletType::FLAMETHROWER) {
		m_texture = g_textures[GAME_TEXTURE::EXPLOSION];
		m_speed = g_gameConfigBlackboard.GetValue("FLAMETHROWER_BULLETSPEED", 1.5f);
		m_lifeTimeSeconds = g_gameConfigBlackboard.GetValue("FLAMETHROWER_LIFETIME_SECONDS", 1.0f);
		m_flameThrowerAnimDef = g_explosionAnimDefinitions[(int)ExplosionScale::SMALL];
		
		float flameThrowerSize = g_gameConfigBlackboard.GetValue("FLAMETHROWER_SIZE", 0.5f);
		m_bulletHalfDim = Vec2(flameThrowerSize, flameThrowerSize);
		m_cosmeticsRadius = flameThrowerSize;
	}

	m_velocity = GetForwardNormal() * m_speed;

	m_turnSpeed = g_gameConfigBlackboard.GetValue("EXPLOSION_ANGULAR_SPEED_SMALL", 500.0f);
}

Bullet::~Bullet()
{
}

void Bullet::Render() const
{
	switch (m_bulletType)
	{
	case BulletType::REGULAR:
		RenderRegularBullet();
		break;
	case BulletType::FLAMETHROWER:
		RenderFlameThrowerBullet();
		break;
	default:
		ERROR_AND_DIE(Stringf("DON'T KNOW THIS KIND OF BULLET: %d", (int)m_bulletType));
		break;
	}
}

void Bullet::RenderRegularBullet() const
{
	OBB2 worldOBB2;
	worldOBB2.m_halfDimensions = m_bulletHalfDim;
	worldOBB2.m_iBasisNormal = Vec2::MakeFromPolarDegrees(m_orientationDegrees, 1.0f);
	worldOBB2.m_center = m_position;

	g_theRenderer->BindTexture(m_texture);

	std::vector<Vertex_PCU> drawVerts;
	AddVertsForOBB2D(drawVerts, worldOBB2, Rgba8::WHITE);

	g_theRenderer->DrawVertexArray((int)drawVerts.size(), drawVerts.data());

	if (g_drawDebug) {
		g_theRenderer->BindTexture(nullptr);
		RenderDebug();
	}
}

void Bullet::RenderFlameThrowerBullet() const
{
	float scale = RangeMapClamped(m_timeAlive, 0.0f, m_lifeTimeSeconds, m_bulletHalfDim.x * 0.1f, m_bulletHalfDim.x);
	OBB2 worldOBB2;
	worldOBB2.m_halfDimensions = Vec2(scale, scale);
	worldOBB2.m_iBasisNormal = Vec2::MakeFromPolarDegrees(m_orientationDegrees, 1.0f);
	worldOBB2.m_center = m_position;


	SpriteDefinition const& currentSprite = m_flameThrowerAnimDef->GetSpriteDefAtTime(m_timeAlive);

	g_theRenderer->BindTexture(&currentSprite.GetTexture());

	std::vector<Vertex_PCU> drawVerts;
	AddVertsForOBB2D(drawVerts, worldOBB2, Rgba8::WHITE, currentSprite.GetUVs());

	g_theRenderer->DrawVertexArray((int)drawVerts.size(), drawVerts.data());

	if (g_drawDebug) {
		g_theRenderer->BindTexture(nullptr);
		RenderDebug();
	}
}


void Bullet::Update(float deltaSeconds)
{
	switch (m_bulletType)
	{
	case BulletType::REGULAR:
		UpdateRegularBullet(deltaSeconds);
		break;
	case BulletType::FLAMETHROWER:
		UpdateFlameThrowerBullet(deltaSeconds);
		break;
	default:
		ERROR_AND_DIE(Stringf("DON'T KNOW THIS KIND OF BULLET: %d", (int)m_bulletType));
		break;
	}

	Entity::Update(deltaSeconds);
}

void Bullet::UpdateRegularBullet(float deltaSeconds)
{
	Vec2 newPos = m_position + m_velocity * deltaSeconds;

	if (m_map->IsPointInSolid(newPos)) {
		Entity::TakeDamage();

		if (m_health > 0 && !m_map->IsTileDestructible(newPos)) {
			IntVec2 currentTileCoords = m_map->GetTileCoordsForPosition(m_position);
			IntVec2 nextTileCoords = m_map->GetTileCoordsForPosition(newPos);

			Vec2 bounceNormal = Vec2(nextTileCoords - currentTileCoords);
			BounceOffNormal(bounceNormal);
			m_map->RollForSpawningRubble(m_position, newPos);
		}
		else if (m_map->IsTileDestructible(newPos)) {
			Entity::Die();
			Tile& tile = m_map->GetMutableTileForPosition(newPos);

			tile.m_health--;
			if (tile.m_health <= 0) {
				tile.DestroyTile();
			}
		}
	}
}

void Bullet::UpdateFlameThrowerBullet(float deltaSeconds)
{
	Vec2 newPos = m_position + m_velocity * deltaSeconds;

	if (m_map->IsPointInSolid(newPos) || m_timeAlive >= m_lifeTimeSeconds) {
		Entity::Die();
	}
	else {
		m_orientationDegrees += m_turnSpeed * deltaSeconds;
	}

	m_timeAlive += deltaSeconds;
}


void Bullet::BounceOffNormal(Vec2 const& surfaceNormal) {
	m_velocity.Reflect(surfaceNormal);
	m_orientationDegrees = m_velocity.GetOrientationDegrees();
}


void Bullet::RenderDebug() const
{

	Vec2 rightDir = GetForwardNormal();
	Vec2 leftDir = rightDir.GetRotated90Degrees() * m_physicsRadius;
	rightDir *= m_physicsRadius;

	rightDir += m_position;
	leftDir += m_position;

	DebugDrawRing(m_position, m_cosmeticsRadius, m_debugLineThickness, Rgba8::MAGENTA);
	DebugDrawRing(m_position, m_physicsRadius, m_debugLineThickness, Rgba8::CYAN);
	DebugDrawLine(m_position, rightDir, m_debugLineThickness, Rgba8::RED);
	DebugDrawLine(m_position, leftDir, m_debugLineThickness, Rgba8::GREEN);
}

