#include "Game/Gameplay/World.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/PlayerTank.hpp"
#include "Game/Gameplay/Game.hpp"

World::World(Game* pointerToGame) :
	m_game(pointerToGame)
{

	CreateMaps();
	m_currentMap = m_maps[0];

	Entity* newPlayer = m_currentMap->SpawnNewEntity(EntityType::PLAYER, EntityFaction::GOOD, m_currentMap->GetPositionForTileCoords(IntVec2(1, 1)), 0.0f);
	m_currentMap->m_player = reinterpret_cast<PlayerTank*>(newPlayer);
}

World::~World()
{
}

void World::Update(float deltaSeconds)
{
	m_currentMap->Update(deltaSeconds);
}

void World::Render() const
{
	m_currentMap->Render();
}

void World::FadeToBlack() const
{
	m_game->FadeToBlack();
}

void World::GoToNextMap()
{
	m_currentMapIndex++;
	if (m_currentMapIndex < m_maps.size()) {
		PlayerTank* player = m_currentMap->m_player;

		player->m_position = Vec2(1.5f, 1.5f);
		m_currentMap->RemoveEntityFromMap(*player);
		m_currentMap = m_maps[m_currentMapIndex];
		player->m_map = m_currentMap;

		m_currentMap->AddEntityToMap(player);

		PlaySound(GAME_SOUND::EXIT_MAP);
	}
	else {
		m_game->WinGame();
	}


}

bool World::IsPlayerAlive() const
{
	return m_currentMap->IsAlive(m_currentMap->m_player);
}

void World::RespawnPlayer()
{
	m_currentMap->RespawnPlayer();
}

void World::CreateMaps()
{

	XMLDoc mapDefsDoc;
	XMLError loadStatus = mapDefsDoc.LoadFile("Data/Definitions/MapDefinitions.xml");

	GUARANTEE_OR_DIE(loadStatus == XMLError::XML_SUCCESS, "MAP DEFINITIONS FILE COULD NOT BE LOADED");

	MapDefinition::CreateMapDefinitions(*mapDefsDoc.RootElement());

	std::string mapsPlaythrough = g_gameConfigBlackboard.GetValue("MAPS_PLAYTHROUGH", "Welcome,Middle,End");
	std::vector<std::string> mapNames = SplitStringOnDelimiter(mapsPlaythrough, ',');


	for (int mapDefIndex = 0; mapDefIndex < mapNames.size(); mapDefIndex ++) {
		MapDefinition const& mapDef = MapDefinition::GetMapDefinition(mapNames[mapDefIndex]);
		m_maps.push_back(new Map(this, mapDef));
	}

	for (int mapIndex = 0; mapIndex < m_maps.size(); mapIndex++) {
		DebuggerPrintf("Amount of tries to create this map [%d]: %d\n", mapIndex, m_maps[mapIndex]->m_numAttempsTotal);
	}
}
