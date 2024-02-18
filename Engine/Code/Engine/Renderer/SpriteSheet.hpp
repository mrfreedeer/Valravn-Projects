#pragma once
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Math/Vec2.hpp"
#include <vector>

struct Vec2;
struct AABB2;
class SpriteSheet;
class SpriteAnimGroupDefinition;

class SpriteDefinition {
public:
	explicit SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs);
	void GetUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const;
	AABB2 GetUVs() const;
	SpriteSheet const& GetSpriteSheet() const;
	Texture const& GetTexture() const;
	float GetAspect();
protected:
	SpriteSheet const& m_spriteSheet;
	int m_spriteIndex = -1;
	Vec2 m_uvAtMins = Vec2::ZERO;
	Vec2 m_uvAtMaxs = Vec2::ONE;
};


class SpriteSheet {
	friend class SpriteAnimGroupDefinition;
public:
	explicit SpriteSheet(Texture const& texture, IntVec2 const& simpleGridLayout);

	Texture const& GetTexture()const;
	int GetNumSprites() const;
	SpriteDefinition const& GetSpriteDef(int spriteIndex) const;
	void GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const;
	AABB2 GetSpriteUVs(int spriteIndex) const;

protected:
	Texture const& m_texture;
	std::vector<SpriteDefinition> m_spriteDefs;
};