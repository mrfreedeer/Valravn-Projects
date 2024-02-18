#pragma once

struct FloatRange {

	float m_min = 0.0f;
	float m_max = 0.0f;

	FloatRange() {};
	FloatRange(float min, float max);

	bool IsOnRange(float valueToCheck) const;
	bool IsOverlappingWith(FloatRange const& compareTo) const;
	FloatRange GetOverlappingRange(FloatRange const& compareTo) const;


	void operator=(FloatRange const& copyFrom);
	bool operator==(FloatRange const& compareTo) const;
	bool operator!=(FloatRange const& compareTo) const;

	void SetFromText(char const* text);

	static FloatRange const ZERO_TO_ONE;
	static FloatRange const ZERO;
};