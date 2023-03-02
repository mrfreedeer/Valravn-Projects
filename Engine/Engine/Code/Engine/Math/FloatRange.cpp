#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/EngineCommon.hpp"


FloatRange const FloatRange::ZERO_TO_ONE = FloatRange(0.0f, 1.0f);
FloatRange const FloatRange::ZERO = FloatRange(0.0f, 0.0f);

FloatRange::FloatRange(float min, float max) :
	m_min(min),
	m_max(max)
{
}

bool FloatRange::IsOnRange(float valueToCheck) const
{
	return (m_min <= valueToCheck) && (valueToCheck <= m_max);
}

bool FloatRange::IsOverlappingWith(FloatRange const& compareTo) const
{
	bool otherRangeMinContained = IsOnRange(compareTo.m_min);
	bool otherRangeMaxContained = IsOnRange(compareTo.m_max);

	bool otherRangeContainsMin = compareTo.IsOnRange(m_min);
	bool otherRangeContainsMax = compareTo.IsOnRange(m_max);

	return  otherRangeMinContained || otherRangeMaxContained || otherRangeContainsMin || otherRangeContainsMax;
}

FloatRange FloatRange::GetOverlappingRange(FloatRange const& compareTo) const
{
	if (!IsOverlappingWith(compareTo)) return FloatRange();

	float biggerMin = 0;
	float smallerMax = 0;
	biggerMin = (compareTo.m_min > m_min) ? compareTo.m_min : m_min;
	smallerMax = (compareTo.m_max < m_max) ? compareTo.m_max : m_max;
	return FloatRange(biggerMin, smallerMax);
}

void FloatRange::operator=(FloatRange const& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

bool FloatRange::operator==(FloatRange const& compareTo) const
{
	return (compareTo.m_min == m_min) && ( compareTo.m_max == m_max);
}

bool FloatRange::operator!=(FloatRange const& compareTo) const
{
	return (compareTo.m_min != m_min) || (compareTo.m_max != m_max);
}

void FloatRange::SetFromText(char const* text)
{
	Strings floatRangeInfo = SplitStringOnDelimiter(text, '~');

	if (floatRangeInfo.size() == 2) {
		m_min = static_cast<float>(std::atof(floatRangeInfo[0].c_str()));
		m_max = static_cast<float>(std::atof(floatRangeInfo[1].c_str()));
		return;
	}

	ERROR_AND_DIE(Stringf("FLOATRANGE SETSTRING VECTOR TOO LONG: %s", text));
}
