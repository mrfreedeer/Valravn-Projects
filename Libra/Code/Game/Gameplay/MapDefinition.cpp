#include "Game/Gameplay/MapDefinition.hpp"

std::vector<MapDefinition> MapDefinition::s_mapDefinitions;


MapDefinition::MapDefinition(std::string const& mapName, IntVec2 const& dimensions, std::string const& mapImageName, IntVec2 mapImgOffset) :
	m_dimensions(dimensions),
	m_name(mapName),
	m_mapImageName(mapImageName),
	m_mapImageOffset(mapImgOffset)
{
}

void MapDefinition::CreateMapDefinitions(XMLElement const& mapsXMLDefinitions)
{
	s_mapDefinitions.clear();
	XMLElement const* currentMapDef = mapsXMLDefinitions.FirstChildElement();

	std::vector<MapDefinition> mapDefinitions;

	while (currentMapDef) {

		std::string name = ParseXmlAttribute(*currentMapDef, "name", "UnnamedMap");
		IntVec2 dimensions = ParseXmlAttribute(*currentMapDef, "dimensions", IntVec2::ZERO);
		std::string fillType = ParseXmlAttribute(*currentMapDef, "fillTileType", "GRASS");
		std::string edgeType = ParseXmlAttribute(*currentMapDef, "edgeTileType", "STONE");
		std::string startFloorTileType = ParseXmlAttribute(*currentMapDef, "startFloorTileType", "GRASS");
		std::string startSolidTileType = ParseXmlAttribute(*currentMapDef, "startSolidTileType", "STONE");
		std::string endFloorTileType = ParseXmlAttribute(*currentMapDef, "endFloorTileType", "GRASS");
		std::string endSolidTileType = ParseXmlAttribute(*currentMapDef, "endSolidTileType", "STONE");
		int ariesCount = ParseXmlAttribute(*currentMapDef, "ariesCount", 0);
		int leoCount = ParseXmlAttribute(*currentMapDef, "leoCount", 0);
		int scorpioCount = ParseXmlAttribute(*currentMapDef, "scorpioCount", 0);
		int capricornCount = ParseXmlAttribute(*currentMapDef, "capricornCount", 0);

		std::string mapImageName = ParseXmlAttribute(*currentMapDef, "mapImageName", "");
		IntVec2 mapImageOffset = ParseXmlAttribute(*currentMapDef, "mapImageOffset", IntVec2::ZERO);

		MapDefinition newMapDef(name, dimensions);

		newMapDef.m_edgeTileType = edgeType;
		newMapDef.m_fillTileType = fillType;
		newMapDef.m_startSolidTileType = startSolidTileType;
		newMapDef.m_startFloorTileType = startFloorTileType;
		newMapDef.m_endSolidTileType = endSolidTileType;
		newMapDef.m_endFloorTileType = endFloorTileType;

		newMapDef.m_numEnemiesToSpawn[(int)EntityType::ARIES] = ariesCount;
		newMapDef.m_numEnemiesToSpawn[(int)EntityType::LEO] = leoCount;
		newMapDef.m_numEnemiesToSpawn[(int)EntityType::SCORPIO] = scorpioCount;
		newMapDef.m_numEnemiesToSpawn[(int)EntityType::CAPRICORN] = capricornCount;

		newMapDef.m_mapImageName = mapImageName;
		newMapDef.m_mapImageOffset = mapImageOffset;

		XMLElement const* currentWormDef = currentMapDef->FirstChildElement();
		while (currentWormDef) {
			std::string wormTileType = ParseXmlAttribute(*currentWormDef, "tileType", "STONE");
			int length = ParseXmlAttribute(*currentWormDef, "length", 6);
			int wormCount = ParseXmlAttribute(*currentWormDef, "count", 2);

			for (int wormIndex = 0; wormIndex < wormCount; wormIndex++) {
				newMapDef.m_worms.emplace_back(wormTileType, length);
			}

			currentWormDef = currentWormDef->NextSiblingElement();
		}
		s_mapDefinitions.push_back(newMapDef);

		currentMapDef = currentMapDef->NextSiblingElement();
	}
}

MapDefinition& MapDefinition::GetMapDefinition(std::string const& mapName)
{
	for (int mapDefIndex = 0; mapDefIndex < s_mapDefinitions.size(); mapDefIndex++) {
		if (s_mapDefinitions[mapDefIndex].m_name == mapName) {
			return s_mapDefinitions[mapDefIndex];
		}
	}

	ERROR_AND_DIE(Stringf("COULD NOT FIND MAP NAMED: %s", mapName.c_str()));
}

MapDefinition::WormTile::WormTile(std::string const& name, int length) :
	m_tileName(name),
	m_length(length)
{
}
