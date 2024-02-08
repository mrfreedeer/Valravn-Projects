#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/PlayerTank.hpp"
#include "Game/Gameplay/Scorpio.hpp"
#include "Game/Gameplay/Aries.hpp"
#include "Game/Gameplay/Capricorn.hpp"
#include "Game/Gameplay/Leo.hpp"
#include "Game/Gameplay/Bullet.hpp"
#include "Game/Gameplay/Rubble.hpp"
#include "Game/Gameplay/Explosion.hpp"
#include "Game/Gameplay/GuidedMissile.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/World.hpp"

IntVec2 const Map::North = IntVec2(0, 1);
IntVec2 const Map::East = IntVec2(-1, 0);
IntVec2 const Map::West = IntVec2(1, 0);
IntVec2 const Map::South = IntVec2(0, -1);

IntVec2 const Map::NorthEast = IntVec2(-1, 1);
IntVec2 const Map::SouthEast = IntVec2(-1, -1);
IntVec2 const Map::NorthWest = IntVec2(1, 1);
IntVec2 const Map::SouthWest = IntVec2(1, -1);


Map::Map(World* pointerToWorld, MapDefinition mapDefinition) :
	m_world(pointerToWorld),
	m_definition(mapDefinition),
	m_solidHeatMap(mapDefinition.m_dimensions),
	m_mapSolvabilityHeatMap(mapDefinition.m_dimensions)
{

	IntVec2 exitCoords = m_definition.m_dimensions;
	exitCoords.x -= 2;
	exitCoords.y -= 2;

	m_exitPosition = GetPositionForTileCoords(exitCoords);

	CreateMap();

	int minimumVertSize = int(m_tiles.size()) * 6;
	m_verts.reserve((size_t)minimumVertSize);

	int amountOfEntitiesToSpawn = 0;
	for (int entityTypeIndex = 0; entityTypeIndex < (int)EntityType::NUM_ENTITIES; entityTypeIndex++) {
		if (entityTypeIndex == (int)EntityType::BULLET || entityTypeIndex == (int)EntityType::PLAYER) continue;
		for (int entityToSpawnIndex = 0; entityToSpawnIndex < m_definition.m_numEnemiesToSpawn[entityTypeIndex]; entityToSpawnIndex++) {
			amountOfEntitiesToSpawn++;
		}
	}

	// Entities to spawn, player and bullets
	int amountEntitiesExpected = amountOfEntitiesToSpawn + 1 + 40;
	m_allEntities.reserve((size_t)(amountEntitiesExpected));

	SpawnEntities();


	m_tiles[GetIndexForTileCoords(exitCoords)].SetType("EXIT");

}

Map::~Map()
{
	delete m_player;
	m_player = nullptr;
}


void Map::CreateMap()
{
	m_tiles.reserve((size_t)m_definition.m_dimensions.y * (size_t)m_definition.m_dimensions.x);

	IntVec2 exitCoords = GetTileCoordsForPosition(m_exitPosition);

	for (int attempCount = 0; attempCount < 100; attempCount++) {
		m_tiles.clear();
		// Creates maps with stone borders
		SetMapPrimaryTiles();

		GenerateMapWithWorms();

		// L Corner Shapes
		HardSetTileMapCorners();

		m_numAttempsTotal++;

		SetMapImageTiles();
		PopulateDistanceField(m_mapSolvabilityHeatMap, IntVec2(1, 1), ARBITRARILY_LARGE_VALUE);
		float valueForExit = m_mapSolvabilityHeatMap.GetValue(exitCoords);
		if (valueForExit != ARBITRARILY_LARGE_VALUE) {
		PopulateSolidHeatMap(m_solidHeatMap);
			FillUnreachableTiles();
			return;
		}
	}

	ERROR_RECOVERABLE(Stringf("SOMETHING IS WRONG WITH THIS MAP (%s) DEFINITION, GENERATION THROUGH WORMS NOT POSSIBLE", m_definition.m_name.c_str()));

}

void Map::SetMapPrimaryTiles()
{
	TileDefinition const* mainTile = TileDefinition::GetTileDefinition(m_definition.m_fillTileType);
	TileDefinition const* borderTile = TileDefinition::GetTileDefinition(m_definition.m_edgeTileType);

	for (int tileY = 0; tileY < m_definition.m_dimensions.y; tileY++) {
		for (int tileX = 0; tileX < m_definition.m_dimensions.x; tileX++) {

			IntVec2 newTileCoords(tileX, tileY);

			bool isEdge = IsTileEdge(newTileCoords);

			TileDefinition const* usedDef = mainTile;


			if (isEdge) {
				usedDef = borderTile;
			}
			m_tiles.emplace_back(newTileCoords, usedDef);
		}
	}
}

void Map::GenerateMapWithWorms()
{
	std::vector<MapDefinition::WormTile>& worms = m_definition.m_worms;

	for (int wormIndex = 0; wormIndex < worms.size(); wormIndex++) {
		IntVec2 randWormSpawnCoords = GetRandomCoordsIncludingCorners();
		MapDefinition::WormTile& currWorm = worms[wormIndex];
		currWorm.m_coords = randWormSpawnCoords;

		for (int wormStep = 0; wormStep < currWorm.m_length; wormStep++) {
			int indexForCoords = GetIndexForTileCoords(currWorm.m_coords);
			m_tiles[indexForCoords].SetType(currWorm.m_tileName);

			IntVec2 newWormCoords = currWorm.m_coords + GetRandomCoordsStep();
			while (IsTileEdge(newWormCoords)) {
				newWormCoords = currWorm.m_coords + GetRandomCoordsStep();
			}
			currWorm.m_coords = newWormCoords;
		}
	}

}

