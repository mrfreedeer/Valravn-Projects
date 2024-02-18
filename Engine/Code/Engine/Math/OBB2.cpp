#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB2::OBB2(Vec2 const& center, Vec2 const& halfDims) :
	m_center(center),
	m_halfDimensions(halfDims)
{
}

void OBB2::GetCornerPoints(Vec2*& cornerPoints) const
{
	Vec2 jBasisNormal = m_iBasisNormal.GetRotated90Degrees();

	Vec2 rightDisp = m_iBasisNormal * m_halfDimensions.x;
	Vec2 forwardDisp = jBasisNormal * m_halfDimensions.y;
	cornerPoints[0] = m_center - rightDisp - forwardDisp;
	cornerPoints[1] = m_center + rightDisp - forwardDisp;
	cornerPoints[2] = m_center + rightDisp + forwardDisp;
	cornerPoints[3] = m_center - rightDisp + forwardDisp;
}

Vec2* const OBB2::GetCornerPoints() const
{
	Vec2* cornerPoints = new Vec2[4];
	GetCornerPoints(cornerPoints);
	return cornerPoints;
}

Vec2 const OBB2::GetLocalPosForWorldPos(Vec2 const& worldPos) const
{
	Vec2 dispToWorldPos = worldPos - m_center;
	float projInI = DotProduct2D(m_iBasisNormal, dispToWorldPos);
	float projInJ = DotProduct2D(m_iBasisNormal.GetRotated90Degrees(), dispToWorldPos);

	return Vec2(projInI, projInJ);
}

Vec2 const OBB2::GetWorldPosForLocalPos(Vec2 const& localPos) const
{
	return m_center + (m_iBasisNormal * localPos.x) + (m_iBasisNormal.GetRotated90Degrees() * localPos.y);
}

Vec2 const OBB2::ConvertDisplacementToLocalSpace(Vec2 const& worldDir) const
{
	float projInI = DotProduct2D(m_iBasisNormal, worldDir);
	float projInJ = DotProduct2D(m_iBasisNormal.GetRotated90Degrees(), worldDir);

	return Vec2(projInI, projInJ);
}

Vec2 const OBB2::ConvertLocalDisplacementToWorldSpace(Vec2 const& localDir) const
{
	return (m_iBasisNormal * localDir.x) + (m_iBasisNormal.GetRotated90Degrees() * localDir.y);
}

bool OBB2::IsPointInside(Vec2 const& refPoint) const
{

	Vec2 pointInLocal = GetLocalPosForWorldPos(refPoint);

	return (-m_halfDimensions.x < pointInLocal.x&& pointInLocal.x < m_halfDimensions.x) && (-m_halfDimensions.y < pointInLocal.y&& pointInLocal.y < m_halfDimensions.y);
}

Vec2 const OBB2::GetNearestPoint(Vec2 const& refPoint) const
{
	Vec2 dispToPoint = refPoint - m_center;

	float projI = DotProduct2D(m_iBasisNormal, dispToPoint);
	float projJ = DotProduct2D(m_iBasisNormal.GetRotated90Degrees(), dispToPoint);

	float nearestIPoint = Clamp(projI, -m_halfDimensions.x, m_halfDimensions.x);
	float nearestJPoint = Clamp(projJ, -m_halfDimensions.y, m_halfDimensions.y);

	return GetWorldPosForLocalPos(Vec2(nearestIPoint, nearestJPoint));
}

void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_iBasisNormal.RotateDegrees(rotationDeltaDegrees);
}
