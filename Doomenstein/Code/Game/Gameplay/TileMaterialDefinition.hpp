#pragma once
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <string>

//------------------------------------------------------------------------------------------------
class Material;
class Texture;

//------------------------------------------------------------------------------------------------
class TileMaterialDefinition
{
public:
	bool LoadFromXmlElement( const XMLElement& element );

public:
	std::string m_name;
	bool m_isVisible = true;
	AABB2 m_uv = AABB2::ZERO_TO_ONE;
	Material* m_material = nullptr;
	const Texture* m_texture = nullptr;

	static void InitializeDefinitions();
	static void DestroyDefinitions();
	static const TileMaterialDefinition* GetByName( const std::string& name );
	static std::vector<TileMaterialDefinition*> s_definitions;
};