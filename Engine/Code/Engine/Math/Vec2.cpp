#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <math.h>
#include "Engine/Core/StringUtils.hpp"

Vec2 const Vec2::ZERO = Vec2();
Vec2 const Vec2::ONE = Vec2(1.0f, 1.0f);


//-----------------------------------------------------------------------------------------------
Vec2::Vec2(Vec2 const& copy)
	: x(copy.x)
	, y(copy.y)
{
}


//-----------------------------------------------------------------------------------------------
Vec2::Vec2(float initialX, float initialY)
	: x(initialX)
	, y(initialY)
{
}

Vec2::Vec2(Vec3 const& copyFrom)
	: x(copyFrom.x)
	, y(copyFrom.y)
{
}

Vec2::Vec2(IntVec2 const& copyFrom)
	: x(static_cast<float>(copyFrom.x))
	, y(static_cast<float>(copyFrom.y))
{
}

const Vec2 Vec2::MakeFromPolarRadians(float orientationRadians, float length)
{
	float x = cosf(orientationRadians);
	float y = sinf(orientationRadians);
	return Vec2(x, y) * length;
}

const Vec2 Vec2::MakeFromPolarDegrees(float orientationDegrees, float length)
{
	float x = CosDegrees(orientationDegrees);
	float y = SinDegrees(orientationDegrees);
	return Vec2(x, y) * length;
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator+ (Vec2 const& vecToAdd) const
{
	return Vec2(this->x + vecToAdd.x, this->y + vecToAdd.y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator-(Vec2 const& vecToSubtract) const
{
	return Vec2(this->x - vecToSubtract.x, this->y - vecToSubtract.y);
}


//------------------------------------------------------------------------------------------------
const Vec2 Vec2::operator-() const
{
	return Vec2(-this->x, -this->y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator*(float uniformScale) const
{
	return Vec2(this->x * uniformScale, this->y * uniformScale);
}


//------------------------------------------------------------------------------------------------
const Vec2 Vec2::operator*(Vec2 const& vecToMultiply) const
{
	return Vec2(this->x * vecToMultiply.x, this->y * vecToMultiply.y);
}


//-----------------------------------------------------------------------------------------------
const Vec2 Vec2::operator/(float inverseScale) const
{
	return Vec2(this->x / inverseScale, this->y / inverseScale);
}

//-----------------------------------------------------------------------------------------------
void Vec2::operator+=(Vec2 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator-=(Vec2 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator/=(const float uniformDivisor)
{
	x /= uniformDivisor;
	y /= uniformDivisor;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator=(Vec2 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

float Vec2::GetLength() const
{
	return sqrtf((x * x) + (y * y));
}

float Vec2::GetLengthSquared() const
{
	return x * x + y * y;
}

const Vec2 Vec2::GetNormalized() const
{
	float length = GetLength();
	if (length == 0) return Vec2::ZERO;

	return Vec2(x / length, y / length);
}

Vec2 const Vec2::GetReflected(Vec2 const& impactSurfaceNormal, float elasticity) const
{
	float projToNormal = DotProduct2D(*this, impactSurfaceNormal);
	Vec2 horizontalComp = impactSurfaceNormal * projToNormal * elasticity;
	Vec2 verticalComp = *this - horizontalComp;

	return verticalComp - horizontalComp;
}

std::string const Vec2::ToString() const
{
	return Stringf("(%.2f, %.2f)", x, y);
}

const Vec2 Vec2::GetRotated90Degrees() const {
	return Vec2(-y, x);
}

const Vec2 Vec2::GetRotatedMinus90Degrees() const {
	return Vec2(y, -x);
}

const Vec2 Vec2::GetRotatedRadians(float deltaRadians) const
{
	float length = GetLength();
	float orientation = GetOrientationRadians();
	orientation += deltaRadians;

	float tempX = cosf(orientation);
	float tempY = sinf(orientation);

	return Vec2(tempX, tempY) * length;
}

const Vec2 Vec2::GetRotatedDegrees(float deltaDegrees) const
{
	float length = GetLength();
	float orientation = GetOrientationDegrees();
	orientation += deltaDegrees;

	float tempX = CosDegrees(orientation);
	float tempY = SinDegrees(orientation);

	return Vec2(tempX, tempY) * length;
}

const Vec2 Vec2::GetClamped(float maxLength) const
{
	float length = GetLength();

	if (length <= maxLength) return *this;
	else {
		Vec2 clampedVec(*this);
		clampedVec.SetLength(maxLength);
		return clampedVec;
	}
}


float Vec2::GetOrientationDegrees() const
{
	return Atan2Degrees(y, x);
}

float Vec2::GetOrientationRadians() const
{
	return atan2f(y, x);
}

void Vec2::SetLength(float newLength)
{
	float orientation = GetOrientationDegrees();
	x = CosDegrees(orientation);
	y = SinDegrees(orientation);

	x *= newLength;
	y *= newLength;
}

void Vec2::ClampLength(float maxLength)
{
	float length = GetLength();

	if (length <= maxLength) {
		return;
	}
	else {
		float orientation = GetOrientationRadians();
		x = cosf(orientation) * maxLength;
		y = sinf(orientation) * maxLength;
	}
}

void Vec2::Rotate90Degrees()
{
	float tempX = x;
	x = -y;
	y = tempX;
}

void Vec2::RotateMinus90Degrees()
{
	float tempX = x;

	x = y;
	y = -tempX;
}

void Vec2::RotateRadians(float deltaRadians)
{
	float orientation = GetOrientationRadians();
	orientation += deltaRadians;

	float length = GetLength();

	x = cosf(orientation) * length;
	y = sinf(orientation) * length;
}

void Vec2::RotateDegrees(float deltaDegrees)
{
	float orientation = GetOrientationDegrees();
	orientation += deltaDegrees;

	float length = GetLength();

	x = CosDegrees(orientation) * length;
	y = SinDegrees(orientation) * length;
}

void Vec2::Normalize()
{
	float length = GetLength();
	if (length == 0) return;

	x /= length;
	y /= length;
}

float Vec2::NormalizeAndGetPreviousLength()
{
	float length = GetLength();

	x /= length;
	y /= length;

	return length;
}

void Vec2::Reflect(Vec2 const& impactSurface, float elasticity)
{
	float projToNormal = DotProduct2D(*this, impactSurface.GetNormalized());
	Vec2 horizontalComp = impactSurface * projToNormal;
	Vec2 verticalComp = *this - horizontalComp;

	Vec2 resultingVec = verticalComp - (horizontalComp * elasticity);
	this->x = resultingVec.x;
	this->y = resultingVec.y;
}


void Vec2::SetOrientationRadians(float newOrientationRadians)
{
	float length = GetLength();

	x = cosf(newOrientationRadians);
	y = sinf(newOrientationRadians);

	x *= length;
	y *= length;

}

void Vec2::SetOrientationDegrees(float newOrientationDegrees)
{
	float length = GetLength();
	x = length * CosDegrees(newOrientationDegrees);
	y = length * SinDegrees(newOrientationDegrees);

}

void Vec2::SetPolarRadians(float newOrientationRadians, float newLength)
{
	x = cosf(newOrientationRadians);
	y = sinf(newOrientationRadians);

	x *= newLength;
	y *= newLength;

}

void Vec2::SetPolarDegrees(float newOrientationDegrees, float newLength)
{
	x = CosDegrees(newOrientationDegrees);
	y = SinDegrees(newOrientationDegrees);

	x *= newLength;
	y *= newLength;
}

void Vec2::SetFromText(const char* text)
{
	Strings numbers = SplitStringOnDelimiter(text, ',');

	GUARANTEE_OR_DIE(numbers.size() == 2, Stringf("VEC2 SET FROM STRING TOO LONG: %s", text));

	x = std::stof(numbers[0]);
	y = std::stof(numbers[1]);
}

//-----------------------------------------------------------------------------------------------
const Vec2 operator*(float uniformScale, Vec2 const& vecToScale)
{
	return Vec2(uniformScale * vecToScale.x, uniformScale * vecToScale.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator==(Vec2 const& compare) const
{
	return (this->x == compare.x && this->y == compare.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=(Vec2 const& compare) const
{
	return !(this->x == compare.x && this->y == compare.y);
}



