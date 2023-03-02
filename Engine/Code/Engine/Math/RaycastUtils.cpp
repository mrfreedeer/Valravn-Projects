#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Plane2D.hpp"
#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <algorithm>


RaycastResult2D RaycastVsDisc(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, Vec2 const& discCenter, float discRadius)
{
	RaycastResult2D result;
	result.m_forwardNormal = rayForward;
	result.m_maxDistance = maxDistance;
	result.m_startPosition = rayStart;

	Vec2 dispToDisc = discCenter - rayStart;
	Vec2 rayLeft = rayForward.GetRotated90Degrees();

	float dispToDiscX = DotProduct2D(dispToDisc, rayForward);
	float dispToDiscY = DotProduct2D(dispToDisc, rayLeft);

	if (dispToDiscY >= discRadius || dispToDiscY <= -discRadius) return result;
	if (dispToDiscX >= maxDistance + discRadius) return result;
	if (dispToDiscX <= -discRadius) return result;

	bool isRayStartInsideDisc = IsPointInsideDisc2D(rayStart, discCenter, discRadius);
	bool isRayEndInsideDisc = IsPointInsideDisc2D(rayStart + rayForward * maxDistance, discCenter, discRadius);
	bool isDistanceToDiscLessThanRadius = (dispToDiscY <= discRadius) && (dispToDiscY >= -discRadius);


	if (isRayStartInsideDisc || isRayEndInsideDisc || isDistanceToDiscLessThanRadius) { // Did impact

		LineSegment2 rayAsLine(rayStart, rayStart + rayForward * maxDistance);
		Vec2 nearestPointOnRay = GetNearestPointOnLineSegment2D(discCenter, rayAsLine);

		if (IsPointInsideDisc2D(nearestPointOnRay, discCenter, discRadius)) {
			result.m_didImpact = true;
			if (isRayStartInsideDisc) {
				result.m_impactPos = rayStart;
				result.m_impactDist = 0.0f;
				result.m_impactNormal = -rayForward;
			}
			else {
				float distanceToHitOnDiscSqr = (discRadius * discRadius) - (dispToDiscY * dispToDiscY);
				float distanceToHit = fabsf(dispToDiscX) - sqrtf(fabsf(distanceToHitOnDiscSqr));
				Vec2 impactPoint = rayStart + rayForward * fabsf(distanceToHit);

				result.m_impactPos = impactPoint;
				result.m_impactDist = distanceToHit;
				result.m_impactNormal = (result.m_impactPos - discCenter).GetNormalized();
			}


		}

	}

	return result;
}



RaycastResult3D RaycastVsSphere(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& sphereCenter, float sphereRadius)
{
	RaycastResult3D result;
	result.m_forwardNormal = rayForward;
	result.m_maxDistance = maxDistance;
	result.m_startPosition = rayStart;

	Vec3 dispToCenter = sphereCenter - rayStart;

	float fwdDistanceToCenter = DotProduct3D(dispToCenter, rayForward);

	FloatRange sphereWithinRayRange(-sphereRadius, maxDistance + sphereRadius);

	bool isDistanceToSphereLessThanRadius = sphereWithinRayRange.IsOnRange(fwdDistanceToCenter);

	if (!isDistanceToSphereLessThanRadius) return result;

	float distanceToCenter = dispToCenter.GetLength();
	float heightToCenter = sqrtf((distanceToCenter * distanceToCenter) - (fwdDistanceToCenter * fwdDistanceToCenter));

	FloatRange sphereRadiusRange(-sphereRadius, sphereRadius);

	bool isHeightToCenterLessThanRadius = sphereRadiusRange.IsOnRange(heightToCenter);
	if (!isHeightToCenterLessThanRadius) return result;

	if (IsPointInsideSphere(rayStart, sphereCenter, sphereRadius)) {
		result.m_didImpact = true;
		result.m_impactNormal = rayForward;
		result.m_impactPos = rayStart;

		return result;
	}

	float distanceToImpact = fwdDistanceToCenter - sqrtf((sphereRadius * sphereRadius) - (heightToCenter * heightToCenter));
	FloatRange allowedLengthRange(0.0f, maxDistance);
	if (!allowedLengthRange.IsOnRange(distanceToImpact)) {
		result.m_maxDistanceReached = true;
		return result;
	}

	Vec3 impactPos = rayStart + (rayForward * distanceToImpact);
	Vec3 impactNormal = (impactPos - sphereCenter).GetNormalized();

	result.m_didImpact = true;
	result.m_impactDist = distanceToImpact;
	result.m_impactFraction = distanceToImpact / maxDistance;
	result.m_impactNormal = impactNormal;
	result.m_impactPos = impactPos;

	return result;
}