void Map::HardSetTileMapCorners()
{
	TileDefinition const* startFloorTileDef = TileDefinition::GetTileDefinition(m_definition.m_startFloorTileType);
	for (int tileY = 1; tileY < 6; tileY++) {
		for (int tileX = 1; tileX < 6; tileX++) {
			int vecPos = GetIndexForTileCoords(tileX, tileY);
			m_tiles[vecPos].SetType(startFloorTileDef);
		}
	}

	TileDefinition const* startSolidFloorTileDef = TileDefinition::GetTileDefinition(m_definition.m_startSolidTileType);

	m_tiles[GetIndexForTileCoords(4, 2)].SetType(startSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(4, 3)].SetType(startSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(4, 3)].SetType(startSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(4, 4)].SetType(startSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(2, 4)].SetType(startSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(3, 4)].SetType(startSolidFloorTileDef);


	TileDefinition const* endFloorTileDef = TileDefinition::GetTileDefinition(m_definition.m_endFloorTileType);

	// L Corner Shapes
	for (int tileY = m_definition.m_dimensions.y - 7; tileY < m_definition.m_dimensions.y - 1; tileY++) {
		for (int tileX = m_definition.m_dimensions.x - 7; tileX < m_definition.m_dimensions.x - 1; tileX++) {
			int vecPos = GetIndexForTileCoords(tileX, tileY);
			m_tiles[vecPos].SetType(endFloorTileDef);
		}
	}

	TileDefinition const* endSolidFloorTileDef = TileDefinition::GetTileDefinition(m_definition.m_endSolidTileType);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 6, m_definition.m_dimensions.y - 6)].SetType(endSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 5, m_definition.m_dimensions.y - 6)].SetType(endSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 4, m_definition.m_dimensions.y - 6)].SetType(endSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 3, m_definition.m_dimensions.y - 6)].SetType(endSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 6, m_definition.m_dimensions.y - 5)].SetType(endSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 6, m_definition.m_dimensions.y - 4)].SetType(endSolidFloorTileDef);

	m_tiles[GetIndexForTileCoords(m_definition.m_dimensions.x - 6, m_definition.m_dimensions.y - 3)].SetType(endSolidFloorTileDef);
}

void Map::FillUnreachableTiles()
{
	TileDefinition const* edgeTileDef = TileDefinition::GetTileDefinition(m_definition.m_edgeTileType);
	for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
		Tile& tile = m_tiles[tileIndex];
		if (tile.IsSolid()) continue;
		if (m_mapSolvabilityHeatMap.GetValue(tileIndex) == ARBITRARILY_LARGE_VALUE && !tile.GetDefinition().m_isWater) {
			tile.SetType(edgeTileDef);
		}
	}
}

void Map::SetMapImageTiles()
{
	if (m_definition.m_mapImageName.empty()) return;
	std::string imagePath = "Data/Images/" + m_definition.m_mapImageName + ".png";
	Image mapImage(imagePath.c_str());
	int offsetIndex = GetIndexForTileCoords(m_definition.m_mapImageOffset);

	for (int texelY = 0; texelY < mapImage.GetDimensions().y; texelY++) {
		for (int texelX = 0; texelX < mapImage.GetDimensions().x; texelX++) {
			IntVec2 texelTileCoords = IntVec2(texelX, texelY);
			Rgba8 const& texel = mapImage.GetTexelColor(texelTileCoords);
			if (texel.a == 0) continue;
			TileDefinition const* tileDef = TileDefinition::GetTileDefByImageColor(texel);

			int chanceToPlaceTile = rng.GetRandomIntInRange(0, 254);
			if (chanceToPlaceTile <= texel.a) {
				int tileIndex = GetIndexForTileCoords(texelTileCoords);
				int resultingTileIndex = tileIndex + offsetIndex;

				if (resultingTileIndex < m_tiles.size()) {
					m_tiles[resultingTileIndex].SetType(tileDef);
				}
			}
		}
	}

}

void Map::SpawnEntities()
{
	for (int entityTypeIndex = 0; entityTypeIndex < (int)EntityType::NUM_ENTITIES; entityTypeIndex++) {
		if (entityTypeIndex == (int)EntityType::BULLET || entityTypeIndex == (int)EntityType::PLAYER) continue;
		for (int entityToSpawnIndex = 0; entityToSpawnIndex < m_definition.m_numEnemiesToSpawn[entityTypeIndex]; entityToSpawnIndex++) {
			Vec2 spawnPos = GetRandomSpawnPoint();
			SpawnNewEntity((static_cast<EntityType>(entityTypeIndex)), EntityFaction::EVIL, spawnPos, rng.GetRandomFloatInRange(0.0f, 360.0f));
		}
	}
}

Entity* Map::SpawnNewEntity(EntityType type, EntityFaction faction, Vec2 const& startingPosition, float orientation)
{
	Entity* newEntity = CreateEntity(type, faction, startingPosition, orientation);
	AddEntityToMap(newEntity);
	return newEntity;
}

void Map::AddEntityToMap(Entity* newEntity)
{
	AddEntityToList(newEntity, m_allEntities);
	AddEntityToList(newEntity, m_entitiesByType[(int)newEntity->m_type]);
	AddEntityToList(newEntity, m_entitiesByFaction[(int)newEntity->m_faction]);
	if (newEntity->IsActor()) {
		AddEntityToList(newEntity, m_actorsByFaction[(int)newEntity->m_faction]);
		if (newEntity->m_pushesEntities || newEntity->m_isPushedByEntities) {
			AddEntityToList(newEntity, m_physicsEntities);
		}
	}
	else {
		AddEntityToList(newEntity, m_bulletsByFaction[(int)newEntity->m_faction]);
	}

	if (newEntity->m_type == EntityType::PLAYER) {
		m_player = reinterpret_cast<PlayerTank*>(newEntity);
	}

	if (newEntity->m_type == EntityType::ENEMYRUBBLE || newEntity->m_type == EntityType::WALLRUBBLE) {
		AddEntityToList(newEntity, m_rubble);
	}
}

void Map::RespawnPlayer()
{
	m_player->Reset();
}

bool Map::IsAlive(Entity const* entity) const
{
	return entity && entity->IsAlive();
}

void Map::GetHeatMapForEntity(TileHeatMap& heatMap, IntVec2 const& refPoint, bool canSwim)
{
	PopulateDistanceField(heatMap, refPoint, ARBITRARILY_LARGE_VALUE, !canSwim, true);
}

void Map::GetSolidMapForEntity(TileHeatMap& solidMap, bool canSwim)
{
	PopulateSolidHeatMap(solidMap, !canSwim);
}

IntVec2 Map::GetDimensions() const
{
	return m_definition.m_dimensions;
}

void Map::PlayDiscoveredSound(float soundBalanceToPlayer)
{
	if (m_timeSinceLastPlayedDiscoverSound >= 0.1f) {
		PlaySound(GAME_SOUND::DISCOVERED, 1.0f, false, soundBalanceToPlayer);
		m_timeSinceLastPlayedDiscoverSound = 0.0f;
	}
}

void Map::RollForSpawningRubble(Vec2 const& bulletPos, Vec2 const& tilePos)
{
	float numberRoll = rng.GetRandomFloatZeroUpToOne();

	static float chanceOfSpawningRubble = g_gameConfigBlackboard.GetValue("RUBBLE_SPAWN_CHANCE", 0.25f);

	if (numberRoll > chanceOfSpawningRubble) return;

	Vec2 dispFwdToBullet = (bulletPos - tilePos).GetNormalized();


	SpawnNewEntity(EntityType::WALLRUBBLE, EntityFaction::NEUTRAL, tilePos + dispFwdToBullet * 0.5f, 0.0f);

}


