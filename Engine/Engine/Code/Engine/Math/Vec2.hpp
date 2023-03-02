#pragma once
#include <string>

struct Vec3;
struct IntVec2;

//-----------------------------------------------------------------------------------------------
struct Vec2
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;

	static Vec2 const ZERO;
	static Vec2 const ONE;

public:
	// Construction/Destruction
	~Vec2() {}												// destructor (do nothing)
	Vec2() {}												// default constructor (do nothing)
	Vec2( Vec2 const& copyFrom );							// copy constructor (from another vec2)
	explicit Vec2( float initialX, float initialY );		// explicit constructor (from x, y)
	explicit Vec2(IntVec2 const& copyFrom);
	explicit Vec2(Vec3 const& copyFrom);
	static const Vec2 MakeFromPolarRadians(float orientationRadians, float length = 1.f);
	static const Vec2 MakeFromPolarDegrees(float orientationDegrees, float length = 1.f);

	// Operators (const)
	bool		operator==( Vec2 const& compare ) const;		// vec2 == vec2
	bool		operator!=( Vec2 const& compare ) const;		// vec2 != vec2
	const Vec2	operator+( Vec2 const& vecToAdd ) const;		// vec2 + vec2
	const Vec2	operator-( Vec2 const& vecToSubtract ) const;	// vec2 - vec2
	const Vec2	operator-() const;								// -vec2, i.e. "unary negation"
	const Vec2	operator*( float uniformScale ) const;			// vec2 * float
	const Vec2	operator*( Vec2 const& vecToMultiply ) const;	// vec2 * vec2
	const Vec2	operator/( float inverseScale ) const;			// vec2 / float
	


	// Operators (self-mutating / non-const)
	void		operator+=( Vec2 const& vecToAdd );				// vec2 += vec2
	void		operator-=( Vec2 const& vecToSubtract );		// vec2 -= vec2
	void		operator*=( const float uniformScale );			// vec2 *= float
	void		operator/=( const float uniformDivisor );		// vec2 /= float
	void		operator=( Vec2 const& copyFrom );				// vec2 = vec2

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
	friend const Vec2 operator*( float uniformScale, Vec2 const& vecToScale );	// float * vec2

	// Accesors
	float GetLength() const;
	float GetLengthSquared() const;
	float GetOrientationDegrees() const;
	float GetOrientationRadians() const;
	const Vec2 GetRotated90Degrees() const;
	const Vec2 GetRotatedMinus90Degrees() const;
	const Vec2 GetRotatedRadians(float deltaRadians) const;
	const Vec2 GetRotatedDegrees(float deltaDegrees) const;
	const Vec2 GetClamped(float maxLength) const;
	const Vec2 GetNormalized() const;
	Vec2 const GetReflected(Vec2 const& impactSurfaceNormal, float elasticity = 1.0f) const;
	std::string const ToString() const;

	// Mutators
	void SetOrientationRadians(float newOrientationRadians);
	void SetOrientationDegrees(float newOrientationDegrees);
	void SetPolarRadians(float newOrientationRadians, float newLength);
	void SetPolarDegrees(float newOrientationDegrees, float newLength);
	void SetFromText(const char* text);

	void SetLength(float newLength);
	void ClampLength(float maxLength);

	void Rotate90Degrees();
	void RotateMinus90Degrees();
	void RotateRadians(float deltaRadians);
	void RotateDegrees(float deltaDegrees);
	void Normalize();
	float NormalizeAndGetPreviousLength();

	void Reflect(Vec2 const& impactSurfaceNormal, float elasticity = 1.0f);
};


