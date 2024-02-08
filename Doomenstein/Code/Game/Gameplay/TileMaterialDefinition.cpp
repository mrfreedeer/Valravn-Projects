#include "Game/Gameplay/TileMaterialDefinition.hpp"

std::vector<TileMaterialDefinition*> TileMaterialDefinition::s_definitions;

bool TileMaterialDefinition::LoadFromXmlElement(const XMLElement& element)
{
	m_name = ParseXmlAttribute(element, "name", "None");
	std::string materialName = ParseXmlAttribute(element, "material", "Default3DMaterial");
	m_material = g_theRenderer->CreateOrGetMaterial(materialName.c_str());
	std::string textureName = ParseXmlAttribute(element, "texture", "None");
	m_texture = (textureName == "None") ? nullptr : g_theRenderer->CreateOrGetTextureFromFile(textureName.c_str());

	if (!m_texture) {
		m_uv = AABB2();
		return true;
	}
	else {
		IntVec2 cellCount = ParseXmlAttribute(element, "cellCount", IntVec2::ZERO);
		IntVec2 cell = ParseXmlAttribute(element, "cell", IntVec2::ZERO);
		int cellNum = (cell.y * cellCount.x) + cell.x;

		SpriteSheet defSpriteSheet(*m_texture, cellCount);
		m_uv = defSpriteSheet.GetSpriteUVs(cellNum);
		return true;
	}

	return false;
	
}

void TileMaterialDefinition::InitializeDefinitions()
{
	tinyxml2::XMLDocument tileMatDefXML;
	XMLError loadConfigStatus = tileMatDefXML.LoadFile("Data/Definitions/TileMaterialDefinitions.xml");
	GUARANTEE_OR_DIE(loadConfigStatus == XMLError::XML_SUCCESS, "TILE MATERIAL DEFINITIONS XML DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* tileMatDefinitionSet = tileMatDefXML.FirstChildElement("Definitions");
	XMLElement const* tileMatDefinition = tileMatDefinitionSet->FirstChildElement();

	while (tileMatDefinition) {
		TileMaterialDefinition* newTileMatDef = new TileMaterialDefinition();
		newTileMatDef->LoadFromXmlElement(*tileMatDefinition);
		tileMatDefinition = tileMatDefinition->NextSiblingElement();

		s_definitions.push_back(newTileMatDef);
	}
}

void TileMaterialDefinition::DestroyDefinitions()
{
	for (int tileMatDefIndex = 0; tileMatDefIndex < s_definitions.size(); tileMatDefIndex++) {
		TileMaterialDefinition*& tileMapDef = s_definitions[tileMatDefIndex];
		if (tileMapDef) {
			delete tileMapDef;
			tileMapDef = nullptr;
		}
	}

	s_definitions.clear();
}

const TileMaterialDefinition* TileMaterialDefinition::GetByName(const std::string& name)
{
	for (int matIndex = 0; matIndex < s_definitions.size(); matIndex++) {
		TileMaterialDefinition* matDef = s_definitions[matIndex];
		if (matDef && matDef->m_name == name) {
			return matDef;
		}
	}

	return nullptr;
}