Vec2 const Map::GetRandomSpawnPoint(bool canSwim) const
{

	bool isSpawnSolid = true;
	bool isInsideCorners = false;

	int randX;
	int randY;

	IntVec2 randcoords;
	while (isSpawnSolid || isInsideCorners) {
		randX = rng.GetRandomIntInRange(1, m_definition.m_dimensions.x - 1);
		randY = rng.GetRandomIntInRange(1, m_definition.m_dimensions.y - 1);

		randcoords = IntVec2(randX, randY);
		isSpawnSolid = IsTileSolid(randcoords) || (IsTileWater(randcoords) && !canSwim);
		isInsideCorners = IsInCorners(randcoords);
	}

	return GetPositionForTileCoords(randcoords);
}

IntVec2 const Map::GetRandomCoordsIncludingCorners() const
{
	bool isSpawnSolid = true;
	int randX;
	int randY;

	IntVec2 randcoords = IntVec2::ZERO;
	while (isSpawnSolid) {
		randX = rng.GetRandomIntInRange(1, m_definition.m_dimensions.x - 1);
		randY = rng.GetRandomIntInRange(1, m_definition.m_dimensions.y - 1);

		randcoords = IntVec2(randX, randY);
		isSpawnSolid = IsTileSolid(randcoords);
	}

	return randcoords;
}

bool Map::IsInCorners(IntVec2 const& coords) const
{
	return IsInLeftCorner(coords) || IsInRightCorner(coords);
}

bool Map::IsInCorners(Vec2 const& position) const
{
	return IsInCorners(GetTileCoordsForPosition(position));
}

bool Map::IsInLeftCorner(IntVec2 const& coords) const
{
	bool isCoordInXCorner = (0 <= coords.x) && (coords.x <= 6);
	bool isCoordInYCorner = (0 <= coords.y) && (coords.y <= 6);
	return (isCoordInXCorner) && (isCoordInYCorner);
}

bool Map::IsInLeftCorner(Vec2 const& position) const
{
	return IsInLeftCorner(GetTileCoordsForPosition(position));
}

bool Map::IsInRightCorner(IntVec2 const& coords) const
{
	bool isCoordInXCorner = (m_definition.m_dimensions.x - 7 <= coords.x) && (coords.x <= m_definition.m_dimensions.x - 1);
	bool isCoordInYCorner = (m_definition.m_dimensions.y - 7 <= coords.y) && (coords.y <= m_definition.m_dimensions.y - 1);
	return (isCoordInXCorner) && (isCoordInYCorner);
}

bool Map::IsInRightCorner(Vec2 const& position) const
{
	return IsInRightCorner(GetTileCoordsForPosition(position));
}


IntVec2 const Map::GetRandomCoordsStep() const
{
	int randNum = rng.GetRandomIntInRange(0, 3);
	switch (randNum) {
	case 0:
		return North;
		break;
	case 1:
		return South;
		break;
	case 2:
		return East;
		break;
	case 3:
		return West;
		break;
	default:
		ERROR_AND_DIE("SOMETHING REALLY STRANGE HAPPENED WITH rng AT GETRANDOMCOORDSSTEP");
		break;
	}
}

void Map::RemoveEntityFromMap(Entity& entityToRemove)
{
	RemoveEntityFromList(entityToRemove, m_allEntities);
	RemoveEntityFromList(entityToRemove, m_entitiesByType[(int)entityToRemove.m_type]);
	RemoveEntityFromList(entityToRemove, m_entitiesByFaction[(int)entityToRemove.m_faction]);
	if (entityToRemove.IsActor()) {
		RemoveEntityFromList(entityToRemove, m_actorsByFaction[(int)entityToRemove.m_faction]);
	}
	else {
		RemoveEntityFromList(entityToRemove, m_bulletsByFaction[(int)entityToRemove.m_faction]);
	}

}

Entity* Map::CreateEntity(EntityType type, EntityFaction faction, Vec2 const& startingPosition, float orientation)
{
	Entity* newEntity = nullptr;
	switch (type) {
	case EntityType::WALLRUBBLE:
		newEntity = new Rubble(this, startingPosition, orientation, faction, EntityType::WALLRUBBLE);
		break;
	case EntityType::ENEMYRUBBLE:
		newEntity = new Rubble(this, startingPosition, orientation, faction, EntityType::ENEMYRUBBLE);
		break;
	case EntityType::ARIES:
		newEntity = new Aries(this, startingPosition, orientation, faction, EntityType::ARIES);
		break;
	case EntityType::CAPRICORN:
		newEntity = new Capricorn(this, startingPosition, orientation, faction, EntityType::CAPRICORN);
		break;
	case EntityType::LEO:
		newEntity = new Leo(this, startingPosition, orientation, faction, EntityType::LEO);
		break;
	case EntityType::SCORPIO:
		newEntity = new Scorpio(this, startingPosition, orientation, faction, EntityType::SCORPIO);
		break;
	case EntityType::BULLET:
		newEntity = new Bullet(this, startingPosition, orientation, faction, EntityType::BULLET, BulletType::REGULAR);
		if (faction == EntityFaction::EVIL) {
			newEntity->m_health = 1;
		}
		break;
	case EntityType::FLAMETHROWER_BULLET:
		newEntity = new Bullet(this, startingPosition, orientation, faction, EntityType::FLAMETHROWER_BULLET, BulletType::FLAMETHROWER);
		break;
	case EntityType::BOLT:
		newEntity = new GuidedMissile(this, startingPosition, orientation, faction, EntityType::BOLT);
		break;
	case EntityType::PLAYER:
		newEntity = new PlayerTank(this, startingPosition, orientation, faction, EntityType::PLAYER);
		m_hasPlayerBeenCreated = true;
		break;
	case EntityType::EXPLOSION:
		newEntity = new Explosion(this, startingPosition, orientation, faction, EntityType::EXPLOSION);
		break;
	default:
		ERROR_AND_DIE("NOT A VALID NEW ENTITY!");
		break;
	}
	return newEntity;
}




void Map::AddEntityToList(Entity* newEntity, EntityList& list)
{
	for (int listIndex = 0; listIndex < (int)list.size(); listIndex++) {
		Entity*& entity = list[listIndex];
		if (!entity) {
			entity = newEntity;
			return;
		}
	}

	list.push_back(newEntity);
}

