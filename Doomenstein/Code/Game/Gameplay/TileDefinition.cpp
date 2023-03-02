#include "Game/Gameplay/TileDefinition.hpp"
#include "Game/Gameplay/TileMaterialDefinition.hpp"

std::vector<TileDefinition*> TileDefinition::s_definitions;

bool TileDefinition::LoadFromXmlElement(const XMLElement& element)
{
	m_name = ParseXmlAttribute(element, "name", "None");
	m_isSolid = ParseXmlAttribute(element, "isSolid", false);
	
	std::string floorMatName = ParseXmlAttribute(element, "floorMaterial", "");
	m_floorMaterialDefinition = TileMaterialDefinition::GetByName(floorMatName);
	
	std::string ceilingMatName = ParseXmlAttribute(element, "ceilingMaterial", "");
	m_ceilingMaterialDefinition = TileMaterialDefinition::GetByName(ceilingMatName);

	std::string wallMaterial = ParseXmlAttribute(element, "wallMaterial", "");
	m_wallMaterialDefinition = TileMaterialDefinition::GetByName(wallMaterial);

	return true;
}

void TileDefinition::InitializeDefinitions()
{
	tinyxml2::XMLDocument tileDefinitionDoc;
	XMLError loadDocStatus = tileDefinitionDoc.LoadFile("Data/Definitions/TileDefinitions.xml");
	GUARANTEE_OR_DIE(loadDocStatus == XMLError::XML_SUCCESS, "TILE MATERIAL DEFINITIONS XML DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* tileDefinitionSet = tileDefinitionDoc.FirstChildElement("Definitions");
	XMLElement const* tileDef = tileDefinitionSet->FirstChildElement();

	while (tileDef) {
		TileDefinition* newTileDef = new TileDefinition();
		newTileDef->LoadFromXmlElement(*tileDef);
		s_definitions.push_back(newTileDef);
		tileDef = tileDef->NextSiblingElement();
	}

}

void TileDefinition::DestroyDefinitions()
{
	for (int tileDefIndex = 0; tileDefIndex < s_definitions.size(); tileDefIndex++) {
		TileDefinition*& tileDef = s_definitions[tileDefIndex];
		if (tileDef) {
			delete tileDef;
			tileDef = nullptr;
		}
	}

	s_definitions.clear();
}

const TileDefinition* TileDefinition::GetByName(const std::string& name)
{
	for (int tileDefIndex = 0; tileDefIndex < s_definitions.size(); tileDefIndex++) {
		TileDefinition const* tileDef = s_definitions[tileDefIndex];
		if (tileDef && tileDef->m_name == name) {
			return tileDef;
		}
	}

	return nullptr;
}

void TileDefinition::LoadTileMaterial(TileMaterialDefinition const*& tileMat)
{
	m_ceilingMaterialDefinition = tileMat;
	m_floorMaterialDefinition = tileMat;
	m_wallMaterialDefinition = tileMat;
}
