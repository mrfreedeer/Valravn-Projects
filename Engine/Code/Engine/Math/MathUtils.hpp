#pragma once
#define _USE_MATH_DEFINES // for C++ using pi constant
#include <math.h>

struct Vec2;
struct Vec3;
struct Vec4;
struct IntVec2;
struct AABB2;
struct AABB3;
struct OBB2;
struct LineSegment2;
struct Capsule2;
struct RaycastResult2D;
struct Mat44;
struct FloatRange;
struct DelaunayConvexPoly2D;
struct ConvexPoly2DEdge;

class ConvexPoly2D;
class ConvexHull2D;
// ---------------------------------------------------------------------------------------------------------------------
// Trigonometry
float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float Atan2Degrees(float y, float x);
float AsinDegrees(float x);


// ---------------------------------------------------------------------------------------------------------------------
// Utilities (2D & 3D)
float GetDistance2D(const Vec2& positionA, const Vec2& positionB);
float GetDistanceSquared2D(const Vec2& positionA, const Vec2& positionB);
float GetDistance3D(const Vec3& positionA, const Vec3& positionB);
float GetDistanceSquared3D(const Vec3& positionA, const Vec3& positionB);
float GetDistanceXY3D(const Vec3& positionA, const Vec3& positionB);
float GetDistanceXYSquared3D(const Vec3& positionA, const Vec3& positionB);
float GetShortestAngularDispDegrees(float startingAngle, float finishAngle);
float GetTurnedTowardDegrees(float startingAngle, float finishAngle, float angleRange);
float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);
Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);
Vec3 const GetProjectedOnto3D(Vec3 const& vectorToProject, Vec3 const& vectorToProjectOnto);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b);
int GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB);

float DotProduct2D(Vec2 const& vecA, Vec2 const& vecB);
float DotProduct3D(Vec3 const& vecA, Vec3 const& vecB);
float DotProduct4D(Vec4 const& vecA, Vec4 const& vecB);

float CrossProduct2D(Vec2 const& a, Vec2 const& b);
Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b);
Vec3 ProjectPointOntoPlane(Vec3 const& refPoint, Vec3 const& planeRefPoint, Vec3 const& planeNormal);

// ---------------------------------------------------------------------------------------------------------------------
// Geometric Query Utilities
bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& center, float radius);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideAABB2D(Vec2 const& refPoint, AABB2 const& bounds);
bool IsPointInsideOBB2D(Vec2 const& refPoint, OBB2 const& bounds);
bool IsPointInsideCapsule2D(Vec2 const& refPoint, Capsule2 const& capsule);
bool IsPointInsideConvexPoly2D(Vec2 const& refPoint, ConvexPoly2D const& convexPoly, float tolerance = 0.025f);
bool IsPointInsideConvexHull2D(Vec2 const& refPoint, ConvexHull2D const& convexHull, float tolerance = 0.025f);


bool IsPointInsideSphere(Vec3 const& point, Vec3 const& center, float radius);
bool IsPointInsideAABB3D(Vec3 const& refPoint, AABB3 const& bounds);


bool DoDiscsOverlap(const Vec2& centerA, float radiusA, const Vec2& centerB, float radiusB);
bool DoDiscAndAABB2Overlap(Vec2 const& center, float radius, AABB2 const& bounds);
bool DoSpheresOverlap(const Vec3& centerA, float radiusA, const Vec3& centerB, float radiusB);
bool DoAABB2sOverlap(AABB2 const& aBounds, AABB2 const& bBounds);
bool DoConvexPolygons2DOverlap(DelaunayConvexPoly2D const& polyA, DelaunayConvexPoly2D const& polyB);

bool DoAABB3sOverlap(AABB3 const& aBounds, AABB3 const& bBounds);
bool DoZCylindersOverlap(Vec2 const& aXYCenter, float aCylinderRadius, FloatRange const& aZRange, Vec2 const& bXYCenter, float bCylinderRadius, FloatRange const& bZRange);
bool DoSphereAndAABB3Overlap(Vec3 const& sphereCenter, float radius, AABB3 const& bounds);
bool DoSphereAndZCylinderOverlap(Vec3 const& sphereCenter, float sphereRadius, Vec2 const& xyCenter, float cylinderRadius, FloatRange const& zRange);
bool DoAABB3AndZCylinderOverlap(AABB3 const& bounds, Vec2 const& xyCenter, float cylinderRadius, FloatRange const& zRange);


