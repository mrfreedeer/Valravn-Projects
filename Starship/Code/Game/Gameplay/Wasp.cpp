#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Gameplay/Wasp.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/PlayerShip.hpp"
#include "Game/Framework/GameCommon.hpp"

Wasp::Wasp(Game* gamePointer, const Vec2& position) :
	Entity(gamePointer, position)
{
	m_color = Rgba8(255, 255, 0, 255);

	m_health = WASP_HEALTH;
	m_physicsRadius = WASP_PHYSICS_RADIUS;
	m_cosmeticRadius = WASP_COSMETIC_RADIUS;
	m_shieldRadius = m_cosmeticRadius * SHIELD_PHYSICS_RADIUS_PERCENTAGE;

	InitializeLocalVerts();
}

void Wasp::Render() const
{

	Vertex_PCU world_verts[WASP_VERTEXES_AMOUNT] = {};
	for (int vertIndex = 0; vertIndex < WASP_VERTEXES_AMOUNT; vertIndex++) {
		world_verts[vertIndex] = m_verts[vertIndex];
	}

	TransformVertexArrayXY3D(WASP_VERTEXES_AMOUNT, world_verts, 1.f, m_orientationDegrees, m_position);

	g_theRenderer->DrawVertexArray(WASP_VERTEXES_AMOUNT, world_verts);

	RenderShield();

}

void Wasp::Update(float deltaTime)
{
	const PlayerShip* nearestPlayer = m_game->GetNearestPlayer();
	if (nearestPlayer->IsAlive()) {
		Vec2 direction = nearestPlayer->m_position - m_position;
		m_orientationDegrees = direction.GetOrientationDegrees();
		m_velocity += this->GetForwardNormal() * WASP_ACCELERATION * deltaTime;
		m_velocity.ClampLength(MAX_WASP_SPEED);
	}

	m_position += m_velocity * deltaTime;

}

void Wasp::Die()
{
	Entity::Die();
	SpawnDeathDebrisCluster();
}

void Wasp::InitializeLocalVerts()
{
	Rgba8& yellow = m_color;
	Rgba8 grey(192, 192, 192, 220);
	Vec2 uvTexCoords;
	// Body
	m_verts[0] = Vertex_PCU(Vec3(-1.15f, 0.f, 0.f), grey, uvTexCoords);
	m_verts[1] = Vertex_PCU(Vec3(0, 0.75f, 0.f), grey, uvTexCoords);
	m_verts[2] = Vertex_PCU(Vec3(1.15f, 0.f, 0.f), grey, uvTexCoords);

	m_verts[3] = Vertex_PCU(Vec3(-1.15f, 0.f, 0.f), grey, uvTexCoords);
	m_verts[4] = Vertex_PCU(Vec3(0, -0.75f, 0.f), grey, uvTexCoords);
	m_verts[5] = Vertex_PCU(Vec3(1.15f, 0.f, 0.f), grey, uvTexCoords);

	// Wings
	// Left Wing
	m_verts[6] = Vertex_PCU(Vec3(1.15f, 0.f, 0.f), yellow, uvTexCoords);
	m_verts[7] = Vertex_PCU(Vec3(-2.0f, 2.15f, 0.f), yellow, uvTexCoords);
	m_verts[8] = Vertex_PCU(Vec3(1.0f, 1.5f, 0.f), yellow, uvTexCoords);

	// Right Wing
	m_verts[9] = Vertex_PCU(Vec3(1.15f, 0.f, 0.f), yellow, uvTexCoords);
	m_verts[10] = Vertex_PCU(Vec3(-2.0f, -2.15f, 0.f), yellow, uvTexCoords);
	m_verts[11] = Vertex_PCU(Vec3(1.0f, -1.5f, 0.f), yellow, uvTexCoords);


	// Stinger
	// Left Half
	m_verts[12] = Vertex_PCU(Vec3(-2.f, 0.f, 0.f), yellow, uvTexCoords);
	m_verts[13] = Vertex_PCU(Vec3(-1.f, 0.0f, 0.f), yellow, uvTexCoords);
	m_verts[14] = Vertex_PCU(Vec3(-0.5f, 0.5f, 0.f), yellow, uvTexCoords);

	// Right Half
	m_verts[15] = Vertex_PCU(Vec3(-2.f, 0.f, 0.f), yellow, uvTexCoords);
	m_verts[16] = Vertex_PCU(Vec3(-1.f, 0.0f, 0.f), yellow, uvTexCoords);
	m_verts[17] = Vertex_PCU(Vec3(-0.5f, -0.5f, 0.f), yellow, uvTexCoords);

	// Head
	m_verts[18] = Vertex_PCU(Vec3(1.15f, 0.f, 0.f), yellow, uvTexCoords);
	m_verts[19] = Vertex_PCU(Vec3(2.15f, -1.f, 0.f), yellow, uvTexCoords);
	m_verts[20] = Vertex_PCU(Vec3(2.15f, 1.f, 0.f), yellow, uvTexCoords);

	// Pincers
	// Left Pincer
	m_verts[21] = Vertex_PCU(Vec3(2.15f, -1.f, 0.f), grey, uvTexCoords);
	m_verts[22] = Vertex_PCU(Vec3(3.15f, -.03f, 0.f), grey, uvTexCoords);
	m_verts[23] = Vertex_PCU(Vec3(2.15f, -.2f, 0.f), grey, uvTexCoords);

	// Right Pincer
	m_verts[24] = Vertex_PCU(Vec3(2.15f, 1.f, 0.f), grey, uvTexCoords);
	m_verts[25] = Vertex_PCU(Vec3(3.15f, .03f, 0.f), grey, uvTexCoords);
	m_verts[26] = Vertex_PCU(Vec3(2.15f, .2f, 0.f), grey, uvTexCoords);

}