RaycastResult2D RaycastVsBox(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, AABB2 const& box)
{
	RaycastResult2D raycastResult;

	raycastResult.m_startPosition = rayStart;
	raycastResult.m_maxDistance = maxDistance;

	if (box.IsPointInside(rayStart)) {
		raycastResult.m_didImpact = true;
		raycastResult.m_forwardNormal = rayForward;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactDist = 0.0f;

		return raycastResult;
	}

	float const& left = box.m_mins.x;
	float const& bottom = box.m_mins.y;
	float const& right = box.m_maxs.x;
	float const& top = box.m_maxs.y;


	float rayLeftXEntry = (left - rayStart.x) / rayForward.x;
	float rayBottomYEntry = (bottom - rayStart.y) / rayForward.y;

	float rayRightXEntry = (right - rayStart.x) / rayForward.x;
	float rayTopYEntry = (top - rayStart.y) / rayForward.y;

	float rayXMin = (rayLeftXEntry < rayRightXEntry) ? rayLeftXEntry : rayRightXEntry;
	float rayXMax = (rayLeftXEntry > rayRightXEntry) ? rayLeftXEntry : rayRightXEntry;

	float rayYMin = (rayBottomYEntry < rayTopYEntry) ? rayBottomYEntry : rayTopYEntry;
	float rayYMax = (rayBottomYEntry > rayTopYEntry) ? rayBottomYEntry : rayTopYEntry;

	FloatRange xFloatRange(rayXMin, rayXMax);
	FloatRange yFloatRange(rayYMin, rayYMax);
	FloatRange allowedRange(0.0f, maxDistance);

	bool isOverlapping = xFloatRange.IsOverlappingWith(yFloatRange);
	if (isOverlapping) {
		FloatRange overlappingRange = xFloatRange.GetOverlappingRange(yFloatRange);
		if (overlappingRange.IsOverlappingWith(allowedRange)) {
			raycastResult.m_didImpact = true;
			raycastResult.m_impactDist = overlappingRange.m_min;
			raycastResult.m_impactPos = rayForward * overlappingRange.m_min + rayStart;

			if (xFloatRange.m_min > yFloatRange.m_min) {
				if (rayForward.x < 0) {
					raycastResult.m_impactNormal = Vec2(1.0f, 0.0f);
				}
				else {
					raycastResult.m_impactNormal = Vec2(-1.0f, 0.0f);
				}
			}
			else {
				if (rayForward.y < 0) {
					raycastResult.m_impactNormal = Vec2(0.0f, 1.0f);
				}
				else {
					raycastResult.m_impactNormal = Vec2(0.0f, -1.0f);
				}
			}
		}
	}
	else {
		raycastResult.m_maxDistanceReached = true;
	}

	return raycastResult;
}

