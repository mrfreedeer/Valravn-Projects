#include "Engine/Math/Easing.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game/Gameplay/Chunk.hpp"
#include "Game/Gameplay/BlockDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/BlockIterator.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/BlockTemplate.hpp"
#include "Game/Gameplay/World.hpp" // For IntVec <
#include "ThirdParty/Squirrel/SmoothNoise.hpp"

constexpr int SEALEVEL = 64;
constexpr int VERTEX_RESERVE_AMOUNT = 10000;

static IntVec3 const NorthStep = IntVec3(0, 1, 0);
static IntVec3 const SouthStep = IntVec3(0, -1, 0);
static IntVec3 const EastStep = IntVec3(1, 0, 0);
static IntVec3 const WestStep = IntVec3(-1, 0, 0);

Chunk::Chunk(Game* pointerToGame, IntVec2 const& globalCoords) :
	m_globalCoordinates(globalCoords),
	m_game(pointerToGame)
{
	m_bounds.m_mins.x = static_cast<float>(globalCoords.x * CHUNK_SIZE_X);
	m_bounds.m_mins.y = static_cast<float>(globalCoords.y * CHUNK_SIZE_Y);
	m_bounds.m_mins.z = 0.0f;

	m_bounds.m_maxs.x = m_bounds.m_mins.x + CHUNK_SIZE_X;
	m_bounds.m_maxs.y = m_bounds.m_mins.y + CHUNK_SIZE_Y;
	m_bounds.m_maxs.z = m_bounds.m_mins.z + CHUNK_SIZE_Z;

}

Chunk::~Chunk()
{

	delete m_chunkVBO;
	m_chunkVBO = nullptr;

	delete m_chunkIBO;
	m_chunkIBO = nullptr;


	delete m_chunkWaterVBO;
	m_chunkVBO = nullptr;

	delete m_chunkWaterIBO;
	m_chunkIBO = nullptr;

	delete[] m_blocks;
	m_blocks = nullptr;


}

void Chunk::GenerateChunk()
{

	double startTime = GetCurrentTimeSeconds();

	m_blocks = new Block[CHUNK_TOTAL_SIZE];
	//bool loadedFromFile = LoadFromFile();
	//if (loadedFromFile) {
	//	double endTime = GetCurrentTimeSeconds();
	//	double totalTime = endTime - startTime;

	//	if (totalTime > m_game->m_worstChunkLoadIncludeRLETime) {
	//		m_game->m_worstChunkLoadIncludeRLETime = totalTime;
	//	}

	//	m_game->m_totalChunkLoadIncludeRLETime += totalTime;
	//	m_game->m_countChunkLoadIncludeRLETime++;
	//	m_game->RefreshLoadStats();
	//	return;
	//}

	int blockIndex = 0;
	BlockDefinition const* air = BlockDefinition::GetDefByName("air");
	BlockDefinition const* water = BlockDefinition::GetDefByName("water");
	BlockDefinition const* stone = BlockDefinition::GetDefByName("stone");
	BlockDefinition const* grass = BlockDefinition::GetDefByName("grass");
	BlockDefinition const* dirt = BlockDefinition::GetDefByName("dirt");
	BlockDefinition const* coal = BlockDefinition::GetDefByName("coal");
	BlockDefinition const* iron = BlockDefinition::GetDefByName("iron");
	BlockDefinition const* gold = BlockDefinition::GetDefByName("gold");
	BlockDefinition const* diamond = BlockDefinition::GetDefByName("diamond");
	BlockDefinition const* sand = BlockDefinition::GetDefByName("sand");
	BlockDefinition const* ice = BlockDefinition::GetDefByName("ice");
	BlockDefinition const* glowstone = BlockDefinition::GetDefByName("glowstone");

	int globalChunkX = (m_globalCoordinates.x) * (CHUNK_SIZE_X);
	int globalChunkY = (m_globalCoordinates.y) * (CHUNK_SIZE_Y);

	int dirtLimit[CHUNK_BLOCKS_PER_LAYER] = {};
	int* terrainHeight = m_terrainHeight; // Terrain Height is worth keeping for placing trees
	float* humidity = m_humidity;
	float* temperature = m_temperature;


	bool calculatedPerlinNoise = false; // So that Perlin noise functions are only calculated one per global x,y
	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int y = 0; y < CHUNK_SIZE_Y; y++) {
			for (int x = 0; x < CHUNK_SIZE_X; x++, blockIndex++) {
				int globalBlockX = globalChunkX + x;
				int globalBlockY = globalChunkY + y;

				int noiseIndex = x + (y * CHUNK_SIZE_X);

				if (!calculatedPerlinNoise) {
					humidity[blockIndex] = 0.5f + (0.5f * Compute2dPerlinNoise((float)globalBlockX, (float)globalBlockY, 500.0f, 9, 0.2f, 4.0f, true, m_worldSeed + 1)); // Limited to 0,1
					temperature[blockIndex] = 0.5f + (0.5f * Compute2dPerlinNoise((float)globalBlockX, (float)globalBlockY, 375.f, 6, 0.5f, 4.0f, true, m_worldSeed + 2)); // Limited to 0,1

					terrainHeight[blockIndex] = GetTerrainHeightAtCoords(IntVec2(globalBlockX, globalBlockY));

					dirtLimit[blockIndex] = terrainHeight[blockIndex] - rng.GetRandomIntInRange(3, 4);
				}

				int const& dirtHeight = dirtLimit[noiseIndex];
				int const& globalHeight = terrainHeight[noiseIndex];
				float const& humidityLevels = humidity[noiseIndex];
				float const& temperatureLevels = temperature[noiseIndex];

				float humidityDeepnessLevels = RangeMap(humidityLevels, m_baseHumidity, 0.0f, 0.0f, float(globalHeight - dirtHeight));
				float iceDeepnessLevels = RangeMap(temperatureLevels, m_waterFreezingLimit, 0.0f, 0.0f, float(SEALEVEL - globalHeight));

				Block& block = m_blocks[blockIndex];

				if (z >= globalHeight) { // Air or water
					if (z <= SEALEVEL) {
						if (temperatureLevels <= m_waterFreezingLimit) {
							if (z + RoundDownToInt(iceDeepnessLevels) >= SEALEVEL) {
								block.m_typeIndex = ice->m_id;
							}
							else {
								block.m_typeIndex = water->m_id;
							}
						}
						else {
							block.m_typeIndex = water->m_id;
						}
					}
					else {
						block.m_typeIndex = air->m_id;
					}
				}



				if (z == globalHeight) {
					block.m_typeIndex = (humidityLevels <= m_baseHumidity) ? sand->m_id : grass->m_id; // Sealevel is either sand or grass
					float randLight = rng.GetRandomFloatZeroUpToOne();
					if (randLight <= 0.0005f) block.m_typeIndex = glowstone->m_id;
				}

				if (z >= dirtHeight && z < globalHeight) {
					if ((humidityLevels <= m_baseHumidity) && (z + int(humidityDeepnessLevels) <= globalHeight)) { // If its not humid, then it is sand
						block.m_typeIndex = sand->m_id;
					}
					else {
						block.m_typeIndex = dirt->m_id;
					}
				}
				else if (z < globalHeight) {
					float chancesForRareBlock = rng.GetRandomFloatZeroUpToOne() * 100.0f;
					if (chancesForRareBlock <= 0.1f) {
						block.m_typeIndex = diamond->m_id;
					}
					else if (chancesForRareBlock <= 0.5f) {
						block.m_typeIndex = gold->m_id;
					}
					else if (chancesForRareBlock <= 2.0f) {
						block.m_typeIndex = iron->m_id;
					}
					else if (chancesForRareBlock <= 5.0f) {
						block.m_typeIndex = coal->m_id;
					}
					else {
						block.m_typeIndex = stone->m_id;
					}
				}

				if (z == SEALEVEL) {
					if ((block.m_typeIndex == grass->m_id)) {
						if ((humidityLevels <= m_beachHumidityLevel)) {
							block.m_typeIndex = sand->m_id;
						}
					}
				}

			}
		}
		calculatedPerlinNoise = true;
	}

	GenerateTrees();


	GenerateCanyons();
	GenerateCaves();

	double endTime = GetCurrentTimeSeconds();
	double totalTime = endTime - startTime;

	if (totalTime > m_game->m_worstPerlinNoiseChunkGenTime) {
		m_game->m_worstPerlinNoiseChunkGenTime = totalTime;
	}

	m_game->m_totalPerlinNoiseChunkGenTime += totalTime;
	m_game->m_countPerlinNoiseChunkGenTime++;
	m_game->RefreshLoadStats();



}

