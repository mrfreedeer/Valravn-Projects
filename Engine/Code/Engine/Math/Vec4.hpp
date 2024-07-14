#pragma once

struct Vec2;
struct Vec3;
struct Mat44;

struct Vec4 {
public:
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;

public:

	Vec4() = default;
	explicit Vec4(float newX, float newY, float newZ, float newW);
	explicit Vec4(Vec2 const& copyFrom);
	explicit Vec4(Vec3 const& copyFrom);
	explicit Vec4(Vec3 const& copyFrom, float newW);
	Vec4(float* floatArray);

	bool operator==(Vec4 const& compareTo) const;
	Vec4 const operator+(Vec4 const& vecToSum) const;
	Vec4 const operator-(Vec4 const& vecToSubtract) const;
	Vec4 const operator*(float scale) const;

	void const operator+=(Vec4 vecToSum);
	void const operator-=(Vec4 vecToSubtract);
	void const operator*=(float scale);

	static Vec4 const ZERO;
	static Vec4 const ONE;
};