RaycastResult2D RaycastVsOBB2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, OBB2 const& orientedBox)
{
	RaycastResult2D raycastResult;

	raycastResult.m_startPosition = rayStart;
	raycastResult.m_maxDistance = maxDistance;

	if (orientedBox.IsPointInside(rayStart)) {
		raycastResult.m_didImpact = true;
		raycastResult.m_forwardNormal = rayForward;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactDist = 0.0f;

		return raycastResult;
	}

	float const& left = -orientedBox.m_halfDimensions.x;
	float const& bottom = -orientedBox.m_halfDimensions.y;
	float const& right = orientedBox.m_halfDimensions.x;
	float const& top = orientedBox.m_halfDimensions.y;

	Vec2 rayFwdLocalSpace = orientedBox.ConvertDisplacementToLocalSpace(rayForward);
	Vec2 rayStartLocalSpace = orientedBox.GetLocalPosForWorldPos(rayStart);

	float rayLeftXEntry = (left - rayStartLocalSpace.x) / rayFwdLocalSpace.x;
	float rayBottomYEntry = (bottom - rayStartLocalSpace.y) / rayFwdLocalSpace.y;

	float rayRightXEntry = (right - rayStartLocalSpace.x) / rayFwdLocalSpace.x;
	float rayTopYEntry = (top - rayStartLocalSpace.y) / rayFwdLocalSpace.y;

	float rayXMin = (rayLeftXEntry < rayRightXEntry) ? rayLeftXEntry : rayRightXEntry;
	float rayXMax = (rayLeftXEntry > rayRightXEntry) ? rayLeftXEntry : rayRightXEntry;

	float rayYMin = (rayBottomYEntry < rayTopYEntry) ? rayBottomYEntry : rayTopYEntry;
	float rayYMax = (rayBottomYEntry > rayTopYEntry) ? rayBottomYEntry : rayTopYEntry;

	FloatRange xFloatRange(rayXMin, rayXMax);
	FloatRange yFloatRange(rayYMin, rayYMax);
	FloatRange allowedRange(0.0f, maxDistance);

	bool isOverlapping = xFloatRange.IsOverlappingWith(yFloatRange);
	if (isOverlapping) {
		FloatRange overlappingRange = xFloatRange.GetOverlappingRange(yFloatRange);
		if (overlappingRange.IsOverlappingWith(allowedRange)) {
			raycastResult.m_didImpact = true;
			raycastResult.m_impactDist = overlappingRange.m_min;
			Vec2 localImpactPos = (rayFwdLocalSpace * overlappingRange.m_min) + rayStartLocalSpace;
			raycastResult.m_impactPos = orientedBox.GetWorldPosForLocalPos(localImpactPos);

			if (xFloatRange.m_min > yFloatRange.m_min) {
				if (rayForward.x < 0) {
					raycastResult.m_impactNormal = Vec2(1.0f, 0.0f);
				}
				else {
					raycastResult.m_impactNormal = Vec2(-1.0f, 0.0f);
				}
			}
			else {
				if (rayForward.y < 0) {
					raycastResult.m_impactNormal = Vec2(0.0f, 1.0f);
				}
				else {
					raycastResult.m_impactNormal = Vec2(0.0f, -1.0f);
				}
			}
			raycastResult.m_impactNormal = orientedBox.ConvertLocalDisplacementToWorldSpace(raycastResult.m_impactNormal);
		}
	}
	else {
		raycastResult.m_maxDistanceReached = true;
	}

	return raycastResult;
}

