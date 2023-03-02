#pragma once
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/Tile.hpp"
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/MapDefinition.hpp"

#include <vector>

class World;
class PlayerTank;
class Entity;


class Map {


public:
	Map(World* pointerToWorld, MapDefinition mapDefinition);
	~Map();

	void Update(float deltaSeconds);
	void Render() const;

	int const GetIndexForTileCoords(IntVec2 const& tileCoords) const;
	int const GetIndexForTileCoords(int tileX, int tileY) const;
	IntVec2 const GetTileCoordsForIndex(int vecIndex) const;

	Vec2 const GetPositionForTileCoords(IntVec2 const& tileCoords) const;
	IntVec2 const GetTileCoordsForPosition(Vec2 const& position) const;
	Tile const* GetTileForPosition(Vec2 const& position) const;
	Tile& GetMutableTileForPosition(Vec2 const& position);
	Tile const* GetTileForCoords(IntVec2 const& tileCoords) const;

	Entity* GetNearestEntityOfType(Vec2 const& position, EntityType type);

	void ShakeScreenCollision();
	void ShakeScreenDeath();
	void ShakeScreenPlayerDeath();
	void StopScreenShake();

	bool IsPointInSolid(Vec2 const& refPoint, bool treatWaterAsSolid = false) const;
	bool IsTileDestructible(Vec2 const& refPointForTile) const;
	bool IsDestructibleTileFinalFormSolid(Tile& tileToCheck) const;
	bool IsTileSolid(IntVec2 const& tileCoords) const;
	bool IsTileEdge(IntVec2 const& tileCoords) const;
	bool IsTileWater(IntVec2 const& tileCoords) const;
	bool IsTileWaterOrSolid(IntVec2 const& tileCoords) const;

	bool HasLineOfSight(Vec2 const& fromPos, Vec2 const& toPos, float maxSightDistance) const;
	RaycastResult2D RaycastVSTiles(Vec2 const& rayStart, Vec2 const& forwardNormal, float maxLength, int stepsPerDist) const;
	bool DoesCircleFitToPosition(Vec2 const& rayStart, Vec2 const& forwardNormal, float maxLength, float radius, bool treatWaterAsSolid = false) const;
	RaycastResult2D VoxelRaycastVsTiles(Vec2 const& rayStart, Vec2 const& forwardNormal, float maxLength, bool treatWaterAsSolid = false) const;
	Entity* SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 const& startingPosition, float orientation);

	void RemoveEntityFromMap(Entity& entityToRemove);
	void AddEntityToMap(Entity* newEntity);
	void RespawnPlayer();
	bool IsAlive(Entity const* entity) const;

	void GetHeatMapForEntity(TileHeatMap& heatMap, IntVec2 const& refPoint, bool canSwim = false);
	void GetSolidMapForEntity(TileHeatMap& solidMap, bool canSwim = false);
	IntVec2 GetDimensions() const;

	void PlayDiscoveredSound(float soundBalanceToPlayer);

	void RollForSpawningRubble(Vec2 const& bulletPos, Vec2 const& tilePos);
public:
	PlayerTank* m_player = nullptr;
	bool m_hasPlayerBeenCreated = false;
	int m_rayCastStepsPerTile = g_gameConfigBlackboard.GetValue("RAYCAST_STEPS_PER_TILE", 800);
	int m_numAttempsTotal = 0;
	TileHeatMap m_solidHeatMap;
	TileHeatMap m_mapSolvabilityHeatMap;

	bool m_areHeatMapsDirty = false; // Used for regenerating heatmaps due to a Scorpio Dying

