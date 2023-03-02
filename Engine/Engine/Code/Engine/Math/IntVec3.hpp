#pragma once
struct IntVec3 {
	int x;
	int y;
	int z;

public:
	static IntVec3 const ZERO;
	static IntVec3 const ONE;

	IntVec3() = default;
	explicit IntVec3(int x, int y, int z);
	IntVec3(const IntVec3& copyFrom) = default;
	

	// Mutators
	void SetFromText(const char* text);

	void Rotate2D90Degrees();
	void Rotate2DMinus90Degrees();

	void operator-=(IntVec3 const& vecToSubtract);
	void operator+=(IntVec3 const& vecToAdd);
	void operator*=(int scale);

	// Accessors
	float const GetLength() const;
	int const GetLengthSquared() const;
	int const GetTaxicabLength() const;
	float const GetOrientation2DRadians() const;
	float const GetOrientation2DDegrees() const;
	IntVec3 const GetRotated2D90Degrees() const;
	IntVec3 const GetRotated2DMinus90Degrees() const;

	IntVec3 const operator-(IntVec3 const& vecToSubtract) const;
	IntVec3 const operator+(IntVec3 const& vecToSubtract) const;
	IntVec3 const operator*(int scale) const;
	bool operator==(IntVec3 const& compareTo) const;
	bool operator!=(IntVec3 const& compareTo) const;
};



