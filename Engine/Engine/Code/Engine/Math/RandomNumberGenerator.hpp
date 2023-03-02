#pragma once

struct FloatRange;
struct IntRange;

class RandomNumberGenerator {
public:
	int GetRandomIntLessThan(int maxNotInclusive);
	int GetRandomIntInRange(int minInclusive, int maxInclusive);
	float GetRandomFloatZeroUpToOne();
	float GetRandomFloatInRange(float minInclusive, float maxInclusive);
	float GetRandomFloatInRange(FloatRange const& range);
	int GetRandomIntInRange(IntRange const& range);
};