RaycastResult2D RaycastVsLineSegment2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, LineSegment2 const& lineSegment)
{
	RaycastResult2D raycastResult = {};
	raycastResult.m_startPosition = rayStart;
	raycastResult.m_forwardNormal = rayForward;
	raycastResult.m_maxDistance = maxDistance;

	Vec2 jFwd = rayForward.GetRotated90Degrees();
	Vec2 dispRayToLineSegmentStart = (lineSegment.m_start - rayStart);
	Vec2 dispRayToLineSegmentEnd = (lineSegment.m_end - rayStart);
	Vec2 dispRaySegment = lineSegment.m_end - lineSegment.m_start;

	float dispRayLineStartJ = DotProduct2D(dispRayToLineSegmentStart, jFwd);
	float dispRayLineEndJ = DotProduct2D(dispRayToLineSegmentEnd, jFwd);

	if (dispRayLineEndJ * dispRayLineStartJ >= 0) return raycastResult;

	float dispRayLineStartI = DotProduct2D(dispRayToLineSegmentStart, rayForward);
	float dispRayLineEndI = DotProduct2D(dispRayToLineSegmentEnd, rayForward);

	if (dispRayLineStartI < 0.0f && dispRayLineEndI < 0.0f) return raycastResult;

	if (dispRayLineStartI > maxDistance && dispRayLineEndI > maxDistance)
	{
		raycastResult.m_maxDistanceReached = true;
		return raycastResult;
	}

	float hitFraction = (dispRayLineStartJ) / (dispRayLineStartJ - dispRayLineEndJ);
	Vec2 impactPos = lineSegment.m_start + (hitFraction * dispRaySegment);
	Vec2 displacementToImpact = impactPos - rayStart;
	float impactDist = DotProduct2D(displacementToImpact, rayForward);

	if (impactDist < 0.0f) return raycastResult;
	if (impactDist > maxDistance) {
		raycastResult.m_maxDistanceReached = true;
		return raycastResult;
	}

	Vec2 impactNormal = rayForward.GetRotated90Degrees().GetNormalized();

	if (dispRayLineStartI > 0.0f) impactNormal = -impactNormal;

	raycastResult.m_didImpact = true;
	raycastResult.m_impactNormal = impactNormal;
	raycastResult.m_impactDist = impactDist;
	raycastResult.m_impactFraction = hitFraction;
	raycastResult.m_impactPos = impactPos;

	return raycastResult;
}

RaycastResult2D RaycastVsPlane(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, Plane2D const& plane)
{
	RaycastResult2D raycastResult;
	raycastResult.m_startPosition = rayStart;
	raycastResult.m_forwardNormal = rayForward;
	raycastResult.m_maxDistance = maxDistance;

	Vec2 endOfRay = (rayStart + rayForward * maxDistance);

	float localRayAltitude = DotProduct2D(rayStart, plane.m_planeNormal) - plane.m_distToPlane;
	float localEndRayAltitude = DotProduct2D(endOfRay, plane.m_planeNormal) - plane.m_distToPlane;

	if (localEndRayAltitude * localRayAltitude > 0.0f) return raycastResult;

	float forwardInPlaneDir = DotProduct2D(rayForward, plane.m_planeNormal);

	float impactDist = -localRayAltitude / forwardInPlaneDir;
	if (impactDist > maxDistance) {
		raycastResult.m_maxDistanceReached = true;
		return raycastResult;
	}

	if (impactDist < 0.0f) return raycastResult;

	raycastResult.m_impactNormal = (DotProduct2D(rayForward, plane.m_planeNormal) > 0.0f) ? -plane.m_planeNormal : plane.m_planeNormal;
	raycastResult.m_didImpact = true;
	raycastResult.m_impactDist = impactDist;
	raycastResult.m_impactPos = rayStart + rayForward * impactDist;
	raycastResult.m_impactFraction = impactDist / maxDistance;

	return raycastResult;
}

RaycastResult2D RaycastVsConvexHull2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, ConvexPoly2D const& convexPoly, float tolerance)
{
	return RaycastVsConvexHull2D(rayStart, rayForward, maxDistance, convexPoly.GetConvexHull(), tolerance);
}

