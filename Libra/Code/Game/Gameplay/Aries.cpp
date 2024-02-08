#include "Game/Gameplay/Aries.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/Bullet.hpp"

Aries::Aries(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor):
	Entity(pointerToGame, startingPosition, orientation, faction, type, isActor)
{
	m_isPushedByWalls = true;
	m_isPushedByEntities = true;
	m_pushesEntities = true;
	m_isHitByBullets = true;

	m_cosmeticsRadius = g_gameConfigBlackboard.GetValue("ARIES_COSMETICS_RADIUS", 1.0f);
	m_physicsRadius = g_gameConfigBlackboard.GetValue("ARIES_PHYSICS_RADIUS", 1.0f);

	m_texture = g_textures[GAME_TEXTURE::ARIES];
	m_health = g_gameConfigBlackboard.GetValue("ARIES_HEALTH", 5);
	m_maxHealth = m_health;
	m_speed = g_gameConfigBlackboard.GetValue("ARIES_SPEED", 0.8f);
	m_turnHalfAperture = g_gameConfigBlackboard.GetValue("ARIES_HALF_TURN_SECTOR", 180.0f);
	m_turnSpeed = g_gameConfigBlackboard.GetValue("ARIES_TURN_SPEED", 45.0f);
}

Aries::~Aries()
{
	Entity::~Entity();
}

void Aries::Update(float deltaSeconds)
{
	Entity::UpdateOrientationWithHeatMap(deltaSeconds);
	Entity::UpdateActionWithHeatMap(deltaSeconds);
	Entity::Update(deltaSeconds);

}

void Aries::Render() const
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

void Aries::RenderDebug() const
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

void Aries::ReactToBullet(Bullet*& bullet)
{
	Vec2 dispToBullet = bullet->m_position - m_position;
	Vec2 fwd = GetForwardNormal();
	float angleToBullet = GetAngleDegreesBetweenVectors2D(fwd, dispToBullet);

	if (angleToBullet <= m_shieldSectorHalfAngle) {
		Vec2 surfaceNormal = dispToBullet.GetNormalized();
		PushDiscOutOfDisc2D(bullet->m_position, bullet->m_physicsRadius, m_position, m_physicsRadius);
		bullet->BounceOffNormal(surfaceNormal);
	}
	else {
		bullet->Die();
		Entity::TakeDamage();
	}
}