void Map::RemoveEntityFromList(Entity& entityToRemove, EntityList& list) {
	for (int listIndex = 0; listIndex < (int)list.size(); listIndex++) {
		Entity*& entity = list[listIndex];
		if (!entity) continue;
		if (entity->m_type == EntityType::PLAYER && entity == &entityToRemove) {
			return;
		}

		if (entity == &entityToRemove) {
			entity = nullptr;
			return;
		}
	}

	ERROR_RECOVERABLE(Stringf("COULD NOT FIND ENITTY OF TYPE %d in list", (int)entityToRemove.m_type));
}

void Map::Update(float deltaSeconds)
{
	m_timeSinceLastPlayedDiscoverSound += deltaSeconds;

	AffectEntitiesInRubble();
	UpdateEntities(deltaSeconds);
	m_areHeatMapsDirty = false;

	CorrectPhysics();

	CheckEntitiesAgainstBullets();
	
	DeleteGarbageEntities();


	m_verts.clear();
	AddVertsForTiles();

	CheckForPlayerExitingMap();
	UpdateCamera(deltaSeconds);

	if (g_drawDebugHeatMapEntity) {
		if (g_theInput->WasKeyJustPressed(KEYCODE_F6)) {
			heatMapDebugEntityIndex++;
			if (heatMapDebugEntityIndex >= m_entitiesByType[heatMapDebugListIndex].size()) {
				heatMapDebugEntityIndex = 0;
			}
		}

		if (g_theInput->WasKeyJustPressed(KEYCODE_F7)) {
			heatMapDebugListIndex++;
			heatMapDebugEntityIndex = 0;
			if (heatMapDebugListIndex >= (int)EntityType::NUM_ENTITIES || heatMapDebugListIndex == (int)EntityType::PLAYER) {
				heatMapDebugListIndex = 0;
			}
		}
	}

}

void Map::UpdateCamera(float deltaSeconds)
{
	AABB2 cameraAABB2 = CorrectCameraBounds();

	if (g_debugCamera) {
		Vec2 minCameraSetToX = Vec2(m_definition.m_dimensions);
		minCameraSetToX.y = minCameraSetToX.x * 0.5f;
		Vec2 minCameraSetToY = Vec2(m_definition.m_dimensions);
		minCameraSetToY.x = minCameraSetToY.y * 2.0f;

		if (m_definition.m_dimensions.y < minCameraSetToY.y) {
			cameraAABB2.SetDimensions(minCameraSetToX);
		}
		else {
			cameraAABB2.SetDimensions(minCameraSetToY);
		}
		cameraAABB2.SetCenter(cameraAABB2.GetDimensions() * 0.5f);
	}

	m_worldCamera.SetOrthoView(cameraAABB2.m_mins, cameraAABB2.m_maxs);

	UpdateCameraScreenShake(deltaSeconds);

}

void Map::UpdateCameraScreenShake(float deltaSeconds)
{
	if (m_screenShake) {
		float easingInScreenShake = m_timeShakingScreen / m_screenShakeDuration;
		easingInScreenShake *= (m_screenShakeTranslation * 0.95f);

		float randAmountX = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);
		float randAmountY = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);

		m_worldCamera.TranslateCamera(randAmountX, randAmountY);

		m_timeShakingScreen += deltaSeconds;

		if (m_timeShakingScreen > m_screenShakeDuration) {
			m_screenShake = false;
			m_timeShakingScreen = 0.0f;;
		}
	}
}


AABB2 Map::CorrectCameraBounds()
{
	AABB2 cameraAABB2;
	if (!m_player) return m_prevCameraBounds;
	cameraAABB2.SetCenter(m_player->m_position);
	cameraAABB2.SetDimensions(m_tilesInViewVertically * 2.0f, m_tilesInViewVertically);

	if (cameraAABB2.m_mins.x < 0.0f) {
		cameraAABB2.SetCenter(cameraAABB2.GetCenter() - Vec2(cameraAABB2.m_mins.x, 0.0f));
	}

	if (cameraAABB2.m_mins.y < 0.0f) {
		cameraAABB2.SetCenter(cameraAABB2.GetCenter() - Vec2(0.0f, cameraAABB2.m_mins.y));
	}

	if (cameraAABB2.m_maxs.x > m_definition.m_dimensions.x) {
		float distanceToMove = cameraAABB2.m_maxs.x - m_definition.m_dimensions.x;
		cameraAABB2.SetCenter(cameraAABB2.GetCenter() - Vec2(distanceToMove, 0.0f));
	}

	if (cameraAABB2.m_maxs.y > m_definition.m_dimensions.y) {
		float distanceToMove = cameraAABB2.m_maxs.y - m_definition.m_dimensions.y;
		cameraAABB2.SetCenter(cameraAABB2.GetCenter() - Vec2(0.0f, distanceToMove));
	}
	m_prevCameraBounds = cameraAABB2;
	return cameraAABB2;
}

void Map::CorrectPhysics()
{
	PushEntitiesOutOfOtherEntities();
	PushEntitiesOutOfWalls();
}

void Map::UpdateEntities(float deltaSeconds)
{
	for (int entityTypeIndex = 0; entityTypeIndex < (int)EntityType::NUM_ENTITIES; entityTypeIndex++) {
		for (int entityIndex = 0; entityIndex < m_entitiesByType[entityTypeIndex].size(); entityIndex++) {
			Entity*& entity = m_entitiesByType[entityTypeIndex][entityIndex];
			if (entity) {
				entity->Update(deltaSeconds);
			}
		}
	}
}


void Map::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);

	RenderTiles();
	RenderEntities();

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	RenderEntitiesHealthBars();

	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->EndCamera(m_worldCamera);
}



void Map::RenderTiles() const
{
	g_theRenderer->BindTexture(g_textures[GAME_TEXTURE::TERRAIN]);
	g_theRenderer->DrawVertexArray((int)m_verts.size(), m_verts.data());

	if (g_drawDebugDistanceField || g_drawDebugHeatMapEntity) {
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(m_heatmapDebugVerts);

		Entity const* debuggedEntity = nullptr;

		if (m_entitiesByType[heatMapDebugListIndex].size() > 0) {
			debuggedEntity = m_entitiesByType[heatMapDebugListIndex][heatMapDebugEntityIndex];
		}

		if (g_drawDebugHeatMapEntity && IsAlive(debuggedEntity)) {
			DebugDrawRing(debuggedEntity->m_position, debuggedEntity->m_cosmeticsRadius * 1.15f, debuggedEntity->m_debugLineThickness, Rgba8::ORANGE);
		}
	}
}


