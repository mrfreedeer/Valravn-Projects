#include "Game/Gameplay/TileSetDefinition.hpp"
#include "Game/Gameplay/TileDefinition.hpp"

std::vector<TileSetDefinition*> TileSetDefinition::s_definitions;

bool TileSetDefinition::LoadFromXmlElement(const XMLElement& element)
{
	m_name = ParseXmlAttribute(element, "name", "Default");
	std::string defaultTileName = ParseXmlAttribute(element, "defaultTile", "None");

	GUARANTEE_OR_DIE(defaultTileName != "None", Stringf("DEFAULT TILE NOT FOUND ON %s TILESET", m_name.c_str()));
	XMLElement const* currentTileMapping = element.FirstChildElement();

	while (currentTileMapping) {
		TileMapping newTileMapping;
		newTileMapping.LoadFromXmlElement(*currentTileMapping);
		m_mappings.push_back(newTileMapping);

		currentTileMapping = currentTileMapping->NextSiblingElement();
	}

	return true;
}

const TileDefinition* TileSetDefinition::GetTileDefinitionByColor(const Rgba8& color) const
{
	for (int tileMapIndex = 0; tileMapIndex < m_mappings.size(); tileMapIndex++) {
		TileMapping const& tileMapping = m_mappings[tileMapIndex];
		if (tileMapping.m_color == color) {
			return tileMapping.m_tileDefinition;
		}
	}

    return m_defaultTile;
}

void TileSetDefinition::InitializeDefinitions()
{
	tinyxml2::XMLDocument tileSetDefXML;
	XMLError loadTileSetDefStatus = tileSetDefXML.LoadFile("Data/Definitions/TileSetDefinitions.xml");
	GUARANTEE_OR_DIE(loadTileSetDefStatus == XMLError::XML_SUCCESS, "TILE SET DEFINITIONS XML DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* multipleTileDefSets = tileSetDefXML.FirstChildElement("Definitions");
	XMLElement const* currenTileSetDef = multipleTileDefSets->FirstChildElement();

	while (currenTileSetDef) {
		TileSetDefinition* newTileSetDef = new TileSetDefinition();
		newTileSetDef->LoadFromXmlElement(*currenTileSetDef);
		s_definitions.push_back(newTileSetDef);

		currenTileSetDef = currenTileSetDef->NextSiblingElement();
	}
}

void TileSetDefinition::DestroyDefinitions()
{
	for (int defIndex = 0; defIndex < s_definitions.size(); defIndex++) {
		TileSetDefinition*& tileSetDef = s_definitions[defIndex];
		if (tileSetDef) {
			delete tileSetDef;
			tileSetDef = nullptr;
		}
	}

	s_definitions.clear();

}

const TileSetDefinition* TileSetDefinition::GetByName(const std::string& name)
{
    for (int tileSetDefIndex = 0; tileSetDefIndex < s_definitions.size(); tileSetDefIndex++) {
        TileSetDefinition const* tileSetDef = s_definitions[tileSetDefIndex];
        if (tileSetDef && tileSetDef->m_name == name) {
            return tileSetDef;
        }
    }
    return nullptr;
}

bool TileMapping::LoadFromXmlElement(const XMLElement& element)
{
	Rgba8 color = ParseXmlAttribute(element, "color", Rgba8::WHITE);
	std::string tileDefname = ParseXmlAttribute(element, "tile", "");

	if (tileDefname.empty()) return false;
	
	TileDefinition const* tileDef = TileDefinition::GetByName(tileDefname);

	m_color = color;
	m_tileDefinition = tileDef;

	return true;
}
