#include "Bullet.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Framework/GameCommon.hpp"

Bullet::Bullet(Game* theGame, const Vec2& startingPosition) :
	Entity(theGame, startingPosition)
{
	InitializeLocalVerts();
	m_physicsRadius = BULLET_PHYSICS_RADIUS;
	m_cosmeticRadius = BULLET_COSMETIC_RADIUS;

	m_velocity = Vec2(BULLET_SPEED, 0.f);
	m_spawnsEffects = false;

}

Bullet::~Bullet()
{
	Entity::~Entity();
}

void Bullet::Render() const
{
	Vertex_PCU world_verts[6] = {};
	for (int vertIndex = 0; vertIndex < 6; vertIndex++) {
		world_verts[vertIndex] = m_verts[vertIndex];
	}

	TransformVertexArrayXY3D(6, world_verts, 1.f, m_orientationDegrees, m_position);

	g_theRenderer->DrawVertexArray(6, world_verts);
}

void Bullet::Update(float deltaTime) {
	Entity::Update(deltaTime);
}

void Bullet::InitializeLocalVerts()
{
	Rgba8 yellow(255, 255, 0, 255);
	m_verts[0] = Vertex_PCU(Vec3(0.f, -.5f, 0.f), yellow, Vec2());
	m_verts[1] = Vertex_PCU(Vec3(.5f, 0.f, 0.f), yellow, Vec2());
	m_verts[2] = Vertex_PCU(Vec3(0.f, .5f, 0.f), yellow, Vec2());

	Rgba8 red(255, 0, 0, 255);
	Rgba8 transparentRed(255, 0, 0, 0);

	m_verts[3] = Vertex_PCU(Vec3(0.f, 0.5f, 0.f), red, Vec2());
	m_verts[4] = Vertex_PCU(Vec3(-2.f, 0.f, 0.f), transparentRed, Vec2());
	m_verts[5] = Vertex_PCU(Vec3(0.f, -.5f, 0.f), red, Vec2());
}