RaycastResult2D RaycastVsConvexHull2D(Vec2 const& rayStart, Vec2 const& rayForward, float maxDistance, ConvexHull2D const& convexPolyAsHull, float tolerance)
{
	RaycastResult2D defaultResult;
	defaultResult.m_startPosition = rayStart;
	defaultResult.m_forwardNormal = rayForward;
	defaultResult.m_maxDistance = maxDistance;

	std::vector<RaycastResult2D> hitPlanesResults;

	bool isRayStartInside = IsPointInsideConvexHull2D(rayStart, convexPolyAsHull, tolerance);
	if (isRayStartInside) {
		defaultResult.m_didImpact = true;
		defaultResult.m_impactDist = 0.0f;
		defaultResult.m_impactPos = rayStart;
		defaultResult.m_impactNormal = -rayForward;

		return defaultResult;
	}
	float maxHitDistance = FLT_MIN;
	RaycastResult2D lastexit;

	for (Plane2D const& plane : convexPolyAsHull.m_planes) {
		if(DotProduct2D(plane.m_planeNormal, rayForward) > 0) continue;
		RaycastResult2D raycastVsPlane = RaycastVsPlane(rayStart, rayForward, maxDistance, plane);
		if (raycastVsPlane.m_didImpact) {
			if (raycastVsPlane.m_impactDist > maxHitDistance) {
				lastexit = raycastVsPlane;
				maxHitDistance = raycastVsPlane.m_impactDist;
			}
		}
	}

	if (IsPointInsideConvexHull2D(lastexit.m_impactPos, convexPolyAsHull, tolerance)) {
		return lastexit;
	}


	return defaultResult;
}


RaycastResult3D RaycastVsZCylinder(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& cylinderBase, float cylinderRadius, float cylinderHeight)
{
	RaycastResult3D raycastResult = {};
	raycastResult.m_startPosition = rayStart;
	raycastResult.m_forwardNormal = rayForward;
	raycastResult.m_maxDistance = maxDistance;

	Vec2 xyForward = Vec2(rayForward.x, rayForward.y);
	xyForward.Normalize();
	Vec2 rayStart2D = Vec2(rayStart.x, rayStart.y);
	Vec2 discStart = Vec2(cylinderBase.x, cylinderBase.y);
	Vec3 rayEnd = rayStart + rayForward * maxDistance;
	float rayHeight = rayEnd.z - rayStart.z;
	float cylinderTopZ = cylinderBase.z + cylinderHeight;

	RaycastResult2D raycastVsCylinder2D = RaycastVsDisc(rayStart2D, xyForward, maxDistance, discStart, cylinderRadius);

	if (raycastVsCylinder2D.m_didImpact) {

		float maxXYLength = (Vec2(rayEnd.x, rayEnd.y) - rayStart2D).GetLength();

		float rayLengthAtHit = (maxDistance * raycastVsCylinder2D.m_impactDist) / maxXYLength;

		Vec3 hitPos = rayStart + rayForward * rayLengthAtHit;

		FloatRange cylinderAllowedRange(cylinderBase.z, cylinderTopZ);

		if (cylinderAllowedRange.IsOnRange(rayStart.z) && IsPointInsideDisc2D(rayStart2D, discStart, cylinderRadius)) {
			raycastResult.m_didImpact = true;
			raycastResult.m_impactPos = rayStart;
			raycastResult.m_impactDist = 0.0f;
			raycastResult.m_impactNormal = rayForward;

			return raycastResult;
		}

		if (cylinderAllowedRange.IsOnRange(hitPos.z)) {
			raycastResult.m_didImpact = true;
			raycastResult.m_impactPos = hitPos;
			raycastResult.m_impactDist = GetDistance3D(rayStart, hitPos);
			raycastResult.m_impactFraction = raycastResult.m_impactDist / maxDistance;
			raycastResult.m_impactNormal = Vec3(hitPos.x - cylinderBase.x, hitPos.y - cylinderBase.y, 0.0f).GetNormalized();
			return raycastResult;
		}

		if ((rayStart.z > cylinderTopZ) && rayForward.z < 0.0f) {
			float distToTop = rayStart.z - cylinderTopZ;
			float xyDistToHit = (maxXYLength * distToTop) / rayHeight;
			rayLengthAtHit = (distToTop * distToTop) + (xyDistToHit * xyDistToHit);
			rayLengthAtHit = sqrtf(rayLengthAtHit);
			hitPos = rayStart + rayForward * rayLengthAtHit;
			if (!IsPointInsideDisc2D(Vec2(hitPos.x, hitPos.y), Vec2(cylinderBase.x, cylinderBase.y), cylinderRadius)) return raycastResult;
			raycastResult.m_didImpact = true;
			raycastResult.m_impactPos = hitPos;
			raycastResult.m_impactDist = rayLengthAtHit;
			raycastResult.m_impactFraction = raycastResult.m_impactDist / maxDistance;
			raycastResult.m_impactNormal = Vec3(0.0f, 0.0f, 1.0f);

			return raycastResult;
		}

		if ((rayStart.z < cylinderBase.z) && rayForward.z > 0.0f) {
			float distToTop = cylinderBase.z - rayStart.z;
			float xyDistToHit = (maxXYLength * distToTop) / rayHeight;
			rayLengthAtHit = (distToTop * distToTop) + (xyDistToHit * xyDistToHit);
			rayLengthAtHit = sqrtf(rayLengthAtHit);
			hitPos = rayStart + rayForward * rayLengthAtHit;
			if (!IsPointInsideDisc2D(Vec2(hitPos.x, hitPos.y), Vec2(cylinderBase.x, cylinderBase.y), cylinderRadius)) return raycastResult;
			raycastResult.m_didImpact = true;
			raycastResult.m_impactPos = hitPos;
			raycastResult.m_impactDist = rayLengthAtHit;
			raycastResult.m_impactFraction = raycastResult.m_impactDist / maxDistance;
			raycastResult.m_impactNormal = Vec3(0.0f, 0.0f, -1.0f);

			return raycastResult;
		}

	}

	return raycastResult;
}

