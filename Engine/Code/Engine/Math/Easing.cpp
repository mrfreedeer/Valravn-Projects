#include "Engine/Math/Easing.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Curves.hpp"

float SmoothStart2(float t)
{
	return t * t;
}

float SmoothStart3(float t)
{
	return t * t * t;
}

float SmoothStart4(float t)
{
	return t * t * t * t;
}

float SmoothStart5(float t)
{
	return t * t * t * t * t;
}

float SmoothStart6(float t)
{
	return t * t * t * t * t * t;
}

float SmoothStop2(float tZeroToOne)
{
	float oneMinusT = 1.0f - tZeroToOne;

	return 1.0f - (oneMinusT * oneMinusT);
}

float SmoothStop3(float tZeroToOne)
{
	float oneMinusT = 1.0f - tZeroToOne;

	return 1.0f - (oneMinusT * oneMinusT * oneMinusT);
}

float SmoothStop4(float tZeroToOne)
{
	float oneMinusT = 1.0f - tZeroToOne;

	return 1.0f - (oneMinusT * oneMinusT * oneMinusT * oneMinusT);
}

float SmoothStop5(float tZeroToOne)
{
	float oneMinusT = 1.0f - tZeroToOne;

	return 1.0f - (oneMinusT * oneMinusT * oneMinusT * oneMinusT * oneMinusT);
}

float SmoothStop6(float tZeroToOne)
{
	float oneMinusT = 1.0f - tZeroToOne;

	return 1.0f - (oneMinusT * oneMinusT * oneMinusT * oneMinusT * oneMinusT * oneMinusT);
}

float SmoothStep3(float tZeroToOne)
{
	return Interpolate(SmoothStart2(tZeroToOne), SmoothStop2(tZeroToOne), tZeroToOne);
}

float SmoothStep5(float tZeroToOne)
{
	return ComputeQuinticBezier1D(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, tZeroToOne);
}

float Hesitate3(float tZeroToOne)
{
	return ComputeCubicBezier1D(0, 1, 0, 1, tZeroToOne);
}

float Hesitate5(float tZeroToOne)
{
	return ComputeQuinticBezier1D(0, 1, 0, 1, 0, 1, tZeroToOne);
}

float CustomEasingFunction(float tZeroToOne)
{
	float tSqr = (tZeroToOne * tZeroToOne) + 0.5f;
	float cosT = cosf(tSqr * 5);
	float thing = fabsf(cosT) - (1 - tZeroToOne);
	

	return  fabsf(thing) + (thing * 0.1f);
}

