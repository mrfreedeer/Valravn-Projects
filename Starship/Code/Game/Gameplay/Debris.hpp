#pragma once
#include "Game/Gameplay/Entity.hpp"
constexpr int DEBRIS_VERTEX_AMOUNT = 15; 
constexpr float deltaAngle = 360.0f / DEBRIS_VERTEX_AMOUNT * 3;

class Debris : public Entity {
public:
	Debris(Game* gamePointer, const Vec2& startingPosition,const Rgba8& color, const Vec2& velocity, float scale, float speed);
	~Debris(){}

	void InitializeLocalVerts();
	void Update(float deltaTime);
	void UpdateAlpha();
	void Render() const;
private:
	Vertex_PCU m_verts[DEBRIS_VERTEX_AMOUNT];
	float timeAlive = 0.0f;
	float m_scale = 1.0f;
};