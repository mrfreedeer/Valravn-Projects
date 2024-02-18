#pragma once
#define CELL_ASPECT 0.7f

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/EngineCommon.hpp"

struct Vertex_PCU;
class Texture;

enum class TextBoxMode {
	SHRINK_TO_FIT,
	OVERRUN
};


class BitmapFont {
	friend class Renderer;

private:
	BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture);
public:
	Texture const& GetTexture() const;

	void AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textMins,
		float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::WHITE, float cellAspect = CELL_ASPECT, int maxGlyphsToDraw = ARBITRARILY_LARGE_INT_VALUE);

	void AddVertsForTextInBox2D(std::vector<Vertex_PCU>& vertexArray, AABB2 const& box, float cellHeight, std::string const& text,
		Rgba8 const& tint = Rgba8::WHITE, float cellAspect = 1.0f, Vec2 const& alignment = Vec2(0.5f, 0.5f), TextBoxMode mode = TextBoxMode::SHRINK_TO_FIT,
		int maxGlyphsToDraw = ARBITRARILY_LARGE_INT_VALUE);

	float GetTextWidth(float cellHeight, std::string const& text, float cellAspect = CELL_ASPECT) const;

protected:
	float GetGlyphAspect(int glyphUnicode) const;
	float GetBiggestTextWidth(Strings const& stringsVector, float cellHeight, float cellAspect) const;

protected:
	std::string m_fontFilePathNameWithNoExtension;
	SpriteSheet m_fontGlyphsSpriteSheet;
};