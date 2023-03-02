#include "Game/Gameplay/Rubble.hpp"

Rubble::Rubble(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type) :
	Entity(pointerToGame, startingPosition, orientation, faction, type, true)
{
	float m_minSize = g_gameConfigBlackboard.GetValue("MIN_RUBBLE_SIZE", 0.5f);
	float m_maxSize = g_gameConfigBlackboard.GetValue("MAX_RUBBLE_SIZE", 1.0f);

	m_size = rng.GetRandomFloatInRange(m_minSize, m_maxSize);

	if (m_type == EntityType::WALLRUBBLE) {
		m_texture = g_textures[GAME_TEXTURE::WALLRUBBLE];
	}
	else {
		m_texture = g_textures[GAME_TEXTURE::ENEMYRUBBLE];
		m_size = 1.0f;
	}
	m_physicsRadius = m_size * 0.3f;
	m_cosmeticsRadius = m_size * 0.4f;

	m_isProjectile = false;
	m_isPushedByEntities = false;
	m_pushesEntities = false;
	m_isHitByBullets = false;
}

Rubble::~Rubble()
{
}

void Rubble::Render() const
{
	AABB2 rubbleBox;
	rubbleBox.SetDimensions(Vec2(m_size, m_size));
	rubbleBox.SetCenter(m_position);

	g_theRenderer->BindTexture(m_texture);

	std::vector<Vertex_PCU> rubbleVerts;
	AddVertsForAABB2D(rubbleVerts, rubbleBox, Rgba8::WHITE);

	g_theRenderer->DrawVertexArray(rubbleVerts);

	if (g_drawDebug) {
		Entity::RenderDebug();
	}
}

void Rubble::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}
