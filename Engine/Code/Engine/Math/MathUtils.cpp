#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Math/ConvexHull2D.hpp"

float ConvertDegreesToRadians(float degrees) {
	return degrees * static_cast<float>(M_PI) / 180.0f;
}

float ConvertRadiansToDegrees(float radians)
{
	return radians / static_cast<float>(M_PI) * 180.0f;
}

float CosDegrees(float degrees)
{
	float radians = ConvertDegreesToRadians(degrees);
	return cosf(radians);
}


float SinDegrees(float degrees) {
	float radians = ConvertDegreesToRadians(degrees);
	return sinf(radians);
}

float Atan2Degrees(float y, float x)
{
	return ConvertRadiansToDegrees(atan2f(y, x));
}

float AsinDegrees(float x)
{
	return ConvertRadiansToDegrees(asinf(x));
}

float GetDistanceSquared3D(const Vec3& positionA, const Vec3& positionB)
{
	float distanceX = positionA.x - positionB.x;
	float distanceY = positionA.y - positionB.y;
	float distanceZ = positionA.z - positionB.z;
	return (distanceX * distanceX) + (distanceY * distanceY) + (distanceZ * distanceZ);
}

float GetDistanceXY3D(const Vec3& positionA, const Vec3& positionB)
{
	return sqrtf(GetDistanceXYSquared3D(positionA, positionB));
}

float GetDistanceXYSquared3D(const Vec3& positionA, const Vec3& positionB)
{
	float distanceX = positionA.x - positionB.x;
	float distanceY = positionA.y - positionB.y;

	return (distanceX * distanceX) + (distanceY * distanceY);
}

void TransformPosition2D(Vec2& position, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{

	position = translation + position.x * iBasis + position.y * jBasis;
}

void TransformPosition2D(Vec2& position, float uniformScale, float rotationDegrees, const Vec2& translationVec)
{
	// Scale
	position *= uniformScale;

	// Rotation
	float r = sqrtf((position.x * position.x) + (position.y * position.y));
	float angle = atan2f(position.y, position.x);

	angle += ConvertDegreesToRadians(rotationDegrees);

	position = Vec2(cosf(angle), sinf(angle));
	position *= r;

	//Translation

	position += translationVec;

}

void TransformPositionXY3D(Vec3& position, float uniformScale, float rotationDegreesAboutZ, Vec2 const& translationVec)
{
	float z = position.z;
	// Scale
	position *= uniformScale;

	//// Rotation
	float r = sqrtf((position.x * position.x) + (position.y * position.y));
	float angle = atan2f(position.y, position.x);

	angle += ConvertDegreesToRadians(rotationDegreesAboutZ);

	position = Vec3(cosf(angle), sinf(angle), 0);
	position *= r;

	//Translation

	position.x += translationVec.x;
	position.y += translationVec.y;
	position.z = z;

}

void TransformPositionXY3D(Vec3& position, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
	float zComp = position.z;
	Vec2 iComp = position.x * iBasis;
	Vec2 jComp = position.y * jBasis;

	position = Vec3(translation.x, translation.y, 0.0f) + Vec3(iComp.x, iComp.y, 0.0f) + Vec3(jComp.x, jComp.y, 0.0f);

	position.z = zComp;
}

void TransformPosition3D(Vec3& position, Mat44 const& model)
{
	Vec3 iBasis = model.GetIBasis3D();
	Vec3 jBasis = model.GetJBasis3D();
	Vec3 kBasis = model.GetKBasis3D();

	Vec3 iComp = position.x * iBasis;
	Vec3 jComp = position.y * jBasis;
	Vec3 kComp = position.z * kBasis;

	position = model.GetTranslation3D() + iComp + jComp + kComp;
}


float GetFractionWithin(float inValue, float inStart, float inEnd)
{
	if (inStart == inEnd) return 0.5f;
	float range = inEnd - inStart;
	return (inValue - inStart) / range;
}

float Interpolate(float outStart, float outEnd, float fraction)
{
	return outStart + fraction * (outEnd - outStart);
}

Vec2 const Interpolate(Vec2 const& outStart, Vec2 const& outEnd, float fraction)
{
	return  outStart + fraction * (outEnd - outStart);
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float fraction = GetFractionWithin(inValue, inStart, inEnd);
	return Interpolate(outStart, outEnd, fraction);
}

float RangeMap(float inValue, FloatRange const& inRange, float outStart, float outEnd)
{
	return RangeMap(inValue, inRange.m_min, inRange.m_max, outStart, outEnd);
}

float RangeMap(float inValue, float inStart, float inEnd, FloatRange const& outRange)
{
	return RangeMap(inValue, inStart, inEnd, outRange.m_min, outRange.m_max);
}

float RangeMap(float inValue, FloatRange const& inRange, FloatRange const& outRange)
{
	return RangeMap(inValue, inRange.m_min, inRange.m_max, outRange.m_min, outRange.m_max);
}

float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float fraction = GetFractionWithin(inValue, inStart, inEnd);
	if (fraction < 0.0f) fraction = 0.0f;
	if (fraction > 1.0f) fraction = 1.0f;

	float interPolatedValue = Interpolate(outStart, outEnd, fraction);

	return interPolatedValue;
}


float RangeMapClamped(float inValue, FloatRange const& inRange, float outStart, float outEnd)
{
	return RangeMapClamped(inValue, inRange.m_min, inRange.m_max, outStart, outEnd);
}

float RangeMapClamped(float inValue, float inStart, float inEnd, FloatRange const& outRange)
{
	return RangeMapClamped(inValue, inStart, inEnd, outRange.m_min, outRange.m_max);
}

float RangeMapClamped(float inValue, FloatRange const& inRange, FloatRange const& outRange)
{
	return RangeMapClamped(inValue, inRange.m_min, inRange.m_max, outRange.m_min, outRange.m_max);
}


float Clamp(float inValue, float const minValue, float const maxValue)
{
	if (inValue < minValue) return minValue;
	if (inValue > maxValue) return maxValue;
	return inValue;
}

float Clamp(float inValue, FloatRange const& range)
{
	return Clamp(inValue, range.m_min, range.m_max);
}

float ClampZeroToOne(float inValue)
{
	return Clamp(inValue, 0.0f, 1.0f);
}