void Map::RenderEntities() const
{
	for (int entityTypeIndex = 0; entityTypeIndex < (int)EntityType::NUM_ENTITIES; entityTypeIndex++) {
		if (entityTypeIndex == (int)EntityType::EXPLOSION || entityTypeIndex == (int)EntityType::FLAMETHROWER_BULLET) {
			g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
		}
		for (int entityIndex = 0; entityIndex < m_entitiesByType[entityTypeIndex].size(); entityIndex++) {
			Entity const* entity = m_entitiesByType[entityTypeIndex][entityIndex];
			if (entity) {
				entity->Render();
			}
		}
	}
}

void Map::RenderEntitiesHealthBars() const
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity const* entity = m_allEntities[entityIndex];
		if (IsAlive(entity)) {
			entity->RenderHealthBar();
		}
	}
}

void Map::DeleteGarbageEntities()
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (!entity) continue;

		if (!IsAlive(entity)) {
			if (entity->m_type == EntityType::PLAYER) {
				continue;
			}
			RemoveEntityFromMap(*entity);
			delete entity;
		}

	}
}

void Map::PopulateSolidHeatMap(TileHeatMap& out_solidMap, bool treatWaterAsSolid)
{
	for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
		bool isTileConsideredSolid = false;
		IntVec2 const& currTileCoords = m_tiles[tileIndex].m_tileCoords;

		TileDefinition const currentTileDef = m_tiles[tileIndex].GetDefinition();

		if (treatWaterAsSolid) {
			isTileConsideredSolid = IsTileWaterOrSolid(currTileCoords);
		}
		else {
			isTileConsideredSolid = currentTileDef.m_isSolid;
		}

		if (currentTileDef.m_isDestructible) {
			isTileConsideredSolid = IsDestructibleTileFinalFormSolid(m_tiles[tileIndex]);
		}

		if (isTileConsideredSolid && !currentTileDef.m_isDestructible) {
			out_solidMap.SetValue(tileIndex, 1.0f);
		}
		else if (currentTileDef.m_isDestructible) {
			out_solidMap.SetValue(tileIndex, 0.5f);
		}
	}

}

void Map::PopulateDistanceField(TileHeatMap& out_distanceField, IntVec2 const& referenceCoords, float maxCost, bool treatWaterAsSolid, bool checkForSolidEntities)
{
	out_distanceField.SetAllValues(maxCost);
	out_distanceField.SetValue(referenceCoords, 0.0f);

	float currTileValue = 0.0f;
	bool modifiedTile = true;
	while (modifiedTile) {

		modifiedTile = false;
		for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
			if (m_tiles[tileIndex].IsSolid()) continue;
			if (treatWaterAsSolid && m_tiles[tileIndex].GetDefinition().m_isWater) continue;

			float tileHeatValue = out_distanceField.GetValue(tileIndex);
			if (tileHeatValue == currTileValue) {

				IntVec2 const& currTileCoords = m_tiles[tileIndex].m_tileCoords;

				bool modifiedNorth = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + North, checkForSolidEntities, treatWaterAsSolid, true);
				bool modifiedSouth = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + South, checkForSolidEntities, treatWaterAsSolid, true);
				bool modifiedEast = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + East, checkForSolidEntities, treatWaterAsSolid, true);
				bool modifiedWest = SetTileHeatMapValue(out_distanceField, currTileValue, currTileCoords + West, checkForSolidEntities, treatWaterAsSolid, true);

				modifiedTile = modifiedTile || modifiedNorth || modifiedSouth || modifiedEast || modifiedWest;
			}

		}
		currTileValue++;
	}
}

bool Map::SetTileHeatMapValue(TileHeatMap& out_distanceField, float currentValue, IntVec2 const& tileCoords, bool checkForSolidEntities, bool treatWaterAsSolid, bool isAdditive)
{
	Tile const* tile = GetTileForCoords(tileCoords);
	bool isSolidEntityOnCurrTile = checkForSolidEntities && IsSolidEntityOnTile(tile);
	bool isTileConsideredSolid = tile->IsSolid() || (treatWaterAsSolid && tile->GetDefinition().m_isWater);

	bool setCondition = false;

	if (isAdditive) {
		setCondition = out_distanceField.GetValue(tileCoords) > currentValue + 1.0f;
	}
	else {
		setCondition = out_distanceField.GetValue(tileCoords) > 0;
	}

	if (!isSolidEntityOnCurrTile && !isTileConsideredSolid && out_distanceField.GetValue(tileCoords) > currentValue + 1.0f) {
		if (isAdditive) {
			out_distanceField.SetValue(tileCoords, currentValue + 1.0f);
		}
		else {
			out_distanceField.SetValue(tileCoords, 0.0f);
		}
		return true;
	}

	return false;
}

bool Map::IsSolidEntityOnTile(Tile const*& tile) const
{
	IntVec2 const& tileCoords = tile->m_tileCoords;
	for (int scorpioIndex = 0; scorpioIndex < m_entitiesByType[(int)EntityType::SCORPIO].size(); scorpioIndex++) {
		Entity const* scorpio = m_entitiesByType[(int)EntityType::SCORPIO][scorpioIndex];
		if (!scorpio) continue;
		IntVec2 const scorpioPos = GetTileCoordsForPosition(scorpio->m_position);
		if (scorpioPos == tileCoords) {
			return true;
		}
	}

	return false;
}

void Map::PushEntitiesOutOfOtherEntities()
{
	for (int entityIndex = 0; entityIndex < m_physicsEntities.size(); entityIndex++) {
		Entity* entity = m_physicsEntities[entityIndex];
		if (!IsAlive(entity)) {
			continue;
		}
		for (int otherEntityIndex = entityIndex + 1; otherEntityIndex < m_physicsEntities.size(); otherEntityIndex++)
		{
			Entity* otherEntity = m_physicsEntities[otherEntityIndex];
			if (!IsAlive(otherEntity)) {
				continue;
			}
			PushEntitiesOutOfEachOther(*entity, *otherEntity);
		}
	}
}


void Map::PushEntitiesOutOfEachOther(Entity& entityA, Entity& entityB) {
	bool canApushB = entityA.m_pushesEntities && entityB.m_isPushedByEntities;
	bool canBpushA = entityB.m_pushesEntities && entityA.m_isPushedByEntities;

	if (canApushB && canBpushA) {
		PushDiscsOutOfEachOther2D(entityA.m_position, entityA.m_physicsRadius, entityB.m_position, entityB.m_physicsRadius);
	}
	else if (canApushB) {
		PushDiscOutOfDisc2D(entityB.m_position, entityB.m_physicsRadius, entityA.m_position, entityA.m_physicsRadius);
	}
	else if (canBpushA) {
		PushDiscOutOfDisc2D(entityA.m_position, entityA.m_physicsRadius, entityB.m_position, entityB.m_physicsRadius);
	}
}

