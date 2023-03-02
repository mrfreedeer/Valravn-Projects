#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

struct AABB2;
struct AABB3;
struct OBB2;
struct LineSegment2;
struct Plane2D;

class ConvexHull2D;
class ConvexPoly2D;

struct RaycastResult2D {
	Vec2 m_startPosition = Vec2::ZERO;
	Vec2 m_forwardNormal = Vec2::ZERO;
	Vec2 m_impactPos = Vec2::ZERO;
	Vec2 m_impactNormal = Vec2::ZERO;

	bool m_didImpact = false;
	bool m_maxDistanceReached = false;
	float m_maxDistance = 0.0f;
	float m_impactDist = 0.0f;
	float m_impactFraction = 0.0f;

};

struct RaycastResult3D {
	Vec3 m_startPosition = Vec3::ZERO;
	Vec3 m_forwardNormal = Vec3::ZERO;
	Vec3 m_impactPos = Vec3::ZERO;
	Vec3 m_impactNormal = Vec3::ZERO;

	float m_impactFraction = 0.0f;
	float m_impactDist = 0.0f;
	bool m_didImpact = false;
	float m_maxDistance = 0.0f;
	bool m_maxDistanceReached = false;
};

RaycastResult2D RaycastVsDisc(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsBox(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, AABB2 const& box);
RaycastResult2D RaycastVsOBB2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, OBB2 const& box);
RaycastResult2D RaycastVsLineSegment2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, LineSegment2 const& lineSegment);
RaycastResult2D RaycastVsPlane(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, Plane2D const& plane);
RaycastResult2D RaycastVsConvexHull2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, ConvexHull2D const& convexPolyAsHull, float tolerance = 0.025f);
RaycastResult2D RaycastVsConvexHull2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, ConvexPoly2D const& convexPoly, float tolerance = 0.025f);


RaycastResult3D RaycastVsSphere(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& sphereCenter, float sphereRadius);
RaycastResult3D RaycastVsZCylinder(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& cylinderBase, float cylinderRadius, float cylinderHeight);
RaycastResult3D RaycastVsBox3D(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, AABB3 const& box);
RaycastResult3D RaycastVsLineSegment3D(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& lineStart, Vec3 const& lineEnd, float tolerance = 0.025f);

// This is an unbound raycast vs raycast. If at any point in the infinite, these to rays impact (within tolerance) this will result in hit
RaycastResult3D RaycastVsRaycast3D(Vec3 const& rayStartA, Vec3 const& rayForwardA, Vec3 const& rayStartB, Vec3 const& rayForwardB, float tolerance = 0.025f);

// Planes are infinite, therefore, any extra discard logic must be done outside of this function, maybe #TODO add raycast vs specific plane sections
RaycastResult3D RaycastVsPlane(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& pointOnplane, Vec3 const& planeNormal, float tolerance = 0.025f);