void Chunk::PlaceTrees(std::map<IntVec2, float> const& perlinNoiseHolder)
{
	BlockTemplate const* oakTree = BlockTemplate::GetByName("oak_tree");
	BlockTemplate const* spruceTree = BlockTemplate::GetByName("spruce_tree");
	BlockTemplate const* cactus = BlockTemplate::GetByName("cactus");


	IntVec3 chunkCoords3D = GetGlobalCoordsForIndex(0);
	IntVec2 chunkCoords(chunkCoords3D.x, chunkCoords3D.y);

	int widthCheck = m_treeSideWidth + 1;

	for (int yCoords = -widthCheck; yCoords < CHUNK_SIZE_Y + widthCheck; yCoords++) { // Iterated this way, because terrainHeight was created in this order
		for (int xCoords = -widthCheck; xCoords < CHUNK_SIZE_X + widthCheck; xCoords++) {
			IntVec2 resultingCoords = chunkCoords + IntVec2(xCoords, yCoords);

			bool isLocalMax = AreCoordsConsideredLocalMaxima(resultingCoords, m_treeSpacing / 2, perlinNoiseHolder);
			if (isLocalMax) {
				int terrainHeightAtCoords = GetTerrainHeightAtCoords(resultingCoords);
				IntVec3 localCoords(xCoords, yCoords, terrainHeightAtCoords);

				BlockTemplate const* treeTemplate = oakTree;

				float humidity = 0.0f;
				float temperature = 0.0f;

				if ((xCoords >= 0 && xCoords < CHUNK_SIZE_X) && (yCoords >= 0 && yCoords < CHUNK_SIZE_Y)) {
					int blockIndex = xCoords + (yCoords * CHUNK_SIZE_Y);
					humidity = m_humidity[blockIndex];
					temperature = m_temperature[blockIndex];
				}
				else {
					humidity = 0.5f + (0.5f * Compute2dPerlinNoise((float)resultingCoords.x, (float)resultingCoords.y, 500.0f, 9, 0.2f, 4.0f, true, m_worldSeed + 1)); // Limited to 0,1
					temperature = 0.5f + (0.5f * Compute2dPerlinNoise((float)resultingCoords.x, (float)resultingCoords.y, 450.f, 4, 0.2f, 4.0f, true, m_worldSeed + 2)); // Limited to 0,1
				}



				if (temperature < m_waterFreezingLimit) {
					treeTemplate = spruceTree;
				}

				if (humidity < m_baseHumidity) {
					treeTemplate = cactus;
				}

				PlaceTemplateAboveCoords(localCoords, *treeTemplate);
			}
		}
	}

}

void Chunk::CarveBlockInRadius(Vec3 const& originPos, IntVec3 const& localCoords, float radius, bool replaceWithDirt, bool placeLampInMiddle)
{
	int radiusAsInt = RoundDownToInt(radius);
	int diameter = radiusAsInt * 2;
	float radiusSqr = radius * radius;
	bool changedAnything = false;

	IntVec3 startingCoords = localCoords - IntVec3(radiusAsInt, radiusAsInt, radiusAsInt);

	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;
	static unsigned char dirtId = BlockDefinition::GetDefByName("dirt")->m_id;
	static unsigned char lampId = BlockDefinition::GetDefByName("glowstone")->m_id;

	for (int zOffset = 0; zOffset < diameter; zOffset++) {
		for (int yOffset = 0; yOffset < diameter; yOffset++) {
			for (int xOffset = 0; xOffset < diameter; xOffset++) {

				IntVec3 resultingCoords = startingCoords + IntVec3(xOffset, yOffset, zOffset);
				if (resultingCoords.z <= 0) continue;

				IntVec3 globalCoords = GetGlobalCoordsForLocalCoords(resultingCoords);


				if (!AreLocalCoordsWithinChunk(resultingCoords)) continue;



				Vec3 blockCenter = Vec3(globalCoords.x + 0.5f, globalCoords.y + 0.5f, globalCoords.z + 0.5f);
				float distanceToBlock = GetDistanceSquared3D(blockCenter, originPos);

				int blockIndex = GetIndexForLocalCoords(resultingCoords);
				Block& carvedBlock = m_blocks[blockIndex];

				if (placeLampInMiddle && distanceToBlock == 0.0f && resultingCoords.z < SEALEVEL) {
					carvedBlock.m_typeIndex = lampId;

				}
				else if (distanceToBlock < radiusSqr) {

					if (carvedBlock.m_typeIndex != 0) {


						changedAnything = true;
						BlockIterator blockIter = BlockIterator(this, blockIndex);
						RemoveBlockAt(blockIter, false);
					}
				}
				else if (replaceWithDirt) {
					if (carvedBlock.m_typeIndex != 0) {
						carvedBlock.m_typeIndex = dirtId;
					}
				}



			}
		}

	}

	if (changedAnything) {
		m_isDirty = true;

		if (m_northChunk) m_northChunk->m_isDirty;
		if (m_southChunk) m_southChunk->m_isDirty;
		if (m_eastChunk) m_eastChunk->m_isDirty;
		if (m_westChunk) m_westChunk->m_isDirty;
	}


}

void Chunk::CarveCanyonInRadius(Vec3 const& originPos, IntVec3 const& localCoords, float radius, int depth, bool replaceWithDirt, bool placeLampInMiddle)
{

	int radiusAsInt = RoundDownToInt(radius);
	int diameter = radiusAsInt * 2;
	bool changedAnything = false;

	IntVec3 startingCoords = localCoords - IntVec3(radiusAsInt, radiusAsInt, 0);

	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;
	static unsigned char dirtId = BlockDefinition::GetDefByName("dirt")->m_id;
	static unsigned char lampId = BlockDefinition::GetDefByName("glowstone")->m_id;

	for (int zOffset = 0; zOffset < depth; zOffset++) {
		for (int yOffset = 0; yOffset < diameter; yOffset++) {
			for (int xOffset = 0; xOffset < diameter; xOffset++) {

				IntVec3 resultingCoords = startingCoords + IntVec3(xOffset, yOffset, -zOffset);
				if (resultingCoords.z <= 0) continue;

				IntVec3 globalCoords = GetGlobalCoordsForLocalCoords(resultingCoords);


				if (!AreLocalCoordsWithinChunk(resultingCoords)) continue;



				Vec3 blockCenter = Vec3(globalCoords.x + 0.5f, globalCoords.y + 0.5f, globalCoords.z + 0.5f);
				float distanceToBlock = GetDistanceSquared3D(blockCenter, originPos);

				int blockIndex = GetIndexForLocalCoords(resultingCoords);
				Block& carvedBlock = m_blocks[blockIndex];

				if (placeLampInMiddle && distanceToBlock == 0.0f && resultingCoords.z < SEALEVEL) {
					carvedBlock.m_typeIndex = lampId;

				}
				else {
					if (carvedBlock.m_typeIndex != 0) {
						BlockIterator blockIter = BlockIterator(this, blockIndex);
						Block* topBlock = blockIter.GetTopNeighbor().GetBlock();

						changedAnything = true;
						if (replaceWithDirt) {
							carvedBlock.m_typeIndex = dirtId;

						}
						else if ( topBlock && (topBlock->m_typeIndex == waterId)) {
							carvedBlock.m_typeIndex = waterId;

						}
						else {
							RemoveBlockAt(blockIter, false);

						}
					}
				}


			}
		}

	}

	if (changedAnything) {
		m_isDirty = true;

		if (m_northChunk) m_northChunk->m_isDirty;
		if (m_southChunk) m_southChunk->m_isDirty;
		if (m_eastChunk) m_eastChunk->m_isDirty;
		if (m_westChunk) m_westChunk->m_isDirty;
	}
}

