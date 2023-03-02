#pragma once
#include "Engine/Math/Plane2D.hpp"
#include <vector>

class ConvexHull2D {
public:
	ConvexHull2D() = default;
public:
	std::vector<Plane2D> m_planes;

};