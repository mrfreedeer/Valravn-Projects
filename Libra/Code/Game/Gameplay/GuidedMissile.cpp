#include "Engine/Math/OBB2.hpp"
#include "Game/Gameplay/GuidedMissile.hpp"
#include "Game/Gameplay/Map.hpp"

GuidedMissile::~GuidedMissile()
{
}

GuidedMissile::GuidedMissile(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type) :
	Entity(pointerToGame, startingPosition, orientation, faction, type, false)
{
	m_isHitByBullets = false;
	m_isPushedByWalls = false;
	m_isPushedByEntities = false;
	m_pushesEntities = false;

	m_physicsRadius = g_gameConfigBlackboard.GetValue("GUIDED_MISSILE_PHYSICS_RADIUS", 0.1f);
	m_cosmeticsRadius = g_gameConfigBlackboard.GetValue("GUIDED_MISSILE_COSMETIC_RADIUS", 0.12f);


	if (m_faction == EntityFaction::EVIL) {
		m_texture = g_textures[GAME_TEXTURE::ENEMY_BOLT];
	}
	else {
		m_texture = g_textures[GAME_TEXTURE::FRIENDLY_BOLT];
	}
	m_health = 1;
	m_speed = g_gameConfigBlackboard.GetValue("GUIDED_MISSILE_SPEED", 7.5f);
	m_turnSpeed = g_gameConfigBlackboard.GetValue("GUIDED_MISSILE_TURN_SPEED", 10.0f);

	m_velocity = GetForwardNormal() * m_speed;
	m_bulletHalfDim = Vec2(g_gameConfigBlackboard.GetValue("GUIDED_MISSILE_HALF_DIMESION_X", 0.08f), g_gameConfigBlackboard.GetValue("GUIDED_MISSILE_HALF_DIMESION_Y", 0.04f));
}



void GuidedMissile::Render() const
{
	OBB2 worldOBB2;
	worldOBB2.m_halfDimensions = m_bulletHalfDim;
	worldOBB2.m_iBasisNormal = Vec2::MakeFromPolarDegrees(m_orientationDegrees, 1.0f);
	worldOBB2.m_center = m_position;

	g_theRenderer->BindTexture(m_texture);

	std::vector<Vertex_PCU> drawVerts;
	AddVertsForOBB2D(drawVerts, worldOBB2, Rgba8::WHITE);

	g_theRenderer->DrawVertexArray((int)drawVerts.size(), drawVerts.data());

	g_theRenderer->BindTexture(nullptr);
	if (g_drawDebug) {
		RenderDebug();
	}
}

void GuidedMissile::Update(float deltaSeconds)
{
	if (m_map->IsAlive(m_goalEntity)) {
		Vec2 dispToGoalEntity = m_goalEntity->m_position - m_position;
		float goalOrientation = dispToGoalEntity.GetOrientationDegrees();

		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, goalOrientation, m_turnSpeed * deltaSeconds);
		m_velocity = GetForwardNormal() * m_speed;
	}
	
	Vec2 newPos = m_position + m_velocity * deltaSeconds;

	if (m_map->IsPointInSolid(newPos)) {
		Entity::Die();
	}

	

	Entity::Update(deltaSeconds);
}

void GuidedMissile::RenderDebug() const
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