bool Chunk::AreLocalCoordsWithinChunk(IntVec3 const& localCoords) const
{
	if (localCoords.x < 0 || localCoords.x >= CHUNK_SIZE_X) return false;
	if (localCoords.y < 0 || localCoords.y >= CHUNK_SIZE_Y) return false;
	if (localCoords.z < 0 || localCoords.z >= CHUNK_SIZE_Z) return false;

	return true;
}

bool Chunk::AreGlobalCoordsWithinChunk(IntVec3 const& globalCoords) const
{
	IntVec3 chunkGlobalMin = GetGlobalCoordsForIndex(0);
	IntVec3 chunkGlobalMax = GetGlobalCoordsForIndex(CHUNK_TOTAL_SIZE - 1);

	if (globalCoords.x < chunkGlobalMin.x || globalCoords.x >= chunkGlobalMax.x) return false;
	if (globalCoords.y < chunkGlobalMin.y || globalCoords.y >= chunkGlobalMax.y) return false;
	if (globalCoords.z < chunkGlobalMin.z || globalCoords.z >= chunkGlobalMax.z) return false;


	return false;
}

void Chunk::GenerateTrees()
{

	std::map<IntVec2, float> treeNoise;

	GetTreeNoiseForChunk(treeNoise);
	PlaceTrees(treeNoise);
}

void Chunk::GenerateCanyons()
{
	std::map<IntVec2, float> startPerlinNoise;
	GetCanyonsStartPerlinNoise(startPerlinNoise);

	int diameter = m_canyonCheckRadius * 2;

	IntVec2 startCoords = m_globalCoordinates - IntVec2(m_canyonCheckRadius, m_canyonCheckRadius);


	for (int yOffset = 0; yOffset < diameter; yOffset++) {
		for (int xOffset = 0; xOffset < diameter; xOffset++) {
			IntVec2 resultingCoords = startCoords + IntVec2(xOffset, yOffset);

			bool isCanyonMaxima = AreCoordsConsideredLocalMaxima(resultingCoords, m_caveCheckRadius, startPerlinNoise);
			if (isCanyonMaxima) {
				std::vector<IntVec3> canyonPoints;
				canyonPoints.reserve(m_canyonNodeAmount);

				GetCanyonPath(resultingCoords, canyonPoints);
				CarveCanyonPath(canyonPoints);

			}

		}
	}

}

void Chunk::GetCanyonsStartPerlinNoise(std::map<IntVec2, float>& perlinNoiseHolder) const
{
	int furthestRadio = m_canyonCheckRadius * 2; // I have to check on the grids corner a radius of chunks as well
	int diameter = furthestRadio * 8;

	IntVec2 startCoords = m_globalCoordinates - IntVec2(furthestRadio, furthestRadio);

	for (int yOffset = 0; yOffset < diameter; yOffset++) {
		for (int xOffset = 0; xOffset < diameter; xOffset++) {
			IntVec2 resultingCoords = startCoords + IntVec2(xOffset, yOffset);
			float perlinNoise = 0.5f + 0.5f * Compute2dPerlinNoise((float)resultingCoords.x, (float)resultingCoords.y, 3.0f, 6, 0.65f, 10.0f, true, m_worldSeed + 6);

			perlinNoiseHolder[resultingCoords] = perlinNoise;

		}
	}

}


void Chunk::GetCanyonPath(IntVec2 const& coords, std::vector<IntVec3>& canyonPoints) const
{

	Vec2 chunkCenter2D = Chunk::GetChunkCenter(coords);
	IntVec2 chunkCenterBlockCoords = IntVec2(RoundDownToInt(chunkCenter2D.x), RoundDownToInt(chunkCenter2D.y));

	Vec3 currentPos = Vec3(chunkCenter2D.x, chunkCenter2D.y, (float)(CHUNK_SIZE_Z / 2));

	IntVec3 currentCoords = IntVec3(chunkCenterBlockCoords.x, chunkCenterBlockCoords.y, 0);

	currentCoords.z = GetTerrainHeightAtCoords(chunkCenterBlockCoords);
	currentPos.z = (float)currentCoords.z;

	float currentYaw = 0.0f;

	canyonPoints.push_back(currentCoords);

	for (int nodeStep = 0; nodeStep < m_canyonNodeAmount; nodeStep++) {

		float yawTurnNoise = Compute2dPerlinNoise((float)currentCoords.x, (float)currentCoords.y, 20.0f, 2, 0.4f, 2.0f, true, m_worldSeed + 7);

		float yawTurnChange = RangeMapClamped(yawTurnNoise, -1.0f, 1.0f, -m_canyonTurnRate, m_canyonTurnRate);

		currentYaw += yawTurnChange;

		EulerAngles dir(currentYaw, 0.0f, 0.0f);
		Vec3 currentFwd = dir.GetXForward();

		currentPos += currentFwd * (float)m_canyonBlockSteps;

		currentCoords.x = RoundDownToInt(currentPos.x);
		currentCoords.y = RoundDownToInt(currentPos.y);
		currentCoords.z = GetTerrainHeightAtCoords(IntVec2(currentCoords.x, currentCoords.y));

		currentPos.x = (float)currentCoords.x;
		currentPos.y = (float)currentCoords.y;
		currentPos.z = (float)currentCoords.z;

		canyonPoints.push_back(currentCoords);

	}

}

void Chunk::CarveCanyonPath(std::vector<IntVec3>& canyonPoints)
{


	for (int canyonPointInd = 0; canyonPointInd < canyonPoints.size(); canyonPointInd++) {
		IntVec3 const& coords = canyonPoints[canyonPointInd];
		IntVec3 localCoords = GetLocalCoordsForGlobalCoords(coords);

		Vec3 const blockCenter = Vec3(coords.x + 0.5f, coords.y + 0.5f, coords.z + 0.5f);
		float radiusNoise = 0.5f + 0.5f * Compute2dPerlinNoise((float)coords.x, (float)coords.y, 2.0f, 5, 0.65f, 2.0f, true, m_worldSeed + 8);
		float radius = RangeMap(radiusNoise, 0.0f, 1.0f, m_canyonMaxRadius - 2.0f, m_canyonMaxRadius);

		CarveCanyonInRadius(blockCenter, localCoords, radius, m_canyonDepth, false, false);

	}
}

