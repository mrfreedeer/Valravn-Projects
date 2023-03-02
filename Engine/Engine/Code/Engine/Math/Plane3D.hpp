#pragma once
#include "Engine/Math/Vec3.hpp"

struct Plane3D
{
	Vec3 m_planeNormal = Vec3(0.0f, 0.0f, 0.0f);
	float m_distToPlane = 0.0f;
};