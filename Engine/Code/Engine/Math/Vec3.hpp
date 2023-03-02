#pragma once
#include <string>

struct Vec2;
struct Vec3 {
public:
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;


	static Vec3 const ZERO;
	static Vec3 const ONE;

public:
	Vec3() = default;
	explicit Vec3(float initialX, float initialY, float initialZ);
	explicit Vec3(Vec2 const& copyFrom);

	float GetLength() const;
	float GetLengthXY() const;
	float GetLengthSquared() const;
	float GetLengthXYSquared() const;
	float GetAngleAboutZRadians() const;
	float GetAngleAboutZDegrees() const;
	float GetAngleAboutYRadians() const;
	float GetAngleAboutYDegrees() const;

	const Vec3 GetRotatedAboutZRadians(float deltaRadians) const;
	const Vec3 GetRotatedAboutZDegrees(float deltaDegrees) const;
	const Vec3 GetClamped(float maxLength) const;
	const Vec3 GetNormalized() const;
	const Vec3 GetReflected(Vec3 const& surfaceNormal) const;
	const Vec3 GetRotatedAroundAxis(float angleDeg, Vec3 const& axis) const;

	// Mutators
	void SetFromNotation(char const* notation);
	void SetFromNotation(std::string const& notation);
	void SetFromText(const char* text);
	void Normalize();
	void Reflect(Vec3 const& surfaceNormal);
	void SetLength(float newLength);
	void ClampLength(float maxLength);

	// Operators Const
	const bool operator==(const Vec3& vecToCompare) const;
	const bool operator!=(const Vec3 & vecToCompare)const;
	const Vec3 operator-(const Vec3& substractor) const;
	const Vec3 operator+(const Vec3& vecToadd) const;
	const Vec3 operator*(float uniformScale) const;
	const Vec3 operator/(float uniformScale) const;
	const Vec3 operator-() const;

	// Operators Mutators
	void operator+=(const Vec3& vecToAdd);
	void operator-=(const Vec3& subtractor);
	void operator*=(float uniformScale);
	void operator/=(float uniformScale);

	std::string const ToString() const;

	// Standalone
	friend const Vec3 operator*(float uniformScale, const Vec3& vecToScale);


	static Vec3 InterpolateVec3(float t, Vec3 const& start, Vec3 const& end);
	static Vec3 InterpolateClampedVec3(float t, Vec3 const& start, Vec3 const& end);
};