void Chunk::GenerateCaves()
{
	std::map<IntVec2, float> startPerlinNoise;
	GetCavesStartPerlinNoise(startPerlinNoise);

	int diameter = m_caveCheckRadius * 2;

	IntVec2 startCoords = m_globalCoordinates - IntVec2(m_caveCheckRadius, m_caveCheckRadius);


	for (int yOffset = 0; yOffset < diameter; yOffset++) {
		for (int xOffset = 0; xOffset < diameter; xOffset++) {
			IntVec2 resultingCoords = startCoords + IntVec2(xOffset, yOffset);

			bool isCaveMaxima = AreCoordsConsideredLocalMaxima(resultingCoords, m_caveCheckRadius, startPerlinNoise);
			if (isCaveMaxima) {
				std::vector<IntVec3> cavePoints;
				cavePoints.reserve(m_caveNodeAmount);

				GetCavePath(resultingCoords, cavePoints);
				CarveCavePath(cavePoints);

			}

		}
	}
}

void Chunk::GetCavesStartPerlinNoise(std::map<IntVec2, float>& perlinNoiseHolder) const
{
	int furthestRadio = m_caveCheckRadius * 2; // I have to check on the grids corner a radius of chunks as well
	int diameter = furthestRadio * 8;

	IntVec2 startCoords = m_globalCoordinates - IntVec2(furthestRadio, furthestRadio);

	for (int yOffset = 0; yOffset < diameter; yOffset++) {
		for (int xOffset = 0; xOffset < diameter; xOffset++) {
			IntVec2 resultingCoords = startCoords + IntVec2(xOffset, yOffset);
			float perlinNoise = 0.5f + 0.5f * Compute2dPerlinNoise((float)resultingCoords.x, (float)resultingCoords.y, 0.35f, 7, 0.9f, 20.0f, true, m_worldSeed + 9); // Good for caves

			perlinNoiseHolder[resultingCoords] = perlinNoise;

		}
	}
}


void Chunk::GetCavePath(IntVec2 const& coords, std::vector<IntVec3>& cavePoints) const
{
	Vec2 chunkCenter2D = Chunk::GetChunkCenter(coords);
	IntVec2 chunkCenterBlockCoords = IntVec2(RoundDownToInt(chunkCenter2D.x), RoundDownToInt(chunkCenter2D.y));

	Vec3 currentPos = Vec3(chunkCenter2D.x, chunkCenter2D.y, (float)(CHUNK_SIZE_Z / 2));

	IntVec3 currentCoords = IntVec3(chunkCenterBlockCoords.x, chunkCenterBlockCoords.y, 0);

	currentCoords.z = GetTerrainHeightAtCoords(chunkCenterBlockCoords) - m_caveDepthStart;
	currentPos.z = (float)currentCoords.z;

	float currentYaw = 0.0f;

	cavePoints.push_back(currentCoords);

	for (int nodeStep = 0; nodeStep < m_caveNodeAmount; nodeStep++) {

		float yawTurnNoise = Compute2dPerlinNoise((float)currentCoords.x, (float)currentCoords.y, 20.0f, 2, 0.4f, 2.0f, true, m_worldSeed + 10);
		float pitch = 89.9f * Compute2dPerlinNoise((float)currentCoords.x, (float)currentCoords.y, 20.0f, 4, 0.4f, 2.0f, true, m_worldSeed + 11);

		float yawTurnChange = RangeMapClamped(yawTurnNoise, -1.0f, 1.0f, -m_caveTurnRate, m_caveTurnRate);

		currentYaw += yawTurnChange;

		EulerAngles dir(currentYaw, pitch, 0.0f);
		Vec3 currentFwd = dir.GetXForward();

		currentPos += currentFwd * (float)m_caveBlockSteps;

		currentCoords.x = RoundDownToInt(currentPos.x);
		currentCoords.y = RoundDownToInt(currentPos.y);
		currentCoords.z = RoundDownToInt(currentPos.z);

		currentPos.x = (float)currentCoords.x;
		currentPos.y = (float)currentCoords.y;
		currentPos.z = (float)currentCoords.z;

		cavePoints.push_back(currentCoords);

	}
}

void Chunk::CarveCavePath(std::vector<IntVec3>& cavePoints)
{
	for (int cavePointInd = 0; cavePointInd < cavePoints.size(); cavePointInd++) {
		IntVec3 const& coords = cavePoints[cavePointInd];
		IntVec3 localCoords = GetLocalCoordsForGlobalCoords(coords);
		localCoords.z += 1;

		Vec3 const blockCenter = Vec3(coords.x + 0.5f, coords.y + 0.5f, coords.z + 0.5f);
		float radiusNoise = 0.5f + 0.5f * Compute2dPerlinNoise((float)coords.x, (float)coords.y, 2.0f, 5, 0.65f, 2.0f, true, m_worldSeed + 11);
		float radius = RangeMap(radiusNoise, 0.0f, 1.0f, m_caveMaxRadius - 1.0f, m_caveMaxRadius);

		CarveBlockInRadius(blockCenter, localCoords, radius, false, true);



	}
}

bool Chunk::AreCoordsConsideredLocalMaxima(IntVec2 const& coords, int radius, std::map<IntVec2, float> const& perlinNoiseHolder) const
{
	IntVec2 startCoords = coords - IntVec2(radius, radius);
	auto localNoiseIt = perlinNoiseHolder.find(coords);

	if (localNoiseIt == perlinNoiseHolder.end()) return false;

	float localNoise = localNoiseIt->second;


	int diameter = radius * 2;

	for (int yOffset = 0; yOffset < diameter; yOffset++) {
		for (int xOffset = 0; xOffset < diameter; xOffset++) {
			IntVec2 resultingCoords = startCoords + IntVec2(xOffset, yOffset);
			if (resultingCoords == coords) continue;

			auto coordsNoiseIt = perlinNoiseHolder.find(resultingCoords);
			if (coordsNoiseIt == perlinNoiseHolder.end()) continue;


			float const& coordsNoise = coordsNoiseIt->second;
			if (localNoise <= coordsNoise) {
				return false;
			}

		}
	}

	return true;
}


void Chunk::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (!m_chunkVBO || !m_chunkWaterVBO) {
		BufferDesc newVBODesc = {};
		newVBODesc.data = nullptr;
		newVBODesc.memoryUsage = MemoryUsage::Default;
		newVBODesc.owner = g_theRenderer;
		newVBODesc.size = sizeof(Vertex_PCU);
		newVBODesc.stride = sizeof(Vertex_PCU);

		m_chunkVBO = new VertexBuffer(newVBODesc);
		m_chunkWaterVBO = new VertexBuffer(newVBODesc);
	}

	if (!m_chunkIBO || !m_chunkWaterIBO) {
		BufferDesc newIBODesc = {};
		newIBODesc.data = nullptr;
		newIBODesc.memoryUsage = MemoryUsage::Default;
		newIBODesc.owner = g_theRenderer;
		newIBODesc.size = sizeof(unsigned int);
		newIBODesc.stride = sizeof(unsigned int);
		m_chunkIBO = new IndexBuffer(newIBODesc);
		m_chunkWaterIBO = new IndexBuffer(newIBODesc);
	}

}

void Chunk::Render() const
{
	bool doesTerrainHaveVerts = (m_chunkVBO->GetSize() > 1) && (m_chunkIBO->GetSize() > 1) && (m_blockIndexes.size() > 0);

	if (doesTerrainHaveVerts) {
		g_theRenderer->DrawIndexedVertexBuffer(m_chunkVBO, m_chunkIBO, (int)m_blockIndexes.size());
	}

}

void Chunk::RenderWater() const
{
	bool doesWaterHaveVerts = (m_chunkWaterVBO->GetSize() > 1) && (m_chunkWaterIBO->GetSize() > 1);

	if (doesWaterHaveVerts) {
		g_theRenderer->DrawIndexedVertexBuffer(m_chunkWaterVBO, m_chunkWaterIBO, (int)m_blockIndexesWater.size());
	}

}

