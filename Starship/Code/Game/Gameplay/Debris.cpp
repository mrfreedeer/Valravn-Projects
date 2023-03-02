#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Gameplay/Debris.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Framework/GameCommon.hpp"


Debris::Debris(Game* gamePointer, const Vec2& startingPosition, const Rgba8& color, const Vec2& velocity, float scale, float speed):
	Entity(gamePointer, startingPosition)
{
	m_color = color;
	m_velocity = velocity;
	m_scale = scale;
	m_velocity.SetLength(speed);
	m_physicsRadius = scale;
	m_cosmeticRadius = scale;
	m_spawnsEffects = false;
	InitializeLocalVerts();
}

void Debris::InitializeLocalVerts()
{
	float angle = 0;
	for (int vertIndex = 0; vertIndex < DEBRIS_VERTEX_AMOUNT; vertIndex += 3, angle += deltaAngle) {
		float randRadiusBottom = rng.GetRandomFloatInRange(0.2f, 1.0f);
		float randRadiusTop = rng.GetRandomFloatInRange(0.2f, 1.0f);

		Vec2 bottomVert(CosDegrees(angle), SinDegrees(angle));
		Vec2 topVert(CosDegrees(angle + deltaAngle), SinDegrees(angle + deltaAngle));

		bottomVert *= randRadiusBottom;
		topVert *= randRadiusTop;

		m_verts[vertIndex] = Vertex_PCU(Vec3(), m_color, Vec2());
		m_verts[vertIndex + 1] = Vertex_PCU(Vec3(bottomVert.x, bottomVert.y, 0.f), m_color, Vec2());
		m_verts[vertIndex + 2] = Vertex_PCU(Vec3(topVert.x, topVert.y, 0.f), m_color, Vec2());
	}
}

void Debris::Update(float deltaTime)
{
	Entity::Update(deltaTime);
	UpdateAlpha();
	timeAlive += deltaTime;

	if (timeAlive >= DEBRIS_LIFE_TIME || IsOffScreen()) {
		Die();
	}
}

void Debris::UpdateAlpha()
{
	if (m_color.a > 0) {
		float alphaPercentage = 1 - (timeAlive / DEBRIS_LIFE_TIME);
		m_color.a = static_cast<unsigned char>(128 * alphaPercentage);
	}
}

void Debris::Render() const
{
	Vertex_PCU world_verts[DEBRIS_VERTEX_AMOUNT] = {};
	for (int vertIndex = 0; vertIndex < DEBRIS_VERTEX_AMOUNT; vertIndex++) {
		world_verts[vertIndex] = m_verts[vertIndex];
		world_verts[vertIndex].m_color = m_color;
	}

	TransformVertexArrayXY3D(DEBRIS_VERTEX_AMOUNT, world_verts, m_scale, m_orientationDegrees, m_position);

	g_theRenderer->DrawVertexArray(DEBRIS_VERTEX_AMOUNT, world_verts);
}
