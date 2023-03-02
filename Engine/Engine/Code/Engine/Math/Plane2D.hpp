#pragma once
#include "Engine/Math/Vec2.hpp"

struct Plane2D
{
	Vec2 m_planeNormal = Vec2(0.0f, 0.0f);
	float m_distToPlane = 0.0f;
};