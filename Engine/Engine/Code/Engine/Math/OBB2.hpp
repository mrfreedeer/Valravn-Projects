#pragma once
#include "Engine/Math/Vec2.hpp"

struct OBB2 {
	OBB2() {};
	OBB2(Vec2 const& center, Vec2 const& halfDims);

	Vec2 m_center = Vec2::ZERO;
	Vec2 m_iBasisNormal = Vec2(1.0f, 0.0f);
	Vec2 m_halfDimensions = Vec2(0.5f, 0.5f);

	void GetCornerPoints(Vec2*& cornerPoints) const;
	Vec2* const GetCornerPoints() const;
	Vec2 const GetLocalPosForWorldPos(Vec2 const& worldPos) const;
	Vec2 const GetWorldPosForLocalPos(Vec2 const& localPos) const;
	Vec2 const ConvertDisplacementToLocalSpace(Vec2 const& worldDir) const;
	Vec2 const ConvertLocalDisplacementToWorldSpace(Vec2 const& localDir)const;

	bool IsPointInside(Vec2 const& refPoint) const;
	Vec2 const GetNearestPoint(Vec2 const& refPoint) const;

	void RotateAboutCenter(float rotationDeltaDegrees);
};