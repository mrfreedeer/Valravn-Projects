#pragma once
#include "Engine/Math/Vec2.hpp"

struct FloatRange;

struct AABB2 {
	Vec2 m_mins;
	Vec2 m_maxs;

public:
	static AABB2 const ZERO_TO_ONE;

	AABB2() = default;
	AABB2(const AABB2& copyFrom) = default;
	explicit AABB2(float const minX, float const minY, float const maxX, float const maxY);
	explicit AABB2(const Vec2& newMinBounds, const Vec2& newMaxBounds);

	bool const IsPointInside(Vec2 const& point) const;
	Vec2 const GetCenter() const;
	Vec2 const GetDimensions() const;
	Vec2 const GetNearestPoint(Vec2 const& pointToCheck) const;
	Vec2 const GetPointAtUV(Vec2 const& uvPoint) const;
	Vec2 const GetPointAtUV(float x, float y) const;
	Vec2 const GetUVForPoint(Vec2 const& point) const;
	void Translate(Vec2 const& translation);
	void SetCenter(Vec2 const& newCenter);
	void SetDimensions(Vec2 const& newDimensions);
	void SetDimensions(float dimX, float dimY);
	void StretchToIncludePoint(Vec2 const& pointToInclude);
	void SetFromText(char const* text);

	void AlignABB2WithinBounds(AABB2& toAlign, Vec2 const& alignment = Vec2(0.5f, 0.5f)) const;
	AABB2 const GetBoxWithin(AABB2 const& boxUVs) const;
	AABB2 const GetBoxWithin(Vec2 const& minUVs, Vec2 const& maxUVs) const;
	AABB2 const GetBoxWithin(float minU, float minV, float maxU, float maxV) const;

	FloatRange const GetXRange() const;
	FloatRange const GetYRange() const;
};