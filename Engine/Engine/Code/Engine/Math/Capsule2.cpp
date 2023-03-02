#include "Engine/Math/Capsule2.hpp"
#include "Engine/Core/EngineCommon.hpp"


Capsule2::Capsule2(Vec2 const& boneStart, Vec2 const& boneEnd, float radius):
	m_bone(boneStart, boneEnd),
	m_radius(radius)
{
}

void Capsule2::Translate(Vec2 const& translation)
{
	m_bone.Translate(translation);
}

void Capsule2::SetCenter(Vec2 const& newCenter)
{
	m_bone.SetCenter(newCenter);
}

void Capsule2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_bone.RotateAboutCenter(rotationDeltaDegrees);
}

bool Capsule2::IsPointInside(Vec2 const& refPoint) const
{
	Vec2 nearestPointToCapsuleBone = m_bone.GetNearestPoint(refPoint);
	float distanceSqrToPoint = GetDistanceSquared2D(nearestPointToCapsuleBone, refPoint);

	return distanceSqrToPoint <= m_radius * m_radius;
}

Vec2 const Capsule2::GetNearestPoint(Vec2 const& refPoint) const
{
	Vec2 nearestPointToCapsule = m_bone.GetNearestPoint(refPoint);
	Vec2 dispToPoint = (refPoint - nearestPointToCapsule).GetClamped(m_radius);

	return nearestPointToCapsule + dispToPoint;
}