Vec2 const GetNearestPointOnDisc2D(Vec2 const& refPoint, Vec2 const& center, float radius);
Vec2 const GetNearestPointOnLineSegment2D(Vec2 const& refPoint, LineSegment2 const& lineSegment);
Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 const& refPoint, LineSegment2 const& line);
Vec2 const GetNearestPointOnAABB2D(Vec2 const& refPoint, AABB2 const& bounds);
Vec2 const GetNearestPointOnOBB2D(Vec2 const& refPoint, OBB2 const& bounds);
Vec2 const GetNearestPointOnCapsule2D(Vec2 const& refPoint, Capsule2 const& capsule);
Vec2 const GetNearestPointOnConvexPoly2D(Vec2 const& refPoint, DelaunayConvexPoly2D const& convexPoly);
ConvexPoly2DEdge const GetNearestEdgeOnConvexPoly2D(Vec2 const& refPoint, DelaunayConvexPoly2D const& convexPoly);

Vec3 const GetNearestPointOnSphere(Vec3 const& refPoint, Vec3 const& center, float radius);
Vec3 const GetNearestPointOnZCylinder(Vec3 const& refPoint, Vec2 const& xyCenter, float radius, FloatRange const& zRange);
Vec3 const GetNearestPointOnAABB3D(Vec3 const& refPoint, AABB3 const& bounds);
Vec3 GetRandomDirectionInCone(Vec3 const& forward, float angle);

Mat44 GetOrthonormalBasis(Vec3 const& nonNormalIbasis);

bool PushDiscOutOfPoint2D(Vec2& mobileDiscCenter, float radius, Vec2 const& fixedPoint);
bool PushDiscOutOfDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius);
bool PushDiscOutOfAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox);
bool PushConvexPolysOutOfEachOther(DelaunayConvexPoly2D& convexPolyA, DelaunayConvexPoly2D& convexPolyB);
bool PushConvexPolyOutOfOtherPoly(DelaunayConvexPoly2D const& fixedPolyA, DelaunayConvexPoly2D& convexPolyB);
bool PushSphereOutOfPoint(Vec3& mobileSpherecenter, float radius, Vec3 const& fixedPoint);

bool PushAABB3OutOfPoint(AABB3& mobileAABB3, Vec3 const& fixedPoint);
bool PushAABB3OutOfAABB3(AABB3 const& fixedAABB3, AABB3& mobileAABB3);

bool BounceDiscOffPoint2D(Vec2& mobileDiscCenter, Vec2& velocity, float radius, Vec2 const& fixed, float elasticity = 1.0f);
bool BounceDiscOffDisc2D(Vec2& mobileDiscCenter, Vec2& velocity, float mobileDiscRadius, Vec2 const& fixedDisc, float fixedDiscRadius, float elasticity = 1.0f);
bool BounceDiscOffEachOther2D(Vec2& aCenter, Vec2& aVelocity, float aRadius, Vec2& bCenter, Vec2& bVelocity, float bRadius, float combinedElasticity = 1.0f);

// ---------------------------------------------------------------------------------------------------------------------
// Transform Utilities
void TransformPosition2D(Vec2& position, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPosition2D(Vec2& position, float uniformScale, float rotationDegrees, const Vec2&  translationVec);
void TransformPositionXY3D(Vec3& poistion, float uniformScale, float rotationDegreesAboutZ, const Vec2& translationVec);
void TransformPositionXY3D(Vec3& position, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPosition3D(Vec3& position, Mat44 const& model);

// ---------------------------------------------------------------------------------------------------------------------
// Lerp and Clamp Utilities
float GetFractionWithin(float inValue, float inStart, float inEnd);
float Interpolate(float outStart, float outEnd, float fraction);
Vec2 const Interpolate(Vec2 const& outStart, Vec2 const& outEnd, float fraction);
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMap(float inValue, FloatRange const& inRange, float outStart, float outEnd);
float RangeMap(float inValue, float inStart, float inEnd, FloatRange const& outRange);
float RangeMap(float inValue, FloatRange const& inRange, FloatRange const& outRange);
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float RangeMapClamped(float inValue, FloatRange const& inRange, float outStart, float outEnd);
float RangeMapClamped(float inValue, float inStart, float inEnd, FloatRange const& outRange);
float RangeMapClamped(float inValue, FloatRange const& inRange, FloatRange const& outRange);
float Clamp(float inValue, float const minValue, float const maxValue);
float Clamp(float inValue, FloatRange const& range);
float ClampZeroToOne(float inValue);
int RoundDownToInt(float numberToRound);


float NormalizeByte(unsigned char byteValue);
unsigned char DenormalizeByte(float zeroToOne);


