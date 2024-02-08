#pragma once
#include "Game/Gameplay/Entity.hpp"

constexpr int AMOUNT_VERTS_SHIELD = 12;

class PickUp: public Entity {
public:
	PickUp(Game* gamePointer, Vec2 startingPosition, bool hasRandomVelocity = false);

	void Update(float deltaSeconds);
	void Render() const;
	void Die();

private:
	void InitializeLocalVerts();

	Vertex_PCU m_verts[AMOUNT_VERTS_SHIELD] = {};
};