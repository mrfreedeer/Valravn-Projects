#pragma once
#include "Engine/Math/Vec3.hpp"

struct FloatRange;
struct AABB2;

struct AABB3 {
	Vec3 m_mins;
	Vec3 m_maxs;

public:
	static AABB3 const ZERO_TO_ONE;

	AABB3() = default;
	AABB3(const AABB3& copyFrom) = default;
	explicit AABB3(float const minX, float const minY, float const minZ, float const maxX, float const maxY, float const maxZ);
	explicit AABB3(const Vec3& newMinBounds, const Vec3& newMaxBounds);

	// Getters (const)
	bool const IsPointInside(Vec3 const& point) const;
	Vec3 const GetCenter() const;
	Vec3 const GetDimensions() const;
	Vec3 const GetNearestPoint(Vec3 const& pointToCheck) const;
	void GetCorners(Vec3* cornersOut) const;

	// Setters and Mutators
	void Translate(Vec3 const& translation);
	void SetCenter(Vec3 const& newCenter);
	void SetDimensions(Vec3 const& newDimensions);
	void SetDimensions(float dimX, float dimY, float dimZ);
	void StretchToIncludePoint(Vec3 const& pointToInclude);
	void SetFromText(char const* text);

	FloatRange const GetXRange() const;
	FloatRange const GetYRange() const;
	FloatRange const GetZRange() const;

	AABB2 const GetXYBounds() const;
	AABB2 const GetXZBounds() const;
	AABB2 const GetYZBounds() const;

	//void AlignABB3WithinBounds(AABB3& toAlign, Vec3 const& alignment = Vec3(0.5f, 0.5f, 0.5f)) const;
};