int RoundDownToInt(float numberToRound)
{
	float roundedFloat = floorf(numberToRound);
	int roundedInt = (int)roundedFloat;

	return roundedInt;
}

float NormalizeByte(unsigned char byteValue)
{
	float convertedValue = static_cast<float>(byteValue);
	if (convertedValue == 255.0f) return 1.0f;
	return (convertedValue) / 256.0f;
}

unsigned char DenormalizeByte(float zeroToOne)
{
	if (zeroToOne == 1.0f) return static_cast<unsigned char>(255);
	return static_cast<unsigned char>(zeroToOne * 256.0f);
}

float GetShortestAngularDispDegrees(float fromDeg, float toDeg)
{
	float angDispDeg = toDeg - fromDeg;

	while (angDispDeg > 180.0f) {
		angDispDeg -= 360.0f;
	}

	while (angDispDeg < -180.0f) {
		angDispDeg += 360.0f;
	}

	return angDispDeg;
}

float GetTurnedTowardDegrees(float fromDeg, float toDeg, float maxDeltaDeg)
{
	float shortestDispDeg = GetShortestAngularDispDegrees(fromDeg, toDeg);

	if (fabsf(shortestDispDeg) <= maxDeltaDeg) {
		return toDeg;
	}
	else if (shortestDispDeg < 0.0f) {
		return fromDeg - maxDeltaDeg;
	}
	else
	{
		return fromDeg + maxDeltaDeg;
	}

}

float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
	return DotProduct2D(vectorToProject, vectorToProjectOnto.GetNormalized());
}

Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
	Vec2 vectorToProjectOntoDir = vectorToProjectOnto.GetNormalized();

	float projDistance = DotProduct2D(vectorToProject, vectorToProjectOntoDir);

	return vectorToProjectOntoDir * projDistance;
}

Vec3 const GetProjectedOnto3D(Vec3 const& vectorToProject, Vec3 const& vectorToProjectOnto)
{
	Vec3 vectorToProjectOntoDir = vectorToProjectOnto.GetNormalized();

	float projDistance = DotProduct3D(vectorToProject, vectorToProjectOntoDir);

	return vectorToProjectOntoDir * projDistance;
}

float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b)
{
	Vec2 aNormal = a.GetNormalized();
	Vec2 bNormal = b.GetNormalized();

	float dotProduct = DotProduct2D(aNormal, bNormal);
	float degrees = acosf(Clamp(dotProduct, -1.0f, 1.0f));
	return ConvertRadiansToDegrees(degrees);
}

float DotProduct2D(Vec2 const& vecA, Vec2 const& vecB)
{
	return (vecA.x * vecB.x) + (vecA.y * vecB.y);
}

float DotProduct3D(Vec3 const& vecA, Vec3 const& vecB)
{
	return (vecA.x * vecB.x) + (vecA.y * vecB.y) + (vecA.z * vecB.z);
}

float DotProduct4D(Vec4 const& vecA, Vec4 const& vecB)
{
	return (vecA.x * vecB.x) + (vecA.y * vecB.y) + (vecA.z * vecB.z) + (vecA.w * vecB.w);
}

float CrossProduct2D(Vec2 const& a, Vec2 const& b)
{
	return (a.x * b.y) - (b.x * a.y);
}

Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b)
{
	Vec3 crossProductResult;
	crossProductResult.x = (a.y * b.z) - (a.z * b.y);
	crossProductResult.y = (a.z * b.x) - (a.x * b.z);
	crossProductResult.z = (a.x * b.y) - (a.y * b.x);

	return crossProductResult;
}

Vec3 ProjectPointOntoPlane(Vec3 const& refPoint, Vec3 const& planeRefPoint, Vec3 const& planeNormal)
{

	Vec3 dispToPoint = refPoint - planeRefPoint;
	float distAlongNormal = DotProduct3D(dispToPoint, planeNormal);

	Vec3 projectedDispToPoint = dispToPoint - (planeNormal * distAlongNormal);
	Vec3 projectedPoint = planeRefPoint + projectedDispToPoint;

	return projectedPoint;
}

int GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB)
{
	return abs(pointA.x - pointB.x) + abs(pointA.y - pointB.y);
}

float GetDistance2D(const Vec2& positionA, const Vec2& positionB)
{
	return sqrtf(GetDistanceSquared2D(positionA, positionB));
}

float GetDistanceSquared2D(const Vec2& positionA, const Vec2& positionB)
{
	float distanceX = positionB.x - positionA.x;
	float distanceY = positionB.y - positionA.y;
	return (distanceX * distanceX) + (distanceY * distanceY);
}

float GetDistance3D(const Vec3& positionA, const Vec3& positionB)
{
	return sqrtf(GetDistanceSquared3D(positionA, positionB));
}

bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& center, float radius)
{
	float distanceSquared = GetDistanceSquared2D(point, center);
	return distanceSquared < (radius* radius);
}

bool IsPointInsideSphere(Vec3 const& point, Vec3 const& center, float radius)
{
	float distanceSquared = GetDistanceSquared3D(point, center);
	return distanceSquared < (radius* radius);
}

bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
	if (!IsPointInsideDisc2D(point, sectorTip, sectorRadius)) {
		return false;
	}

	float dispToPointDeg = (point - sectorTip).GetOrientationDegrees();

	float shortestDispDegToPoint = GetShortestAngularDispDegrees(sectorForwardDegrees, dispToPointDeg);

	return (shortestDispDegToPoint > -sectorApertureDegrees * 0.5f) && (shortestDispDegToPoint < sectorApertureDegrees * 0.5f);
}

bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
	if (!IsPointInsideDisc2D(point, sectorTip, sectorRadius)) {
		return false;
	}

	float angBetweenVecs = GetAngleDegreesBetweenVectors2D(sectorForwardNormal, point - sectorTip);
	return angBetweenVecs < sectorApertureDegrees * 0.5f;
}

bool IsPointInsideAABB2D(Vec2 const& refPoint, AABB2 const& bounds)
{
	bool insideX = (bounds.m_mins.x < refPoint.x) && (refPoint.x < bounds.m_maxs.x);
	bool insideY = (bounds.m_mins.y < refPoint.y) && (refPoint.y < bounds.m_maxs.y);

	return insideX && insideY;
}