void Chunk::RemoveBlockBelow(Vec3 const& referencePos)
{
	IntVec3 localCoords = Chunk::GetLocalCoordsForWorldPos(referencePos);

	for (int height = localCoords.z; height >= 0; height--) {
		IntVec3 blockCoords = localCoords;
		blockCoords.z = height;
		int blockIndex = Chunk::GetIndexForLocalCoords(blockCoords);
		Block& block = m_blocks[blockIndex];
		BlockIterator blockIterator = BlockIterator(this, blockIndex);

		if (block.m_typeIndex != 0) {
			block.m_typeIndex = 0;
			m_isDirty = true;
			m_needsSaving = true;

			if (localCoords.y == CHUNK_MAX_Y) {
				if (blockIterator.GetNorthNeighbor().GetBlock()->m_typeIndex != 0) {
					m_northChunk->m_isDirty = true;
				}
			}
			if (localCoords.x == 0) {
				if (blockIterator.GetWestNeighbor().GetBlock()->m_typeIndex != 0) {
					m_westChunk->m_isDirty = true;
				}
			}
			if (localCoords.y == 0) {
				if (blockIterator.GetSouthNeighbor().GetBlock()->m_typeIndex != 0) {
					m_southChunk->m_isDirty = true;
				}
			}
			if (localCoords.x == CHUNK_MAX_X) {
				if (blockIterator.GetEastNeighbor().GetBlock()->m_typeIndex != 0) {
					m_eastChunk->m_isDirty = true;
				}
			}
		}
	}

}

void Chunk::RemoveBlockAt(BlockIterator const& blockIterator, bool needSaving)
{

	Block* block = blockIterator.GetBlock();
	if (!block) return;
	IntVec3 localCoords = Chunk::GetLocalCoordsForIndex(blockIterator.GetIndex());

	block->m_typeIndex = 0;
	m_isDirty = true;
	m_needsSaving = needSaving;

	if (localCoords.y == CHUNK_MAX_Y) {

		BlockIterator northNeighbor = blockIterator.GetNorthNeighbor();
		Block* neighborBlock = northNeighbor.GetBlock();

		if (neighborBlock && neighborBlock->m_typeIndex != 0) {
			m_northChunk->m_isDirty = true;
		}
	}
	if (localCoords.x == 0) {
		BlockIterator westNeighbor = blockIterator.GetWestNeighbor();
		Block* neighborBlock = westNeighbor.GetBlock();

		if (neighborBlock && neighborBlock->m_typeIndex != 0) {
			m_westChunk->m_isDirty = true;
		}
	}
	if (localCoords.y == 0) {
		BlockIterator southNeighbor = blockIterator.GetSouthNeighbor();
		Block* neighborBlock = southNeighbor.GetBlock();
		if (neighborBlock && neighborBlock->m_typeIndex != 0) {
			m_southChunk->m_isDirty = true;
		}
	}
	if (localCoords.x == CHUNK_MAX_X) {
		BlockIterator eastNeighbor = blockIterator.GetEastNeighbor();
		Block* neighborBlock = eastNeighbor.GetBlock();

		if (neighborBlock && neighborBlock->m_typeIndex != 0) {
			m_eastChunk->m_isDirty = true;
		}
	}

}

void Chunk::PlaceBlockBelow(Vec3 const& referencePos, unsigned char blockType)
{
	IntVec3 localCoords = Chunk::GetLocalCoordsForWorldPos(referencePos);

	for (int height = localCoords.z; height > 0; height--) {
		IntVec3 blockBelowCoords = localCoords;

		blockBelowCoords.z = height - 1;
		int blockBelowIndex = Chunk::GetIndexForLocalCoords(blockBelowCoords);
		Block& blockBelow = m_blocks[blockBelowIndex];

		BlockIterator blockIterator = BlockIterator(this, blockBelowIndex);

		if (blockBelow.m_typeIndex != 0) { // Block below must be solid to turn current air to solid
			IntVec3 blockCoords = localCoords;
			blockCoords.z = height;

			int currentBlockIndex = Chunk::GetIndexForLocalCoords(blockCoords);

			Block& currentBlock = m_blocks[currentBlockIndex];
			currentBlock.m_typeIndex = blockType;
			m_isDirty = true;
			m_needsSaving = true;
			if (localCoords.y == CHUNK_MAX_Y) {
				if (blockIterator.GetNorthNeighbor().GetBlock()->m_typeIndex != 0) {
					m_northChunk->m_isDirty = true;
				}
			}
			if (localCoords.x == 0) {
				if (blockIterator.GetWestNeighbor().GetBlock()->m_typeIndex != 0) {
					m_westChunk->m_isDirty = true;
				}
			}
			if (localCoords.y == 0) {
				if (blockIterator.GetSouthNeighbor().GetBlock()->m_typeIndex != 0) {
					m_southChunk->m_isDirty = true;
				}
			}
			if (localCoords.x == CHUNK_MAX_X) {
				if (blockIterator.GetEastNeighbor().GetBlock()->m_typeIndex != 0) {
					m_eastChunk->m_isDirty = true;
				}
			}
		}
	}

}

void Chunk::PlaceBlockAt(BlockIterator const& blockIterator, unsigned char blockType)
{
	Block* block = blockIterator.GetBlock();
	if (!block) return;

	block->m_typeIndex = blockType;
	IntVec3 localCoords = GetLocalCoordsForIndex(blockIterator.GetIndex());

	m_isDirty = true;
	m_needsSaving = true;
	if (localCoords.y == CHUNK_MAX_Y) {
		if (blockIterator.GetNorthNeighbor().GetBlock()->m_typeIndex != 0) {
			m_northChunk->m_isDirty = true;
		}
	}
	if (localCoords.x == 0) {
		if (blockIterator.GetWestNeighbor().GetBlock()->m_typeIndex != 0) {
			m_westChunk->m_isDirty = true;
		}
	}
	if (localCoords.y == 0) {
		if (blockIterator.GetSouthNeighbor().GetBlock()->m_typeIndex != 0) {
			m_southChunk->m_isDirty = true;
		}
	}
	if (localCoords.x == CHUNK_MAX_X) {
		if (blockIterator.GetEastNeighbor().GetBlock()->m_typeIndex != 0) {
			m_eastChunk->m_isDirty = true;
		}
	}

}

void Chunk::GetTreeNoiseForChunk(std::map<IntVec2, float>& perlinNoiseHolder)
{
	IntVec3 bottomLeftCoords3D = GetGlobalCoordsForIndex(0);

	int widthCheck = m_treeSideWidth + 1;

	IntVec2 bottomLeftCoords(bottomLeftCoords3D.x - widthCheck, bottomLeftCoords3D.y - widthCheck);


	for (int heightIndex = 0; heightIndex < CHUNK_SIZE_Y + m_treeSideWidth + widthCheck; heightIndex++) {
		for (int widthIndex = 0; widthIndex < CHUNK_SIZE_X + m_treeSideWidth + widthCheck; widthIndex++) {

			IntVec2 localCoords(widthIndex, heightIndex);
			localCoords += bottomLeftCoords;


			float persistence = Compute2dPerlinNoise((float)localCoords.x, (float)localCoords.y, 150.0f, 4, 0.5f, 2.0f, true, m_worldSeed + 4);

			float noise = Compute2dPerlinNoise((float)localCoords.x, (float)localCoords.y, 500.0f, 6, persistence, 2.0f, true, m_worldSeed + 5);
			perlinNoiseHolder[localCoords] = noise;


		}

	}

}

