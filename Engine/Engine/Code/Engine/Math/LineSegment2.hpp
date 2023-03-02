#pragma once
#include "Engine/Math/Vec2.hpp"

struct LineSegment2 {
	Vec2 m_start;
	Vec2 m_end;

	LineSegment2() {};
	LineSegment2(Vec2 const& lineStart, Vec2 const& lineEnd);

	void Translate(Vec2 const& translation);
	void SetCenter(Vec2 const& newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);

	Vec2 const GetNearestPoint(Vec2 const& refPoint) const;
};