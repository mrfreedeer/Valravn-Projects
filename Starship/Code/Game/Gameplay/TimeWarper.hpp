#pragma once

#include "Game/Gameplay/Entity.hpp"

constexpr int TIMEWARPER_VERTEX_AMOUNT = 27;

class TimeWarper : public Entity {
public:
	TimeWarper(Game* gamePointer, Vec2 const& startingPosition);
	~TimeWarper();

	void Update(float deltaSeconds);
	void Render() const;

private:
	void InitializeLocalVerts();

	Vertex_PCU m_verts[TIMEWARPER_VERTEX_AMOUNT] = {};
	float m_timeWarpingRadius;
	float m_timeAlive = 0.0f;
	bool m_slowsDownTime = false;
	bool m_speedsUpTime = false;
};