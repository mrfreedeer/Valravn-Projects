#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture):
	m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension),
	m_fontGlyphsSpriteSheet(fontTexture, IntVec2(16, 16))
{
}

Texture const& BitmapFont::GetTexture() const
{
	return m_fontGlyphsSpriteSheet.GetTexture();
}

void BitmapFont::AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textMins, float cellHeight, std::string const& text, Rgba8 const& tint, float cellAspect, int maxGlyphsToDraw)
{
	Vec2 letterPosition = textMins;
	letterPosition.x += cellAspect * cellHeight * 0.5f;
	letterPosition.y += cellHeight * 0.5f;
	for (int charIndex = 0, glyphsDrawn = 0; charIndex < text.size(); charIndex++, glyphsDrawn++) {
		if (glyphsDrawn >= maxGlyphsToDraw) return;
		int const& letter = text[charIndex];
		AABB2 letterUVs = m_fontGlyphsSpriteSheet.GetSpriteUVs(letter);

		OBB2 letterOBB2;
		letterOBB2.m_center = letterPosition;
		letterOBB2.m_halfDimensions = Vec2(cellAspect * cellHeight * 0.5f, cellHeight * 0.5f);

		AddVertsForOBB2D(vertexArray, letterOBB2, tint, letterUVs);
		letterPosition.x += cellAspect * cellHeight;
	}

}

void BitmapFont::AddVertsForTextInBox2D(std::vector<Vertex_PCU>& vertexArray, AABB2 const& box, float cellHeight, std::string const& text, Rgba8 const& tint,
										float cellAspect, Vec2 const& alignment, TextBoxMode mode, int maxGlyphsToDraw)
{
	Strings textSplitByNewline = SplitStringOnDelimiter(text, '\n');

	float textHeight = textSplitByNewline.size() * cellHeight;
	float biggestTextWidth = GetBiggestTextWidth(textSplitByNewline, cellHeight, cellAspect);

	Vec2 const boxDimensions = box.GetDimensions();
	float recommendedYScale = boxDimensions.y / textHeight;
	float recommendedXScale = boxDimensions.x / biggestTextWidth;

	// Is recommendedXscale smaller or equal than recommendedYScale, then assign it to smallestScale. Else assign recommendedYScale
	float smallestScale = (recommendedXScale <= recommendedYScale) ? recommendedXScale : recommendedYScale;

	float usedTextWidth = biggestTextWidth;
	float usedTextHeight = textHeight;
	float usedCellHeight = cellHeight;

	if (mode == TextBoxMode::SHRINK_TO_FIT && smallestScale < 1.0f) {
		usedTextWidth *= smallestScale;
		usedTextHeight *= smallestScale;
		usedCellHeight *= smallestScale;
	}

	AABB2 allTextABB2(Vec2::ZERO, Vec2(usedTextWidth, usedTextHeight));
	box.AlignABB2WithinBounds(allTextABB2, alignment);

	float textGroupMinY = (boxDimensions.y - (usedTextHeight)) * alignment.y;
	for (int subTextIndex = 0, glyphsDrawn = 0; subTextIndex < (int)textSplitByNewline.size(); subTextIndex++) {
		int remainingGlyphs = maxGlyphsToDraw - glyphsDrawn;
		if (remainingGlyphs <= 0) {
			return;
		}

		std::string const& textToDraw = textSplitByNewline[subTextIndex];
		float lineTextWidth = GetTextWidth(usedCellHeight, textToDraw, cellAspect);
		AABB2 textLineABB2(Vec2::ZERO, Vec2(lineTextWidth, textHeight));
		allTextABB2.AlignABB2WithinBounds(textLineABB2, alignment);

		float textLineYPos = textGroupMinY + (usedCellHeight * float((textSplitByNewline.size() - subTextIndex - 1)));

		textLineABB2.m_mins.y = box.m_mins.y + textLineYPos;

		AddVertsForText2D(vertexArray, textLineABB2.m_mins, usedCellHeight, textToDraw, tint, cellAspect, remainingGlyphs);

		glyphsDrawn += (int)textToDraw.size();

	}

}

float BitmapFont::GetTextWidth(float cellHeight, std::string const& text, float cellAspect) const
{
	return text.size() * cellHeight * cellAspect;
}

float BitmapFont::GetGlyphAspect(int glyphUnicode) const
{
	UNUSED(glyphUnicode);
	return 1.0f;
}

float BitmapFont::GetBiggestTextWidth(Strings const& stringsVector, float cellHeight, float cellAspect) const
{
	float biggestTextWidth = -1.0f;

	for (int subTextIndex = 0; subTextIndex < stringsVector.size(); subTextIndex++) {
		float textWidth = GetTextWidth(cellHeight, stringsVector[subTextIndex], cellAspect);
		if (textWidth > biggestTextWidth) {
			biggestTextWidth = textWidth;
		}
	}

	return biggestTextWidth;
}
