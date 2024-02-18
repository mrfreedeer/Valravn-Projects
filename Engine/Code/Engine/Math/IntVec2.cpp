#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <math.h>


IntVec2 const IntVec2::ZERO = IntVec2();
IntVec2 const IntVec2::ONE = IntVec2(1, 1);

IntVec2::IntVec2(int x, int y)
{
	this->x = x;
	this->y = y;
}

IntVec2::IntVec2(const Vec2& copyFrom):
	x(RoundDownToInt(copyFrom.x)),
	y(RoundDownToInt(copyFrom.y))
{

}

float const IntVec2::GetLength() const
{
	return sqrtf(static_cast<float>((x * x) + (y * y)));
}

int const IntVec2::GetLengthSquared() const
{
	return (x * x) + (y * y);
}

int const IntVec2::GetTaxicabLength() const
{
	return abs(x) + abs(y);
}

float const IntVec2::GetOrientationRadians() const
{
	return atan2f(static_cast<float>(y) , static_cast<float>(x));
}

float const IntVec2::GetOrientationDegrees() const
{
	return Atan2Degrees(static_cast<float>(y), static_cast<float>(x));
}

IntVec2 const IntVec2::GetRotated90Degrees() const
{
	return IntVec2(-y, x);
}

IntVec2 const IntVec2::GetRotatedMinus90Degrees() const
{
	return IntVec2(y, -x);
}

void IntVec2::SetFromText(const char* text)
{
	Strings numbers = SplitStringOnDelimiter(text, ',');
	GUARANTEE_OR_DIE(numbers.size() == 2, Stringf("INTVEC2 SET FROM STRING TOO LONG: %s", text));

	x = std::stoi(numbers[0]);
	y = std::stoi(numbers[1]);
}

void IntVec2::Rotate90Degrees()
{
	int tempX = x;
	x = -y;
	y = tempX;
}

void IntVec2::RotateMinus90Degrees()
{
	int tempX = x;
	x = y;
	y = -tempX;
}

IntVec2 const IntVec2::operator-(IntVec2 const& vecToSubtract) const {
	return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}

IntVec2 const IntVec2::operator+(IntVec2 const& vecToAdd) const {
	return IntVec2(x + vecToAdd.x, y + vecToAdd.y);
}

IntVec2 const IntVec2::operator*(int scale) const {
	return IntVec2(x * scale, y * scale);
}

IntVec2 const IntVec2::operator/(int scale) const
{
	return IntVec2(x / scale, y / scale);
}

void IntVec2::operator-=(IntVec2 const& vecToSubtract) {
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}

void IntVec2::operator+=(IntVec2 const& vecToAdd) {
	x += vecToAdd.x;
	y += vecToAdd.y;
}

void IntVec2::operator*=(int scale) {
	x *= scale;
	y *= scale;
}

bool IntVec2::operator==(IntVec2 const& compareTo) const {
	return (compareTo.x == x) && (compareTo.y == y);
}

bool IntVec2::operator!=(IntVec2 const& compareTo) const {
	return (compareTo.x != x) || (compareTo.y != y);
}