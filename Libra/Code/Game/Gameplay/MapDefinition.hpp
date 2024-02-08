#pragma once

#include <vector>
#include <string>
#include "Engine/Core/XmlUtils.hpp"
#include "Game/Gameplay/Entity.hpp"

struct IntVec2;


struct MapDefinition {

public:
	MapDefinition(std::string const& mapName, IntVec2 const& dimensions, std::string const& mapImageName = "", IntVec2 mapImgOffset = IntVec2::ZERO);
	static void CreateMapDefinitions(XMLElement const& mapsXMLDefinitions);
	static MapDefinition& GetMapDefinition(std::string const& mapName);


	struct WormTile {
		WormTile(std::string const& tileName, int length);

		std::string m_tileName = "UNNAMED";
		int m_length = 6;
		IntVec2 m_coords = IntVec2::ZERO;
	};

public:
	static std::vector<MapDefinition> s_mapDefinitions;

	IntVec2 m_dimensions = IntVec2::ZERO;
	IntVec2 m_mapImageOffset = IntVec2::ZERO;

	std::string m_name;
	std::string m_mapImageName;
	std::string m_edgeTileType;
	std::string m_fillTileType;

	std::string m_startFloorTileType;
	std::string m_startSolidTileType;
	std::string m_endFloorTileType;
	std::string m_endSolidTileType;

	int m_numEnemiesToSpawn[(int)EntityType::NUM_ENTITIES] = {};

	std::vector<WormTile> m_worms;


};