void Map::PushEntitiesOutOfWalls()
{

	for (int entityTypeIndex = 0; entityTypeIndex < (int)EntityType::NUM_ENTITIES; entityTypeIndex++) {
		if (g_ignorePhysics && entityTypeIndex == (int)EntityType::PLAYER) continue;
		for (int entityIndex = 0; entityIndex < m_entitiesByType[entityTypeIndex].size(); entityIndex++) {
			Entity* entity = m_entitiesByType[entityTypeIndex][entityIndex];
			if (!entity) {
				continue;
			}
			PushEntityOutOfNeighborWalls(*entity);
		}
	}
}

void Map::CheckEntityAgainstBullets(Entity& entityToCheck)
{
	bool checkEvil = entityToCheck.m_faction != EntityFaction::EVIL;
	bool checkGood = !checkEvil;

	CheckEntityAgainstBulletGroup(entityToCheck, m_bulletsByFaction[(int)EntityFaction::NEUTRAL]);

	if (checkEvil) {
		CheckEntityAgainstBulletGroup(entityToCheck, m_bulletsByFaction[(int)EntityFaction::EVIL]);
	}

	if (checkGood) {
		CheckEntityAgainstBulletGroup(entityToCheck, m_bulletsByFaction[(int)EntityFaction::GOOD]);
	}

}

void Map::CheckEntitiesAgainstBullets()
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (!IsAlive(entity)) continue;

		if (entity->IsActor() && entity->m_isHitByBullets)
		{
			CheckEntityAgainstBullets(*entity);
		}
	}
}

void Map::CheckEntityAgainstBulletGroup(Entity& entityToCheck, EntityList& bulletList)
{
	for (int bulletIndex = 0; bulletIndex < bulletList.size(); bulletIndex++) {
		Entity*& bullet = bulletList[bulletIndex];
		if (!bullet) continue;
		if (DoDiscsOverlap(bullet->m_position, bullet->m_physicsRadius, entityToCheck.m_position, entityToCheck.m_physicsRadius)) {
			Bullet* certainlyABullet = reinterpret_cast<Bullet*>(bullet);
			GUARANTEE_OR_DIE(certainlyABullet, "WHOA! THERE'S SOMETHING POSING OFF AS A BULLET ON THE BULLET LISTS!!");
			entityToCheck.ReactToBullet(certainlyABullet);
		}
	}
}

void Map::CheckForPlayerExitingMap()
{
	if (!IsAlive(m_player)) return;
	bool cheatCodePressed = g_theInput->WasKeyJustPressed(KEYCODE_F9);
	if (!m_transitioningToOtherMaps && (cheatCodePressed || m_player && DoDiscsOverlap(m_exitPosition, 0.1f, m_player->m_position, m_player->m_physicsRadius))) {
		m_world->FadeToBlack();
		m_player->StartCrossingMapsAnimation();
		m_transitioningToOtherMaps = true;
	}
}

void Map::PushEntityOutOfNeighborWalls(Entity& entityToPush)
{
	if (!entityToPush.m_isPushedByWalls) return;
	IntVec2 tileCoord = GetTileCoordsForPosition(entityToPush.m_position);

	// First cardinal points to avoid toe stubbing
	PushEntityOutOfTile(entityToPush, tileCoord + North);
	PushEntityOutOfTile(entityToPush, tileCoord + South);
	PushEntityOutOfTile(entityToPush, tileCoord + East);
	PushEntityOutOfTile(entityToPush, tileCoord + West);

	PushEntityOutOfTile(entityToPush, tileCoord + NorthEast);
	PushEntityOutOfTile(entityToPush, tileCoord + SouthEast);
	PushEntityOutOfTile(entityToPush, tileCoord + NorthWest);
	PushEntityOutOfTile(entityToPush, tileCoord + SouthWest);

}

void Map::PushEntityOutOfTile(Entity& entityToPush, IntVec2 const& tileCoords)
{
	if (tileCoords.x < 0 || tileCoords.y < 0) return;
	Tile const* tile = GetTileForCoords(tileCoords);
	bool isNotSolidNorWater = !tile->IsSolid() && !tile->GetDefinition().m_isWater;
	bool isWaterButCanSwim = entityToPush.m_canSwim && tile->GetDefinition().m_isWater;

	if (isNotSolidNorWater || isWaterButCanSwim) return;
	AABB2 tileAABB2;
	tileAABB2.SetCenter(GetPositionForTileCoords(tile->m_tileCoords));
	tileAABB2.SetDimensions(1.0f, 1.0f);

	PushDiscOutOfAABB2D(entityToPush.m_position, entityToPush.m_physicsRadius, tileAABB2);
}

void Map::AffectEntitiesInRubble()
{
	for (int entityIndex = 0; entityIndex < m_rubble.size(); entityIndex++) {
		Entity* rubble = m_rubble[entityIndex];
		if (!rubble) {
			continue;
		}
		for (int otherEntityIndex = 0; otherEntityIndex < m_physicsEntities.size(); otherEntityIndex++)
		{
   			Entity* otherEntity = m_physicsEntities[otherEntityIndex];
			if (!IsAlive(otherEntity)) {
				continue;
			}

			bool doesEntityOverlapWithRubble = DoDiscsOverlap(rubble->m_position, rubble->m_physicsRadius, otherEntity->m_position, otherEntity->m_physicsRadius);
			if (doesEntityOverlapWithRubble) {
				otherEntity->RubbleSlowDown();
			}
		}
	}
}