RaycastResult3D RaycastVsBox3D(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, AABB3 const& box)
{
	RaycastResult3D raycastResult;

	raycastResult.m_startPosition = rayStart;
	raycastResult.m_maxDistance = maxDistance;

	if (box.IsPointInside(rayStart)) {
		raycastResult.m_didImpact = true;
		raycastResult.m_forwardNormal = rayForward;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactDist = 0.0f;

		return raycastResult;
	}

	float const& left = box.m_mins.x;
	float const& bottom = box.m_mins.y;
	float const& right = box.m_maxs.x;
	float const& top = box.m_maxs.y;
	float const& nearZ = box.m_mins.z;
	float const& farZ = box.m_maxs.z;

	float rayLeftXEntry = (left - rayStart.x) / rayForward.x;
	float rayBottomYEntry = (bottom - rayStart.y) / rayForward.y;
	float rayBottomZEntry = (nearZ - rayStart.z) / rayForward.z;

	float rayRightXEntry = (right - rayStart.x) / rayForward.x;
	float rayTopYEntry = (top - rayStart.y) / rayForward.y;
	float rayTopZEntry = (farZ - rayStart.z) / rayForward.z;

	float rayXMin = (rayLeftXEntry < rayRightXEntry) ? rayLeftXEntry : rayRightXEntry;
	float rayXMax = (rayLeftXEntry > rayRightXEntry) ? rayLeftXEntry : rayRightXEntry;

	float rayYMin = (rayBottomYEntry < rayTopYEntry) ? rayBottomYEntry : rayTopYEntry;
	float rayYMax = (rayBottomYEntry > rayTopYEntry) ? rayBottomYEntry : rayTopYEntry;

	float rayZMin = (rayBottomZEntry < rayTopZEntry) ? rayBottomZEntry : rayTopZEntry;
	float rayZMax = (rayBottomZEntry > rayTopZEntry) ? rayBottomZEntry : rayTopZEntry;

	FloatRange xFloatRange(rayXMin, rayXMax);
	FloatRange yFloatRange(rayYMin, rayYMax);
	FloatRange zFloatRange(rayZMin, rayZMax);
	FloatRange allowedRange(0.0f, maxDistance);

	bool isOverlapping = xFloatRange.IsOverlappingWith(yFloatRange) && xFloatRange.IsOverlappingWith(zFloatRange) && zFloatRange.IsOverlappingWith(yFloatRange);
	if (isOverlapping) {
		FloatRange overlappingRange2D = xFloatRange.GetOverlappingRange(yFloatRange);
		FloatRange overlappingRange = overlappingRange2D.GetOverlappingRange(zFloatRange);
		if (overlappingRange.IsOverlappingWith(allowedRange)) {
			raycastResult.m_didImpact = true;
			raycastResult.m_impactDist = overlappingRange.m_min;
			raycastResult.m_impactPos = rayForward * overlappingRange.m_min + rayStart;

			if (overlappingRange.m_min == xFloatRange.m_min) {
				raycastResult.m_impactNormal = (rayForward.x < 0) ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(-1.0f, 0.0f, 0.0f);
			}
			else if (overlappingRange.m_min == yFloatRange.m_min) {
				raycastResult.m_impactNormal = (rayForward.y < 0) ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(0.0f, -1.0f, 0.0f);
			}
			else {
				raycastResult.m_impactNormal = (rayForward.z < 0) ? Vec3(0.0f, 0.0f, 1.0f) : Vec3(0.0f, 0.0f, -1.0f);
			}
		}
	}
	else {
		raycastResult.m_maxDistanceReached = true;
	}

	return raycastResult;
}

