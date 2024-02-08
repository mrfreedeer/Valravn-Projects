#pragma once
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/Block.hpp"
#include "Game/Framework/GameCommon.hpp"
#include <vector>

struct IntVec3;
struct IntVec2;
struct Vec3;
struct Vec2;
struct AABB3;
struct Vertex_PCU;

class VertexBuffer;
class IndexBuffer;
class BlockDefinition;
class BlockIterator;
class BlockTemplate;


constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 7;

constexpr int CHUNK_SIZE_X = (1 << CHUNK_BITS_X);
constexpr int CHUNK_SIZE_Y = (1 << CHUNK_BITS_Y);
constexpr int CHUNK_SIZE_Z = (1 << CHUNK_BITS_Z);

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;

constexpr int CHUNKSHIFT_Y = CHUNK_BITS_X;
constexpr int CHUNKSHIFT_Z = CHUNK_BITS_X + CHUNK_BITS_Y;
constexpr int CHUNK_TOTAL_SIZE = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

constexpr int CHUNK_MASK_X = (CHUNK_SIZE_X - 1);
constexpr int CHUNK_MASK_Y = ((CHUNK_SIZE_Y - 1) << CHUNKSHIFT_Y);
constexpr int CHUNK_MASK_Z = ((CHUNK_SIZE_Z - 1) << CHUNKSHIFT_Z);

constexpr int CHUNK_BLOCKS_PER_LAYER = CHUNK_SIZE_X * CHUNK_SIZE_Y;

class Game;

enum class ChunkState {
	INITIALIZING,
	GENERATING,
	LOADING_FROM_DISK,
	ACTIVE
};

constexpr int DISK_JOB_TYPE = 1;
constexpr int CHUNK_GENERATION_JOB_TYPE = 1 << 1;

class Chunk {
public:
	Chunk(Game* pointerToGame, IntVec2 const& globalCoords);
	~Chunk();
	void GenerateChunk();

	void GenerateCPUMesh();
	void Update(float deltaSeconds);
	void Render() const;
	void RenderDebug() const;
	void RenderWater() const;

	IntVec2 const GetChunkCoords() const { return m_globalCoordinates; }
	void RemoveBlockBelow(Vec3 const& referencePos);
	void RemoveBlockAt(BlockIterator const& blockIterator, bool needSaving = true);
	void PlaceBlockBelow(Vec3 const& referencePos, unsigned char blockType);
	void PlaceBlockAt(BlockIterator const& blockIterator, unsigned char blockType);

	void PlaceTemplateAboveCoords(IntVec3 localCoords, BlockTemplate const& blockTemplate);

	Vec3 const GetChunkCenter() const;
	bool CanBeLoadedFromFile() const;
	bool LoadFromFile();
	void SaveChunkToDisk();
	void CarveBlockInRadius(Vec3 const& originPos, IntVec3 const& localCoords, float radius, bool replaceWithDirt = true, bool placeLampInMiddle = true);
	void CarveCanyonInRadius(Vec3 const& originPos, IntVec3 const& localCoords, float radius, int depth, bool replaceWithDirt = true, bool placeLampInMiddle = true);
	Game* GetGame() { return m_game; }

	static IntVec3 GetLocalCoordsForWorldPos(Vec3 const& position);
	static int GetIndexForLocalCoords(IntVec3 const& localCoords);
	static IntVec3 GetLocalCoordsForIndex(int index);
	static IntVec2 GetChunkCoordsForWorldPos(Vec3 const& position);
	IntVec3 GetGlobalCoordsForIndex(int blockIndex) const;
	IntVec3 GetLocalCoordsForGlobalCoords(IntVec3 const& globalCoords) const;
	IntVec3 GetGlobalCoordsForLocalCoords(IntVec3 const& localCoords) const;
	static Vec2 const GetChunkCenter(IntVec2 const& coords);

	Block* m_blocks = nullptr;

	std::vector<Vertex_PCU> m_blockVertexes;
	std::vector<unsigned int> m_blockIndexes;

	std::vector<Vertex_PCU> m_blockVertexesWater;
	std::vector<unsigned int> m_blockIndexesWater;

	Chunk* m_northChunk = nullptr;
	Chunk* m_eastChunk = nullptr;
	Chunk* m_southChunk = nullptr;
	Chunk* m_westChunk = nullptr;

	bool HasExistingNeighors() const;

	bool m_isDirty = true;
	std::atomic<bool> m_needsSaving = false;

	std::atomic<ChunkState> m_state = ChunkState::INITIALIZING;

private:
	int GetTerrainHeightAtCoords(IntVec2 const& coords) const;
	void AddVertsForBlock(Block const& block, int x, int y, int z);
	void AddVertsForHRSBlockQuad(Block* neighborBlock, Vec3 const& pos1, Vec3 const& pos2, Vec3 const& pos3, Vec3 const& pos4, AABB2 const& uvs, bool isWater, bool isTop = false);
	std::string GetChunkFileName() const;

	void LRECompress(std::vector<uint8_t>& dataBytes) const;

	bool AreLocalCoordsWithinChunk(IntVec3 const& localCoords) const;
	bool AreGlobalCoordsWithinChunk(IntVec3 const& globalCoords) const;

