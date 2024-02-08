#include "Game/Gameplay/Scorpio.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/PlayerTank.hpp"

Scorpio::Scorpio(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor) :
	Entity(pointerToGame, startingPosition, orientation, faction, type, isActor)
{
	m_isPushedByWalls = true;
	m_isPushedByEntities = false;
	m_pushesEntities = true;
	m_isHitByBullets = true;

	m_baseTexture = g_textures[GAME_TEXTURE::ENEMY_TURRET_BASE];
	m_topTexture = g_textures[GAME_TEXTURE::ENEMY_CANNON];


	m_physicsRadius = g_gameConfigBlackboard.GetValue("SCORPIO_PHYSICS_RADIUS", 0.35f);
	m_cosmeticsRadius = g_gameConfigBlackboard.GetValue("SCORPIO_COSMETICS_RADIUS", 0.45f);
	m_health = g_gameConfigBlackboard.GetValue("SCORPIO_HEALTH", 5);
	m_maxHealth = m_health;
	m_turnSpeed = g_gameConfigBlackboard.GetValue("SCORPIO_TURN_RATE", 45.0f);
}

Scorpio::~Scorpio()
{
	Entity::~Entity();
}

void Scorpio::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	worldVerts.reserve(6);

	OBB2 baseOBB2;
	baseOBB2.m_halfDimensions = Vec2(0.5f, 0.5f);
	baseOBB2.m_center = m_position;

	OBB2 turretOBB2;
	turretOBB2.m_halfDimensions = Vec2(0.5f, 0.5f);
	turretOBB2.m_center = m_position;
	turretOBB2.RotateAboutCenter(m_orientationDegrees);

	AddVertsForOBB2D(worldVerts, baseOBB2, Rgba8::WHITE);
	g_theRenderer->BindTexture(m_baseTexture);
	g_theRenderer->DrawVertexArray(worldVerts);

	worldVerts.clear();
	AddVertsForOBB2D(worldVerts, turretOBB2, Rgba8::WHITE);
	g_theRenderer->BindTexture(m_topTexture);
	g_theRenderer->DrawVertexArray(worldVerts);

	RenderFadingLaser();
	if (g_drawDebug) {
		Entity::RenderDebug();
	}
}

void Scorpio::Update(float deltaSeconds)
{
	Entity* player = m_map->GetNearestEntityOfType(m_position, EntityType::PLAYER);
	Vec2 fwd = GetForwardNormal();

	UpdateOrientation(deltaSeconds, player);
	UpdateShooting(deltaSeconds, player);


}

void Scorpio::UpdateOrientation(float deltaSeconds, Entity* player)
{

	if (m_map->IsAlive(player)) {
		m_hasSightOfPlayer = m_map->HasLineOfSight(m_position, player->m_position, m_rayCastLength);
		if (m_hasSightOfPlayer) {
			Vec2 dispToPlayer = player->m_position - m_position;
			m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, dispToPlayer.GetOrientationDegrees(), m_turnSpeed * deltaSeconds);
			return;
		}

	}

	m_orientationDegrees += m_turnSpeed * deltaSeconds;
}

void Scorpio::UpdateShooting(float deltaSeconds, Entity* player)
{
	if (!m_map->IsAlive(player)) return;
	Vec2 forwardDir = GetForwardNormal();

	m_lastHit = m_map->VoxelRaycastVsTiles(m_position, forwardDir, m_rayCastLength);

	Vec2 dispToPlayerDir = player->m_position - m_position;
	m_timeSinceLastTimeFired += deltaSeconds;

	if (m_hasSightOfPlayer && (GetAngleDegreesBetweenVectors2D(forwardDir, dispToPlayerDir) <= 5.0f) && m_canShoot) {
		m_map->SpawnNewEntity(EntityType::BULLET, EntityFaction::EVIL, m_position + forwardDir * m_cosmeticsRadius, m_orientationDegrees);
		m_canShoot = false;
		m_timeSinceLastTimeFired = 0.0f;
		float soundBalanceToPlayer = GetSoundBalanceToPlayer();
		PlaySound(GAME_SOUND::ENEMY_SHOOTING, 1.0f, false, soundBalanceToPlayer);
	}

	if (m_timeSinceLastTimeFired >= m_fireCooldown) {
		m_canShoot = true;
	}

}

void Scorpio::RenderFadingLaser() const
{
	Vec2  fwd = GetForwardNormal();
	Vec2 lineStart = m_position + fwd * m_cosmeticsRadius;
	Vec2 lineEnd;

	Rgba8 color = Rgba8::RED;
	Rgba8 fadedColor = Rgba8::RED;

	float distSqrToImpact = GetDistanceSquared2D(m_position, m_lastHit.m_impactPos);
	if (m_lastHit.m_didImpact) {
		fadedColor.a = static_cast<unsigned char>(RangeMapClamped(distSqrToImpact, 0.0f, m_rayCastLength * m_rayCastLength, 255, 100));
		lineEnd = m_lastHit.m_impactPos;
	}
	else {
		fadedColor.a = 100;
		lineEnd = m_position + fwd * m_rayCastLength;
	}

	Vec2 left = fwd.GetRotated90Degrees();
	left *= m_debugLineThickness * .5f;
	fwd *= m_debugLineThickness * .5f;

	Vertex_PCU vertexes[6];

	Vec2 startLeft = lineStart + left - fwd;
	Vec2 startRight = lineStart - left - fwd;

	Vec2 endLeft = lineEnd + left + fwd;
	Vec2 endRight = lineEnd - left + fwd;

	vertexes[0] = Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), color, Vec2());
	vertexes[1] = Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), color, Vec2());
	vertexes[2] = Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), fadedColor, Vec2());

	vertexes[3] = Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), fadedColor, Vec2());
	vertexes[4] = Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), color, Vec2());
	vertexes[5] = Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), fadedColor, Vec2());

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(6, vertexes);
}

void Scorpio::Die()
{
   	m_map->m_areHeatMapsDirty = true;
	Entity::Die();
}

