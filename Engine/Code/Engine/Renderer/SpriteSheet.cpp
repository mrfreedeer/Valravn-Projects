#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/EngineCommon.hpp"



SpriteDefinition::SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs) :
	m_spriteSheet(spriteSheet),
	m_spriteIndex(spriteIndex),
	m_uvAtMins(uvAtMins),
	m_uvAtMaxs(uvAtMaxs)
{
}

void SpriteDefinition::GetUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const
{
	out_uvAtMins = m_uvAtMins;
	out_uvAtMaxs = m_uvAtMaxs;
}

AABB2 SpriteDefinition::GetUVs() const
{
	return AABB2(m_uvAtMins, m_uvAtMaxs);
}

Texture const& SpriteDefinition::GetTexture() const
{
	return m_spriteSheet.GetTexture();
}

float SpriteDefinition::GetAspect()
{
	Vec2 uvSize = m_uvAtMaxs - m_uvAtMins;
	IntVec2 texDims = m_spriteSheet.GetTexture().GetDimensions();
	float texelsWide = texDims.x * uvSize.x;
	float texelsHigh = texDims.y * uvSize.y;
	return texelsWide / texelsHigh;
}


SpriteDefinition const& SpriteSheet::GetSpriteDef(int spriteIndex) const
{
	GUARANTEE_OR_DIE(spriteIndex < m_spriteDefs.size(), "THE SPRITE INDEX IS OUT OF BOUNDS!");
	return m_spriteDefs[spriteIndex];
}

SpriteSheet const& SpriteDefinition::GetSpriteSheet() const
{
	return m_spriteSheet;
}


SpriteSheet::SpriteSheet(Texture const& texture, IntVec2 const& simpleGridLayout) :
	m_texture(texture)
{
	float deltaX = 1.0f / simpleGridLayout.x;
	float deltaY = 1.0f / simpleGridLayout.y;

	int spriteIndex = 0;
	
	float hundredOfATexelX = (1.0f / static_cast<float>(m_texture.GetDimensions().x)) * 0.01f;
	float hundredOfATexelY = (1.0f / static_cast<float>(m_texture.GetDimensions().y)) * 0.01f;
	
	for (int y = simpleGridLayout.y - 1; y >= 0; y--) {
		for (int x = 0; x < simpleGridLayout.x; x++, spriteIndex++) {
			Vec2 minUvs((deltaX * x) + hundredOfATexelX,  (deltaY * y) + hundredOfATexelY);
			Vec2 maxUvs((deltaX * (x + 1)) - hundredOfATexelX, (deltaY * (y + 1)) - hundredOfATexelY);
			m_spriteDefs.emplace_back(*this, spriteIndex, minUvs, maxUvs);
		}
	}
}

Texture const& SpriteSheet::GetTexture() const
{
	return m_texture;
}

int SpriteSheet::GetNumSprites() const
{
	return (int)m_spriteDefs.size();
}


void SpriteSheet::GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const
{
	return m_spriteDefs[spriteIndex].GetUVs(out_uvAtMins, out_uvAtMaxs);
}

AABB2 SpriteSheet::GetSpriteUVs(int spriteIndex) const
{
	return m_spriteDefs[spriteIndex].GetUVs();
}