RaycastResult3D RaycastVsLineSegment3D(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& lineStart, Vec3 const& lineEnd, float tolerance)
{
	RaycastResult3D raycastResult = {};
	raycastResult.m_startPosition = rayStart;
	raycastResult.m_forwardNormal = rayForward;
	raycastResult.m_maxDistance = maxDistance;

	// Getting displacement for expressing in parametric terms
	Vec3 rayDisp = rayForward * maxDistance;
	Vec3 lineDisp = lineEnd - lineStart;
	Vec3 dispBetStarts = rayStart - lineStart;

	float toleranceSqr = tolerance * tolerance;

	if (lineDisp.GetLengthSquared() < (toleranceSqr)) return raycastResult;

	if (rayDisp.GetLengthSquared() < (toleranceSqr)) return raycastResult;

	// The shortest distance between two lines is another line, perpendicular to both.
	// Therefore, Dotproduct(ray, normal) DotProduct(line, normal) both have to = 0

	// This is the resulting after solving those equations

	float a = DotProduct3D(lineDisp, dispBetStarts);
	float b = DotProduct3D(lineDisp, rayDisp);
	float c = DotProduct3D(dispBetStarts, rayDisp);
	float d = DotProduct3D(lineDisp, lineDisp);
	float e = DotProduct3D(rayDisp, rayDisp);

	float f = (e * d) - (b * b);


	if (fabsf(f) < toleranceSqr) return raycastResult;

	float g = (a * b) - (c * d);

	float t = g / f; // Parametric value for closest point to other line

	if (t < -toleranceSqr || t > 1.0f + toleranceSqr) {
		raycastResult.m_maxDistanceReached = true;
		return raycastResult;
	}

	float s = (a + (b * t)) / d; // Parametric value for closest point to other line
	if (s < -toleranceSqr || s > 1.0f + toleranceSqr) return raycastResult;

	Vec3 resultingPointRay = rayStart + (t * rayDisp);
	Vec3 resultingPoinLine = lineStart + (s * lineDisp);


	float distBetPoints = GetDistanceSquared3D(resultingPointRay, resultingPoinLine);

	if (distBetPoints > toleranceSqr) return raycastResult;

	raycastResult.m_didImpact = true;
	raycastResult.m_impactPos = resultingPoinLine;
	raycastResult.m_impactFraction = t;
	raycastResult.m_impactNormal = CrossProduct3D(lineDisp, rayDisp).GetNormalized();


	return raycastResult;

}