void Map::AddVertsForTiles()
{
	m_heatmapDebugVerts.clear();


	Entity const* debuggedEntity = nullptr;
	if (m_entitiesByType[heatMapDebugListIndex].size() > 0) {
		debuggedEntity = m_entitiesByType[heatMapDebugListIndex][heatMapDebugEntityIndex];
	}

	float maxHeatMapValue = 0.0f;

	if (g_drawDebugHeatMapEntity && IsAlive(debuggedEntity)) {
		maxHeatMapValue = debuggedEntity->m_heatMap.GetMaxValue();
	}

	for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
		Tile& tile = m_tiles[tileIndex];
		AABB2 tileAABB2;
		tileAABB2.SetDimensions(Vec2::ONE);
		tileAABB2.SetCenter(GetPositionForTileCoords(tile.m_tileCoords));

		TileDefinition const& tileDef = tile.GetDefinition();

		AddVertsForAABB2D(m_verts, tileAABB2, tileDef.m_tint, tileDef.m_UVs);

		if (g_drawDebugDistanceField) {

			if (m_solidHeatMap.GetValue(tileIndex) >= 0.5f) {

				AddVertsForAABB2D(m_heatmapDebugVerts, tileAABB2, Rgba8::WHITE, AABB2::ZERO_TO_ONE);
			}
			else {
				AddVertsForAABB2D(m_heatmapDebugVerts, tileAABB2, Rgba8::BLACK, AABB2::ZERO_TO_ONE);
			}
			IntVec2 tileCoords = GetTileCoordsForIndex(tileIndex);
		}

		if (g_drawDebugHeatMapEntity && IsAlive(debuggedEntity)) {
			float heatMapValue = debuggedEntity->m_heatMap.GetValue(tileIndex);
			float gradient = RangeMapClamped(heatMapValue, 0, maxHeatMapValue, 0, 255);
			unsigned char gradientAsUChar = static_cast<unsigned char>(gradient);
			AddVertsForAABB2D(m_heatmapDebugVerts, tileAABB2, Rgba8(gradientAsUChar, gradientAsUChar, gradientAsUChar, 255), AABB2::ZERO_TO_ONE);
		}
	}
}

int const Map::GetIndexForTileCoords(IntVec2 const& tileCoords) const
{
	return tileCoords.y * m_definition.m_dimensions.x + tileCoords.x;
}

int const Map::GetIndexForTileCoords(int tileX, int tileY) const
{
	return tileY * m_definition.m_dimensions.x + tileX;
}

IntVec2 const Map::GetTileCoordsForIndex(int vecIndex) const
{
	int tileY = vecIndex / m_definition.m_dimensions.x;
	return IntVec2(vecIndex - tileY * m_definition.m_dimensions.x, tileY);
}

Vec2 const Map::GetPositionForTileCoords(IntVec2 const& tileCoords) const
{

	return Vec2((float)tileCoords.x + 0.5f, (float)tileCoords.y + 0.5f);
}

IntVec2 const Map::GetTileCoordsForPosition(Vec2 const& position) const
{
	IntVec2 roundedDownPosition;
	roundedDownPosition.x = RoundDownToInt(position.x);
	roundedDownPosition.y = RoundDownToInt(position.y);

	return roundedDownPosition;
}

Tile const* Map::GetTileForPosition(Vec2 const& position) const
{
	if (position.x < 0.0f || position.y < 0.0f) return &m_tiles[0];
	IntVec2 const tileCoords = GetTileCoordsForPosition(position);

	int vecIndexForCoords = GetIndexForTileCoords(tileCoords);
	if (vecIndexForCoords >= m_tiles.size()) {
		vecIndexForCoords = (int)m_tiles.size() - 1;
	}
	return &m_tiles[vecIndexForCoords];
}

Tile& Map::GetMutableTileForPosition(Vec2 const& position)
{
	if (position.x < 0.0f || position.y < 0.0f) return m_tiles[0];
	IntVec2 const tileCoords = GetTileCoordsForPosition(position);

	int vecIndexForCoords = GetIndexForTileCoords(tileCoords);
	if (vecIndexForCoords >= m_tiles.size()) {
		vecIndexForCoords = (int)m_tiles.size() - 1;
	}
	return m_tiles[vecIndexForCoords];
}

Tile const* Map::GetTileForCoords(IntVec2 const& tileCoords) const
{
	if (tileCoords.x < 0 || tileCoords.y < 0) return nullptr;
	return &m_tiles[GetIndexForTileCoords(tileCoords)];
}


Entity* Map::GetNearestEntityOfType(Vec2 const& position, EntityType type)
{
	Entity* nearestEntity = nullptr;
	float bestDistance = ARBITRARILY_LARGE_VALUE;

	for (int entityIndex = 0; entityIndex < m_entitiesByType[(int)type].size(); entityIndex++) {
		Entity* entity = m_entitiesByType[(int)type][entityIndex];
		if (entity) {
			float distanceSquared = GetDistanceSquared2D(position, entity->m_position);
			if (distanceSquared < bestDistance) {
				bestDistance = distanceSquared;
				nearestEntity = entity;
			}
		}
	}
	return nearestEntity;
}

void Map::ShakeScreenCollision()
{
	m_screenShakeDuration = m_defaultScreenShakeDuration;
	m_screenShakeTranslation = m_maxDefaultScreenShakeTranslation;
	m_screenShake = true;
}

void Map::ShakeScreenDeath()
{
	m_screenShake = true;
	m_screenShakeDuration = m_deathScreenShakeDuration;
	m_screenShakeTranslation = m_maxDeathScreenShakeTranslation;
}

void Map::ShakeScreenPlayerDeath()
{
	m_screenShake = true;
	m_screenShakeDuration = m_playerDeathScreenShakeDuration;
	m_screenShakeTranslation = m_maxPlayerDeathScreenShakeTranslation;
}

void Map::StopScreenShake() {
	m_screenShake = false;
	m_screenShakeDuration = 0.0f;
	m_screenShakeTranslation = 0.0f;
}

bool Map::IsPointInSolid(Vec2 const& refPoint, bool treatWaterAsSolid) const
{
	Tile const* tile = GetTileForPosition(refPoint);

	if (treatWaterAsSolid) {
		return tile->IsSolid() || tile->GetDefinition().m_isWater;
	}

	return tile->IsSolid();
}

bool Map::IsTileDestructible(Vec2 const& refPointForTile) const
{
	Tile const* tile = GetTileForPosition(refPointForTile);
	return tile->GetDefinition().m_isDestructible;
}

bool Map::IsDestructibleTileFinalFormSolid(Tile& tileToCheck) const
{
	TileDefinition const* currentTileDef = &tileToCheck.GetDefinition();
	while (currentTileDef->m_isDestructible) {
		currentTileDef = TileDefinition::GetTileDefinition(currentTileDef->m_turnsInto);
	}
	return currentTileDef->m_isSolid;
}

bool Map::IsTileSolid(IntVec2 const& tileCoords) const
{
	return GetTileForCoords(tileCoords)->IsSolid();
}

bool Map::IsTileEdge(IntVec2 const& tileCoords) const
{
	if (tileCoords.y == 0 || tileCoords.x == 0 || tileCoords.x == m_definition.m_dimensions.x - 1 || tileCoords.y == m_definition.m_dimensions.y - 1) {
		return true;
	}
	return false;
}

