#pragma once
#include "Game/Framework/GameCommon.hpp"

class Game;
class Map;

class World {
public:
	World(Game* pointerToGame);
	~World();

	void Update(float deltaSeconds);
	void Render() const;
	void FadeToBlack() const;
	void GoToNextMap();
	bool IsPlayerAlive() const;
	void RespawnPlayer();

private:
	void CreateMaps();
	std::vector<Map*> m_maps;
	Map* m_currentMap = nullptr;
	int m_currentMapIndex = 0;
	Game* m_game = nullptr;
};