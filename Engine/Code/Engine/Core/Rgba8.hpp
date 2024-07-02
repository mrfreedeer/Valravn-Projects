#pragma once
#include <string>

struct Rgba8 {
public:
	unsigned char r = 255;
	unsigned char g = 255;
	unsigned char b = 255;
	unsigned char a = 255;

public:
	Rgba8();
	Rgba8(float colorIntensity);
	Rgba8(float const* colorAsFloats);
	Rgba8(unsigned char inR, unsigned char inG, unsigned char inB, unsigned char inA = 255);
	static Rgba8 const InterpolateColors(Rgba8 const& colorA, Rgba8 const& colorB, float fraction);

	void SetFromText(const char* text);
	std::string ToString() const;
	bool Equals(Rgba8 const& compareTo, bool includeAlpha = true) const;

	void SetIntesity(float uniformColorIntensity);
	void GetAsFloats(float* colorAsFloats) const;

	bool operator==(Rgba8 const& otherColor) const;
	void operator*=(float colorIntensity);

	static Rgba8 const WHITE;
	static Rgba8 const TRANSPARENT_WHITE;
	static Rgba8 const BLACK;
	static Rgba8 const TRANSPARENT_BLACK;
	static Rgba8 const SILVER;
	static Rgba8 const GRAY;
	static Rgba8 const TRANSPARENT_GRAY;
	static Rgba8 const RED;
	static Rgba8 const LIGHTRED;
	static Rgba8 const BLUE;
	static Rgba8 const LIGHTBLUE;
	static Rgba8 const GREEN;
	static Rgba8 const CYAN;
	static Rgba8 const YELLOW;
	static Rgba8 const MAGENTA;
	static Rgba8 const ORANGE;


};