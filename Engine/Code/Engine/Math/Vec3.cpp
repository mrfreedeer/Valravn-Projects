#include "Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <math.h>



Vec3 const Vec3::ZERO = Vec3();
Vec3 const Vec3::ONE = Vec3(1.0f, 1.0f, 1.0f);

Vec3::Vec3(float initialX, float initialY, float initialZ) :
	x(initialX),
	y(initialY),
	z(initialZ)
{}

Vec3::Vec3(Vec2 const& copyFrom) :
	x(copyFrom.x),
	y(copyFrom.y),
	z(0.0f)
{
}

const bool Vec3::operator==(const Vec3& vecToCompare) const
{
	return (x == vecToCompare.x) && (y == vecToCompare.y) && (z == vecToCompare.z);
}

const bool Vec3::operator!=(const Vec3& vecToCompare) const
{
	return (x != vecToCompare.x) || (y != vecToCompare.y) || (z != vecToCompare.z);
}

const Vec3 Vec3::operator-(const Vec3& substractor) const
{
	return Vec3(x - substractor.x, y - substractor.y, z - substractor.z);
}

const Vec3 Vec3::operator-() const
{
	return Vec3(-x, -y, -z);
}

const Vec3 Vec3::operator+(const Vec3& vecToadd) const
{
	return Vec3(x + vecToadd.x, y + vecToadd.y, z + vecToadd.z);
}

const Vec3 Vec3::operator*(float uniformScale) const
{
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}

const Vec3 Vec3::operator/(float uniformScale) const
{
	return Vec3(x / uniformScale, y / uniformScale, z / uniformScale);
}

void Vec3::operator+=(const Vec3& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

void Vec3::operator-=(const Vec3& subtractor)
{
	x -= subtractor.x;
	y -= subtractor.y;
	z -= subtractor.z;
}

//-----------------------------------------------------------------------------------------------
void Vec3::operator*=(float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}

void Vec3::operator/=(float uniformScale)
{
	x /= uniformScale;
	y /= uniformScale;
	z /= uniformScale;
}



float Vec3::GetLength() const
{
	float lengthSquared = (x * x) + (y * y) + (z * z);
	return sqrtf(lengthSquared);
}

float Vec3::GetLengthXY() const
{
	float distance = x * x + y * y;
	return sqrtf(distance);
}

float Vec3::GetLengthSquared() const
{
	return (x * x) + (y * y) + (z * z);
}

float Vec3::GetLengthXYSquared() const
{
	return (x * x) + (y * y);
}

float Vec3::GetAngleAboutZRadians() const
{
	return atan2f(y, x);
}

float Vec3::GetAngleAboutZDegrees() const
{
	return Atan2Degrees(y, x);
}

float Vec3::GetAngleAboutYRadians() const {
	float length2D = Vec2(x, y).GetLength();
	return atan2f(-z, length2D);
}

float Vec3::GetAngleAboutYDegrees() const {
	float length2D = Vec2(x, y).GetLength();
	return Atan2Degrees(-z, length2D);
}

const Vec3 Vec3::GetRotatedAboutZRadians(float deltaRadians) const
{
	float lengthXY = GetLengthXY();
	float angleAboutZ = GetAngleAboutZRadians();
	angleAboutZ += deltaRadians;

	float rotatedX = cosf(angleAboutZ) * lengthXY;
	float rotatedY = sinf(angleAboutZ) * lengthXY;

	return Vec3(rotatedX, rotatedY, z);
}

const Vec3 Vec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
	float lengthXY = GetLengthXY();
	float angleAboutZ = GetAngleAboutZDegrees();
	angleAboutZ += deltaDegrees;

	float rotatedX = CosDegrees(angleAboutZ) * lengthXY;
	float rotatedY = SinDegrees(angleAboutZ) * lengthXY;

	return Vec3(rotatedX, rotatedY, z);
}

const Vec3 Vec3::GetClamped(float maxLength) const
{
	float length = GetLength();

	if (length <= maxLength) {
		return *this;
	}
	else {
		float clampedX = x / length;
		float clampedY = y / length;
		float clampedZ = z / length;

		Vec3 clampedVec(clampedX, clampedY, clampedZ);
		clampedVec *= maxLength;
		return clampedVec;
	}
}

