#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

AABB3 const AABB3::ZERO_TO_ONE = AABB3(Vec3::ZERO, Vec3::ONE);

AABB3::AABB3(float const minX, float const minY, float const minZ, float const maxX, float const maxY, float const maxZ) :
	m_mins(minX, minY, minZ),
	m_maxs(maxX, maxY, maxZ)
{
}

AABB3::AABB3(const Vec3& newMinBounds, const Vec3& newMaxBounds) :
	m_mins(newMinBounds),
	m_maxs(newMaxBounds)
{
}

bool const AABB3::IsPointInside(Vec3 const& point) const
{
	bool insideX = (m_mins.x < point.x) && (point.x < m_maxs.x);
	bool insideY = (m_mins.y < point.y) && (point.y < m_maxs.y);
	bool insideZ = (m_mins.z < point.z) && (point.z < m_maxs.z);
	return insideX && insideY && insideZ;
}

Vec3 const AABB3::GetCenter() const
{
	float centerX = (m_maxs.x + m_mins.x) * 0.5f;
	float centerY = (m_maxs.y + m_mins.y) * 0.5f;
	float centerZ = (m_maxs.z + m_mins.z) * 0.5f;

	return Vec3(centerX, centerY, centerZ);
}

Vec3 const AABB3::GetDimensions() const
{
	return m_maxs - m_mins;
}

Vec3 const AABB3::GetNearestPoint(Vec3 const& pointToCheck) const
{
	float clampedX = Clamp(pointToCheck.x, m_mins.x, m_maxs.x);
	float clampedY = Clamp(pointToCheck.y, m_mins.y, m_maxs.y);
	float clampedZ = Clamp(pointToCheck.z, m_mins.z, m_maxs.z);

	return Vec3(clampedX, clampedY, clampedZ);
}

void AABB3::GetCorners(Vec3* cornersOut) const
{
	// Counter clockwise front
	cornersOut[0] = m_mins;
	cornersOut[1] = Vec3(m_maxs.x, m_mins.y, m_mins.z);
	cornersOut[2] = Vec3(m_maxs.x, m_mins.y, m_maxs.z);
	cornersOut[3] = Vec3(m_mins.x, m_mins.y, m_maxs.z);

	// Counter clockwise back
	cornersOut[4] = Vec3(m_mins.x, m_maxs.y, m_mins.z);
	cornersOut[5] = Vec3(m_maxs.x, m_maxs.y, m_mins.z);
	cornersOut[6] = m_maxs;
	cornersOut[7] = Vec3(m_mins.x, m_maxs.y, m_maxs.z);
}

void AABB3::Translate(Vec3 const& translation)
{
	m_mins += translation;
	m_maxs += translation;
}

void AABB3::SetCenter(Vec3 const& newCenter)
{
	Vec3 dim = GetDimensions();
	m_mins = newCenter - dim * 0.5f;
	m_maxs = m_mins + dim;
}

void AABB3::SetDimensions(Vec3 const& newDimensions)
{
	Vec3 center = GetCenter();
	m_mins = center - newDimensions * 0.5f;
	m_maxs = m_mins + newDimensions;
}

void AABB3::SetDimensions(float dimX, float dimY, float dimZ)
{
	Vec3 newDimensions(dimX, dimY, dimZ);
	Vec3 center = GetCenter();
	m_mins = center - newDimensions * 0.5f;
	m_maxs = m_mins + newDimensions;
}

void AABB3::SetFromText(char const* text)
{
	Strings aabb2Info = SplitStringOnDelimiter(text, '~');


	if (aabb2Info.size() == 2) {
		m_mins.SetFromText(aabb2Info[0].c_str());
		m_maxs.SetFromText(aabb2Info[1].c_str());
		return;
	}

	ERROR_AND_DIE(Stringf("FLOATRANGE SETSTRING VECTOR TOO LONG: %s", text));


}

void AABB3::StretchToIncludePoint(Vec3 const& pointToInclude)
{
	bool isPointInside = IsPointInside(pointToInclude);
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

	if (pointToInclude.z < m_mins.z) {
		m_mins.z = pointToInclude.z;
	}
	else if (pointToInclude.z > m_maxs.z) {
		m_maxs.z = pointToInclude.z;
	}
}

FloatRange const AABB3::GetXRange() const
{
	return FloatRange(m_mins.x, m_maxs.x);
}

FloatRange const AABB3::GetYRange() const
{
	return FloatRange(m_mins.y, m_maxs.y);
}

FloatRange const AABB3::GetZRange() const
{
	return FloatRange(m_mins.z, m_maxs.z);
}

AABB2 const AABB3::GetXYBounds() const
{
	return AABB2(m_mins.x, m_mins.y, m_maxs.x, m_maxs.y);
}

AABB2 const AABB3::GetXZBounds() const
{
	return AABB2(m_mins.x, m_mins.z, m_maxs.x, m_maxs.z);
}

AABB2 const AABB3::GetYZBounds() const
{
	return AABB2(m_mins.y, m_mins.z, m_maxs.y, m_maxs.z);
}