#include "Game/Gameplay/MapDefinition.hpp"
#include "Game/Gameplay/TileSetDefinition.hpp"

std::vector<MapDefinition*> MapDefinition::s_definitions;

bool MapDefinition::LoadFromXmlElement(XMLElement const& element)
{
	std::string mapName = ParseXmlAttribute(element, "name", "UnnamedMap");
	std::string imageSrc = ParseXmlAttribute(element, "image", "");
	std::string defaultTileSet = ParseXmlAttribute(element, "tileSet", "");
	m_isHordeMode = ParseXmlAttribute(element, "isHordeMode", false);

	m_name = mapName;
	m_image = new Image(imageSrc.c_str());
	m_tileSetDefinition = TileSetDefinition::GetByName(defaultTileSet);

	XMLElement const* spawnInfoDefSet = element.FirstChildElement("SpawnInfos");

	if (spawnInfoDefSet) {
		XMLElement const* spawnInfoDef = spawnInfoDefSet->FirstChildElement();

		while (spawnInfoDef) {
			SpawnInfo newSpawnInfo;
			newSpawnInfo.LoadFromXmlElement(*spawnInfoDef);
			spawnInfoDef = spawnInfoDef->NextSiblingElement();
			m_spawnInfos.push_back(newSpawnInfo);
		}
	}

	XMLElement const* hordeInfo = element.FirstChildElement("Horde");
	if (hordeInfo) {
		XMLElement const* waveInfo = hordeInfo->FirstChildElement();
		while (waveInfo) {

			int amountOfEnemies = ParseXmlAttribute(*waveInfo, "amountOfEnemies", 10);
			float amountOfTimeBeforeNextWave = ParseXmlAttribute(*waveInfo, "timeBeforeNextWave", 60.0f);

			WaveInfo newWave =
			{
				amountOfEnemies,
				amountOfTimeBeforeNextWave
			};

			m_wavesInfos.push_back(newWave);

			waveInfo = waveInfo->NextSiblingElement();
		}
	}

	return true;
}

void MapDefinition::InitializeDefinitions()
{
	tinyxml2::XMLDocument mapDefSetXML;
	XMLError mapDefLoadStatus = mapDefSetXML.LoadFile("Data/Definitions/MapDefinitions.xml");
	GUARANTEE_OR_DIE(mapDefLoadStatus == XMLError::XML_SUCCESS, "MAP DEFINITIONS XML DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* multipleMapDefs = mapDefSetXML.FirstChildElement("Definitions");
	XMLElement const* currentMapDef = multipleMapDefs->FirstChildElement();

	while (currentMapDef) {
		MapDefinition* mapDef = new MapDefinition();
		mapDef->LoadFromXmlElement(*currentMapDef);
		s_definitions.push_back(mapDef);

		currentMapDef = currentMapDef->NextSiblingElement();
	}
}

void MapDefinition::DestroyDefinitions()
{
	for (int mapDefIndex = 0; mapDefIndex < (int)s_definitions.size(); mapDefIndex++) {
		MapDefinition*& mapDef = s_definitions[mapDefIndex];
		if (mapDef) {
			delete mapDef;
			mapDef = nullptr;
		}
	}

	s_definitions.clear();
}

const MapDefinition* MapDefinition::GetByName(const std::string& name)
{
	for (int mapDefIndex = 0; mapDefIndex < (int)s_definitions.size(); mapDefIndex++) {
		MapDefinition const* mapDef = s_definitions[mapDefIndex];
		if (mapDef && mapDef->m_name == name) {
			return mapDef;
		}
	}

	return nullptr;
}
