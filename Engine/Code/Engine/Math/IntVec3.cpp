#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <math.h>


IntVec3 const IntVec3::ZERO = IntVec3();
IntVec3 const IntVec3::ONE = IntVec3(1, 1, 1);

IntVec3::IntVec3(int x, int y, int z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

float const IntVec3::GetLength() const
{
	return sqrtf(static_cast<float>((x * x) + (y * y) + (z*z)));
}

int const IntVec3::GetLengthSquared() const
{
	return (x * x) + (y * y) + (z*z);
}

int const IntVec3::GetTaxicabLength() const
{
	return abs(x) + abs(y) + abs(z);
}

float const IntVec3::GetOrientation2DRadians() const
{
	return atan2f(static_cast<float>(y) , static_cast<float>(x));
}

float const IntVec3::GetOrientation2DDegrees() const
{
	return Atan2Degrees(static_cast<float>(y), static_cast<float>(x));
}

IntVec3 const IntVec3::GetRotated2D90Degrees() const
{
	return IntVec3(-y, x, z);
}

IntVec3 const IntVec3::GetRotated2DMinus90Degrees() const
{
	return IntVec3(y, -x, z);
}

void IntVec3::SetFromText(const char* text)
{
	Strings numbers = SplitStringOnDelimiter(text, ',');
	GUARANTEE_OR_DIE(numbers.size() == 3, Stringf("IntVec3 SET FROM STRING TOO LONG: %s", text));

	x = std::stoi(numbers[0]);
	y = std::stoi(numbers[1]);
	z = std::stoi(numbers[2]);
}

void IntVec3::Rotate2D90Degrees()
{
	int tempX = x;
	x = -y;
	y = tempX;
}

void IntVec3::Rotate2DMinus90Degrees()
{
	int tempX = x;
	x = y;
	y = -tempX;
}

IntVec3 const IntVec3::operator-(IntVec3 const& vecToSubtract) const {
	return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

IntVec3 const IntVec3::operator+(IntVec3 const& vecToAdd) const {
	return IntVec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

IntVec3 const IntVec3::operator*(int scale) const {
	return IntVec3(x * scale, y * scale, z * scale);
}

void IntVec3::operator-=(IntVec3 const& vecToSubtract) {
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}

void IntVec3::operator+=(IntVec3 const& vecToAdd) {
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}

void IntVec3::operator*=(int scale) {
	x *= scale;
	y *= scale;
	z *= scale;
}

bool IntVec3::operator==(IntVec3 const& compareTo) const {
	return (compareTo.x == x) && (compareTo.y == y) && (compareTo.z == z);
}

bool IntVec3::operator!=(IntVec3 const& compareTo) const {
	return (compareTo.x != x) || (compareTo.y != y) || (compareTo.z != z);
}