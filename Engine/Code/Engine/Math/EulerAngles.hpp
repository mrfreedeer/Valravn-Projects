#pragma once
#include <string>
struct Vec3;
struct Mat44;

struct EulerAngles {
public:
	EulerAngles() = default;
	EulerAngles(float yawDegres, float pitchDegrees, float rollDegrees);
	void GetVectors_XFwd_YLeft_ZUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const;
	Mat44 GetMatrix_XFwd_YLeft_ZUp() const;
	Vec3 const GetXForward() const;
	Vec3 const GetYLeft() const;
	Vec3 const GetZUp() const;

	// Operators const
	EulerAngles const operator*(float multiplier) const;
	bool operator!=(EulerAngles const& otherAngles) const;

	// Operators Mutators
	void operator+=(EulerAngles const& otherAngles);
	void operator*=(float multiplier);
	void SetFromText(const char* text);
	
	static EulerAngles const ZERO;

	EulerAngles const operator-(EulerAngles const& otherAngles) const;
	std::string const ToString() const;

	// this DOES NOT take into account the roll, which is fine for now. 
	static EulerAngles CreateEulerAngleFromForward(Vec3 const& forward);
public:
	float m_yawDegrees = 0.0f;
	float m_pitchDegrees = 0.0f;
	float m_rollDegrees = 0.0f;

};