void Chunk::PlaceTemplateAboveCoords(IntVec3 localCoords, BlockTemplate const& blockTemplate)
{
	IntVec3 placementCoords = localCoords;
	placementCoords.z += 1;

	if (localCoords.z <= SEALEVEL) return;

	for (int templateEntryIndex = 0; templateEntryIndex < blockTemplate.m_blockTemplateEntries.size(); templateEntryIndex++) {
		BlockTemplateEntry const& templateEntry = blockTemplate.m_blockTemplateEntries[templateEntryIndex];
		IntVec3 offSet = placementCoords + templateEntry.m_offset;
		if (offSet.x >= CHUNK_SIZE_X || offSet.x < 0) continue;
		if (offSet.y >= CHUNK_SIZE_Y || offSet.y < 0) continue;
		if (offSet.z >= CHUNK_SIZE_Z || offSet.z < 0) continue;

		int blockIn = GetIndexForLocalCoords(offSet);
		if (blockIn >= CHUNK_TOTAL_SIZE) continue;
		if (blockIn < 0) continue;

		m_blocks[blockIn].m_typeIndex = templateEntry.m_blockType;

	}
}

Vec3 const Chunk::GetChunkCenter() const
{
	int centerX = (m_globalCoordinates.x * CHUNK_SIZE_X) + CHUNK_SIZE_X / 2;
	int centerY = (m_globalCoordinates.y * CHUNK_SIZE_Y) + CHUNK_SIZE_Y / 2;
	int centerZ = CHUNK_SIZE_Z / 2;

	return Vec3(float(centerX), float(centerY), float(centerZ));
}

bool Chunk::CanBeLoadedFromFile() const
{
	std::string fileName = GetChunkFileName();

	return FileExists(fileName);
}



IntVec3 Chunk::GetLocalCoordsForWorldPos(Vec3 const& position)
{
	IntVec3 referenceCoords((int)floorf(position.x), (int)floorf(position.y), (int)floorf(position.z));
	referenceCoords.x = referenceCoords.x & (CHUNK_SIZE_X - 1);
	referenceCoords.y = referenceCoords.y & (CHUNK_SIZE_Y - 1);


	return referenceCoords;
}

int Chunk::GetIndexForLocalCoords(IntVec3 const& localCoords)
{
	return localCoords.x | (localCoords.y << CHUNKSHIFT_Y) | (localCoords.z << (CHUNKSHIFT_Z));
}

IntVec3 Chunk::GetLocalCoordsForIndex(int index)
{
	int z = (index >> CHUNKSHIFT_Z) & (CHUNK_SIZE_Z - 1);
	int y = (index >> CHUNKSHIFT_Y) & (CHUNK_SIZE_Y - 1);
	int x = (index) & (CHUNK_SIZE_X - 1);

	return IntVec3(x, y, z);
}

IntVec2 Chunk::GetChunkCoordsForWorldPos(Vec3 const& position)
{
	int chunkX = static_cast<int>(floorf(position.x)) >> CHUNK_BITS_X;
	int chunkY = static_cast<int>(floorf(position.y)) >> CHUNK_BITS_Y;

	return IntVec2(chunkX, chunkY);

}

IntVec3 Chunk::GetGlobalCoordsForIndex(int blockIndex) const
{
	IntVec3 localCoords = GetLocalCoordsForIndex(blockIndex);
	return IntVec3(m_globalCoordinates.x * CHUNK_SIZE_X, m_globalCoordinates.y * CHUNK_SIZE_Y, 0) + localCoords;
}

IntVec3 Chunk::GetLocalCoordsForGlobalCoords(IntVec3 const& globalCoords) const
{
	int chunkX = m_globalCoordinates.x * CHUNK_SIZE_X;
	int chunkY = m_globalCoordinates.y * CHUNK_SIZE_Y;

	return IntVec3(globalCoords.x - chunkX, globalCoords.y - chunkY, globalCoords.z);
}

IntVec3 Chunk::GetGlobalCoordsForLocalCoords(IntVec3 const& localCoords) const
{
	int chunkX = m_globalCoordinates.x * CHUNK_SIZE_X;
	int chunkY = m_globalCoordinates.y * CHUNK_SIZE_Y;

	return IntVec3(localCoords.x + chunkX, localCoords.y + chunkY, localCoords.z);
}

Vec2 const Chunk::GetChunkCenter(IntVec2 const& coords)
{
	int centerX = (coords.x * CHUNK_SIZE_X) + CHUNK_SIZE_X / 2;
	int centerY = (coords.y * CHUNK_SIZE_Y) + CHUNK_SIZE_Y / 2;

	return Vec2(float(centerX), float(centerY));
}

bool Chunk::HasExistingNeighors() const
{
	return m_northChunk && m_southChunk && m_eastChunk && m_westChunk;
}

int Chunk::GetTerrainHeightAtCoords(IntVec2 const& coords) const
{

	float terrainPerlinNoise = fabsf(Compute2dPerlinNoise((float)coords.x, (float)coords.y, 200.f, 7, 0.35f, 2.0f, true, m_worldSeed));
	float hilliness = 0.5f + (0.5f * Compute2dPerlinNoise((float)coords.x, (float)coords.y, 600.0f, 5, 0.4f, 2.0f, true, m_worldSeed + 3)); // Limited to 0,1
	float oceanness = 0.5f + (0.5f * Compute2dPerlinNoise((float)coords.x, (float)coords.y, 1000.0f, 7, 0.05f, 2.0f, false, m_worldSeed + 4)); // Limited to 0,1
	// Worldseed + 4 reserved to treeness


	oceanness = SmoothStart6(oceanness);
	hilliness = SmoothStop2(hilliness); // So low hilliness is still there, but not as common

	float terrainVarianceLow = (1.0f - hilliness) * 0.25f; // By multiplying it by 0.25f, I can influence hilliness against just high plains
	float terrainVarianceHigh = (hilliness);

	float proposedTerrainVariance = RangeMapClamped(terrainPerlinNoise, 0.0f, 1.0f, terrainVarianceLow, terrainVarianceHigh);
	int terrainHeightWithHilliness = SEALEVEL - 3 + (int)(60.0f * SmoothStep3(proposedTerrainVariance)); // - 3 Gives a nice balance of rivers

	return (int)RangeMap(oceanness, 0.0f, 0.5f, (float)terrainHeightWithHilliness, float(SEALEVEL - m_oceanDepth));
}

