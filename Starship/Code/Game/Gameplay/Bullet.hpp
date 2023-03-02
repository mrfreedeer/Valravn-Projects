#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

class Bullet : public Entity {
public:
	Bullet(Game* theGame, const Vec2& startingPosition);
	~Bullet();
	void Render() const;
	void Update(float deltaTime);
	Vertex_PCU m_verts[6] = {};
private:
	void InitializeLocalVerts();
};