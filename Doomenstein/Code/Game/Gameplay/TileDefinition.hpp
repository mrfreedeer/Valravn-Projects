#pragma once
#include "Game/Framework/GameCommon.hpp"
#include <string>

//------------------------------------------------------------------------------------------------
class TileMaterialDefinition;

//------------------------------------------------------------------------------------------------
class TileDefinition
{
public:
	bool LoadFromXmlElement( const XMLElement& element );

public:
	std::string m_name;
	bool m_isSolid = false;

	const TileMaterialDefinition* m_ceilingMaterialDefinition = nullptr;
	const TileMaterialDefinition* m_floorMaterialDefinition = nullptr;
	const TileMaterialDefinition* m_wallMaterialDefinition = nullptr;

	static void	InitializeDefinitions();
	static void DestroyDefinitions();
	static const TileDefinition* GetByName( const std::string& name );
	static std::vector<TileDefinition*> s_definitions;

private:
	void LoadTileMaterial(TileMaterialDefinition const*& tileMat);
};