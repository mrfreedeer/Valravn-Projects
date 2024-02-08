#pragma once
#include "Engine/Core/Image.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/SpawnInfo.hpp"
#include <string>

struct WaveInfo {
	int m_amountOfEnemies = 0;
	float m_timeBeforeNextWave = 0;
};

//------------------------------------------------------------------------------------------------
class TileSetDefinition;

//------------------------------------------------------------------------------------------------
class MapDefinition
{
public:
	bool LoadFromXmlElement( const XMLElement& element );
	~MapDefinition() { delete m_image; m_image = nullptr; }
public:
	std::string m_name;
	Image* m_image = nullptr;
	const TileSetDefinition* m_tileSetDefinition = nullptr;
	std::vector<SpawnInfo> m_spawnInfos;
	bool m_isHordeMode = false;
	std::vector<WaveInfo> m_wavesInfos;

	static void InitializeDefinitions();
	static void DestroyDefinitions();
	static const MapDefinition* GetByName( const std::string& name );
	static std::vector<MapDefinition*> s_definitions;
};

