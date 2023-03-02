#include "Game/Gameplay/Leo.hpp"
#include "Game/Gameplay/Map.hpp"

Leo::Leo(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor) :
	Entity(pointerToGame, startingPosition, orientation, faction, type, isActor)
{
	m_isPushedByWalls = true;
	m_isPushedByEntities = true;
	m_pushesEntities = true;
	m_isHitByBullets = true;

	m_physicsRadius = g_gameConfigBlackboard.GetValue("LEO_PHYSICS_RADIUS", 0.35f);
	m_cosmeticsRadius = g_gameConfigBlackboard.GetValue("LEO_COSMETICS_RADIUS", 0.45f);

	m_texture = g_textures[GAME_TEXTURE::ENEMY_TANK_4];
	m_health = g_gameConfigBlackboard.GetValue("LEO_HEALTH", 5);
	m_maxHealth = m_health;
	m_speed = g_gameConfigBlackboard.GetValue("LEO_SPEED", 0.8f);
	m_turnSpeed = g_gameConfigBlackboard.GetValue("LEO_TURN_RATE", 45.0f);

	m_turnHalfAperture = g_gameConfigBlackboard.GetValue("LEO_HALF_TURN_SECTOR", 45.0f);

}

Leo::~Leo()
{
	Entity::~Entity();
}

void Leo::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	worldVerts.reserve(6);

	OBB2 tankOBB2;
	tankOBB2.m_halfDimensions = Vec2(0.5f, 0.5f);
	tankOBB2.m_center = m_position;
	tankOBB2.RotateAboutCenter(m_orientationDegrees);

	AddVertsForOBB2D(worldVerts, tankOBB2, Rgba8::WHITE);

	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(worldVerts);

	if (g_drawDebug) {
		RenderDebug();
	}

}

void Leo::RenderDebug() const
{
	g_theRenderer->BindTexture(nullptr);
	if (m_chasingPlayer) {
		DebugDrawLine(m_position, m_goalPosition, m_debugLineThickness, Rgba8::SILVER);
	}
	DrawCircle(m_goalPosition, 0.1f, Rgba8::SILVER);
	DebugDrawLine(m_position, m_nextWayPoint, m_debugLineThickness, Rgba8::GREEN);
	DrawCircle(m_nextWayPoint, 0.1f, Rgba8::GREEN);
	Entity::RenderDebug();
}

void Leo::Update(float deltaSeconds)
{

	Entity::UpdateOrientationWithHeatMap(deltaSeconds);
	UpdateActionWithHeatMap(deltaSeconds);

	Entity::Update(deltaSeconds);
}

void Leo::UpdateActionWithHeatMap(float deltaSeconds)
{
	Entity::UpdateActionWithHeatMap(deltaSeconds);
	Vec2 fwd = GetForwardNormal();
	Vec2 dispToNextWayPoint = m_nextWayPoint - m_position;

	m_timeSinceLastShot += deltaSeconds;
	Entity* nearestPlayer = m_map->GetNearestEntityOfType(m_position, EntityType::PLAYER);
	if (!m_map->IsAlive(nearestPlayer)) return;

	Vec2 dispToNearestPlayer = nearestPlayer->m_position - m_position;

	if (m_hasSightOfPlayer && GetAngleDegreesBetweenVectors2D(fwd, dispToNearestPlayer) <= 5.0f) {
		if (m_timeSinceLastShot >= m_fireCoolDown) {
			m_map->SpawnNewEntity(EntityType::BULLET, EntityFaction::EVIL, m_position + fwd * m_cosmeticsRadius, m_orientationDegrees);
			m_timeSinceLastShot = 0.0f;

			float soundBalanceToPlayer = GetSoundBalanceToPlayer();
			PlaySound(GAME_SOUND::ENEMY_SHOOTING, 1.0f, false, soundBalanceToPlayer);
		}
	}

}


