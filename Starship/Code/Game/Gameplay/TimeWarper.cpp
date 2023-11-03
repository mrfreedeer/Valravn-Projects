#include "Game/Gameplay/TimeWarper.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/PlayerShip.hpp"

TimeWarper::TimeWarper(Game* gamePointer, Vec2 const& startingPosition) :
	Entity(gamePointer, startingPosition)
{
	m_cosmeticRadius = TIMEWARPER_COSMETIC_RADIUS;
	m_physicsRadius = TIMEWARPER_PHYSICS_RADIUS;
	m_timeWarpingRadius = TIMEWARPER_EFFECT_RADIUS;
	m_shieldRadius = m_physicsRadius * SHIELD_PHYSICS_RADIUS_PERCENTAGE;
	m_health = TIMEWARPER_HEALTH;

	m_speedsUpTime = rng.GetRandomIntInRange(0, 1);
	m_slowsDownTime = !m_speedsUpTime;

	InitializeLocalVerts();
}

TimeWarper::~TimeWarper()
{
}

void TimeWarper::Update(float deltaSeconds)
{


	PlayerShip* nearestPlayer = m_game->GetNearestPlayer();

	Vec2 displacementToPlayer = nearestPlayer->m_position - m_position;
	m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, displacementToPlayer.GetOrientationDegrees(), TIMEWARPER_TURNSPEED * deltaSeconds);

	m_velocity = GetForwardNormal();
	m_velocity.SetLength(TIMEWARPER_SPEED);

	if (DoDiscsOverlap(m_position, TIMEWARPER_EFFECT_RADIUS, nearestPlayer->m_position, nearestPlayer->m_physicsRadius)) {
		if (nearestPlayer->IsAlive()) {
			if (m_slowsDownTime) {
				nearestPlayer->m_isSlowedDown = m_slowsDownTime;
			}
			else {
				nearestPlayer->m_isSpedUp = m_speedsUpTime;
			}
		}
	}


	Entity::Update(deltaSeconds);
	m_timeAlive += deltaSeconds;
}

void TimeWarper::Render() const
{
	Vertex_PCU worldVerts[TIMEWARPER_VERTEX_AMOUNT];

	float animationSpeed = 10.0f;
	for (int vertIndex = 0; vertIndex < TIMEWARPER_VERTEX_AMOUNT; vertIndex++) {
		worldVerts[vertIndex] = m_verts[vertIndex];
	}

	worldVerts[4].m_position.x = RangeMap(sinf(m_timeAlive * animationSpeed), -1.0f, 1.0f, 1.0f, 0.0f);
	worldVerts[10].m_position.x = RangeMap(sinf(m_timeAlive * animationSpeed), -1.0f, 1.0f, 1.0f, 0.0f);

	worldVerts[19].m_position.y = RangeMap(sinf(m_timeAlive * animationSpeed), -1.0f, 1.0f, 1.0f, 0.0f);
	worldVerts[25].m_position.y = RangeMap(sinf(m_timeAlive * animationSpeed), -1.0f, 1.0f, -1.0f, 0.0f);


	TransformVertexArrayXY3D(TIMEWARPER_VERTEX_AMOUNT, worldVerts, 1.0f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(TIMEWARPER_VERTEX_AMOUNT, worldVerts);

	RenderShield();
}

void TimeWarper::InitializeLocalVerts()
{
	Vec2 uvTexCoords;

	Rgba8 blue = Rgba8(0, 0, 255, 255);
	Rgba8 red = Rgba8(255, 0, 0, 255);

	Rgba8 bodyColor = (m_slowsDownTime) ? blue : red; // blue
	Rgba8 jawsColor = (m_slowsDownTime) ? red : blue; // red

	m_color = bodyColor;

	// Body
	//Left Lower Triangle
	m_verts[0] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[1] = Vertex_PCU(Vec3(1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[2] = Vertex_PCU(Vec3(-1.0f, 2.0f, 0.0f), bodyColor, uvTexCoords);

	// Left Upper Triangle
	m_verts[3] = Vertex_PCU(Vec3(1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[4] = Vertex_PCU(Vec3(1.0f, 2.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[5] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);

	// Right Lower Triangle
	m_verts[6] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[7] = Vertex_PCU(Vec3(1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[8] = Vertex_PCU(Vec3(-1.0f, -2.0f, 0.0f), bodyColor, uvTexCoords);

	// Right Upper Triangle
	m_verts[9] = Vertex_PCU(Vec3(1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[10] = Vertex_PCU(Vec3(1.0f, -2.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[11] = Vertex_PCU(Vec3(-1.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);

	// Head
	m_verts[12] = Vertex_PCU(Vec3(1.0f, 1.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[13] = Vertex_PCU(Vec3(1.0f, -1.0f, 0.0f), bodyColor, uvTexCoords);
	m_verts[14] = Vertex_PCU(Vec3(2.0f, 0.0f, 0.0f), bodyColor, uvTexCoords);

	//// Left Lower Jaw
	m_verts[15] = Vertex_PCU(Vec3(1.0f, 1.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[16] = Vertex_PCU(Vec3(2.0f, 0.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[17] = Vertex_PCU(Vec3(2.0f, 1.0f, 0.0f), jawsColor, uvTexCoords);

	// Left Upper Jaw
	m_verts[18] = Vertex_PCU(Vec3(2.0f, 0.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[19] = Vertex_PCU(Vec3(3.0f, 1.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[20] = Vertex_PCU(Vec3(2.0f, 1.0f, 0.0f), jawsColor, uvTexCoords);

	//// Right Lower Jaw
	m_verts[21] = Vertex_PCU(Vec3(1.0f, -1.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[22] = Vertex_PCU(Vec3(2.0f, 0.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[23] = Vertex_PCU(Vec3(2.0f, -1.0f, 0.0f), jawsColor, uvTexCoords);

	// Right Upper Jaw
	m_verts[24] = Vertex_PCU(Vec3(2.0f, 0.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[25] = Vertex_PCU(Vec3(3.0f, -1.0f, 0.0f), jawsColor, uvTexCoords);
	m_verts[26] = Vertex_PCU(Vec3(2.0f, -1.0f, 0.0f), jawsColor, uvTexCoords);
}
