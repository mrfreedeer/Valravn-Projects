#include "Game/Gameplay/PickUp.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"

PickUp::PickUp(Game* gamePointer, Vec2 startingPosition, bool hasRandomVelocity) :
	Entity(gamePointer, startingPosition)
{
	if (hasRandomVelocity) {
		float randX = rng.GetRandomFloatInRange(-1.0f, 1.0f);
		float randY = rng.GetRandomFloatInRange(-1.0f, 1.0f);

		m_velocity = Vec2(randX, randY);
	}
	m_physicsRadius = SHIELD_PICKUP_RADIUS;
	m_cosmeticRadius = SHIELD_PICKUP_RADIUS;
	m_shieldRadius = m_cosmeticRadius;
	m_shieldHealth = 1;
	InitializeLocalVerts();
}

void PickUp::Update(float deltaSeconds)
{
	m_position += m_velocity * deltaSeconds;
}

void PickUp::Render() const
{
	Vertex_PCU worldVerts[AMOUNT_VERTS_SHIELD] = {};
	for (int vertIndex = 0; vertIndex < AMOUNT_VERTS_SHIELD; vertIndex++) {
		worldVerts[vertIndex] = m_verts[vertIndex];
	}

	TransformVertexArrayXY3D(AMOUNT_VERTS_SHIELD, worldVerts, 1.0f, 0.0f, m_position);

	g_theRenderer->DrawVertexArray(AMOUNT_VERTS_SHIELD, worldVerts);
	RenderShield();
}

void PickUp::Die()
{
	m_isDead = true;
	m_isGarbage = true;
	if (m_spawnsEffects) {
		SpawnDeathDebrisCluster();
	}
	g_theAudio->StartSound(pickUpSound);
	m_game->m_pickupsInGame--;
}

void PickUp::InitializeLocalVerts()
{
	m_color = Rgba8(255, 255, 0, 255);
	Vec2 uvTexCoords;

	// Shield Center
	m_verts[0] = Vertex_PCU(Vec3(-1.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[1] = Vertex_PCU(Vec3(1.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[2] = Vertex_PCU(Vec3(0.0f, 1.0f, 0.0f), m_color, uvTexCoords);

	// Shield Left
	m_verts[3] = Vertex_PCU(Vec3(-1.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[4] = Vertex_PCU(Vec3(0.0f, 0.0f, 0.0f), m_color, uvTexCoords);
	m_verts[5] = Vertex_PCU(Vec3(-1.0f, 1.0f, 0.0f), m_color, uvTexCoords);

	// Shield Right
	m_verts[6] = Vertex_PCU(Vec3(1.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[7] = Vertex_PCU(Vec3(0.0f, 0.0f, 0.0f), m_color, uvTexCoords);
	m_verts[8] = Vertex_PCU(Vec3(1.0f, 1.0f, 0.0f), m_color, uvTexCoords);

	// Shield Bottom
	m_verts[9] = Vertex_PCU(Vec3(-1.0f, -1.0f, 0.0f), m_color, uvTexCoords);
	m_verts[10] = Vertex_PCU(Vec3(0.0f, -2.0f, 0.0f), m_color, uvTexCoords);
	m_verts[11] = Vertex_PCU(Vec3(1.0f, -1.0f, 0.0f), m_color, uvTexCoords);
}