bool IsPointInsideAABB3D(Vec3 const& refPoint, AABB3 const& bounds)
{
	FloatRange xRange(bounds.m_mins.x, bounds.m_maxs.x);
	FloatRange yRange(bounds.m_mins.y, bounds.m_maxs.y);
	FloatRange zRange(bounds.m_mins.z, bounds.m_maxs.z);

	return (xRange.IsOnRange(refPoint.x)) && (yRange.IsOnRange(refPoint.y)) && (zRange.IsOnRange(refPoint.z));
}

bool IsPointInsideOBB2D(Vec2 const& refPoint, OBB2 const& bounds)
{

	Vec2 pointInLocal = bounds.GetLocalPosForWorldPos(refPoint);
	Vec2 halfDimensions = bounds.m_halfDimensions;

	return (-halfDimensions.x < pointInLocal.x&& pointInLocal.x < halfDimensions.x) && (-halfDimensions.y < pointInLocal.y&& pointInLocal.y < halfDimensions.y);
}

bool IsPointInsideCapsule2D(Vec2 const& refPoint, Capsule2 const& capsule)
{
	Vec2 nearestPointToCapsuleBone = GetNearestPointOnLineSegment2D(refPoint, capsule.m_bone);
	float distanceSqrToPoint = GetDistanceSquared2D(nearestPointToCapsuleBone, refPoint);

	return distanceSqrToPoint <= capsule.m_radius * capsule.m_radius;
	return false;
}

bool IsPointInsideConvexPoly2D(Vec2 const& refPoint, ConvexPoly2D const& convexPoly, float tolerance)
{
	ConvexHull2D asConvexHull = convexPoly.GetConvexHull();

	return IsPointInsideConvexHull2D(refPoint, asConvexHull, tolerance);
}

bool IsPointInsideConvexHull2D(Vec2 const& refPoint, ConvexHull2D const& convexHull, float tolerance)
{
	for (Plane2D const& plane : convexHull.m_planes) {
		float dotProd = DotProduct2D(plane.m_planeNormal, refPoint) - plane.m_distToPlane;

		if (dotProd > tolerance) return false;
	}

	return true;
}

bool DoDiscsOverlap(const Vec2& centerA, float radiusA, const Vec2& centerB, float radiusB)
{
	float radiusSum = radiusA + radiusB;
	float distanceSquared = GetDistanceSquared2D(centerA, centerB);

	return distanceSquared < (radiusSum* radiusSum);
}

bool DoDiscAndAABB2Overlap(Vec2 const& center, float radius, AABB2 const& bounds)
{
	Vec2 nearestPointOnBox = GetNearestPointOnAABB2D(center, bounds);
	return IsPointInsideDisc2D(nearestPointOnBox, center, radius);
}

bool DoSpheresOverlap(const Vec3& centerA, float radiusA, const Vec3& centerB, float radiusB)
{
	float distanceSquared = GetDistanceSquared3D(centerA, centerB);
	float radiusSum = radiusA + radiusB;
	return distanceSquared < radiusSum* radiusSum;
}

bool DoAABB2sOverlap(AABB2 const& aBounds, AABB2 const& bBounds)
{
	FloatRange aXBounds = aBounds.GetXRange();
	FloatRange aYBounds = aBounds.GetYRange();

	FloatRange bXBounds = bBounds.GetXRange();
	FloatRange bYBounds = bBounds.GetYRange();

	bool overlapOnX = aXBounds.IsOverlappingWith(bXBounds);
	bool overlapOnY = aYBounds.IsOverlappingWith(bYBounds);
	return (overlapOnX && overlapOnY);
}

bool DoConvexPolygons2DOverlap(DelaunayConvexPoly2D const& polyA, DelaunayConvexPoly2D const& polyB)
{
	std::vector<ConvexPoly2DEdge> polyAEdges = polyA.GetEdges();
	std::vector<ConvexPoly2DEdge> polyBEdges = polyB.GetEdges();

	for (int polyAIndex = 0; polyAIndex < polyAEdges.size(); polyAIndex++) {
		ConvexPoly2DEdge const& edgeA = polyAEdges[polyAIndex];
		Vec2 dispEdge = edgeA.m_pointTwo - edgeA.m_pointOne;
		Vec2 iBasis = (dispEdge).GetNormalized();

		Vec2 edgeNormal = iBasis.GetRotated90Degrees();

		Vec2 const& origin = edgeA.m_pointOne;

		FloatRange polyAProj = polyA.ProjectOntoAxis(edgeNormal, origin);
		FloatRange polyBProj = polyB.ProjectOntoAxis(edgeNormal, origin);

		if (!polyAProj.IsOverlappingWith(polyBProj)) return false;
	}

	for (int polyBIndex = 0; polyBIndex < polyBEdges.size(); polyBIndex++) {
		ConvexPoly2DEdge const& edgeB = polyBEdges[polyBIndex];
		Vec2 dispEdge = edgeB.m_pointTwo - edgeB.m_pointOne;
		Vec2 iBasis = (dispEdge).GetNormalized();

		Vec2 edgeNormal = iBasis.GetRotated90Degrees();
		Vec2 const& origin = edgeB.m_pointOne;

		FloatRange polyAProj = polyA.ProjectOntoAxis(edgeNormal, origin
		);
		FloatRange polyBProj = polyB.ProjectOntoAxis(edgeNormal, origin
		);

		if (!polyAProj.IsOverlappingWith(polyBProj)) return false;
	}


	return true;
}

bool DoAABB3sOverlap(AABB3 const& aBounds, AABB3 const& bBounds)
{
	FloatRange aXBounds = aBounds.GetXRange();
	FloatRange aYBounds = aBounds.GetYRange();
	FloatRange aZBounds = aBounds.GetZRange();

	FloatRange bXBounds = bBounds.GetXRange();
	FloatRange bYBounds = bBounds.GetYRange();
	FloatRange bZBounds = bBounds.GetZRange();

	bool overlapOnX = aXBounds.IsOverlappingWith(bXBounds);
	if (!overlapOnX) return false;

	bool overlapOnY = aYBounds.IsOverlappingWith(bYBounds);
	if (!overlapOnY) return false;

	bool overlapOnZ = aZBounds.IsOverlappingWith(bZBounds);
	if (!overlapOnZ) return false;

	return true;
}