	void GenerateTrees();
	void GetTreeNoiseForChunk(std::map<IntVec2, float>& perlinNoiseHolder);
	void PlaceTrees(std::map<IntVec2, float> const& perlinNoiseHolder);

	void GenerateCanyons();
	void GetCanyonsStartPerlinNoise(std::map<IntVec2, float>& perlinNoiseHolder) const;
	void GetCanyonPath(IntVec2 const& coords, std::vector<IntVec3>& canyonPoints) const;
	void CarveCanyonPath(std::vector<IntVec3>& canyonPoints);


	void GenerateCaves();
	void GetCavesStartPerlinNoise(std::map<IntVec2, float>& perlinNoiseHolder) const;
	void GetCavePath(IntVec2 const& coords, std::vector<IntVec3>& cavePoints) const;
	void CarveCavePath(std::vector<IntVec3>& cavePoints);

	bool AreCoordsConsideredLocalMaxima(IntVec2 const& coords, int radius, std::map<IntVec2, float> const& perlinNoiseHolder) const;

private:
	IntVec2 m_globalCoordinates = IntVec2::ZERO;
	AABB3 m_bounds = AABB3::ZERO_TO_ONE;

	VertexBuffer* m_chunkVBO = nullptr;
	IndexBuffer* m_chunkIBO = nullptr;

	VertexBuffer* m_chunkWaterVBO = nullptr;
	IndexBuffer* m_chunkWaterIBO = nullptr;

	Game* m_game = nullptr;
	unsigned int m_worldSeed = g_gameConfigBlackboard.GetValue("WORLD_SEED", (int)GetCurrentTimeSeconds());
	float m_baseHumidity = g_gameConfigBlackboard.GetValue("BASE_HUMIDITY_LEVEL", 0.5f);
	float m_beachHumidityLevel = g_gameConfigBlackboard.GetValue("BEACH_HUMIDITY_LEVEL", 0.7f);
	float m_waterFreezingLimit = g_gameConfigBlackboard.GetValue("WATER_FREEZING_LIMIT", 0.25f);
	int m_oceanDepth = g_gameConfigBlackboard.GetValue("MAX_OCEAN_DEPTH", 20);
	int m_treeSpacing = g_gameConfigBlackboard.GetValue("TREE_TILE_RADIUS", 5);
	int m_treeSideWidth = g_gameConfigBlackboard.GetValue("TREE_MAX_SIDE_WIDTH", 2);

	int m_terrainHeight[CHUNK_BLOCKS_PER_LAYER] = {};
	float m_humidity[CHUNK_BLOCKS_PER_LAYER] = {};
	float m_temperature[CHUNK_BLOCKS_PER_LAYER] = {};

	int m_canyonCheckRadius = g_gameConfigBlackboard.GetValue("CANYON_CHUNK_RADIUS", 20);
	int m_canyonBlockSteps = g_gameConfigBlackboard.GetValue("CANYON_BLOCK_STEPS_AMOUNT", 4);
	int m_canyonNodeAmount = g_gameConfigBlackboard.GetValue("CANYON_NODE_AMOUNT", 30);
	float m_canyonTurnRate = g_gameConfigBlackboard.GetValue("CANYON_TURN_RATE", 35.0f);
	float m_canyonMaxRadius = g_gameConfigBlackboard.GetValue("CANYON_MAX_RADIUS", 10.0f);
	int m_canyonDepth = g_gameConfigBlackboard.GetValue("CANYON_DEPTH", 10);

	int m_caveCheckRadius = g_gameConfigBlackboard.GetValue("CAVE_CHUNK_RADIUS", 40);
	int m_caveBlockSteps = g_gameConfigBlackboard.GetValue("CAVE_BLOCK_STEPS_AMOUNT", 8);
	int m_caveNodeAmount = g_gameConfigBlackboard.GetValue("CAVE_NODE_AMOUNT", 70);
	int m_caveDepthStart = g_gameConfigBlackboard.GetValue("CAVE_DEPTH_START", 20);
	float m_caveTurnRate = g_gameConfigBlackboard.GetValue("CAVE_TURN_RATE", 35.0f);
	float m_caveMaxRadius = g_gameConfigBlackboard.GetValue("CAVE_MAX_RADIUS", 10.0f);
};

class ChunkGenerationJob : public Job {
public:
	ChunkGenerationJob(Chunk* chunk) :
		m_chunk(chunk),
		Job::Job(CHUNK_GENERATION_JOB_TYPE)
	{}

	virtual void Execute() override;
	virtual void OnFinished() override;
	Chunk* m_chunk = nullptr;

};

class ChunkDiskLoadJob : public Job {
public:
	ChunkDiskLoadJob(Chunk* chunk) :
		m_chunk(chunk),
		Job::Job(DISK_JOB_TYPE) {}

	virtual void Execute() override;
	virtual void OnFinished() override;

	Chunk* m_chunk = nullptr;
	bool m_loadingSuccessful = false;
};

class ChunkDiskSaveJob : public Job {
public:
	ChunkDiskSaveJob(Chunk* chunk) :
		m_chunk(chunk),
		Job::Job(DISK_JOB_TYPE) {}

	virtual void Execute() override;
	virtual void OnFinished() override;

	Chunk* m_chunk = nullptr;
};