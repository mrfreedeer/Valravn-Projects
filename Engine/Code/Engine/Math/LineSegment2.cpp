#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"

LineSegment2::LineSegment2(Vec2 const& lineStart, Vec2 const& lineEnd) :
	m_start(lineStart),
	m_end(lineEnd)
{
}

void LineSegment2::Translate(Vec2 const& translation)
{
	m_start += translation;
	m_end += translation;
}

void LineSegment2::SetCenter(Vec2 const& newCenter)
{
	Vec2 dispDir = (m_end - m_start).GetNormalized();
	float lineLength = (m_end - m_start).GetLength();

	m_start = newCenter + dispDir * (-lineLength * 0.5f);
	m_end = newCenter + dispDir * (lineLength * 0.5f);

}

void LineSegment2::RotateAboutCenter(float rotationDeltaDegrees)
{
	Vec2 dispDir = (m_end - m_start).GetNormalized();
	float lineLength = (m_end - m_start).GetLength();
	Vec2 center = (m_end + m_start) * 0.5f;

	dispDir.RotateDegrees(rotationDeltaDegrees);
	m_start = center + dispDir * (-lineLength * 0.5f);
	m_end = center + dispDir * (lineLength * 0.5f);
}

Vec2 const LineSegment2::GetNearestPoint(Vec2 const& refPoint) const
{
	Vec2 dispLineSegment = (m_end - m_start).GetNormalized();
	Vec2 dispEndToRefPoint = (refPoint - m_end).GetNormalized();

	if (DotProduct2D(dispLineSegment, dispEndToRefPoint) >= 0.0f) {
		return m_end;
	}

	Vec2 dispStartToRefPoint = (refPoint - m_start).GetNormalized();

	if (DotProduct2D(dispLineSegment, dispStartToRefPoint) <= 0.0f) {
		return m_start;
	}

	Vec2 projectedOntoSegment = GetProjectedOnto2D((refPoint - m_start), dispLineSegment);

	return m_start + projectedOntoSegment;

}
