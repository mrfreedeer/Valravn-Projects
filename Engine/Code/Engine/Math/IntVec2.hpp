#pragma once
struct Vec2;

struct IntVec2 {
	int x;
	int y;
public:
	static IntVec2 const ZERO;
	static IntVec2 const ONE;

	IntVec2() = default;
	explicit IntVec2(int x, int y);
	IntVec2(const IntVec2& copyFrom) = default;
	IntVec2(const Vec2& copyFrom);
	

	// Mutators
	void SetFromText(const char* text);

	void Rotate90Degrees();
	void RotateMinus90Degrees();

	void operator-=(IntVec2 const& vecToSubtract);
	void operator+=(IntVec2 const& vecToAdd);
	void operator*=(int scale);

	// Accessors
	float const GetLength() const;
	int const GetLengthSquared() const;
	int const GetTaxicabLength() const;
	float const GetOrientationRadians() const;
	float const GetOrientationDegrees() const;
	IntVec2 const GetRotated90Degrees() const;
	IntVec2 const GetRotatedMinus90Degrees() const;



	IntVec2 const operator-(IntVec2 const& vecToSubtract) const;
	IntVec2 const operator+(IntVec2 const& vecToSubtract) const;
	IntVec2 const operator*(int scale) const;
	IntVec2 const operator/(int scale) const;
	bool operator==(IntVec2 const& compareTo) const;
	bool operator!=(IntVec2 const& compareTo) const;



};



