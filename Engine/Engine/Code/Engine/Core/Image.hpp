#pragma once
#include "Engine/Math/IntVec2.hpp"
#include <string>
#include <vector>
 
struct Rgba8;
struct Vec2;

class Image {
	friend class Renderer;
public:
	Image() = delete;
	Image(char const* imageFilePath);
	Image(IntVec2 const& size, Rgba8 color);
	~Image();

	IntVec2 GetDimensions() const;
	std::string const& GetImageFilePath() const;
	Rgba8 GetTexelColor(Vec2 const& uv) const;
	Rgba8 GetTexelColor(IntVec2 const& texelCoords) const;
	Rgba8 GetTexelColor(int x, int y) const;
	Rgba8 GetTexelColor(float x, float y) const;
	void SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor);
	void* const GetRawData() const;
	size_t GetSizeBytes() const;

private:
	std::string m_imageFilePath;
	IntVec2 m_dimensions = IntVec2::ZERO;
	std::vector<Rgba8> m_rgbaTexels;

};