void Chunk::GenerateCPUMesh()
{
	if (!(m_northChunk && m_southChunk && m_eastChunk && m_westChunk)) return;
	double startTime = GetCurrentTimeSeconds();

	m_blockVertexes.clear();
	m_blockVertexes.reserve(VERTEX_RESERVE_AMOUNT);
	m_blockIndexes.clear();
	m_blockIndexes.reserve(VERTEX_RESERVE_AMOUNT);

	m_blockVertexesWater.clear();
	m_blockVertexesWater.reserve(VERTEX_RESERVE_AMOUNT);
	m_blockIndexesWater.clear();
	m_blockIndexesWater.reserve(VERTEX_RESERVE_AMOUNT);

	for (int z = 0; z < CHUNK_SIZE_Z; z++) {
		for (int y = 0; y < CHUNK_SIZE_Y; y++) {
			for (int x = 0; x < CHUNK_SIZE_X; x++) {
				int index = Chunk::GetIndexForLocalCoords(IntVec3(x, y, z));
				AddVertsForBlock(m_blocks[index], x, y, z);
			}
		}
	}

	m_isDirty = false;

	if (!m_chunkVBO || !m_chunkWaterVBO) {
		BufferDesc newVBODesc = {};
		newVBODesc.data = nullptr;
		newVBODesc.memoryUsage = MemoryUsage::Default;
		newVBODesc.owner = g_theRenderer;
		newVBODesc.size = sizeof(Vertex_PCU);
		newVBODesc.stride = sizeof(Vertex_PCU);

		m_chunkVBO = new VertexBuffer(newVBODesc);
		m_chunkWaterVBO = new VertexBuffer(newVBODesc);
	}

	if (!m_chunkIBO || !m_chunkWaterIBO) {

		BufferDesc newIBODesc = {};
		newIBODesc.data = nullptr;
		newIBODesc.memoryUsage = MemoryUsage::Default;
		newIBODesc.owner = g_theRenderer;
		newIBODesc.size = sizeof(unsigned int);
		newIBODesc.stride = sizeof(unsigned int);

		m_chunkIBO = new IndexBuffer(newIBODesc);
		m_chunkWaterIBO = new IndexBuffer(newIBODesc);
	}


	size_t chunkVBOSize = m_chunkVBO->GetStride() * m_blockVertexes.size();
	size_t chunkIBOSize = m_chunkIBO->GetStride() * m_blockIndexes.size();
	m_chunkVBO->GuaranteeBufferSize(chunkVBOSize);
	m_chunkIBO->GuaranteeBufferSize(chunkIBOSize);

	m_chunkVBO->CopyCPUToGPU(m_blockVertexes.data(), chunkVBOSize);
	m_chunkIBO->CopyCPUToGPU(m_blockIndexes.data(), chunkIBOSize);


	size_t chunkWaterVBOSize = m_chunkWaterVBO->GetStride() * m_blockVertexesWater.size();
	size_t chunkWaterIBOSize = m_chunkWaterIBO->GetStride() * m_blockIndexesWater.size();
	m_chunkWaterVBO->GuaranteeBufferSize(chunkWaterVBOSize);
	m_chunkWaterIBO->GuaranteeBufferSize(chunkWaterIBOSize);

	m_chunkWaterVBO->CopyCPUToGPU(m_blockVertexesWater.data(), chunkWaterVBOSize);
	m_chunkWaterIBO->CopyCPUToGPU(m_blockIndexesWater.data(), chunkWaterIBOSize);

	double endTime = GetCurrentTimeSeconds();

	double totalTime = endTime - startTime;

	if (totalTime > m_game->m_worstChunkMeshRegen) {
		m_game->m_worstChunkMeshRegen = totalTime;
	}



	m_game->m_totalChunkMeshRegen += totalTime;
	m_game->m_countChunkMeshRegen++;
	m_game->RefreshLoadStats();

}

void Chunk::AddVertsForBlock(Block const& block, int x, int y, int z)
{
	BlockDefinition const* blockDef = BlockDefinition::GetDefById(block.m_typeIndex);
	if (!blockDef->m_isVisible) return;

	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;

	bool isWater = (waterId == blockDef->m_id);

	float flX = static_cast<float>(x) + static_cast<float>(m_globalCoordinates.x * CHUNK_SIZE_X);
	float flY = static_cast<float>(y) + static_cast<float>(m_globalCoordinates.y * CHUNK_SIZE_Y);
	float flZ = static_cast<float>(z);


	Vec3 const& rightFrontBottom = Vec3(flX + 1.0f, flY, flZ);
	Vec3 const& rightBackBottom = Vec3(flX + 1.0f, flY + 1.0f, flZ);
	Vec3 const& rightBackTop = Vec3(flX + 1.0f, flY + 1.0f, flZ + 1.0f);
	Vec3 const& rightFrontTop = Vec3(flX + 1.0f, flY, flZ + 1.0f);
	Vec3 const& leftFrontBottom = Vec3(flX, flY, flZ);
	Vec3 const& leftBackBottom = Vec3(flX, flY + 1.0f, flZ);
	Vec3 const& leftBackTop = Vec3(flX, flY + 1.0f, flZ + 1.0f);
	Vec3 const& leftFrontTop = Vec3(flX, flY, flZ + 1.0f);

	Rgba8 verticalColor = Rgba8(255, 255, 255);
	Rgba8 sideColor = Rgba8(230, 230, 230);
	Rgba8 frontBackColor = Rgba8(205, 205, 205);

	BlockIterator currentBlock = BlockIterator(this, GetIndexForLocalCoords(IntVec3(x, y, z)));
	Block* northBlock = currentBlock.GetNorthNeighbor().GetBlock();
	Block* southBlock = currentBlock.GetSouthNeighbor().GetBlock();
	Block* eastBlock = currentBlock.GetEastNeighbor().GetBlock();
	Block* westBlock = currentBlock.GetWestNeighbor().GetBlock();
	Block* topBlock = currentBlock.GetTopNeighbor().GetBlock();
	Block* bottomBlock = currentBlock.GetBottomNeighbor().GetBlock();

	static bool isHSREnabled = !g_gameConfigBlackboard.GetValue("DEBUG_DISABLE_HSR", false);


	if (isHSREnabled) {
		AddVertsForHRSBlockQuad(northBlock, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, blockDef->m_sideUVs, isWater);
		AddVertsForHRSBlockQuad(eastBlock, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, blockDef->m_sideUVs, isWater);
		AddVertsForHRSBlockQuad(southBlock, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, blockDef->m_sideUVs, isWater);
		AddVertsForHRSBlockQuad(westBlock, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, blockDef->m_sideUVs, isWater);
		AddVertsForHRSBlockQuad(bottomBlock, leftFrontBottom, leftBackBottom, rightBackBottom, rightFrontBottom, blockDef->m_bottomUVs, isWater);
		AddVertsForHRSBlockQuad(topBlock, rightFrontTop, rightBackTop, leftBackTop, leftFrontTop, blockDef->m_topUVs, isWater, true);
	}
	else {
		std::vector<Vertex_PCU>& usedVerts = (isWater) ? m_blockVertexesWater : m_blockVertexes;
		std::vector<unsigned int>& usedIndexes = (isWater) ? m_blockIndexesWater : m_blockIndexes;

		AddVertsForIndexedQuad3D(usedVerts, usedIndexes, rightBackBottom, leftBackBottom, leftBackTop, rightBackTop, frontBackColor, blockDef->m_sideUVs); // Back
		AddVertsForIndexedQuad3D(usedVerts, usedIndexes, rightFrontBottom, rightBackBottom, rightBackTop, rightFrontTop, sideColor, blockDef->m_sideUVs); // Right
		AddVertsForIndexedQuad3D(usedVerts, usedIndexes, leftFrontBottom, rightFrontBottom, rightFrontTop, leftFrontTop, frontBackColor, blockDef->m_sideUVs); // Front
		AddVertsForIndexedQuad3D(usedVerts, usedIndexes, leftBackBottom, leftFrontBottom, leftFrontTop, leftBackTop, sideColor, blockDef->m_sideUVs); // Left
		AddVertsForIndexedQuad3D(usedVerts, usedIndexes, leftFrontBottom, leftBackBottom, rightBackBottom, rightFrontBottom, verticalColor, blockDef->m_bottomUVs); // Bottom;
		AddVertsForIndexedQuad3D(usedVerts, usedIndexes, rightFrontTop, rightBackTop, leftBackTop, leftFrontTop, verticalColor, blockDef->m_topUVs); // Top
	}

}

