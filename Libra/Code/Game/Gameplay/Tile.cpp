#include "Game/Gameplay/Tile.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

Tile::Tile(IntVec2 const& tileCoords, TileDefinition const* tileDef):
	m_tileCoords(tileCoords),
	m_tileDef(tileDef)
{
	m_health = tileDef->m_health;
}

TileDefinition const& Tile::GetDefinition() const
{
	return *m_tileDef;
}

void Tile::SetType(std::string const& tileDefName)
{
	m_tileDef = TileDefinition::GetTileDefinition(tileDefName);
	m_health = m_tileDef->m_health;
}

void Tile::SetType(TileDefinition const* newTileDefinition)
{
	m_tileDef = newTileDefinition;
	m_health = m_tileDef->m_health;
}

Tile::~Tile()
{
}

bool Tile::IsSolid() const
{
 	return m_tileDef->m_isSolid;
}

void Tile::DestroyTile()
{
	if (!m_tileDef->m_isDestructible) return;

	m_destroyedTileType = m_tileDef;
	SetType(m_tileDef->m_turnsInto);
}

std::vector<TileDefinition> TileDefinition::s_tileDefinitions;
IntVec2 TileDefinition::m_spriteSheetLayout = IntVec2(8, 8);
bool TileDefinition::m_createdTileDefs = false;

void TileDefinition::CreateTileDefinitions(XMLElement const& tileXMLDefinition)
{
	if (m_createdTileDefs) return;
	
	XMLElement const* currentTileDefinition = tileXMLDefinition.FirstChildElement();

	while (currentTileDefinition) {
		std::string name = ParseXmlAttribute(*currentTileDefinition, "name", "UNNAMED");
		std::string turnsInto = ParseXmlAttribute(*currentTileDefinition, "turnsInto", "UNNAMED");

		int health = ParseXmlAttribute(*currentTileDefinition, "health", -1);

		IntVec2 spriteCoords = ParseXmlAttribute(*currentTileDefinition, "spriteCoords", IntVec2::ZERO);
		Rgba8 tint = ParseXmlAttribute(*currentTileDefinition, "tint", Rgba8::WHITE);
		Rgba8 mapImageColor = ParseXmlAttribute(*currentTileDefinition, "mapImageColor", Rgba8::WHITE);
		
		bool isWater = ParseXmlAttribute(*currentTileDefinition, "isWater", false);
		bool isSolid = ParseXmlAttribute(*currentTileDefinition, "isSolid", false);
		bool isDestructible = ParseXmlAttribute(*currentTileDefinition, "isDestructible", false);
		


		DefineTile(name, isSolid, isWater, spriteCoords, tint, mapImageColor, isDestructible, turnsInto, health);

		currentTileDefinition = currentTileDefinition->NextSiblingElement();
	}

	m_createdTileDefs = true;
}

void TileDefinition::DefineTile(std::string const& name, bool isSolid, bool isWater, IntVec2 spriteCoords, Rgba8 tint, Rgba8 mapImageColor, bool isDestructible, std::string const& turnsInto, int health)
{
	int const spriteIndex = GetSpriteIndex(spriteCoords.x, spriteCoords.y);
	TileDefinition tileDef;

	tileDef.m_name = name;
	tileDef.m_isSolid = isSolid;
	tileDef.m_isWater = isWater;
	tileDef.m_tint = tint;
	tileDef.m_UVs = g_tileSpriteSheet->GetSpriteUVs(spriteIndex);
	tileDef.m_mapImageColor = mapImageColor;
	tileDef.m_health = health;
	tileDef.m_isDestructible = isDestructible;
	tileDef.m_turnsInto = turnsInto;

	s_tileDefinitions.push_back(tileDef);
}


TileDefinition const* TileDefinition::GetTileDefinition(std::string const& nameToFind)
{
	for (int tileDefinitionIndex = 0; tileDefinitionIndex < s_tileDefinitions.size(); tileDefinitionIndex++) {
		if (s_tileDefinitions[tileDefinitionIndex].m_name == nameToFind) {
			return &s_tileDefinitions[tileDefinitionIndex];
		}
	}

	ERROR_AND_DIE(Stringf("TILE WITH NAME: %s WAS NOT FOUND ON TILE DEFINITIONS", nameToFind.c_str()));
}

TileDefinition const* TileDefinition::GetTileDefByImageColor(Rgba8 const& mapColor)
{
	for (int tileDefIndex = 0; tileDefIndex < s_tileDefinitions.size(); tileDefIndex++) {
		TileDefinition const& tileDef = s_tileDefinitions[tileDefIndex];
		if (tileDef.m_mapImageColor.Equals(mapColor, false)) {
			return &tileDef;
		}
	}

	ERROR_AND_DIE(Stringf("COULD NOT FIND TILE DEFINITION MATCHING COLOR: %s", mapColor.ToString().c_str()));

}

int const TileDefinition::GetSpriteIndex(int spriteX, int spriteY)
{
	return spriteY * m_spriteSheetLayout.x + spriteX;
}
