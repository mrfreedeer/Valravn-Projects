#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

AABB2 const AABB2::ZERO_TO_ONE = AABB2(0.0f, 0.0f, 1.0f, 1.0f);

AABB2::AABB2(const Vec2& newMinBounds, const Vec2& newMaxBounds)
{
	m_mins = newMinBounds;
	m_maxs = newMaxBounds;
}

AABB2::AABB2(float const minX, float const minY, float const maxX, float const maxY)
{
	m_mins = Vec2(minX, minY);
	m_maxs = Vec2(maxX, maxY);
}

bool const AABB2::IsPointInside(Vec2 const& point) const
{
	bool insideX = (m_mins.x < point.x) && (point.x < m_maxs.x);
	bool insideY = (m_mins.y < point.y) && (point.y < m_maxs.y);

	return insideX && insideY;
}

Vec2 const AABB2::GetCenter() const
{
	float centerX = (m_maxs.x + m_mins.x) * 0.5f;
	float centerY = (m_maxs.y + m_mins.y) * 0.5f;
	return Vec2(centerX, centerY);
}

Vec2 const AABB2::GetDimensions() const
{
	return  m_maxs - m_mins;
}

Vec2 const AABB2::GetNearestPoint(Vec2 const& pointToCheck) const
{

	float clampedX = Clamp(pointToCheck.x, m_mins.x, m_maxs.x);
	float clampedY = Clamp(pointToCheck.y, m_mins.y, m_maxs.y);
	return Vec2(clampedX, clampedY);
}

Vec2 const AABB2::GetPointAtUV(Vec2 const& point) const
{
	float u = Interpolate(m_mins.x, m_maxs.x, point.x);
	float v = Interpolate(m_mins.y, m_maxs.y, point.y);

	return Vec2(u, v);
}

Vec2 const AABB2::GetPointAtUV(float x, float y) const
{
	float u = Interpolate(m_mins.x, m_maxs.x, x);
	float v = Interpolate(m_mins.y, m_maxs.y, y);

	return Vec2(u, v);
}

Vec2 const AABB2::GetUVForPoint(Vec2 const& point) const
{
	Vec2 translatedPoint = point - m_mins;
	Vec2 dim = GetDimensions();

	return Vec2(translatedPoint.x / dim.x, translatedPoint.y / dim.y);
}

void AABB2::Translate(Vec2 const& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB2::SetCenter(Vec2 const& newCenter)
{
	Vec2 dim = GetDimensions();
	m_mins = newCenter - dim * 0.5f;
	m_maxs = m_mins + dim;
}

void AABB2::SetDimensions(Vec2 const& newDimensions)
{
	Vec2 center = GetCenter();
	m_mins = center - newDimensions * 0.5f;
	m_maxs = m_mins + newDimensions;

}

void AABB2::SetDimensions(float dimX, float dimY)
{
	Vec2 newDimensions(dimX, dimY);
	Vec2 center = GetCenter();
	m_mins = center - newDimensions * 0.5f;
	m_maxs = m_mins + newDimensions;

}

void AABB2::StretchToIncludePoint(Vec2 const& pointToInclude)
{
	bool isPointInside = (m_mins.x <= pointToInclude.x) && (pointToInclude.x <= m_maxs.x) &&
		(m_mins.y <= pointToInclude.y) && (pointToInclude.y <= m_maxs.y);
	if (isPointInside) return;

	if (pointToInclude.x < m_mins.x) {
		m_mins.x = pointToInclude.x;
	}
	else if (pointToInclude.x > m_maxs.x) {
		m_maxs.x = pointToInclude.x;
	}

	if (pointToInclude.y < m_mins.y) {
		m_mins.y = pointToInclude.y;
	}
	else if (pointToInclude.y > m_maxs.y) {
		m_maxs.y = pointToInclude.y;
	}
}

void AABB2::SetFromText(char const* text)
{
	Strings aabb2Info = SplitStringOnDelimiter(text, '~');


	if (aabb2Info.size() == 2) {
		m_mins.SetFromText(aabb2Info[0].c_str());
		m_maxs.SetFromText(aabb2Info[1].c_str());
		return;
	}

	ERROR_AND_DIE(Stringf("FLOATRANGE SETSTRING VECTOR TOO LONG: %s", text));


}

void AABB2::AlignABB2WithinBounds(AABB2& toAlign, Vec2 const& alignment) const
{
	Vec2 const toAlignDimensions = toAlign.GetDimensions();
	Vec2 const padding = GetDimensions() - toAlignDimensions;


	Vec2 newAlignedCenter = Vec2::ZERO;
	newAlignedCenter.x = m_mins.x + (padding.x * alignment.x) + (toAlignDimensions.x * 0.5f);
	newAlignedCenter.y = m_mins.y + (padding.y * alignment.y) + (toAlignDimensions.y * 0.5f);

	toAlign.SetCenter(newAlignedCenter);
}

FloatRange const AABB2::GetXRange() const
{
	return FloatRange(m_mins.x, m_maxs.x);
}

FloatRange const AABB2::GetYRange() const
{
	return FloatRange(m_mins.y, m_maxs.y);
}

AABB2 const AABB2::GetBoxWithin(AABB2 const& boxUVs) const
{
	Vec2 bottomLeft = GetPointAtUV(boxUVs.m_mins);
	Vec2 topRight = GetPointAtUV(boxUVs.m_maxs);

	return AABB2(bottomLeft, topRight);
}


AABB2 const AABB2::GetBoxWithin(Vec2 const& minUVs, Vec2 const& maxUVs) const
{
	Vec2 bottomLeft = GetPointAtUV(minUVs);
	Vec2 topRight = GetPointAtUV(maxUVs);

	return AABB2(bottomLeft, topRight);
}

AABB2 const AABB2::GetBoxWithin(float minU, float minV, float maxU, float maxV) const
{
	return GetBoxWithin(Vec2(minU, minV), Vec2(maxU, maxV));
}