bool Map::IsTileWater(IntVec2 const& tileCoords) const
{
	Tile const* tile = GetTileForCoords(tileCoords);
	return tile->GetDefinition().m_isWater;
}

bool Map::IsTileWaterOrSolid(IntVec2 const& tileCoords) const
{
	Tile const* tile = GetTileForCoords(tileCoords);
	return tile->IsSolid() || tile->GetDefinition().m_isWater;
}


bool Map::HasLineOfSight(Vec2 const& fromPos, Vec2 const& toPos, float maxSightDistance) const
{
	Vec2 forwdToOtherEntity = (toPos - fromPos).GetNormalized();
	float distanceToOtherEntity = GetDistance2D(fromPos, toPos);

	float distanceToCheck = maxSightDistance;

	if (distanceToOtherEntity <= maxSightDistance) {
		distanceToCheck = distanceToOtherEntity;
	}
	else {
		return false;
	}


	RaycastResult2D hits = VoxelRaycastVsTiles(fromPos, forwdToOtherEntity, distanceToCheck);
	return !hits.m_didImpact;
}


RaycastResult2D Map::RaycastVSTiles(Vec2 const& rayStart, Vec2 const& forwardNormal, float maxLength, int stepsPerDist) const
{
	float distPerStep = 1.0f / static_cast<float>(stepsPerDist);
	RaycastResult2D hit;
	for (int stepIndex = 0; stepIndex < stepsPerDist * (int)maxLength; stepIndex++) {
		Vec2 newPos = rayStart + forwardNormal * (stepIndex * distPerStep);
		if (distPerStep * stepIndex > maxLength) return hit;
		if (IsPointInSolid(newPos)) {
			hit.m_didImpact = true;
			hit.m_impactDist = GetDistance2D(newPos, rayStart);
			hit.m_impactPos = newPos;

			return hit;
		}
	}

	return hit;
}

bool Map::DoesCircleFitToPosition(Vec2 const& rayStart, Vec2 const& forwardNormal, float maxLength, float radius, bool treatWaterAsSolid) const
{

	Vec2 leftPointAtRadius = rayStart + forwardNormal.GetRotated90Degrees() * radius;
	Vec2 rightPointAtRadius = rayStart + forwardNormal.GetRotatedMinus90Degrees() * radius;

	RaycastResult2D leftPointRaycast = VoxelRaycastVsTiles(leftPointAtRadius, forwardNormal, maxLength, treatWaterAsSolid);
	RaycastResult2D rightPointRaycast = VoxelRaycastVsTiles(rightPointAtRadius, forwardNormal, maxLength, treatWaterAsSolid);


	return (!leftPointRaycast.m_didImpact) && (!rightPointRaycast.m_didImpact);
}

RaycastResult2D Map::VoxelRaycastVsTiles(Vec2 const& rayStart, Vec2 const& forwardNormal, float maxLength, bool treatWaterAsSolid) const
{
	RaycastResult2D hitInfo;
	hitInfo.m_startPosition = rayStart;
	hitInfo.m_forwardNormal = forwardNormal;
	hitInfo.m_maxDistance = maxLength;

	if (IsPointInSolid(rayStart, treatWaterAsSolid)) {
		hitInfo.m_didImpact = true;
		hitInfo.m_impactDist = 0.0f;
		hitInfo.m_impactPos = rayStart;

		return hitInfo;
	}

	IntVec2 tilePosition = GetTileCoordsForPosition(rayStart);

	float fwdDistPerXCrossing = 0.0f;
	float fwdDistPerYCrossing = 0.0f;

	int tileStepDirectionX = 1;
	int tileStepDirectionY = 1;

	if (forwardNormal.x < 0) {
		tileStepDirectionX = -1;
	}

	if (forwardNormal.y < 0) {
		tileStepDirectionY = -1;
	}

	if (forwardNormal.x == 0) {
		fwdDistPerXCrossing = ARBITRARILY_LARGE_VALUE;
	}
	else {
		fwdDistPerXCrossing = 1.0f / fabsf(forwardNormal.x);
	}

	if (forwardNormal.y == 0) {
		fwdDistPerYCrossing = ARBITRARILY_LARGE_VALUE;
	}
	else {
		fwdDistPerYCrossing = 1.0f / fabsf(forwardNormal.y);
	}

	float xAtFirstCrossing = tilePosition.x + static_cast<float>((tileStepDirectionX + 1) / 2);
	float xDistToFirstCrossing = xAtFirstCrossing - rayStart.x;

	float fwdDistAtNextXCrossing = fabsf(xDistToFirstCrossing) * fwdDistPerXCrossing;

	float yAtFirstCrossing = tilePosition.y + static_cast<float>((tileStepDirectionY + 1) / 2);
	float yDistToFirstCrossing = yAtFirstCrossing - rayStart.y;

	float fwdDistAtNextYCrossing = fabsf(yDistToFirstCrossing) * fwdDistPerYCrossing;

	while (!hitInfo.m_didImpact) {

		if (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing) {
			if (fwdDistAtNextXCrossing > maxLength) {
				hitInfo.m_maxDistanceReached = true;
				return hitInfo;
			}

			tilePosition.x += tileStepDirectionX;

			bool isTileConsideredSolid = false;
			if (treatWaterAsSolid) {
				isTileConsideredSolid = IsTileWaterOrSolid(tilePosition);
			}
			else {
				isTileConsideredSolid = IsTileSolid(tilePosition);
			}

			if (isTileConsideredSolid) {
				hitInfo.m_didImpact = true;
				hitInfo.m_impactDist = fwdDistAtNextXCrossing;
				hitInfo.m_impactPos = rayStart + forwardNormal * fwdDistAtNextXCrossing;
			}
			else {
				fwdDistAtNextXCrossing += fwdDistPerXCrossing;
			}
		}
		else {
			if (fwdDistAtNextYCrossing > maxLength) {
				hitInfo.m_maxDistanceReached = true;
				return hitInfo;
			}

			tilePosition.y += tileStepDirectionY;

			bool isTileConsideredSolid = false;
			if (treatWaterAsSolid) {
				isTileConsideredSolid = IsTileWaterOrSolid(tilePosition);
			}
			else {
				isTileConsideredSolid = IsTileSolid(tilePosition);
			}

			if (isTileConsideredSolid) {
				hitInfo.m_didImpact = true;
				hitInfo.m_impactDist = fwdDistAtNextYCrossing;
				hitInfo.m_impactPos = rayStart + forwardNormal * fwdDistAtNextYCrossing;
			}
			else {
				fwdDistAtNextYCrossing += fwdDistPerYCrossing;
			}
		}

	}

	return hitInfo;
}