bool DoZCylindersOverlap(Vec2 const& aXYCenter, float aCylinderRadius, FloatRange const& aZRange, Vec2 const& bXYCenter, float bCylinderRadius, FloatRange const& bZRange)
{
	bool doOverlapFromTop = DoDiscsOverlap(aXYCenter, aCylinderRadius, bXYCenter, bCylinderRadius);
	if (!doOverlapFromTop) return false;

	AABB2 aXZBounds(Vec2(aXYCenter.x - aCylinderRadius, aZRange.m_min), Vec2(aXYCenter.x + aCylinderRadius, aZRange.m_max));
	AABB2 bXZBounds(Vec2(bXYCenter.x - bCylinderRadius, bZRange.m_min), Vec2(bXYCenter.x + bCylinderRadius, bZRange.m_max));

	bool doOverlapFromXZSide = DoAABB2sOverlap(aXZBounds, bXZBounds);

	if (!doOverlapFromXZSide) return false;

	/*AABB2 aYZBounds(Vec2(aXYCenter.y - aCylinderRadius, aZRange.m_min), Vec2(aXYCenter.y + aCylinderRadius, aZRange.m_max));
	AABB2 bYZBounds(Vec2(bXYCenter.y - bCylinderRadius, bZRange.m_min), Vec2(bXYCenter.y + bCylinderRadius, bZRange.m_max));

	bool doOverlapFromYZSide = DoAABB2sOverlap(aYZBounds, bYZBounds);

	if (!doOverlapFromYZSide) return false;*/

	return true;
}

bool DoSphereAndAABB3Overlap(Vec3 const& sphereCenter, float radius, AABB3 const& bounds)
{
	Vec3 nearestPointOnBox = GetNearestPointOnAABB3D(sphereCenter, bounds);
	return IsPointInsideSphere(nearestPointOnBox, sphereCenter, radius);
}

bool DoSphereAndZCylinderOverlap(Vec3 const& sphereCenter, float sphereRadius, Vec2 const& xyCenter, float cylinderRadius, FloatRange const& zRange)
{
	Vec3 point = GetNearestPointOnZCylinder(sphereCenter, xyCenter, cylinderRadius, zRange);

	return IsPointInsideSphere(point, sphereCenter, sphereRadius);
}

bool DoAABB3AndZCylinderOverlap(AABB3 const& bounds, Vec2 const& xyCenter, float cylinderRadius, FloatRange const& zRange)
{
	bool doOverlapFromTop = DoDiscAndAABB2Overlap(xyCenter, cylinderRadius, bounds.GetXYBounds());

	if (!doOverlapFromTop) return false;

	AABB2 xzBounds(Vec2(xyCenter.x - cylinderRadius, zRange.m_min), Vec2(xyCenter.x + cylinderRadius, zRange.m_max));

	bool doOverlapFromXZSide = DoAABB2sOverlap(xzBounds, bounds.GetXZBounds());
	if (!doOverlapFromXZSide) return false;

	//AABB2 yzBounds(Vec2(xyCenter.y - cylinderRadius, zRange.m_min), Vec2(xyCenter.y + cylinderRadius, zRange.m_max));
	//
	//bool doOverlapFromYZSide = DoAABB2sOverlap(yzBounds, bounds.GetYZBounds());
	//if (!doOverlapFromYZSide) return false;

	return true;
}

Vec2 const GetNearestPointOnDisc2D(Vec2 const& referencePos, Vec2 const& center, float radius)
{
	Vec2 nearestPoint = referencePos - center;
	nearestPoint.ClampLength(radius);
	return nearestPoint + center;
}

Vec2 const GetNearestPointOnLineSegment2D(Vec2 const& refPoint, LineSegment2 const& lineSegment)
{
	Vec2 dispLineSegment = (lineSegment.m_end - lineSegment.m_start).GetNormalized();
	Vec2 dispEndToRefPoint = (refPoint - lineSegment.m_end).GetNormalized();

	if (DotProduct2D(dispLineSegment, dispEndToRefPoint) >= 0.0f) {
		return lineSegment.m_end;
	}

	Vec2 dispStartToRefPoint = (refPoint - lineSegment.m_start).GetNormalized();

	if (DotProduct2D(dispLineSegment, dispStartToRefPoint) <= 0.0f) {
		return lineSegment.m_start;
	}

	Vec2 projectedOntoSegment = GetProjectedOnto2D((refPoint - lineSegment.m_start), dispLineSegment);

	return lineSegment.m_start + projectedOntoSegment;
}

Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 const& refPoint, LineSegment2 const& line)
{
	Vec2 dispToPoint = (refPoint - line.m_start).GetNormalized();
	Vec2 lineDirection = (line.m_end - line.m_start).GetNormalized();
	return line.m_start + GetProjectedOnto2D(dispToPoint, lineDirection);
}

Vec2 const GetNearestPointOnAABB2D(Vec2 const& refPoint, AABB2 const& bounds)
{
	float clampedX = Clamp(refPoint.x, bounds.m_mins.x, bounds.m_maxs.x);
	float clampedY = Clamp(refPoint.y, bounds.m_mins.y, bounds.m_maxs.y);
	return Vec2(clampedX, clampedY);
}

Vec2 const GetNearestPointOnOBB2D(Vec2 const& refPoint, OBB2 const& bounds)
{
	Vec2 dispToPoint = refPoint - bounds.m_center;
	float projI = DotProduct2D(bounds.m_iBasisNormal, dispToPoint);
	float projJ = DotProduct2D(bounds.m_iBasisNormal.GetRotated90Degrees(), dispToPoint);

	Vec2 halfDimensions = bounds.m_halfDimensions;

	float nearestIPoint = Clamp(projI, -halfDimensions.x, halfDimensions.x);
	float nearestJPoint = Clamp(projJ, -halfDimensions.y, halfDimensions.y);

	return bounds.GetWorldPosForLocalPos(Vec2(nearestIPoint, nearestJPoint));
}

Vec2 const GetNearestPointOnCapsule2D(Vec2 const& refPoint, Capsule2 const& capsule)
{
	Vec2 nearestPointToCapsule = GetNearestPointOnLineSegment2D(refPoint, capsule.m_bone);
	Vec2 dispToPoint = (refPoint - nearestPointToCapsule).GetClamped(capsule.m_radius);

	return nearestPointToCapsule + dispToPoint;
}

