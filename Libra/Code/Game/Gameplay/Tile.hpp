#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"

#include <vector>

class Map;
class SpriteSheet;

class TileDefinition {
public:
	static std::vector<TileDefinition> s_tileDefinitions;
	static void CreateTileDefinitions(XMLElement const& tileXMLDefinition);
	static TileDefinition const* GetTileDefinition(std::string const& nameToFind);
	static TileDefinition const* GetTileDefByImageColor(Rgba8 const& mapColor);
	
	bool m_isSolid = false;
	bool m_isWater = false;
	bool m_isDestructible = false;

	int m_health = -1;

	AABB2 m_UVs = AABB2::ZERO_TO_ONE;

	std::string m_turnsInto = "UNNAMED";
	std::string m_name = "UNNAMED";

	Rgba8 m_tint = Rgba8::WHITE;
	Rgba8 m_mapImageColor = Rgba8::WHITE;



protected:
	static int const GetSpriteIndex(int spriteX, int spriteY);
	static void DefineTile(std::string const& name, bool isSolid, bool isWater, IntVec2 spriteCoords, Rgba8 tint, Rgba8 mapImageColor, bool isDestructible, std::string const& turnsInto, int health);
	static IntVec2 m_spriteSheetLayout;
	static bool m_createdTileDefs;
};

class Tile {
public:
	Tile(IntVec2 const& tileCoords, TileDefinition const*  tileDef);
	~Tile();

	bool IsSolid() const;
	TileDefinition const& GetDefinition() const;
	void SetType(std::string const& tileDefName);
	void SetType(TileDefinition const* newTileDefinition);
	void DestroyTile();

public:
	IntVec2 m_tileCoords = IntVec2(-1, -1);
	TileDefinition const* m_tileDef = nullptr;
	TileDefinition const* m_destroyedTileType = nullptr;
	int m_health = -1;
	
};