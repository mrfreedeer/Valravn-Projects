#pragma once

struct IntRange {

	int m_min = 0;
	int m_max = 0;

	IntRange() {};
	IntRange(int min, int max);

	bool IsOnRange(int valueToCheck) const;
	bool IsOverlappingWith(IntRange const& compareTo) const;
	IntRange GetOverlappingRange(IntRange const& compareTo) const;

	void operator=(IntRange const& copyFrom);
	bool operator==(IntRange const& compareTo) const;
	bool operator!=(IntRange const& compareTo) const;

	void SetFromText(char const* text);
	static IntRange const ZERO_TO_ONE;

};