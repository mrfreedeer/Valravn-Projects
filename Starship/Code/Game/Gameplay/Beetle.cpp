#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Gameplay/Beetle.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/PlayerShip.hpp"
#include "Game/Framework/GameCommon.hpp"

Beetle::Beetle(Game* gamePointer, const Vec2& position) :
	Entity(gamePointer, position)
{
	m_color = Rgba8(0, 255, 0, 255);
	m_health = BEETLE_HEALTH;
	m_cosmeticRadius = BEETLE_COSMETIC_RADIUS;
	m_physicsRadius = BEETLE_PHYSICS_RADIUS;
	m_shieldRadius = m_cosmeticRadius * SHIELD_PHYSICS_RADIUS_PERCENTAGE;
	InitializeLocalVerts();
}

void Beetle::Render() const
{

	Vertex_PCU world_verts[BEETLE_VERTEXES_AMOUNT] = {};
	for (int vertIndex = 0; vertIndex < BEETLE_VERTEXES_AMOUNT; vertIndex++) {
		world_verts[vertIndex] = m_verts[vertIndex];
	}

	TransformVertexArrayXY3D(BEETLE_VERTEXES_AMOUNT, world_verts, 1.f, m_orientationDegrees, m_position);

	g_theRenderer->DrawVertexArray(BEETLE_VERTEXES_AMOUNT, world_verts);

	RenderShield();

}

void Beetle::Update(float deltaTime)
{
	const PlayerShip* nearestPlayer = m_game->GetNearestPlayer();
	if (nearestPlayer->IsAlive()) {

		m_velocity = nearestPlayer->m_position - m_position;

		m_velocity.SetLength(BEETLE_SPEED);
	}
		m_position += m_velocity * deltaTime;
		m_orientationDegrees = m_velocity.GetOrientationDegrees();

}

void Beetle::Die()
{
	Entity::Die();
	SpawnDeathDebrisCluster();
}

void Beetle::InitializeLocalVerts()
{
	Vec2 uvTexCoords;
	// Body
	m_verts[0] = Vertex_PCU(Vec3(-2.25f, -1.5f, 0.f), m_color, uvTexCoords);
	m_verts[1] = Vertex_PCU(Vec3(2.25f, 1.5f, 0.f), m_color, uvTexCoords);
	m_verts[2] = Vertex_PCU(Vec3(-2.25f, 1.5f, 0.f), m_color, uvTexCoords);

	m_verts[3] = Vertex_PCU(Vec3(-2.25f, -1.5f, 0.f), m_color, uvTexCoords);
	m_verts[4] = Vertex_PCU(Vec3(2.25f, -1.5f, 0.f), m_color, uvTexCoords);
	m_verts[5] = Vertex_PCU(Vec3(2.25f, 1.5f, 0.f), m_color, uvTexCoords);

	// Front Legs
	// Left Leg
	m_verts[6] = Vertex_PCU(Vec3(3.f, -0.15f, 0.f), m_color, uvTexCoords);
	m_verts[7] = Vertex_PCU(Vec3(1.f, -0.15f, 0.f), m_color, uvTexCoords);
	m_verts[8] = Vertex_PCU(Vec3(1.f, -2.15f, 0.f), m_color, uvTexCoords);

	// Right Leg
	m_verts[9] = Vertex_PCU(Vec3(3.f, 0.15f, 0.f), m_color, uvTexCoords);
	m_verts[10] = Vertex_PCU(Vec3(1.f, 0.15f, 0.f), m_color, uvTexCoords);
	m_verts[11] = Vertex_PCU(Vec3(1.f, 2.15f, 0.f), m_color, uvTexCoords);

	// Back Legs
	// Back Left Leg
	m_verts[12] = Vertex_PCU(Vec3(-3.f, .25f, 0.f), m_color, uvTexCoords);
	m_verts[13] = Vertex_PCU(Vec3(-2.f, .25f, 0.f), m_color, uvTexCoords);
	m_verts[14] = Vertex_PCU(Vec3(-2.f, 1.5f, 0.f), m_color, uvTexCoords);

	// Back Right Leg
	m_verts[15] = Vertex_PCU(Vec3(-3.f, -.25f, 0.f), m_color, uvTexCoords);
	m_verts[16] = Vertex_PCU(Vec3(-2.f, -.25f, 0.f), m_color, uvTexCoords);
	m_verts[17] = Vertex_PCU(Vec3(-2.f, -1.5f, 0.f), m_color, uvTexCoords);

}
