#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

constexpr int ASTEROID_VERTEX_NUMBER = 48;
class Asteroid : public Entity {
public:
	Asteroid(Game* gamePointer, const Vec2& startingPosition);
	~Asteroid();

	void Render() const;
	void Update(float deltaTime) override;
	void WrapAroundScreen();
	void TakeAHit();
	void Die();
	
private:
	Vertex_PCU m_verts[ASTEROID_VERTEX_NUMBER] = {};
	void InitializeLocalVerts();
};