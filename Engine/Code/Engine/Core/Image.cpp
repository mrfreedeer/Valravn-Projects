#include "Engine/Core/Image.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#define STB_IMAGE_IMPLEMENTATION // Exactly one .CPP (this Image.cpp) should #define this before #including stb_image.h
#include "ThirdParty/stb/stb_image.h"

Image::Image(char const* imageFilePath) :
	m_imageFilePath(imageFilePath)
{
	int bytesPerTexel = 0; // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
	int numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

									// Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	unsigned char* texelData = stbi_load(imageFilePath, &m_dimensions.x, &m_dimensions.y, &bytesPerTexel, numComponentsRequested);

	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));

	for (int y = 0, texelIndex = 0; y < m_dimensions.y; y++) {
		for (int x = 0; x < m_dimensions.x; x++, texelIndex += bytesPerTexel) {
			
			unsigned char r = texelData[texelIndex];
			unsigned char g = texelData[texelIndex + 1];
			unsigned char b = texelData[texelIndex + 2];
			unsigned char a = 255;
			if (bytesPerTexel == 4) {
				a = texelData[texelIndex + 3];
			}

			m_rgbaTexels.emplace_back(r, g, b, a);
		}
	}

	stbi_image_free(texelData);

}

Image::Image(IntVec2 const& size, Rgba8 color):
	m_dimensions(size)
{
	size_t texelAmount = static_cast<size_t>(size.x) * static_cast<size_t>(size.y);
	m_rgbaTexels.resize(texelAmount);

	for (int texelIndex = 0; texelIndex < texelAmount; texelIndex++) {
		m_rgbaTexels.push_back(color);
	}
}

Image::~Image()
{
}

std::string const& Image::GetImageFilePath() const
{
	return m_imageFilePath;
}

Rgba8 Image::GetTexelColor(Vec2 const& uv) const
{
	float xCoord = uv.x * static_cast<float>(m_dimensions.x - 1.0f);
	float yCoord = uv.y * static_cast<float>(m_dimensions.y - 1.0f);

	int xRounded = RoundDownToInt(xCoord);
	int yRounded = RoundDownToInt(yCoord);

	return GetTexelColor(xRounded, yRounded);
}

IntVec2 Image::GetDimensions() const
{
	return m_dimensions;
}

Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const
{
	int index = texelCoords.y * m_dimensions.x + texelCoords.x;
	return m_rgbaTexels[index];
}
Rgba8 Image::GetTexelColor(int x, int y) const
{
	int index = y * m_dimensions.x + x;
	return m_rgbaTexels[index];
}

Rgba8 Image::GetTexelColor(float x, float y) const
{
	return GetTexelColor(Vec2(x,y));
}


void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{
	int index = texelCoords.y * m_dimensions.x + texelCoords.x;
	m_rgbaTexels[index] = newColor;
}

void* const Image::GetRawData() const
{
	return (void *)m_rgbaTexels.data();
}

size_t Image::GetSizeBytes() const
{
	int totalDim = m_dimensions.x * m_dimensions.y;
	return (totalDim) * sizeof(Rgba8);
}