RaycastResult3D RaycastVsRaycast3D(Vec3 const& rayStartA, Vec3 const& rayForwardA, Vec3 const& rayStartB, Vec3 const& rayForwardB, float tolerance)
{
	RaycastResult3D raycastResult = {};
	raycastResult.m_startPosition = rayStartA;
	raycastResult.m_forwardNormal = rayForwardA;
	raycastResult.m_maxDistance = FLT_MAX;

	// Getting displacement for expressing in parametric terms
	Vec3 dispBetStarts = rayStartA - rayStartB;

	float toleranceSqr = tolerance * tolerance;

	if (rayForwardB.GetLengthSquared() < (toleranceSqr)) return raycastResult;

	if (rayForwardA.GetLengthSquared() < (toleranceSqr)) return raycastResult;

	// The shortest distance between two lines is another line, perpendicular to both.
	// Therefore, Dotproduct(ray, normal) DotProduct(line, normal) both have to = 0

	// This is the resulting after solving those equations

	float a = DotProduct3D(rayForwardB, dispBetStarts);
	float b = DotProduct3D(rayForwardB, rayForwardA);
	float c = DotProduct3D(dispBetStarts, rayForwardA);
	float d = DotProduct3D(rayForwardB, rayForwardB);
	float e = DotProduct3D(rayForwardA, rayForwardA);

	float f = (e * d) - (b * b);


	if (fabsf(f) < toleranceSqr) return raycastResult;

	float g = (a * b) - (c * d);

	float t = g / f; // Parametric value for closest point to other line


	float s = (a + (b * t)) / d; // Parametric value for closest point to other line

	Vec3 resultingPointRay = rayStartA + (t * rayForwardA);
	Vec3 resultingPoinLine = rayStartB + (s * rayForwardB);


	float distBetPoints = GetDistanceSquared3D(resultingPointRay, resultingPoinLine);

	if (distBetPoints > toleranceSqr) return raycastResult;

	raycastResult.m_didImpact = true;
	raycastResult.m_impactPos = resultingPoinLine;
	raycastResult.m_impactFraction = t;
	raycastResult.m_impactNormal = CrossProduct3D(rayForwardB, rayForwardA).GetNormalized();


	return raycastResult;
}

RaycastResult3D RaycastVsPlane(Vec3 const& rayStart, Vec3 const& rayForward, float maxDistance, Vec3 const& pointOnplane, Vec3 const& planeNormal, float tolerance)
{
	RaycastResult3D raycastResult = {};
	raycastResult.m_startPosition = rayStart;
	raycastResult.m_forwardNormal = rayForward;
	raycastResult.m_maxDistance = maxDistance;


	Vec3 distToAnyPlanePoint = pointOnplane - rayStart;
	Vec3 rayDisp = rayForward * maxDistance;

	float slopeNum = DotProduct3D(planeNormal, distToAnyPlanePoint);
	float slopeDenom = DotProduct3D(planeNormal, rayDisp);

	if (fabsf(slopeDenom) < tolerance) return raycastResult; // Normal is perpendicular to line. Which means, then, that line is parallel to plain

	float t = slopeNum / slopeDenom;

	if (t < 0 || t > 1.0f) return raycastResult;

	Vec3 impactPoint = rayStart + (rayDisp * t);

	//if(!planeBounds.IsPointInside(impactPoint)) return raycastResult;

	raycastResult.m_didImpact = true;
	raycastResult.m_impactFraction = t;
	raycastResult.m_impactNormal = CrossProduct3D(rayForward, planeNormal);
	raycastResult.m_impactPos = impactPoint;


	return raycastResult;
}