void Chunk::AddVertsForHRSBlockQuad(Block* neighborBlock, Vec3 const& pos1, Vec3 const& pos2, Vec3 const& pos3, Vec3 const& pos4, AABB2 const& uvs, bool isWater, bool isTop)
{
	static float maxLightValue = 15.0f;
	static unsigned char unusedChannel = 0;
	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;

	BlockDefinition const* neighborBlockDef = (neighborBlock) ? BlockDefinition::GetDefById(neighborBlock->m_typeIndex) : nullptr;
	if (!neighborBlockDef) return;

	float normalizedOutdoorInfluence = (float)neighborBlock->GetOutdoorLightInfluence() / maxLightValue;
	unsigned char redChannel = DenormalizeByte(normalizedOutdoorInfluence);

	float normalizedIndoorInfluence = (float)neighborBlock->GetIndoorLightInfluence() / maxLightValue;
	unsigned char greenChannel = DenormalizeByte(normalizedIndoorInfluence);

	Rgba8 blockColor(redChannel, greenChannel, unusedChannel);


	bool neighborIsWater = neighborBlockDef->m_id == waterId;

	if (!isWater) {
		if(neighborBlockDef->m_isOpaque) return;
	}
	else {
	

		blockColor.a = DenormalizeByte(0.5f);
		if (isTop) {
			blockColor.b = 255;
		}
		else {
			if (neighborIsWater) return;
		}
	}


	std::vector<Vertex_PCU>& usedVerts = (isWater) ? m_blockVertexesWater : m_blockVertexes;
	std::vector<unsigned int>& usedIndexes = (isWater) ? m_blockIndexesWater : m_blockIndexes;

	AddVertsForIndexedQuad3D(usedVerts, usedIndexes, pos1, pos2, pos3, pos4, blockColor, uvs); // Bottom;

}

void Chunk::RenderDebug() const
{
	std::vector<Vertex_PCU> debugVerts;
	AddVertsForWireAABB3D(debugVerts, m_bounds);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(debugVerts);

}

std::string Chunk::GetChunkFileName() const
{
	return Stringf("Saves/%d/Chunk(%d, %d).chunk", m_worldSeed, m_globalCoordinates.x, m_globalCoordinates.y);
}

void Chunk::SaveChunkToDisk()
{
	double startTime = GetCurrentTimeSeconds();

	std::string fileName = GetChunkFileName();
	std::vector<uint8_t> compressedChunkInfo;
	LRECompress(compressedChunkInfo);
	FileWriteFromBuffer(compressedChunkInfo, fileName);

	double endTime = GetCurrentTimeSeconds();
	double totalTime = endTime - startTime;

	if (totalTime > m_game->m_worstChunkSaveIncludeRLETime) {
		m_game->m_worstChunkSaveIncludeRLETime = totalTime;
	}

	m_game->m_totalChunkSaveIncludeRLETime += totalTime;
	m_game->m_countChunkSaveIncludeRLETime++;
	m_game->RefreshLoadStats();
}

void Chunk::LRECompress(std::vector<uint8_t>& dataBytes) const
{
	dataBytes.push_back('G');
	dataBytes.push_back('C');
	dataBytes.push_back('H');
	dataBytes.push_back('K');
	dataBytes.push_back((uint8_t)g_gameConfigBlackboard.GetValue("LSR_VERSION", 1));
	dataBytes.push_back((uint8_t)CHUNK_BITS_X);
	dataBytes.push_back((uint8_t)CHUNK_BITS_Y);
	dataBytes.push_back((uint8_t)CHUNK_BITS_Z);


	uint8_t currentCountingBlockType = m_blocks[0].m_typeIndex;
	uint8_t currentBlockCounter = 0;
	bool finishedExactly = false;
	for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_SIZE; blockIndex++, currentBlockCounter++) {
		uint8_t currentBlockType = m_blocks[blockIndex].m_typeIndex;
		if ((currentBlockCounter == 255) || (currentBlockType != currentCountingBlockType)) {
			dataBytes.push_back(currentCountingBlockType);
			dataBytes.push_back(currentBlockCounter);
			currentBlockCounter = 0;
			currentCountingBlockType = currentBlockType;
			if (blockIndex == CHUNK_TOTAL_SIZE - 1) {
				finishedExactly = true;
			}
		}
	}

	if (!finishedExactly) {
		dataBytes.push_back(currentCountingBlockType);
		dataBytes.push_back(currentBlockCounter);
	}
}

bool Chunk::LoadFromFile()
{
	std::string fileName = GetChunkFileName();
	if (!FileExists(fileName)) return false;
	std::vector<uint8_t> fileBytes;

	int loadStatus = FileReadToBuffer(fileBytes, fileName);
	bool foundG = fileBytes[0] == 'G';
	bool foundC = fileBytes[1] == 'C';
	bool foundH = fileBytes[2] == 'H';
	bool foundK = fileBytes[3] == 'K';

	if (!(foundG && foundC && foundH && foundK)) {
		ERROR_RECOVERABLE("TRYING TO LOAD FILE FROM NON-COMPLIANT STANDARD");
		return false;
	}

	uint8_t expectedLSRVersion = (uint8_t)g_gameConfigBlackboard.GetValue("LSR_VERSION", 1);

	bool versionsMatch = fileBytes[4] == expectedLSRVersion;
	bool chunkBitsXMatch = fileBytes[5] == (unsigned char)CHUNK_BITS_X;
	bool chunkBitsYMatch = fileBytes[6] == (unsigned char)CHUNK_BITS_Y;
	bool chunkBitsZMatch = fileBytes[7] == (unsigned char)CHUNK_BITS_Z;


	if (!(versionsMatch && chunkBitsXMatch && chunkBitsYMatch && chunkBitsZMatch)) return false;

	int blockIndex = 0;
	if (loadStatus == 0) {
		m_blocks = new Block[CHUNK_TOTAL_SIZE];

		for (int byteIndex = 8; byteIndex < fileBytes.size(); byteIndex += 2) {
			uint8_t blockType = fileBytes[byteIndex];
			int nextIndex = byteIndex + 1;
			uint8_t blockAmount = fileBytes[nextIndex];
			for (int blockCount = 0; blockCount < blockAmount; blockCount++, blockIndex++) {
				m_blocks[blockIndex].m_typeIndex = blockType;
			}
		}

		if (blockIndex != CHUNK_TOTAL_SIZE) {
			ERROR_AND_DIE("CHUNK LOADED SIZE IS NOT THE EXPECTED ONE");
		}
		return true;
	}

	return false;
}

void ChunkGenerationJob::Execute()
{
	m_chunk->m_state = ChunkState::GENERATING;
	m_chunk->GenerateChunk();
}

void ChunkGenerationJob::OnFinished()
{
	m_chunk->m_state = ChunkState::ACTIVE;

	World* world = m_chunk->GetGame()->GetWorld();

	world->FlagSkyBlocks(m_chunk);
	world->MarkLightingDirtyOnChunkBorders(m_chunk);

}

void ChunkDiskLoadJob::Execute()
{
	m_chunk->m_state = ChunkState::LOADING_FROM_DISK;
	m_loadingSuccessful = m_chunk->LoadFromFile();
}

void ChunkDiskLoadJob::OnFinished()
{
	if (!m_loadingSuccessful) {
		ChunkGenerationJob* generationJob = new ChunkGenerationJob(m_chunk);
		g_theJobSystem->QueueJob(generationJob);
	}
	else {
		m_chunk->m_state = ChunkState::ACTIVE;
		World* world = m_chunk->GetGame()->GetWorld();

		world->FlagSkyBlocks(m_chunk);
		world->MarkLightingDirtyOnChunkBorders(m_chunk);

	}
}

void ChunkDiskSaveJob::Execute()
{
	m_chunk->SaveChunkToDisk();
}

void ChunkDiskSaveJob::OnFinished()
{
}