Vec2 const GetNearestPointOnConvexPoly2D(Vec2 const& refPoint, DelaunayConvexPoly2D const& convexPoly)
{
	Vec2 nearestVertex = Vec2::ZERO;
	float closestDist = FLT_MAX;
	int closestVertInd = 0;

	for (int vertIndex = 0; vertIndex < convexPoly.m_vertexes.size(); vertIndex++) {
		Vec2 const& vertex = convexPoly.m_vertexes[vertIndex];
		float distToVertex = GetDistanceSquared2D(vertex, refPoint);
		if (distToVertex < closestDist) {
			closestDist = distToVertex;
			nearestVertex = vertex;
			closestVertInd = vertIndex;
		}
	}

	// There are potentially 2 edges with nearest point. We calculate nearest point of each edge, and return whichever is closest
	int prevVertInd = closestVertInd - 1;
	if (prevVertInd < 0) prevVertInd = (int)convexPoly.m_vertexes.size() - 1;

	int nextVertInd = (closestVertInd + 1) % (int)convexPoly.m_vertexes.size();

	Vec2 const& prevVert = convexPoly.m_vertexes[prevVertInd];
	Vec2 const& nextVert = convexPoly.m_vertexes[nextVertInd];

	Vec2 edgePrev = nearestVertex - prevVert;
	Vec2 edgeNext = nextVert - nearestVertex;

	float lengthEdgePrev = edgePrev.GetLength();
	float lengthEdgeNext = edgeNext.GetLength();

	Vec2 prevVertDir = (edgePrev).GetNormalized();
	Vec2 nextVertDir = (edgeNext).GetNormalized();

	Vec2 dispToPrevOrigin = refPoint - prevVert;
	Vec2 dispToNextOrigin = refPoint - nearestVertex;

	float dotProdPrev = DotProduct2D(prevVertDir, dispToPrevOrigin);
	float dotProdNext = DotProduct2D(nextVertDir, dispToNextOrigin);

	dotProdPrev = Clamp(dotProdPrev, 0.0f, lengthEdgePrev);
	dotProdNext = Clamp(dotProdNext, 0.0f, lengthEdgeNext);

	Vec2 nearestPointOnPrev = prevVert + (prevVertDir * dotProdPrev);
	Vec2 nearestPointOnNext = nearestVertex + (nextVertDir * dotProdNext);

	if (GetDistanceSquared2D(nearestPointOnPrev, refPoint) < GetDistanceSquared2D(nearestPointOnNext, refPoint)) {
		return nearestPointOnPrev;
	}

	return nearestPointOnNext;
}

ConvexPoly2DEdge const GetNearestEdgeOnConvexPoly2D(Vec2 const& refPoint, DelaunayConvexPoly2D const& convexPoly)
{
	Vec2 nearestVertex = Vec2::ZERO;
	float closestDist = FLT_MAX;
	int closestVertInd = 0;

	for (int vertIndex = 0; vertIndex < convexPoly.m_vertexes.size(); vertIndex++) {
		Vec2 const& vertex = convexPoly.m_vertexes[vertIndex];
		float distToVertex = GetDistanceSquared2D(vertex, refPoint);
		if (distToVertex < closestDist) {
			closestDist = distToVertex;
			nearestVertex = vertex;
			closestVertInd = vertIndex;
		}
	}

	// There are potentially 2 edges with nearest point. We calculate nearest point of each edge, and return whichever is closest
	int prevVertInd = closestVertInd - 1;
	if (prevVertInd < 0) prevVertInd = (int)convexPoly.m_vertexes.size() - 1;

	int nextVertInd = (closestVertInd + 1) % (int)convexPoly.m_vertexes.size();

	Vec2 const& prevVert = convexPoly.m_vertexes[prevVertInd];
	Vec2 const& nextVert = convexPoly.m_vertexes[nextVertInd];

	Vec2 edgePrev = nearestVertex - prevVert;
	Vec2 edgeNext = nextVert - nearestVertex;

	float lengthEdgePrev = edgePrev.GetLength();
	float lengthEdgeNext = edgeNext.GetLength();

	Vec2 prevVertDir = (edgePrev).GetNormalized();
	Vec2 nextVertDir = (edgeNext).GetNormalized();

	Vec2 dispToPrevOrigin = refPoint - prevVert;
	Vec2 dispToNextOrigin = refPoint - nearestVertex;

	float dotProdPrev = DotProduct2D(prevVertDir, dispToPrevOrigin);
	float dotProdNext = DotProduct2D(nextVertDir, dispToNextOrigin);

	dotProdPrev = Clamp(dotProdPrev, 0.0f, lengthEdgePrev);
	dotProdNext = Clamp(dotProdNext, 0.0f, lengthEdgeNext);

	Vec2 nearestPointOnPrev = prevVert + (prevVertDir * dotProdPrev);
	Vec2 nearestPointOnNext = nearestVertex + (nextVertDir * dotProdNext);


	float distToPrev = GetDistanceSquared2D(nearestPointOnPrev, refPoint);
	float distToNext = GetDistanceSquared2D(nearestPointOnNext, refPoint);

	if (distToPrev < distToNext) {
		return ConvexPoly2DEdge(prevVert, nearestVertex);
	}

	// If both distances are equal, then maybe one sides Xrange or Y range contain the Vertex y or x, if both do, then whichever is chosen is good enough
	if (distToPrev == distToNext) {


		int scorePrev = 0;
		if ((prevVert.x <= refPoint.x) && (refPoint.x <= nearestVertex.x)) scorePrev++;
		if ((prevVert.y <= refPoint.y) && (refPoint.y <= nearestVertex.y)) scorePrev++;

		int scoreNext = 0;

		if ((nearestVertex.x <= refPoint.x) && (refPoint.x <= nextVert.x)) scoreNext++;
		if ((nearestVertex.y <= refPoint.y) && (refPoint.y <= nextVert.y)) scoreNext++;

		if (scoreNext != scorePrev) {
			if (scoreNext < scorePrev) {
				return ConvexPoly2DEdge(prevVert, nearestVertex);
			}
			else {
				return ConvexPoly2DEdge(nearestVertex, nextVert);
			}
		}
	}


	return ConvexPoly2DEdge(nearestVertex, nextVert);
}

