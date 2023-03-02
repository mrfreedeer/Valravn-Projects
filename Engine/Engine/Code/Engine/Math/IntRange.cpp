#include "Engine/Math/IntRange.hpp"
#include "Engine/Core/EngineCommon.hpp"


IntRange const IntRange::ZERO_TO_ONE = IntRange(0, 1);

IntRange::IntRange(int min, int max) :
	m_min(min),
	m_max(max)
{
}

bool IntRange::IsOnRange(int valueToCheck) const
{
	return (m_min <= valueToCheck) && (valueToCheck <= m_max);
}

bool IntRange::IsOverlappingWith(IntRange const& compareTo) const
{
	bool otherRangeMinContained = IsOnRange(compareTo.m_min);
	bool otherRangeMaxContained = IsOnRange(compareTo.m_max);

	bool otherRangeContainsMin = compareTo.IsOnRange(m_min);
	bool otherRangeContainsMax = compareTo.IsOnRange(m_max);

	return  otherRangeMinContained || otherRangeMaxContained || otherRangeContainsMin || otherRangeContainsMax;
}

IntRange IntRange::GetOverlappingRange(IntRange const& compareTo) const
{
	if (!IsOverlappingWith(compareTo)) return IntRange();

	int biggerMin = 0;
	int smallerMax = 0;
	biggerMin = (compareTo.m_min > m_min) ? compareTo.m_min : m_min;
	smallerMax = (compareTo.m_max < m_max) ? compareTo.m_max : m_max;
	return IntRange(biggerMin, smallerMax);
}

void IntRange::operator=(IntRange const& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

bool IntRange::operator==(IntRange const& compareTo) const
{
	return (compareTo.m_min == m_min) && (compareTo.m_max == m_max);
}

bool IntRange::operator!=(IntRange const& compareTo) const
{
	return (compareTo.m_min != m_min) || (compareTo.m_max != m_max);
}


void IntRange::SetFromText(char const* text)
{
	Strings intRangeInfo = SplitStringOnDelimiter(text, '~');

	if (intRangeInfo.size() == 2) {
		m_min = static_cast<int>(std::stoi(intRangeInfo[0].c_str()));
		m_max = static_cast<int>(std::stoi(intRangeInfo[1].c_str()));
		return;
	}


	ERROR_AND_DIE(Stringf("INTRANGE SETSTRING VECTOR TOO LONG: %s", text));
}