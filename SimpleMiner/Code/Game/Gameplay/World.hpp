#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Gameplay/BlockIterator.hpp"
#include <map>
#include <deque>

class Game;
class Chunk;
class Entity;

struct IntVec3;
struct RaycastResult3D;

struct SimpleMinerRaycast : RaycastResult3D {
	SimpleMinerRaycast() = default;
	BlockIterator m_blockIter;
};

class World {
public:
	World(Game* gamePointer);
	~World();

	void Update(float deltaSeconds);
	void Render() const;
	Chunk* GetChunk(IntVec2 const& coords) const;
	void RemoveBlockBelow(Vec3 const& referencePos);
	void DigBlockAtRaycastHit();
	void StopDigging();
	void RemoveBlockAtRaycastHit();
	void PlaceBlockBelow(Vec3 const& referencePos, unsigned char blockType);
	void PlaceBlockAtRaycastHit(unsigned char blockType);
	SimpleMinerRaycast RaycastVsTiles(Vec3 const& rayStart, Vec3 const& forwardNormal, float maxLength);
	void SetCameraPositionConstant(Vec3 const& cameraPos) const;
	Rgba8 const GetSkyColor() const;

	void PushEntityOutOfWorld(Entity& entity) const;
	void CarveBlocksInRadius(Vec3 const& refPoint, float radius);
	void MarkLightingDirty(BlockIterator& blockIter);
	void FlagSkyBlocks(Chunk* chunk);
	void MarkLightingDirtyOnChunkBorders(Chunk* chunk);
	void ToggleFog() const;
public:
	std::map<IntVec2, Chunk*> m_activeChunks;

	std::map<IntVec2, Chunk*> m_initializedChunks;
	std::mutex m_initiliazedChunksMutex;

	Game* m_game = nullptr;
	int m_vertexAmount = 0;
	int m_indexAmount = 0;
	bool m_freezeRaycast = false;
	
	SimpleMinerRaycast m_storedRaycast;
	SimpleMinerRaycast m_latestRaycast;

	bool m_stepDebugLighting = true;
	float m_worldTimeScale = 1.0f / g_gameConfigBlackboard.GetValue("WORLD_TIME_SCALE", 200.0f);


private:
	void UpdateDayAndNightCycle(float deltaSeconds);
	void UpdateLightningValues();
	void UpdateIndoorLightFlicker();
	void UpdateRaycast();

	bool CheckChunksForActivation();
	bool CheckChunksForDeactivation();
	void CheckChunksForMeshRegen();

	void CheckForCompletedJobs();

	void InitiliazeChunk(IntVec2 const& coords);
	void QueueForSaving(Chunk* chunk);
	void DeactivateChunk(Chunk* chunk);
	void ActivateChunk(Chunk* chunk);

	void LinkChunkNeighbors(Chunk* newChunk) const;
	void UnlinkChunkFromNeighbors(Chunk* chunkToUnlink) const;

	void ProcessDirtyLighting();
	void ProcessNextDirtyLightBlock(BlockIterator& blockIter);

	bool CorrectOutdoorLighting(BlockIterator& blockIter);
	bool CorrectIndoorLighting(BlockIterator& blockIter);

	void MarkLightEmittingBlocksAsDirty(Chunk* chunk);
	void MarkLightingForDirtyNeighborBlocks(BlockIterator& blockIter);
	int GetHighestNeighborLightValue(BlockIterator& blockIter, bool isIndoorLighting);
	void UndirtyAllBlocksInChunk(Chunk* chunk);

	bool IsPointInSolid(Vec3 const& refPoint) const;

	void AddNeighborBlocksToPhysicsCheck(std::vector<BlockIterator>& cardinalBlocksToCheck, std::vector<BlockIterator>& diagonalBlocksToCheck, Entity const& entity, BlockIterator& blockIter) const;
	void CalulateRenderingOrder();

private:
	float m_activationRange = g_gameConfigBlackboard.GetValue("ACTIVATION_RANGE", 250.0f);
	int m_numActiveChunks = 0;

	std::deque<BlockIterator> m_dirtyLightBlocks;

	std::mutex m_lightQueueMutex;
	std::deque<BlockIterator> m_queueForMarkingAsDirty;
	bool m_isLightingStepEnabled = g_gameConfigBlackboard.GetValue("DEBUG_ENABLE_LIGHTSTEP", false);
	bool m_canUseDebugLightingDraw = false;

	ConstantBuffer* m_gameCBO = nullptr;
	bool m_disableWorldShader = g_gameConfigBlackboard.GetValue("DEBUG_DISABLE_WORLD_SHADER", false);
	Material* m_worldShader = nullptr;

	float m_worldDay = 0.5f;

	Clock m_clock;

	bool m_isDiggingBlock = false;
	float m_elapsedDiggingBlock = 0.0f;
	BlockIterator m_blockBeingDug;

	float m_timeLimitForDigging = 1.0f;

	SpriteAnimDefinition* m_diggingAnimation = nullptr;
	SpriteSheet* m_simpleMinerSpritesheet = nullptr;

	std::vector<IntVec2> m_orderedRenderingOffsets;
};

bool operator<(IntVec2 const& coords, IntVec2 const& compareTo);