Vec3 const GetNearestPointOnSphere(Vec3 const& refPoint, Vec3 const& center, float radius)
{
	Vec3 dispToCenter = (refPoint - center).GetClamped(radius);
	return center + dispToCenter;
}

Vec3 const GetNearestPointOnZCylinder(Vec3 const& refPoint, Vec2 const& xyCenter, float radius, FloatRange const& zRange)
{
	Vec2 dispToCenterXY = Vec2(refPoint.x, refPoint.y) - xyCenter;
	dispToCenterXY.ClampLength(radius);
	float zHeight = Clamp(refPoint.z, zRange);

	Vec2 nearestXY = xyCenter + dispToCenterXY;
	Vec3 nearestPoint(nearestXY.x, nearestXY.y, zHeight);
	return nearestPoint;
}

Vec3 const GetNearestPointOnAABB3D(Vec3 const& refPoint, AABB3 const& bounds)
{
	FloatRange xRange = bounds.GetXRange();
	FloatRange yRange = bounds.GetYRange();
	FloatRange zRange = bounds.GetZRange();

	float nearestX = Clamp(refPoint.x, xRange);
	float nearestY = Clamp(refPoint.y, yRange);
	float nearestZ = Clamp(refPoint.z, zRange);

	return Vec3(nearestX, nearestY, nearestZ);
}

Vec3 GetRandomDirectionInCone(Vec3 const& forward, float angle)
{
	RandomNumberGenerator randomNumGen;
	Vec3 worldJBasis = Vec3(0.0, 1.0f, 0.0f);
	Vec3 worldKBasis = Vec3(0.0f, 0.0, 1.0f);

	Vec3 jBasis = Vec3::ZERO;
	Vec3 kBasis = Vec3::ZERO;

	if (fabsf(DotProduct3D(forward, worldJBasis)) < 1) {
		jBasis = CrossProduct3D(forward, worldJBasis).GetNormalized();
		kBasis = CrossProduct3D(jBasis, forward).GetNormalized();
	}
	else {
		jBasis = CrossProduct3D(forward, worldKBasis).GetNormalized();
		kBasis = CrossProduct3D(jBasis, forward).GetNormalized();
	}

	float halfAngle = angle * 0.5f;

	float halfAngleRad = ConvertDegreesToRadians(halfAngle);

	float coneWidth = tanf(halfAngleRad);

	float randomJ = randomNumGen.GetRandomFloatInRange(-coneWidth, coneWidth);
	float randomK = randomNumGen.GetRandomFloatInRange(-coneWidth, coneWidth);


	Vec3 randomDir = forward + (randomJ * jBasis) + (randomK * kBasis);

	return randomDir.GetNormalized();
}

Mat44 GetOrthonormalBasis(Vec3 const& nonNormalIbasis) // Frisvad's Revised method for finding orthonormal basis
{
	float sign = copysignf(1.0f, nonNormalIbasis.z);
	float const a = -1.0f / (sign + nonNormalIbasis.z);
	float const b = nonNormalIbasis.x * nonNormalIbasis.y * a;

	Vec3 jBasis = Vec3(1.0f + sign * nonNormalIbasis.x * nonNormalIbasis.x * a, sign * b, -sign * nonNormalIbasis.x);
	Vec3 kBasis = Vec3(b, sign + nonNormalIbasis.y * nonNormalIbasis.y * a, -nonNormalIbasis.y);

	Mat44 orthoNormalBasis = Mat44(nonNormalIbasis, jBasis, kBasis, Vec3::ZERO);
	return orthoNormalBasis;
}

bool PushDiscOutOfPoint2D(Vec2& mobileDiscCenter, float radius, Vec2 const& fixedPoint)
{
	if (!IsPointInsideDisc2D(fixedPoint, mobileDiscCenter, radius)) {
		return false;
	}

	float distanceToPoint = GetDistance2D(mobileDiscCenter, fixedPoint);
	float overlapDist = radius - distanceToPoint;

	Vec2 pushDisp = mobileDiscCenter - fixedPoint;
	if (distanceToPoint == 0) {
		pushDisp = Vec2(1.0f, 0.0f);
	}

	pushDisp.SetLength(overlapDist);

	mobileDiscCenter += pushDisp;
	return true;

}

bool PushDiscOutOfDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius)
{
	if (!DoDiscsOverlap(mobileDiscCenter, mobileDiscRadius, fixedDiscCenter, fixedDiscRadius)) {
		return false;
	}
	Vec2 directionToPush = mobileDiscCenter - fixedDiscCenter;
	float ideaDistanceToOtherCircle = fixedDiscRadius + mobileDiscRadius;
	float overlapDistance = ideaDistanceToOtherCircle - GetDistance2D(mobileDiscCenter, fixedDiscCenter);

	directionToPush.SetLength(overlapDistance);
	mobileDiscCenter += directionToPush;

	return true;
}

bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius)
{
	if (!DoDiscsOverlap(aCenter, aRadius, bCenter, bRadius)) {
		return false;
	}

	float ideaDistanceToOtherCircle = aRadius + bRadius;
	float overlapDistance = ideaDistanceToOtherCircle - GetDistance2D(aCenter, bCenter);

	Vec2 pushDirectionA = (aCenter - bCenter).GetNormalized();
	Vec2 pushDirectionB = (bCenter - aCenter).GetNormalized();

	aCenter += pushDirectionA * overlapDistance * 0.5f;
	bCenter += pushDirectionB * overlapDistance * 0.5f;

	return true;
}

bool PushDiscOutOfAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox)
{
	Vec2 nearestPointToDisc = fixedBox.GetNearestPoint(mobileDiscCenter);

	return PushDiscOutOfPoint2D(mobileDiscCenter, discRadius, nearestPointToDisc);
}

