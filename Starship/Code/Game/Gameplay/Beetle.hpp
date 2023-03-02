#pragma once
#include "Game/Gameplay/Entity.hpp"
constexpr int BEETLE_VERTEXES_AMOUNT = 18;
class Game;
class PlayerShip;

class Beetle:public Entity {
public:
	Beetle(Game* gamePointer, const Vec2& position);
	void Render() const;
	void Update(float deltaTime);
	void Die();
private:
	void InitializeLocalVerts();
	Vertex_PCU m_verts[BEETLE_VERTEXES_AMOUNT] = {};
};