#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Mat44.hpp"


Vec4 const Vec4::ZERO = Vec4();
Vec4 const Vec4::ONE = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

Vec4::Vec4(float newX, float newY, float newZ, float newW):
	x(newX),
	y(newY),
	z(newZ),
	w(newW)
{
}

Vec4::Vec4(Vec2 const& copyFrom):
	x(copyFrom.x),
	y(copyFrom.y)
{
}

Vec4::Vec4(Vec3 const& copyFrom) :
	x(copyFrom.x),
	y(copyFrom.y),
	z(copyFrom.z)
{
}

Vec4::Vec4(Vec3 const& copyFrom, float newW):
	x(copyFrom.x),
	y(copyFrom.y),
	z(copyFrom.z),
	w(newW)
{
}

Vec4::Vec4(float* floatArray):
	x(floatArray[0]),
	y(floatArray[1]),
	z(floatArray[2]),
	w(floatArray[3])
{

}

bool Vec4::operator==(Vec4 const& compareTo) const
{
	return (x == compareTo.x) && (y == compareTo.y) && (z == compareTo.z) && (w == compareTo.w);
}

Vec4 const Vec4::operator+(Vec4 const& vecToSum) const
{
	return Vec4(x + vecToSum.x, y + vecToSum.y, z + vecToSum.z, w + vecToSum.w);
}

Vec4 const Vec4::operator-(Vec4 const& vecToSubtract) const
{
	return Vec4(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z, w - vecToSubtract.w);
}

Vec4 const Vec4::operator*(float scale) const
{
	return Vec4(x * scale, y * scale, z * scale, w * scale);
}

void const Vec4::operator+=(Vec4 vecToSum)
{
	x += vecToSum.x;
	y += vecToSum.y; 
	z += vecToSum.z; 
	w += vecToSum.w;
}

void const Vec4::operator-=(Vec4 vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	w -= vecToSubtract.w;
	z -= vecToSubtract.z;
}

void const Vec4::operator*=(float scale)
{
	x *= scale;
	y *= scale;
	z *= scale;
	w *= scale;
}