// This functions DOES NOT DEAL COLLISIONS PERFECTLY. It is a good enough approximation, at the time of writing this 06/22
bool PushConvexPolysOutOfEachOther(DelaunayConvexPoly2D& convexPolyA, DelaunayConvexPoly2D& convexPolyB)
{
	std::vector<ConvexPoly2DEdge> polyAEdges = convexPolyA.GetEdges();
	std::vector<ConvexPoly2DEdge> polyBEdges = convexPolyB.GetEdges();

	float shortestOvelap = FLT_MAX;
	Vec2 shortestOverlapNormal = Vec2::ZERO;
	bool edgeIsFromA = true;

	// This is the same SAT algorithm
	for (int polyAIndex = 0; polyAIndex < polyAEdges.size(); polyAIndex++) {
		ConvexPoly2DEdge const& edgeA = polyAEdges[polyAIndex];
		Vec2 dispEdge = edgeA.m_pointTwo - edgeA.m_pointOne;
		Vec2 iBasis = (dispEdge).GetNormalized();

		Vec2 edgeNormal = iBasis.GetRotated90Degrees();

		Vec2 const& origin = edgeA.m_pointOne;

		FloatRange polyAProj = convexPolyA.ProjectOntoAxis(edgeNormal, origin);
		FloatRange polyBProj = convexPolyB.ProjectOntoAxis(edgeNormal, origin);

		if (!polyAProj.IsOverlappingWith(polyBProj)) {
			return false;
		}
		else {
			float overlapDist1 = fabsf(polyBProj.m_max - polyAProj.m_min);
			float overlapDist2 = fabsf(polyAProj.m_max - polyAProj.m_max);

			float minOverlap = 0.0f;
			if ((overlapDist1 > 0.0f) && (overlapDist2 > 0.0f)) {
				minOverlap = (overlapDist1 < overlapDist2) ? overlapDist1 : overlapDist2;
			}
			else {
				minOverlap = (overlapDist1 > 0.0f) ? overlapDist1 : overlapDist2;
			}

			if ((minOverlap < shortestOvelap) && (minOverlap > 0.0f)) {
				shortestOvelap = minOverlap;
				shortestOverlapNormal = edgeNormal;
			}

		}
	}

	for (int polyBIndex = 0; polyBIndex < polyBEdges.size(); polyBIndex++) {
		ConvexPoly2DEdge const& edgeB = polyBEdges[polyBIndex];
		Vec2 dispEdge = edgeB.m_pointTwo - edgeB.m_pointOne;
		Vec2 iBasis = (dispEdge).GetNormalized();

		Vec2 edgeNormal = iBasis.GetRotated90Degrees();
		Vec2 const& origin = edgeB.m_pointOne;

		FloatRange polyAProj = convexPolyA.ProjectOntoAxis(edgeNormal, origin);
		FloatRange polyBProj = convexPolyB.ProjectOntoAxis(edgeNormal, origin);

		if (!polyAProj.IsOverlappingWith(polyBProj)) {
			return false;
		}
		else {
			float overlapDist1 = fabsf(polyBProj.m_max - polyAProj.m_min);
			float overlapDist2 = fabsf(polyAProj.m_max - polyBProj.m_min);

			float minOverlap = 0.0f;
			if ((overlapDist1 > 0.0f) && (overlapDist2 > 0.0f)) {
				minOverlap = (overlapDist1 < overlapDist2) ? overlapDist1 : overlapDist2;
			}
			else {
				minOverlap = (overlapDist1 > 0.0f) ? overlapDist1 : overlapDist2;
			}

			if ((minOverlap < shortestOvelap) && (minOverlap > 0.0f)) {
				edgeIsFromA = false;
				shortestOvelap = minOverlap;
				shortestOverlapNormal = edgeNormal;
			}

		}
	}

	Vec2 dirToPushA = (edgeIsFromA) ? -shortestOverlapNormal : shortestOverlapNormal;
	Vec2 dirToPushB = -dirToPushA;


	convexPolyA.Translate(dirToPushA * (shortestOvelap * 0.5f));
	convexPolyB.Translate(dirToPushB * (shortestOvelap * 0.5f));

	return true;
}

bool PushConvexPolyOutOfOtherPoly(DelaunayConvexPoly2D const& fixedPolyA, DelaunayConvexPoly2D& convexPolyB)
{

	std::vector<ConvexPoly2DEdge> polyAEdges = fixedPolyA.GetEdges();

	float shortestOvelap = FLT_MAX;
	Vec2 shortestOverlapNormal = Vec2::ZERO;

	for (int polyAIndex = 0; polyAIndex < polyAEdges.size(); polyAIndex++) {
		ConvexPoly2DEdge const& edgeA = polyAEdges[polyAIndex];
		Vec2 dispEdge = edgeA.m_pointTwo - edgeA.m_pointOne;
		Vec2 iBasis = (dispEdge).GetNormalized();

		Vec2 edgeNormal = iBasis.GetRotated90Degrees();

		Vec2 const& origin = edgeA.m_pointOne;

		FloatRange polyAProj = fixedPolyA.ProjectOntoAxis(edgeNormal, origin);
		FloatRange polyBProj = convexPolyB.ProjectOntoAxis(edgeNormal, origin);

		if (!polyAProj.IsOverlappingWith(polyBProj)) {
			return false;
		}
		else {
			float overlapDist1 = fabsf(polyBProj.m_max - polyAProj.m_min);
			float overlapDist2 = fabsf(polyAProj.m_max - polyBProj.m_min);

			float minOverlap = 0.0f;
			if ((overlapDist1 > 0.0f) && (overlapDist2 > 0.0f)) {
				minOverlap = (overlapDist1 < overlapDist2) ? overlapDist1 : overlapDist2;
			}
			else {
				minOverlap = (overlapDist1 > 0.0f) ? overlapDist1 : overlapDist2;
			}

			if ((minOverlap < shortestOvelap) && (minOverlap > 0.0f)) {
				shortestOvelap = minOverlap;
				shortestOverlapNormal = edgeNormal;
			}

		}
	}

	Vec2 dirToPushB = shortestOverlapNormal;

	convexPolyB.Translate(dirToPushB * (shortestOvelap));

	return false;
}

bool PushSphereOutOfPoint(Vec3& mobileSpherecenter, float radius, Vec3 const& fixedPoint)
{
	if(!IsPointInsideSphere(fixedPoint, mobileSpherecenter, radius)) return false;
	float distance = GetDistance3D(mobileSpherecenter, fixedPoint);

	float distanceToPush = distance - radius;
	Vec3 dispToSphere = mobileSpherecenter - fixedPoint;
	if (dispToSphere.GetLengthSquared() == 0.0f) {
		dispToSphere = Vec3(-1.0f, 0.0f, 0.0f);
	}

	dispToSphere.SetLength(distanceToPush);
	mobileSpherecenter += dispToSphere;
	return true;
}

