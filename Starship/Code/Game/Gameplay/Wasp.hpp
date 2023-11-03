#pragma once
#include "Game/Gameplay/Entity.hpp"
constexpr int WASP_VERTEXES_AMOUNT = 27;
class Game;
class PlayerShip;

class Wasp :public Entity {
public:
	Wasp(Game* gamePointer, const Vec2& position);
	void Render() const;
	void Update(float deltaTime);
	void Die();
private:
	void InitializeLocalVerts();
	Vertex_PCU m_verts[WASP_VERTEXES_AMOUNT] = {};
}; 