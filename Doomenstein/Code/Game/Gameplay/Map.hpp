#pragma once

#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Tile.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"

#include <vector>
//------------------------------------------------------------------------------------------------
class Actor;
class Game;
class Player;
class MapDefinition;
class Shader;
class Texture;
class IndexBuffer;
class VertexBuffer;
class SpawnInfo;
struct Vertex_PCU;
struct AABB2;
struct ActorUID;
struct PlayerInfo;

struct RaycastFilter
{
	Actor* m_ignoreActor = nullptr;
};

struct RaycastResultDoomenstein : public RaycastResult3D {
	RaycastResultDoomenstein() = default;
	RaycastResultDoomenstein(RaycastResult3D const& raycastResult3D) {
		m_startPosition = raycastResult3D.m_startPosition;
		m_forwardNormal = raycastResult3D.m_forwardNormal;
		m_maxDistance = raycastResult3D.m_maxDistance;
		m_didImpact = raycastResult3D.m_didImpact;
		m_impactPos = raycastResult3D.m_impactPos;
		m_impactFraction = raycastResult3D.m_impactFraction;
		m_impactDist = raycastResult3D.m_impactDist;
		m_impactNormal = raycastResult3D.m_impactNormal;
	}

	Actor* m_impactActor = nullptr;
	bool m_hitFloor = false;
};

//------------------------------------------------------------------------------------------------
class Map
{
public:
	Map(Game* game, const MapDefinition* definition, std::vector<PlayerInfo> const& playersInfo);
	~Map();

	void Update(float deltaSeconds);

	void Render(Camera const& camera, int playerIndex) const;

	void CreateTiles();
	void CreateGeometry();
	void CreateBuffers();

	void RaycastVsMap(Vec3 const& start, Vec3 const& direction, float distance, Player* playerToExclude = nullptr) const;
	RaycastResultDoomenstein RaycastAll(Vec3 const& start, Vec3 const& direction, float distance, RaycastFilter const& filter = RaycastFilter()) const;
	RaycastResultDoomenstein RaycastWorldActors(Vec3 const& start, Vec3 const& direction, float distance, RaycastFilter const& filter = RaycastFilter()) const;
	RaycastResultDoomenstein RaycastWorldXY(Vec3 const& start, Vec3 const& direction, float distance) const;
	RaycastResultDoomenstein RaycastWorldZ(Vec3 const& start, Vec3 const& direction, float distance) const;

	void CollideActors();
	void CollideActors(Actor* actorA, Actor* actorB);
	void CollideActorsWithMap();
	void CollideActorWithMap(Actor& actor);
	void DeleteDestroyedActors();

	Actor* SpawnActor(const SpawnInfo& spawnInfo);
	void DestroyActor(const ActorUID uid);
	Actor* GetActorByUID(const ActorUID uid) const;
	Actor* GetClosestVisibleEnemy(Actor* actor);

	// Excludes actor 
	std::vector<Actor*> GetActorsWithinRadius(Actor const* actor, float radius) const;
	Player* GetPlayer(int index) const;
	Player const* GetConstPlayer(int index) const;
	std::vector<Player*> const& GetPlayers() const { return m_players; }
	Player* GetPlayerWithKeyboardInput() const;
	Game* GetGame();

	Actor* GetNextPossessableActor(Actor* currentlyPossessedActor) const;
	bool IsActorAlive(Actor* actor) const;
	int GetPlayerAmount() const { return (int)m_playerControllers.size(); }
	Actor const* GetRandomSpawn() const;

	void KillAllDemons() const;
	void SetDirectionalLightIntensity(Rgba8 const& newIntensity, int playerIndex);
	void SetAmbientLightIntensity(Rgba8 const& newIntensity, int playerIndex);

	bool CanEnemyBeSpawned() const;
	bool IsHordeMode() const;
	float GetRemainingWaveTime() const;
	float GetTotalWaveTime() const;

	Clock& GetSpawnClock() { return m_spawnClock; }

	void PopulateSolidHeatMap();
	void PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 const& referenceCoords, float maxCost);
	bool SetTileHeatMapValue(TileHeatMap& out_distanceField, float currentValue, IntVec2 const& tileCoords);
	IntVec2 const GetCoordsForPosition(Vec3 const& position) const;
	Vec3 const GetPositionForTileCoords(IntVec2 const& tileCoords) const;

	float m_defaultDirectionalLightIntensity = g_gameConfigBlackboard.GetValue("DEFAULT_DIRECTIONAL_LIGHT_INTENSITY", 0.4f);
	float m_defaultAmbientLightIntensity = g_gameConfigBlackboard.GetValue("DEFAULT_AMBIENT_LIGHT_INTENSITY", 0.4f);
	int m_enemiesSpawned = 0;
	int m_enemiesStillAlive = 0;
	bool m_hasFinishedSpawningWave = false;
	IntVec2 m_dimensions = IntVec2::ZERO;
	TileHeatMap m_solidMap;
protected:
	void UpdatePlayerControllers(float deltaSeconds);
	void UpdateAIControllers(float deltaSeconds);

	void UpdateActors(float deltaSeconds);
	void UpdateActorsPhyiscs(float deltaSeconds);
	void UpdateListeners();
	void UpdatePlayerCameras();

	void SpawnActorsInMap(std::vector<PlayerInfo> const& playersInformation);

	void PushActorOutOfWall(Actor& actor, IntVec2 const& tileCoords);
	void RemoveFromLists(Actor& actor);
	void UpdateDeveloperCheatCodes();
protected:
	// Info
	Game* m_game = nullptr;
	std::vector<Player*>m_players;

	// Map
	const MapDefinition* m_definition = nullptr;
	std::vector<Tile> m_tiles;
	std::vector<Actor*> m_actors;
	std::vector<Actor*> m_actorsByFaction[(int)Faction::NUM_FACTIONS];
	std::vector<Actor*> m_spawnPoints;
	std::vector<Player*> m_playerControllers;
	std::vector<AI*> m_AIControllers;
	int m_actorSalt = 0x0000fffe;
	AABB3 m_bounds = AABB3::ZERO_TO_ONE;

	// Rendering
	std::vector<Vertex_PNCU> m_vertexes;
	std::vector<unsigned int> m_indexes;
	const Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer = nullptr;

	int m_currentWave = 0;
	Stopwatch m_waveStopwatch;


private:
	void AddVertsForTile(Tile const& tile, IntVec2 const& tileCoords);
	Tile const* GetTileForCoords(int x, int y) const;
	Tile const* GetTileForCoords(IntVec2 const& coords) const;
	IntVec2 const GetCoordsForTileIndex(int index) const;

	EulerAngles m_directionalLightOrientation = EulerAngles(0.0f, 135.0f, 0.0f);
	std::vector<Rgba8> m_directionalLightIntensities;
	std::vector<Rgba8> m_ambientLightIntensities;
	Clock m_spawnClock;
};
