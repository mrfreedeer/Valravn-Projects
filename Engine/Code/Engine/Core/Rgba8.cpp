#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"

Rgba8 const Rgba8::WHITE = Rgba8();
Rgba8 const Rgba8::TRANSPARENT_WHITE = Rgba8(255,255,255,100);
Rgba8 const Rgba8::BLACK = Rgba8(0, 0, 0, 255);
Rgba8 const Rgba8::TRANSPARENT_BLACK = Rgba8(0, 0, 0, 200);
Rgba8 const Rgba8::SILVER = Rgba8(192, 192, 192, 255);
Rgba8 const Rgba8::GRAY = Rgba8(128, 128, 128, 255);
Rgba8 const Rgba8::TRANSPARENT_GRAY = Rgba8(128, 128, 128, 200);
Rgba8 const Rgba8::RED = Rgba8(255, 0, 0, 255);
Rgba8 const Rgba8::LIGHTRED = Rgba8(125, 0, 0, 255);
Rgba8 const Rgba8::BLUE = Rgba8(0, 0, 255, 255);
Rgba8 const Rgba8::LIGHTBLUE = Rgba8(125, 125, 255, 255);
Rgba8 const Rgba8::GREEN = Rgba8(0, 255, 0, 255);
Rgba8 const Rgba8::CYAN = Rgba8(0, 255, 255, 255);
Rgba8 const Rgba8::YELLOW = Rgba8(255, 255, 0, 255);
Rgba8 const Rgba8::MAGENTA = Rgba8(255, 0, 255, 255);
Rgba8 const Rgba8::ORANGE = Rgba8(255, 128, 0, 255);

Rgba8::Rgba8()
{
}

Rgba8::Rgba8(float colorIntensity)
{
	r = DenormalizeByte(colorIntensity);
	g = DenormalizeByte(colorIntensity);
	b = DenormalizeByte(colorIntensity);
	a = DenormalizeByte(1.0f);
}

Rgba8::Rgba8(float const* colorAsFloats)
{
	r = DenormalizeByte(colorAsFloats[0]);
	g = DenormalizeByte(colorAsFloats[1]);
	b = DenormalizeByte(colorAsFloats[2]);
	a = DenormalizeByte(colorAsFloats[3]);
}

Rgba8::Rgba8(unsigned char inR, unsigned char inG, unsigned char inB, unsigned char inA) :
	r(inR),
	g(inG),
	b(inB),
	a(inA)
{
}

Rgba8 const Rgba8::InterpolateColors(Rgba8 const& colorA, Rgba8 const& colorB, float fraction)
{
	Rgba8 interpolatedColor;
	interpolatedColor.r = static_cast<unsigned char>(Interpolate(colorA.r, colorB.r, fraction));
	interpolatedColor.g = static_cast<unsigned char>(Interpolate(colorA.g, colorB.g, fraction));
	interpolatedColor.b = static_cast<unsigned char>(Interpolate(colorA.b, colorB.b, fraction));
	interpolatedColor.a = static_cast<unsigned char>(Interpolate(colorA.a, colorB.a, fraction));

	return interpolatedColor;
}


void Rgba8::SetFromText(const char* text)
{
	
	Strings colorInfo = SplitStringOnDelimiter(text, ',');

	if (colorInfo.size() == 3) {
		r = static_cast<unsigned char>(std::atof(colorInfo[0].c_str()));
		g = static_cast<unsigned char>(std::atof(colorInfo[1].c_str()));
		b = static_cast<unsigned char>(std::atof(colorInfo[2].c_str()));
		a = 255;
		return;
	}
	else if (colorInfo.size() == 4) {
		r = static_cast<unsigned char>(std::atof(colorInfo[0].c_str()));
		g = static_cast<unsigned char>(std::atof(colorInfo[1].c_str()));
		b = static_cast<unsigned char>(std::atof(colorInfo[2].c_str()));
		a = static_cast<unsigned char>(std::atof(colorInfo[3].c_str()));
		return;
	}

	ERROR_AND_DIE(Stringf("RGBA8 SETSTRING VECTOR TOO LONG: %s", text));
}

std::string Rgba8::ToString() const
{
	return Stringf("R:%u G:%u B:%u A:%u", r, g, b, a);
}

bool Rgba8::Equals(Rgba8 const& compareTo, bool includeAlpha) const
{
	bool areEqual = (compareTo.r == r) && (compareTo.g == g) && (compareTo.b == b);
	if (includeAlpha) {
		(areEqual == areEqual) && (compareTo.a == a);
	}

	return areEqual;
}

void Rgba8::SetIntesity(float colorIntensity)
{
	r = DenormalizeByte(colorIntensity);
	g = DenormalizeByte(colorIntensity);
	b = DenormalizeByte(colorIntensity);
	a = DenormalizeByte(1.0f);
}

void Rgba8::GetAsFloats(float* colorAsFloats) const
{
	colorAsFloats[0] = NormalizeByte(r);
	colorAsFloats[1] = NormalizeByte(g);
	colorAsFloats[2] = NormalizeByte(b);
	colorAsFloats[3] = NormalizeByte(a);
}

bool Rgba8::operator==(Rgba8 const& otherColor) const {
	return (otherColor.r == r) && (otherColor.g == g) && (otherColor.b == b) && (otherColor.a == a);
}

void Rgba8::operator*=(float colorIntensity) {
	float newR = NormalizeByte(r) * colorIntensity;
	float newG = NormalizeByte(g) * colorIntensity;
	float newB = NormalizeByte(b) * colorIntensity;

	r = DenormalizeByte(newR);
	g = DenormalizeByte(newG);
	b = DenormalizeByte(newB);

}