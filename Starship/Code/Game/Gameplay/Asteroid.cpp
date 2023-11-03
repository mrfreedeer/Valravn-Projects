#include "Asteroid.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"

Asteroid::Asteroid(Game* gamePointer, const Vec2& startingPosition):
	Entity(gamePointer, startingPosition)
{
	m_color = Rgba8(100, 100, 100, 255);
	InitializeLocalVerts();
	Vec2 randomVelocity(1.f, 0.f);
	float randomOrientation = rng.GetRandomFloatInRange(0.f, 360.f);
	
	randomVelocity.SetOrientationDegrees(randomOrientation);
	randomVelocity.SetLength(ASTEROID_SPEED);

	m_velocity = randomVelocity;
	m_angularVelocity = rng.GetRandomFloatInRange(-200.f, 200.f);
	m_physicsRadius = ASTEROID_PHYSICS_RADIUS;
	m_cosmeticRadius = ASTEROID_COSMETIC_RADIUS;
	m_health = 3;
}

Asteroid::~Asteroid()
{
}

void Asteroid::Render() const
{
	Vertex_PCU world_verts[ASTEROID_VERTEX_NUMBER] = {};
	for (int vertIndex = 0; vertIndex < ASTEROID_VERTEX_NUMBER; vertIndex++) {
		world_verts[vertIndex] = m_verts[vertIndex];
	}

	TransformVertexArrayXY3D(ASTEROID_VERTEX_NUMBER, world_verts, 1.f, m_orientationDegrees, m_position);

	g_theRenderer->DrawVertexArray(ASTEROID_VERTEX_NUMBER, world_verts);
}

void Asteroid::Update(float deltaTime) {
	m_position += m_velocity * deltaTime;
	m_orientationDegrees += m_angularVelocity * deltaTime;

	if (IsOffScreen()) {
		WrapAroundScreen();
	}
}


void Asteroid::WrapAroundScreen()
{

	if (m_position.x > WORLD_SIZE_X + (m_cosmeticRadius + WRAP_AROUND_SPACE)) {
		m_position.x  = - WRAP_AROUND_SPACE - m_cosmeticRadius;
	}
	if (m_position.x < - (m_cosmeticRadius + WRAP_AROUND_SPACE) ) {
		m_position.x = WORLD_SIZE_X + WRAP_AROUND_SPACE;
	}

	if (m_position.y > WORLD_SIZE_Y + (m_cosmeticRadius + WRAP_AROUND_SPACE)) {
		m_position.y = -WRAP_AROUND_SPACE - m_cosmeticRadius;
	}

	if (m_position.y < -(m_cosmeticRadius + WRAP_AROUND_SPACE)) {
		m_position.y = WORLD_SIZE_Y + WRAP_AROUND_SPACE;
	}

}

void Asteroid::TakeAHit()
{
	Entity::TakeAHit(Vec2());
	m_game->ShakeScreenCollision();
}

void Asteroid::Die()
{
	Entity::Die();
	SpawnDeathDebrisCluster();
}

void Asteroid::InitializeLocalVerts()
{
	float angle = 0; 
	for (int vertIndex = 0; vertIndex < ASTEROID_VERTEX_NUMBER; vertIndex+=3, angle += 22.5f) {
		float randRadiusBottom = rng.GetRandomFloatInRange(ASTEROID_PHYSICS_RADIUS, ASTEROID_COSMETIC_RADIUS);
		float randRadiusTop = rng.GetRandomFloatInRange(ASTEROID_PHYSICS_RADIUS, ASTEROID_COSMETIC_RADIUS);

		Vec2 bottomVert(CosDegrees(angle), SinDegrees(angle));
		Vec2 topVert(CosDegrees(angle + 22.5f), SinDegrees(angle + 22.5f));

		bottomVert *= randRadiusBottom;
		topVert *= randRadiusTop;

		m_verts[vertIndex] = Vertex_PCU(Vec3(), m_color, Vec2());
		m_verts[vertIndex + 1] = Vertex_PCU(Vec3(bottomVert.x, bottomVert.y, 0.f), m_color, Vec2());
		m_verts[vertIndex + 2] = Vertex_PCU(Vec3(topVert.x, topVert.y, 0.f), m_color, Vec2());
	}
}

