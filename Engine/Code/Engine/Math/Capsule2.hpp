#pragma once
#include "Engine/Math/LineSegment2.hpp"

struct Capsule2 {
	Capsule2() = default;
	Capsule2(Vec2 const& boneStart, Vec2 const& boneEnd, float radius);
	LineSegment2 m_bone;

	float m_radius;

	void Translate(Vec2 const& translation);
	void SetCenter(Vec2 const& newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);

	bool IsPointInside(Vec2 const& refPoint) const;
	Vec2 const GetNearestPoint(Vec2 const& refPoint) const;
};