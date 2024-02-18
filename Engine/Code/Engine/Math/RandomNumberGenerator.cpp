#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"
#include <stdlib.h>

int RandomNumberGenerator::GetRandomIntLessThan(int maxNotInclusive)
{
	return rand() % maxNotInclusive;
}

int RandomNumberGenerator::GetRandomIntInRange(int minInclusive, int maxInclusive)
{
	int range = 1 + maxInclusive - minInclusive;
	return (rand() % range) + minInclusive;
}

float RandomNumberGenerator::GetRandomFloatZeroUpToOne()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float RandomNumberGenerator::GetRandomFloatInRange(float minInclusive, float maxInclusive)
{
	float range = maxInclusive - minInclusive;
	return (GetRandomFloatZeroUpToOne() * range) + minInclusive;
}

float RandomNumberGenerator::GetRandomFloatInRange(FloatRange const& range)
{
	return GetRandomFloatInRange(range.m_min, range.m_max);
}

int RandomNumberGenerator::GetRandomIntInRange(IntRange const& range)
{
	return GetRandomIntInRange(range.m_min, range.m_max);
}