const Vec3 Vec3::GetNormalized() const
{
	float length = GetLength();
	if(length == 0.0f) return Vec3::ZERO;

	float normX = x / length;
	float normY = y / length;
	float normZ = z / length;

	return Vec3(normX, normY, normZ);
}

const Vec3 Vec3::GetReflected(Vec3 const& surfaceNormal) const
{
	float projToNormal = DotProduct3D(*this, surfaceNormal.GetNormalized());
	Vec3 horizontalComp = surfaceNormal * projToNormal;
	Vec3 verticalComp = *this - horizontalComp;

	Vec3 resultingVec = verticalComp - horizontalComp;
	return resultingVec;
}

const Vec3 Vec3::GetRotatedAroundAxis(float angleDeg, Vec3 const& axis) const
{
		Vec3 rotatedVec = *this * CosDegrees(angleDeg) + (CrossProduct3D(axis, *this)) * SinDegrees(angleDeg) + axis * (DotProduct3D(axis, *this)) * (1 - CosDegrees(angleDeg)); // Rodrigues' rotation formula

		return rotatedVec.GetNormalized();

}

void Vec3::SetFromNotation(std::string const& notation) {
	SetFromNotation(notation.c_str());
}


void Vec3::SetFromNotation(char const* notation)
{
	if (!_stricmp(notation, "i")) {
		x = 1.0f;
		y = 0.0f;
		z = 0.0f;
	}

	if (!_stricmp(notation, "-i")) {
		x = -1.0f;
		y = 0.0f;
		z = 0.0f;
	}

	if (!_stricmp(notation, "j")) {
		x = 0.0f;
		y = 1.0f;
		z = 0.0f;
	}

	if (!_stricmp(notation, "-j")) {
		x = 0.0f;
		y = -1.0f;
		z = 0.0f;
	}

	if (!_stricmp(notation, "k")) {
		x = 0.0f;
		y = 0.0f;
		z = 1.0f;
	}

	if (!_stricmp(notation, "-k")) {
		x = 0.0f;
		y = 0.0f;
		z = -1.0f;
	}
}

void Vec3::SetFromText(const char* text)
{
	Strings numbers = SplitStringOnDelimiter(text, ',');

	GUARANTEE_OR_DIE(numbers.size() == 3, Stringf("VEC3 SET FROM STRING TOO LONG: %s", text));

	x = std::stof(numbers[0]);
	y = std::stof(numbers[1]);
	z = std::stof(numbers[2]);
}

void Vec3::Normalize()
{
	float length = GetLength();
	if (length == 0) return;

	x /= length;
	y /= length;
	z /= length;
}

void Vec3::Reflect(Vec3 const& surfaceNormal)
{
	float projToNormal = DotProduct3D(*this, surfaceNormal.GetNormalized());
	Vec3 horizontalComp = surfaceNormal * projToNormal;
	Vec3 verticalComp = *this - horizontalComp;

	Vec3 resultingVec = verticalComp - horizontalComp;
	this->x = resultingVec.x;
	this->y = resultingVec.y;
}

void Vec3::SetLength(float newLength)
{
	Normalize();
	*this *= newLength;
}

void Vec3::ClampLength(float maxLength)
{
	float length = GetLength();
	if (length > maxLength) {
		Normalize();
		*this *= maxLength;
	}
}

std::string const Vec3::ToString() const
{
	return Stringf("(%.2f, %.2f, %.2f)", x, y, z);
}

const Vec3 operator*(float uniformScale, const Vec3& vecToScale)
{
	return vecToScale * uniformScale;
}

Vec3 Vec3::InterpolateVec3(float t, Vec3 const& start, Vec3 const& end)
{
	float x = Interpolate(t, start.x, end.x);
	float y = Interpolate(t, start.y, end.y);
	float z = Interpolate(t, start.z, end.z);

	return Vec3(x, y, z);
}

Vec3 Vec3::InterpolateClampedVec3(float t, Vec3 const& start, Vec3 const& end)
{

	float x = Interpolate(t, start.x, end.x);
	float y = Interpolate(t, start.y, end.y);
	float z = Interpolate(t, start.z, end.z);

	x = Clamp(x, start.x, end.x);
	y = Clamp(y, start.y, end.y);
	z = Clamp(z, start.z, end.z);
	
	return Vec3(x,y,z);
}