private:
	void CreateMap();
	void SetMapPrimaryTiles();
	void GenerateMapWithWorms();
	void HardSetTileMapCorners();
	void FillUnreachableTiles();
	void SetMapImageTiles();

	void SpawnEntities();
	Entity* CreateEntity(EntityType type, EntityFaction faction, Vec2 const& startingPosition, float orientation);

	Vec2 const GetRandomSpawnPoint(bool canSwim = false) const;

	bool IsInCorners(IntVec2 const& coords) const;
	bool IsInCorners(Vec2 const& position) const;
	bool IsInLeftCorner(IntVec2 const& coords) const;
	bool IsInLeftCorner(Vec2 const& position) const; 
	bool IsInRightCorner(IntVec2 const& coords) const;
	bool IsInRightCorner(Vec2 const& position) const;

	IntVec2 const GetRandomCoordsIncludingCorners() const;
	IntVec2 const GetRandomCoordsStep() const;

	void AddEntityToList(Entity* newEntity, EntityList& list);
	void RemoveEntityFromList(Entity& entityToRemove, EntityList& list);

	void UpdateCamera(float deltaSeconds);
	AABB2 CorrectCameraBounds();
	void UpdateCameraScreenShake(float deltaSeconds);

	void CorrectPhysics();
	void PushEntitiesOutOfOtherEntities();
	void PushEntitiesOutOfEachOther(Entity& entityA, Entity& entityB);
	void PushEntitiesOutOfWalls();
	void PushEntityOutOfNeighborWalls(Entity& entityToPush);
	void PushEntityOutOfTile(Entity& entityToPush, IntVec2 const& tileCoords);
	void AffectEntitiesInRubble();

	void UpdateEntities(float deltaSeconds);
	void CheckEntitiesAgainstBullets();
	void CheckEntityAgainstBullets(Entity& entityToCheck);
	void CheckEntityAgainstBulletGroup(Entity& entityToCheck, EntityList& bulletList);
	void CheckForPlayerExitingMap();

	void AddVertsForTiles();

	void RenderEntities() const;
	void RenderEntitiesHealthBars() const;
	void RenderTiles() const;

	void DeleteGarbageEntities();

	void PopulateSolidHeatMap(TileHeatMap& out_distanceField, bool treatWaterAsSolid = true);
	void PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 const& referenceCoords, float maxCost, bool treatWaterAsSolid = true, bool checkForSolidEntities = false);
	bool SetTileHeatMapValue(TileHeatMap& out_distanceField, float currentValue, IntVec2 const& tileCoords, bool checkForSolidEntities, bool treatWaterAsSolid, bool isAdditive = false);
	bool IsSolidEntityOnTile(Tile const*& tile) const;
private:
	Camera m_worldCamera;
	float m_tilesInViewVertically = g_gameConfigBlackboard.GetValue("CAMERA_TILES_IN_VIEW_VERTICALLY", 8.0f);
	AABB2 m_prevCameraBounds;
	World* m_world = nullptr;

	std::vector<Tile> m_tiles;
	EntityList m_allEntities;
	EntityList m_entitiesByType[(int)EntityType::NUM_ENTITIES];
	EntityList m_entitiesByFaction[(int)EntityFaction::NUM_FACTIONS];
	EntityList m_physicsEntities;
	EntityList m_actorsByFaction[(int)EntityFaction::NUM_FACTIONS];
	EntityList m_bulletsByFaction[(int)EntityFaction::NUM_FACTIONS];
	EntityList m_rubble;

	int heatMapDebugEntityIndex = 0;
	int heatMapDebugListIndex = 0;

	std::vector<Vertex_PCU> m_verts;
	std::vector<Vertex_PCU> m_heatmapDebugVerts;

	bool m_screenShake = false;
	bool m_transitioningToOtherMaps = false;

	float m_screenShakeDuration = 0.0f;
	float m_screenShakeTranslation = 0.0f;
	float m_timeShakingScreen = 0.0f;

	float m_defaultScreenShakeDuration = g_gameConfigBlackboard.GetValue("SCREENSHAKE_DURATION", 1.0f);
	float m_deathScreenShakeDuration = g_gameConfigBlackboard.GetValue("SCREENSHAKE_DEATH_DURATION", 1.0f);
	float m_playerDeathScreenShakeDuration = g_gameConfigBlackboard.GetValue("SCREENSHAKE_PLAYER_DEATH_DURATION", 1.0f);

	float m_maxDefaultScreenShakeTranslation = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_TRANSLATION", 1.0f);
	float m_maxDeathScreenShakeTranslation = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_TRANSLATION_DEATH", 1.0f);
	float m_maxPlayerDeathScreenShakeTranslation = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_TRANSLATION_PLAYER_DEATH", 1.0f);

	float m_timeSinceLastPlayedDiscoverSound = 0.1f;

	Vec2 m_exitPosition = Vec2::ZERO;

	MapDefinition m_definition;

	static IntVec2 const North;
	static IntVec2 const East;
	static IntVec2 const West;
	static IntVec2 const South;

	static IntVec2 const NorthEast;
	static IntVec2 const SouthEast;
	static IntVec2 const NorthWest;
	static IntVec2 const SouthWest;
};