bool PushAABB3OutOfPoint(AABB3& mobileAABB3, Vec3 const& fixedPoint)
{
	if (!IsPointInsideAABB3D(fixedPoint, mobileAABB3))return false;

	Vec3 distToPoint = Vec3::ZERO;
	Vec3 const& center = mobileAABB3.GetCenter();
	Vec3 dirToCenter = center - fixedPoint;
	distToPoint.x = fabsf(fixedPoint.x - center.x);
	distToPoint.y = fabsf(fixedPoint.y - center.y);
	distToPoint.z = fabsf(fixedPoint.z - center.z);

	Vec3 overlapDist = (mobileAABB3.GetDimensions() * 0.5f) - distToPoint;

	Vec3 pushDir = Vec3::ZERO;
	if ((overlapDist.x <= overlapDist.y) && (overlapDist.x <= overlapDist.z)) {
		float dir = (dirToCenter.x >= 0) ? 1.0f : -1.0f;
		pushDir.x = overlapDist.x * dir;
	}
	else if ((overlapDist.y <= overlapDist.x) && (overlapDist.y <= overlapDist.z)) {
		float dir = (dirToCenter.y >= 0) ? 1.0f : -1.0f;
		pushDir.y = overlapDist.y * dir;
	}
	else {
		float dir = (dirToCenter.z >= 0) ? 1.0f : -1.0f;
		pushDir.z = overlapDist.z * dir;
	}

	mobileAABB3.Translate(pushDir);

	return true;
}

bool PushAABB3OutOfAABB3(AABB3 const& fixedAABB3, AABB3& mobileAABB3)
{
	if (!DoAABB3sOverlap(fixedAABB3, mobileAABB3))return false;

	Vec3 displacementBetweenCenters = mobileAABB3.GetCenter() - fixedAABB3.GetCenter(); // this is signed, we want only distance
	Vec3 distBetweenCenters = displacementBetweenCenters;

	distBetweenCenters.x = fabsf(displacementBetweenCenters.x);
	distBetweenCenters.y = fabsf(displacementBetweenCenters.y);
	distBetweenCenters.z = fabsf(displacementBetweenCenters.z);

	Vec3 idealDistanceBetweenAABBs = (fixedAABB3.GetDimensions() + mobileAABB3.GetDimensions()) * 0.5f;
	Vec3 overlapDist = idealDistanceBetweenAABBs - distBetweenCenters;

	//if((overlapDist.x == 0.0f) || (overlapDist.y == 0.0f) || (overlapDist.z == 0.0f)) return false;

	Vec3 pushDir = Vec3::ZERO;
	if ((overlapDist.x <= overlapDist.y) && (overlapDist.x <= overlapDist.z)) {
		if (displacementBetweenCenters.x >= 0) {
			pushDir.x = overlapDist.x;
		}
		else {
			pushDir.x = -overlapDist.x;
		}
	}
	else if ((overlapDist.y <= overlapDist.x) && (overlapDist.y <= overlapDist.z)) {
		if (displacementBetweenCenters.y >= 0) {
			pushDir.y = overlapDist.y;
		}
		else {
			pushDir.y = -overlapDist.y;
		}
	}
	else {
		if (displacementBetweenCenters.z >= 0) {
			pushDir.z = overlapDist.z;
		}
		else {
			pushDir.z = -overlapDist.z;
		}
	}

	mobileAABB3.Translate(pushDir);

	return true;
}

bool BounceDiscOffPoint2D(Vec2& mobileDiscCenter, Vec2& velocity, float radius, Vec2 const& fixed, float elasticity)
{
	if (!DoDiscsOverlap(mobileDiscCenter, radius, fixed, 0.0f)) return false;

	PushDiscOutOfPoint2D(mobileDiscCenter, radius, fixed);
	Vec2 surfaceNormal = (mobileDiscCenter - fixed).GetNormalized();

	velocity.Reflect(surfaceNormal, elasticity);

	return true;
}

bool BounceDiscOffDisc2D(Vec2& mobileDiscCenter, Vec2& velocity, float mobileDiscRadius, Vec2 const& fixedDisc, float fixedDiscRadius, float elasticity)
{
	if (!DoDiscsOverlap(mobileDiscCenter, mobileDiscRadius, fixedDisc, fixedDiscRadius)) return false;

	PushDiscOutOfDisc2D(mobileDiscCenter, mobileDiscRadius, fixedDisc, fixedDiscRadius);
	Vec2 bounceNormal = (mobileDiscCenter - fixedDisc).GetNormalized();
	Vec2 pointToBounceOff = fixedDisc + (bounceNormal * fixedDiscRadius);

	BounceDiscOffPoint2D(mobileDiscCenter, velocity, mobileDiscRadius, pointToBounceOff, elasticity);

	return true;
}

bool BounceDiscOffEachOther2D(Vec2& aCenter, Vec2& aVelocity, float aRadius, Vec2& bCenter, Vec2& bVelocity, float bRadius, float combinedElasticity)
{
	if (!DoDiscsOverlap(aCenter, aRadius, bCenter, bRadius)) return false;

	PushDiscsOutOfEachOther2D(aCenter, aRadius, bCenter, bRadius);

	Vec2 normal = (bCenter - aCenter).GetNormalized();
	Vec2 bVelocityNormalComp = GetProjectedOnto2D(bVelocity, normal);
	Vec2 aVelocityNormalComp = GetProjectedOnto2D(aVelocity, normal);

	Vec2 bVelocityTangComp, aVelocityTangComp;

	bVelocityTangComp = bVelocity - bVelocityNormalComp;
	aVelocityTangComp = aVelocity - aVelocityNormalComp;

	if (DotProduct2D(normal, bVelocity) < DotProduct2D(normal, aVelocity)) {
		aVelocity = aVelocityTangComp + (bVelocityNormalComp * combinedElasticity);
		bVelocity = bVelocityTangComp + (aVelocityNormalComp * combinedElasticity);
	}

	return true;
}
