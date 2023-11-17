#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Game/Gameplay/World.hpp"
#include "Game/Gameplay/Chunk.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/BlockDefinition.hpp"
#include "Game/Gameplay/GameCamera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include <algorithm>

static IntVec2 const NorthStep = IntVec2(0, 1);
static IntVec2 const SouthStep = IntVec2(0, -1);
static IntVec2 const EastStep = IntVec2(1, 0);
static IntVec2 const WestStep = IntVec2(-1, 0);

constexpr float secondFractionFromDay = 1.0f / (60.0f * 60.0f * 24.0f);

struct GameConstants
{
	Vec4 CameraWorldPosition;
	float GlobalIndoorLight[4];
	float GlobalOutdoorLight[4];
	float SkyColor[4];
	float FogStartDistance;
	float FogEndDistance;
	float FogEndAlpha = 1.0f;
	float Time;
};

GameConstants g_gameConstants = {};

World::World(Game* gamePointer) :
	m_game(gamePointer)
{
	BufferDesc newCBODesc = {};
	newCBODesc.data = nullptr;
	newCBODesc.descriptorHeap = nullptr;
	newCBODesc.memoryUsage = MemoryUsage::Dynamic;
	newCBODesc.owner = g_theRenderer;
	newCBODesc.size = sizeof(GameConstants);
	newCBODesc.stride = sizeof(GameConstants);
	m_gameCBO = new ConstantBuffer(newCBODesc);

	Rgba8 defaultIndoorLightColor = g_gameConfigBlackboard.GetValue("DEFAULT_INDOOR_LIGHT_COLOR", Rgba8::WHITE);
	Rgba8 defaultOutdoorLightColor = g_gameConfigBlackboard.GetValue("DEFAULT_OUTOOR_LIGHT_COLOR", Rgba8::WHITE);
	Rgba8 skyColor = Rgba8::WHITE;
	float fogEndDistance = (m_activationRange - 16.0f);
	float fogStartDistance = fogEndDistance * 0.25f;

	defaultIndoorLightColor.GetAsFloats(g_gameConstants.GlobalIndoorLight);
	defaultOutdoorLightColor.GetAsFloats(g_gameConstants.GlobalOutdoorLight);
	skyColor.GetAsFloats(g_gameConstants.SkyColor);

	g_gameConstants.FogStartDistance = fogStartDistance;

	if (g_gameConfigBlackboard.GetValue("DEBUG_DISABLE_FOG", false)) {
		g_gameConstants.FogEndDistance = FLT_MAX;
	}
	else {
		g_gameConstants.FogEndDistance = fogEndDistance;
	}

	m_worldShader = g_theRenderer->CreateOrGetMaterial("Data/Materials/World");

	g_theJobSystem->ClearCompletedJobs();

	g_theJobSystem->SetThreadJobType(0, DISK_JOB_TYPE);
	for (int jobThreadId = 1; jobThreadId < g_theJobSystem->GetNumThreads(); jobThreadId++) {
		g_theJobSystem->SetThreadJobType(jobThreadId, CHUNK_GENERATION_JOB_TYPE);
	}

	m_simpleMinerSpritesheet = new SpriteSheet(*g_textures[(int)GAME_TEXTURE::SimpleMinerSprites], IntVec2(64, 64));
	int animationIndStart = 32 + (46 * 64);
	int animationIndEnd = animationIndStart + 5;

	m_diggingAnimation = new SpriteAnimDefinition(*m_simpleMinerSpritesheet, animationIndStart, animationIndEnd, 1.0f);

	CalulateRenderingOrder();
}

World::~World()
{
	g_theJobSystem->ClearQueuedJobs();
	g_theJobSystem->WaitUntilCurrentJobsCompletion();
	g_theJobSystem->ClearCompletedJobs();

	for (std::map<IntVec2, Chunk*>::iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) { // Unfortunately, now I have to queue jobs to be save, which requires forlooping twice
		Chunk* chunk = chunkIt->second;
		if (chunk->m_needsSaving) {
			QueueForSaving(chunk);
		}
	}

	g_theJobSystem->WaitUntilQueuedJobsCompletion();
	g_theJobSystem->ClearCompletedJobs();

	for (std::map<IntVec2, Chunk*>::iterator chunkIt = m_initializedChunks.begin(); chunkIt != m_initializedChunks.end(); chunkIt++) {
		Chunk*& chunk = chunkIt->second;
		if (chunk) {
			delete chunk;
			chunk = nullptr;
		}
	}

	for (std::map<IntVec2, Chunk*>::iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) {
		Chunk*& chunk = chunkIt->second;
		if (chunk) {
			delete chunk;
			chunk = nullptr;
		}
	}

	delete m_gameCBO;
	m_gameCBO = nullptr;

	for (int jobThreadId = 0; jobThreadId < g_theJobSystem->GetNumThreads(); jobThreadId++) {
		g_theJobSystem->SetThreadJobType(jobThreadId, DEFAULT_JOB_ID);
	}

	delete m_diggingAnimation;
	m_diggingAnimation = nullptr;

	delete m_simpleMinerSpritesheet;
	m_simpleMinerSpritesheet = nullptr;
}

void World::Update(float deltaSeconds)
{
	UpdateDayAndNightCycle(deltaSeconds);

	g_gameConstants.Time = static_cast<float>(GetCurrentTimeSeconds());

	m_gameCBO->CopyCPUToGPU(&g_gameConstants, sizeof(GameConstants));
	//g_theRenderer->CopyCPUToGPU(&g_gameConstants, sizeof(GameConstants), m_gameCBO);
	g_theRenderer->BindConstantBuffer(m_gameCBO, 2);

	m_vertexAmount = 0;
	m_indexAmount = 0;

	bool activatedAnyChunk = CheckChunksForActivation();
	if (!activatedAnyChunk) {
		CheckChunksForDeactivation();
	}

	CheckForCompletedJobs();

	CheckChunksForMeshRegen();

	for (std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) {
		Chunk* chunk = chunkIt->second;
		chunk->Update(deltaSeconds);
		m_vertexAmount += (int)chunk->m_blockVertexes.size();
		m_indexAmount += (int)chunk->m_blockIndexes.size();
	}

	UpdateRaycast();

	ProcessDirtyLighting();

	if (m_isDiggingBlock) {
		m_elapsedDiggingBlock += deltaSeconds;
	}

	//IntVec2 coords = Chunk::GetChunkCoordsForWorldPos(m_game->m_player->m_position);
	//Chunk* chunk = GetChunk(coords);

	////IntVec3 localCoords = Chunk::GetLocalCoordsForWorldPos(m_game->m_player->m_position);
	////int ind = localCoords.x + (localCoords.y * CHUNK_SIZE_X);
	////if (chunk) {

	////	//DebugAddMessage(Stringf("Hilliness: %f\t TerrainHeight: %f", chunk->m_hilliness[ind], chunk->m_terrainNoise[ind]), 0.0f, Rgba8::LIGHTBLUE, Rgba8::LIGHTBLUE);
	////}
}

void World::Render() const
{
	if (m_disableWorldShader) {
		g_theRenderer->BindMaterial(nullptr);
	}
	else {
		g_theRenderer->BindMaterial(m_worldShader);
	}

	g_theRenderer->SetModelMatrix(Mat44());
	//g_theRenderer->CopyAndBindModelConstants();
	g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::SimpleMinerSprites]);

	for (std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) {
		Chunk* chunk = chunkIt->second;
		chunk->Render();
	}

	


	IntVec2 const& playerChunkCoords = Chunk::GetChunkCoordsForWorldPos(m_game->m_player->m_position);


	g_theRenderer->SetBlendMode(BlendMode::ALPHA);

	for (int renderIndex = 0; renderIndex < m_orderedRenderingOffsets.size(); renderIndex++) {
		IntVec2 const& offset = m_orderedRenderingOffsets[renderIndex];
		IntVec2 resultingCoords = offset + playerChunkCoords;

		std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.find(resultingCoords);

		if (chunkIt != m_activeChunks.end()) {
			Chunk* chunk = chunkIt->second;

			if (chunk) {
				chunk->RenderWater();
			}
		}

	}

	if (g_drawDebug) {
		g_theRenderer->BindTexture(nullptr);

		for (std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) {
			Chunk* chunk = chunkIt->second;
			chunk->RenderDebug();
		}
	}

	if (m_isDiggingBlock) {
		float thickness = 0.001f;

		AABB3 blockBounds = m_blockBeingDug.GetBlockBounds();
		blockBounds.m_mins -= Vec3(thickness, thickness, thickness);
		blockBounds.m_maxs += Vec3(thickness, thickness, thickness);

		std::vector<Vertex_PCU> digVerts;
		digVerts.reserve(36);
		SpriteDefinition const& spriteDef = m_diggingAnimation->GetSpriteDefAtTime(m_elapsedDiggingBlock);

		AddVertsForAABB3D(digVerts, blockBounds, Rgba8(255, 255, 0), spriteDef.GetUVs()); // No blue or it will behave like water

		g_theRenderer->DrawVertexArray(digVerts);

	}


	std::vector<Vertex_PCU> raycastVerts;
	if (m_freezeRaycast) {
		AddVertsForLineSegment3D(raycastVerts, m_storedRaycast.m_startPosition, m_storedRaycast.m_impactPos, Rgba8::YELLOW);
	}
	else {
		AddVertsForLineSegment3D(raycastVerts, m_latestRaycast.m_startPosition, m_latestRaycast.m_impactPos, Rgba8::YELLOW);
	}


	g_theRenderer->BindMaterial(nullptr);
	g_theRenderer->BindTexture(nullptr);


	g_theRenderer->DrawVertexArray(raycastVerts);
}

Chunk* World::GetChunk(IntVec2 const& coords) const
{
	std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.find(coords);
	if (chunkIt != m_activeChunks.end()) {
		return chunkIt->second;
	}

	return nullptr;
}

void World::RemoveBlockBelow(Vec3 const& referencePos)
{
	IntVec2 chunkCoords = Chunk::GetChunkCoordsForWorldPos(referencePos);
	Chunk* chunk = GetChunk(chunkCoords);
	if (chunk) {
		chunk->RemoveBlockBelow(referencePos);
	}

}

void World::DigBlockAtRaycastHit()
{
	SimpleMinerRaycast& usedRaycast = (m_freezeRaycast) ? m_storedRaycast : m_latestRaycast;

	if (!usedRaycast.m_didImpact) return;

	BlockIterator blockIter = usedRaycast.m_blockIter;

	if (blockIter.GetBlock()) {
		if (m_blockBeingDug.GetBlock() != blockIter.GetBlock()) {
			m_elapsedDiggingBlock = 0.0f;
			m_blockBeingDug = blockIter;
		}
	}

	m_isDiggingBlock = true;

	if (m_elapsedDiggingBlock >= m_timeLimitForDigging) {
		m_elapsedDiggingBlock = 0.0f;
		m_isDiggingBlock = false;
		RemoveBlockAtRaycastHit();
	}

}

void World::StopDigging()
{
	m_isDiggingBlock = false;
	m_elapsedDiggingBlock = 0.0f;
	m_blockBeingDug = BlockIterator();
}

void World::RemoveBlockAtRaycastHit()
{

	SimpleMinerRaycast& usedRaycast = (m_freezeRaycast) ? m_storedRaycast : m_latestRaycast;

	if (!usedRaycast.m_didImpact) return;

	BlockIterator blockIter = usedRaycast.m_blockIter;
	Chunk* chunk = blockIter.GetChunk();

	chunk->RemoveBlockAt(blockIter);
	MarkLightingDirty(blockIter);
	Block* topNeighbor = blockIter.GetTopNeighbor().GetBlock();

	if (topNeighbor && topNeighbor->IsSky()) {
		Block* currentBlock = blockIter.GetBlock();
		BlockDefinition const* blockDef = BlockDefinition::GetDefById(currentBlock->m_typeIndex);
		while (currentBlock && !blockDef->m_isOpaque) {
			currentBlock->SetIsSky(true);
			MarkLightingDirty(blockIter);

			blockIter = blockIter.GetBottomNeighbor();
			currentBlock = blockIter.GetBlock();
			blockDef = BlockDefinition::GetDefById(currentBlock->m_typeIndex);
		}
	}
	m_canUseDebugLightingDraw = true;
}

void World::PlaceBlockBelow(Vec3 const& referencePos, unsigned char blockType)
{
	IntVec2 chunkCoords = Chunk::GetChunkCoordsForWorldPos(referencePos);
	Chunk* chunk = GetChunk(chunkCoords);
	if (chunk) {
		chunk->PlaceBlockBelow(referencePos, blockType);
	}
}

void World::PlaceBlockAtRaycastHit(unsigned char blockType)
{
	SimpleMinerRaycast& usedRaycast = (m_freezeRaycast) ? m_storedRaycast : m_latestRaycast;

	if (!usedRaycast.m_didImpact) return;

	BlockIterator blockIter = usedRaycast.m_blockIter;

	Vec3 const& normal = usedRaycast.m_impactNormal;

	if (normal.x != 0) {
		if (normal.x > 0) {
			blockIter = blockIter.GetEastNeighbor();
		}
		else {
			blockIter = blockIter.GetWestNeighbor();
		}
	}

	if (normal.y != 0) {
		if (normal.y > 0) {
			blockIter = blockIter.GetNorthNeighbor();
		}
		else {
			blockIter = blockIter.GetSouthNeighbor();
		}

	}

	if (normal.z != 0) {
		if (normal.z > 0) {
			blockIter = blockIter.GetTopNeighbor();
		}
		else {
			blockIter = blockIter.GetBottomNeighbor();
		}
	}

	Chunk* chunk = blockIter.GetChunk();
	chunk->PlaceBlockAt(blockIter, blockType);

	Block* currentBlock = blockIter.GetBlock();
	MarkLightingDirty(blockIter);

	if (currentBlock && currentBlock->IsSky()) {
		currentBlock->SetIsSky(false);

		blockIter = blockIter.GetBottomNeighbor();
		currentBlock = blockIter.GetBlock();
		BlockDefinition const* blockDef = BlockDefinition::GetDefById(currentBlock->m_typeIndex);
		while (currentBlock && !blockDef->m_isOpaque) {
			currentBlock->SetIsSky(false);
			MarkLightingDirty(blockIter);

			blockIter = blockIter.GetBottomNeighbor();
			currentBlock = blockIter.GetBlock();
			blockDef = BlockDefinition::GetDefById(currentBlock->m_typeIndex);
		}
	}

	m_canUseDebugLightingDraw = true;
}

void World::UpdateDayAndNightCycle(float deltaSeconds)
{
	m_worldDay += (deltaSeconds * m_worldTimeScale) * secondFractionFromDay;
	double discardVariable;
	float dayFraction = (float)modf(m_worldDay, &discardVariable);

	float dayTValue = 0.0f;
	if (dayFraction <= 0.5f) {
		dayTValue = RangeMapClamped(dayFraction, 0.0f, 0.5f, 0.0f, 1.0f);
	}
	else {
		dayTValue = RangeMapClamped(dayFraction, 0.5f, 1.0f, 1.0f, 0.0f);
	}
	Rgba8 newSkyColor = Rgba8::InterpolateColors(Rgba8(20, 20, 40), Rgba8(200, 230, 255), dayTValue);
	newSkyColor.GetAsFloats(g_gameConstants.SkyColor);

	Rgba8 newOutdoorLight = Rgba8::InterpolateColors(Rgba8(14, 14, 28), Rgba8::WHITE, dayTValue);
	newOutdoorLight.GetAsFloats(g_gameConstants.GlobalOutdoorLight);

	UpdateLightningValues();
	UpdateIndoorLightFlicker();
}

void World::UpdateLightningValues()
{
	float lightningNoise = Compute1dPerlinNoise((float)m_clock.GetTotalTime(), 1.0f, 9);
	float lightningStrength = RangeMapClamped(lightningNoise, 0.6f, 0.9f, 0.0f, 1.0f);

	Rgba8 currentSkyColor = Rgba8(g_gameConstants.SkyColor);
	Rgba8 currentOutdoorLight = Rgba8(g_gameConstants.GlobalOutdoorLight);


	Rgba8 newSkyColor = Rgba8::InterpolateColors(currentSkyColor, Rgba8::WHITE, lightningStrength);
	newSkyColor.GetAsFloats(g_gameConstants.SkyColor);

	Rgba8 newOutdoorLight = Rgba8::InterpolateColors(currentOutdoorLight, Rgba8::WHITE, lightningStrength);
	newOutdoorLight.GetAsFloats(g_gameConstants.GlobalOutdoorLight);

}

void World::UpdateIndoorLightFlicker()
{
	float glowNoise = Compute1dPerlinNoise((float)m_clock.GetTotalTime(), 1.0f, 5);
	float glowStrength = RangeMapClamped(glowNoise, -1.0f, 1.0f, 0.8f, 1.0f);

	Rgba8 indoorLightColor = g_gameConfigBlackboard.GetValue("DEFAULT_INDOOR_LIGHT_COLOR", Rgba8::CYAN);
	indoorLightColor *= glowStrength;

	indoorLightColor.GetAsFloats(g_gameConstants.GlobalIndoorLight);

}

void World::UpdateRaycast()
{

	static float maxLength = g_gameConfigBlackboard.GetValue("MAX_RAYCAST_LENGTH", 10.0f);

	Vec3 cameraPos;
	Vec3 cameraFwd;

	if (m_freezeRaycast) {
		cameraPos = m_storedRaycast.m_startPosition;
		cameraFwd = m_storedRaycast.m_forwardNormal;
		if (m_storedRaycast.m_didImpact) {
			DebugAddWorldLine(m_storedRaycast.m_impactPos, cameraPos + (cameraFwd * maxLength), 0.048f, 0.0f, Rgba8::GRAY, Rgba8::GRAY, DebugRenderMode::USEDEPTH);
			DebugAddWorldLine(cameraPos, m_storedRaycast.m_impactPos, 0.05f, 0.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);

			DebugAddWorldPoint(m_storedRaycast.m_impactPos, 0.1f, 0.0f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::USEDEPTH);
			Vec3 endPos = m_storedRaycast.m_impactPos + (m_storedRaycast.m_impactNormal * 0.5f);
			DebugAddWorldArrow(m_storedRaycast.m_impactPos, endPos, 0.025f, 0.0f, Rgba8::RED, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);
		}
		else {
			DebugAddWorldLine(cameraPos, cameraPos + (cameraFwd * maxLength), 0.048f, 0.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);
		}
	}
	else {
		cameraPos = m_game->m_player->m_position;
		cameraFwd = m_game->m_player->m_orientation.GetXForward();


		m_latestRaycast = RaycastVsTiles(cameraPos, cameraFwd, maxLength);

		if (m_latestRaycast.m_didImpact) {
			DebugAddWorldPoint(m_latestRaycast.m_impactPos, 0.1f, 0.0f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::USEDEPTH);
			Vec3 endPos = m_latestRaycast.m_impactPos + (m_latestRaycast.m_impactNormal * 0.5f);
			DebugAddWorldArrow(m_latestRaycast.m_impactPos, endPos, 0.025f, 0.0f, Rgba8::RED, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);
		}

	}

}

bool World::CheckChunksForActivation()
{
	bool result = false;
	constexpr int amountOfCandidates = 1;

	IntVec2 playerCoods = Chunk::GetChunkCoordsForWorldPos(m_game->m_player->m_position);

	bool foundActivationCandidate = false;
	IntVec2 bestActivationCandidatesCoords[amountOfCandidates] = {};
	float bestActivationSqrDist[amountOfCandidates] = {};
	std::fill_n(bestActivationSqrDist, amountOfCandidates, FLT_MAX);

	static int maxChunkRadiusX = 1 + int(m_activationRange) / CHUNK_SIZE_X;
	static int maxChunkRadiusY = 1 + int(m_activationRange) / CHUNK_SIZE_Y;
	static int maxChunks = (2 * maxChunkRadiusX) * (2 * maxChunkRadiusY);

	float activationRangeSqr = m_activationRange * m_activationRange;

	if (m_numActiveChunks < maxChunks) {
		for (int chunkPosX = playerCoods.x - maxChunkRadiusX; chunkPosX < playerCoods.x + maxChunkRadiusX; chunkPosX++) {
			for (int chunkPosY = playerCoods.y - maxChunkRadiusY; chunkPosY < playerCoods.y + maxChunkRadiusY; chunkPosY++) {
				IntVec2 chunkCoords = IntVec2(chunkPosX, chunkPosY);
				Vec2 chunkCenter = Chunk::GetChunkCenter(chunkCoords);

				float distSqrToChunk = GetDistanceSquared2D(chunkCenter, Vec2(m_game->m_player->m_position));
				if (distSqrToChunk > (activationRangeSqr)) continue;

				bool savedCandidate = false;

				for (int candidateIndex = 0; (candidateIndex < amountOfCandidates) && !savedCandidate; candidateIndex++) {
					float& candidateSqrDst = bestActivationSqrDist[candidateIndex];
					IntVec2& candidateCoords = bestActivationCandidatesCoords[candidateIndex];

					if (distSqrToChunk < candidateSqrDst) {
						std::map<IntVec2, Chunk*>::const_iterator chunkInitializedIt = m_initializedChunks.find(chunkCoords);
						std::map<IntVec2, Chunk*>::const_iterator chunkActiveIt = m_activeChunks.find(chunkCoords);

						if ((chunkInitializedIt == m_initializedChunks.end()) && (chunkActiveIt == m_activeChunks.end())) {
							foundActivationCandidate = true;
							savedCandidate = true;
							candidateCoords = chunkCoords;
							candidateSqrDst = distSqrToChunk;
						}
					}
				}


			}
		}
	}

	if (foundActivationCandidate) {
		result = true;
		for (int candidateIndex = 0; candidateIndex < amountOfCandidates; candidateIndex++) {
			IntVec2& candidateCoords = bestActivationCandidatesCoords[candidateIndex];
			float& candidateSqrDst = bestActivationSqrDist[candidateIndex];

			if (candidateSqrDst != FLT_MAX) {
				InitiliazeChunk(candidateCoords);
			}
		}
	}

	return result;
}

bool World::CheckChunksForDeactivation()
{
	static float deactivationRange = m_activationRange + CHUNK_SIZE_X + CHUNK_SIZE_Y;

	Chunk* bestCandidateForDeactivation = nullptr;
	float bestDistanceForDeactivation = FLT_MIN;
	Vec2 playerXYPos = Vec2(m_game->m_player->m_position);
	float sqrDeactivationRange = deactivationRange * deactivationRange;

	for (std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) {
		Chunk* chunk = chunkIt->second;
		float distanceToChunk = GetDistanceSquared2D(chunk->GetChunkCenter(chunk->GetChunkCoords()), playerXYPos);
		if (distanceToChunk <= sqrDeactivationRange) continue;

		if (distanceToChunk > bestDistanceForDeactivation) {
			bestCandidateForDeactivation = chunk;
			bestDistanceForDeactivation = distanceToChunk;
		}
	}

	if (bestCandidateForDeactivation) {
		if (bestCandidateForDeactivation->m_needsSaving) {
			QueueForSaving(bestCandidateForDeactivation);
		}
		else {
			DeactivateChunk(bestCandidateForDeactivation);
		}
		return true;
	}

	return false;
}

void World::InitiliazeChunk(IntVec2 const& coords)
{
	Chunk* newChunk = nullptr;

	m_initiliazedChunksMutex.lock();

	newChunk = new Chunk(m_game, coords);
	m_initializedChunks[coords] = newChunk;

	m_initiliazedChunksMutex.unlock();

	if (newChunk->CanBeLoadedFromFile()) {
		ChunkDiskLoadJob* newChunkLoadJob = new ChunkDiskLoadJob(newChunk);
		g_theJobSystem->QueueJob(newChunkLoadJob);
	}
	else {
		ChunkGenerationJob* newChunkGenJob = new ChunkGenerationJob(newChunk);
		g_theJobSystem->QueueJob(newChunkGenJob);
	}

	//m_activeChunks[coords] = newChunk;

	//MarkLightingDirtyOnChunkBorders(newChunk);
	//FlagSkyBlocks(newChunk);
	//MarkLightEmittingBlocksAsDirty(newChunk);
	//LinkChunkNeighbors(newChunk);
}

void World::CheckForCompletedJobs()
{
	Job* chunkJob = g_theJobSystem->RetrieveCompletedJob();

	while (chunkJob) {
		switch (chunkJob->m_jobType)
		{
		case DISK_JOB_TYPE: {
			ChunkDiskLoadJob* chunkLoadJob = dynamic_cast<ChunkDiskLoadJob*>(chunkJob);
			if (chunkLoadJob) {
				ActivateChunk(chunkLoadJob->m_chunk);
			}
			else {
				ChunkDiskSaveJob* chunkSaveJob = dynamic_cast<ChunkDiskSaveJob*>(chunkJob);
				DeactivateChunk(chunkSaveJob->m_chunk);
			}

			break; }

		case CHUNK_GENERATION_JOB_TYPE: {
			ChunkGenerationJob* chunkGenJob = dynamic_cast<ChunkGenerationJob*>(chunkJob);
			ActivateChunk(chunkGenJob->m_chunk);

			break; }
		}

		delete chunkJob;

		chunkJob = g_theJobSystem->RetrieveCompletedJob();
	}

}

void World::ActivateChunk(Chunk* newChunk)
{
	if (!newChunk) return;
	IntVec2 chunkCoords = newChunk->GetChunkCoords();
	m_numActiveChunks++;

	newChunk->m_state = ChunkState::ACTIVE;
	LinkChunkNeighbors(newChunk);
	//FlagSkyBlocks(newChunk);
	//MarkLightingDirtyOnChunkBorders(newChunk);
	MarkLightEmittingBlocksAsDirty(newChunk);


	m_initiliazedChunksMutex.lock();		// lock
	m_initializedChunks.erase(chunkCoords);
	m_initiliazedChunksMutex.unlock();		// unlock

	m_activeChunks[chunkCoords] = newChunk;
}

void World::DeactivateChunk(Chunk* chunk)
{
	if (!chunk) return;
	auto chunkIt = m_activeChunks.find(chunk->GetChunkCoords());

	if (chunkIt != m_activeChunks.end()) {
		m_activeChunks.erase(chunk->GetChunkCoords());
	}

	UnlinkChunkFromNeighbors(chunk);

	delete chunk;
	m_numActiveChunks--;
}

void World::CheckChunksForMeshRegen()
{
	Chunk* nearestTwoChunks[2] = {};
	float nearestTwoDistances[2] = { FLT_MAX, FLT_MAX };

	for (std::map<IntVec2, Chunk*>::const_iterator chunkIt = m_activeChunks.begin(); chunkIt != m_activeChunks.end(); chunkIt++) {
		Chunk* chunk = chunkIt->second;
		if (chunk) {

			if (!chunk->m_isDirty || !chunk->HasExistingNeighors()) continue;

			Vec3 chunkCenter = chunk->GetChunkCenter();

			float distToCamera = GetDistanceSquared3D(m_game->m_player->m_position, chunkCenter);

			for (int distIndex = 0; distIndex < 2; distIndex++) {
				float& nearestDistance = nearestTwoDistances[distIndex];
				if (distToCamera < nearestDistance) {
					nearestTwoChunks[distIndex] = chunk;
					nearestDistance = distToCamera;
					break;
				}
			}
		}
	}

	for (int distIndex = 0; distIndex < 2; distIndex++) {
		Chunk* chunk = nearestTwoChunks[distIndex];
		if (!chunk) continue;

		chunk->GenerateCPUMesh();

	}


}

void World::LinkChunkNeighbors(Chunk* newChunk) const
{
	IntVec2 chunkCoords = newChunk->GetChunkCoords();
	Chunk* neighborChunk = GetChunk(chunkCoords + NorthStep);
	if (neighborChunk) {
		neighborChunk->m_southChunk = newChunk;
		newChunk->m_northChunk = neighborChunk;
	}

	neighborChunk = GetChunk(chunkCoords + SouthStep);
	if (neighborChunk) {
		neighborChunk->m_northChunk = newChunk;
		newChunk->m_southChunk = neighborChunk;
	}

	neighborChunk = GetChunk(chunkCoords + WestStep);
	if (neighborChunk) {
		neighborChunk->m_eastChunk = newChunk;
		newChunk->m_westChunk = neighborChunk;
	}

	neighborChunk = GetChunk(chunkCoords + EastStep);
	if (neighborChunk) {
		neighborChunk->m_westChunk = newChunk;
		newChunk->m_eastChunk = neighborChunk;
	}

}

void World::UnlinkChunkFromNeighbors(Chunk* chunkToUnlink) const
{
	IntVec2 chunkCoords = chunkToUnlink->GetChunkCoords();
	Chunk* neighborChunk = GetChunk(chunkCoords + NorthStep);
	if (neighborChunk) {
		neighborChunk->m_southChunk = nullptr;
	}

	neighborChunk = GetChunk(chunkCoords + SouthStep);
	if (neighborChunk) {
		neighborChunk->m_northChunk = nullptr;
	}

	neighborChunk = GetChunk(chunkCoords + WestStep);
	if (neighborChunk) {
		neighborChunk->m_eastChunk = nullptr;
	}

	neighborChunk = GetChunk(chunkCoords + EastStep);
	if (neighborChunk) {
		neighborChunk->m_westChunk = nullptr;
	}
}

void World::ProcessDirtyLighting()
{
	double startTime = GetCurrentTimeSeconds();

	std::deque<BlockIterator> markqueue;

	m_lightQueueMutex.lock();
	while (!m_queueForMarkingAsDirty.empty()) {
		BlockIterator blockIter = m_queueForMarkingAsDirty.front();
		m_queueForMarkingAsDirty.pop_front();

		MarkLightingDirty(blockIter);
	}

	m_lightQueueMutex.unlock();


	std::deque<BlockIterator> debugDeque;
	if (m_isLightingStepEnabled) {
		debugDeque = m_dirtyLightBlocks;
	}

	std::deque<BlockIterator>& usedDeque = (m_isLightingStepEnabled) ? debugDeque : m_dirtyLightBlocks;

	if (m_isLightingStepEnabled && !m_stepDebugLighting) return;
	else {
		if (m_isLightingStepEnabled) {
			m_dirtyLightBlocks.clear();
		}
	}

	bool processedSomeLight = false;
	while (!usedDeque.empty()) {
		processedSomeLight = true;
		BlockIterator blockIter = usedDeque.front();
		usedDeque.pop_front();
		ProcessNextDirtyLightBlock(blockIter);
	}

	if (m_isLightingStepEnabled) {
		m_stepDebugLighting = false;

	}

	if (processedSomeLight) {
		double endTime = GetCurrentTimeSeconds();
		double totalTime = endTime - startTime;
		if (totalTime > m_game->m_worstLightResolveTime) {
			m_game->m_worstLightResolveTime = totalTime;
		}

		m_game->m_totalLightResolveTime += totalTime;
		m_game->m_countLightResolveTime++;
		m_game->RefreshLoadStats();
	}
}

void World::QueueForSaving(Chunk* chunk)
{
	ChunkDiskSaveJob* newSaveJob = new ChunkDiskSaveJob(chunk);
	m_activeChunks.erase(chunk->GetChunkCoords());

	g_theJobSystem->QueueJob(newSaveJob);
}

void World::ProcessNextDirtyLightBlock(BlockIterator& blockIter)
{
	if (!blockIter.GetBlock()) return;

	Block* block = blockIter.GetBlock();
	block->SetIsLightDirty(false);

	bool wasOutdoorLightCorrected = CorrectOutdoorLighting(blockIter);
	bool wasIndoorLightCorrected = CorrectIndoorLighting(blockIter);

	if (wasOutdoorLightCorrected || wasIndoorLightCorrected) {
		MarkLightingForDirtyNeighborBlocks(blockIter);
	}

}


bool World::CorrectOutdoorLighting(BlockIterator& blockIter)
{
	Block* block = blockIter.GetBlock();
	block->SetIsLightDirty(false);

	BlockDefinition const* blockDef = BlockDefinition::GetDefById(block->m_typeIndex);
	bool wasAnyLightValueCorrected = false;
	int currentOutdoorInfluence = (int)block->GetOutdoorLightInfluence();

	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;
	static unsigned char iceId = BlockDefinition::GetDefByName("ice")->m_id;

	bool isWaterInAnyState = (waterId == blockDef->m_id) || (iceId == blockDef->m_id);

	if (block->IsSky()) {
		if (currentOutdoorInfluence != 15) {
			block->SetOutdoorLightInfluence(15);
			wasAnyLightValueCorrected = true;
		}
	}
	else {
		if (blockDef->m_outdoorLightInfluence > 0) {
			if (currentOutdoorInfluence < blockDef->m_outdoorLightInfluence) {
				block->SetOutdoorLightInfluence(blockDef->m_outdoorLightInfluence);
				wasAnyLightValueCorrected = true;
			}
		}
		else {
			if (!blockDef->m_isOpaque || isWaterInAnyState) { // Animated waves created the need for light to spread through water
				int highestOutdoorInfluence = GetHighestNeighborLightValue(blockIter, false);
				if (highestOutdoorInfluence > 0) highestOutdoorInfluence--;

				if (currentOutdoorInfluence != highestOutdoorInfluence) {
					block->SetOutdoorLightInfluence(highestOutdoorInfluence);
					wasAnyLightValueCorrected = true;
				}

			}
			else {
				if (currentOutdoorInfluence != 0) {
					block->SetOutdoorLightInfluence(0);
					wasAnyLightValueCorrected = true;
				}
			}
		}
	}

	return wasAnyLightValueCorrected;
}

bool World::CorrectIndoorLighting(BlockIterator& blockIter)
{
	Block* block = blockIter.GetBlock();
	block->SetIsLightDirty(false);

	BlockDefinition const* blockDef = BlockDefinition::GetDefById(block->m_typeIndex);
	bool wasAnyLightValueCorrected = false;
	int currentIndoorInfluence = (int)block->GetIndoorLightInfluence();
	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;
	static unsigned char iceId = BlockDefinition::GetDefByName("ice")->m_id;

	bool isWaterInAnyState = (waterId == blockDef->m_id) || (iceId == blockDef->m_id);

	if (blockDef->m_indoorLightInfluence > 0) {
		if (currentIndoorInfluence < blockDef->m_indoorLightInfluence) {
			block->SetIndoorLightInfluence(blockDef->m_indoorLightInfluence);
			wasAnyLightValueCorrected = true;
		}
	}
	else {
		if (!blockDef->m_isOpaque || isWaterInAnyState) { // Animated waves created the need for light to spread through water
			int highestIndoorInfluence = GetHighestNeighborLightValue(blockIter, true);
			if (highestIndoorInfluence > 0) highestIndoorInfluence--;

			if (currentIndoorInfluence != highestIndoorInfluence) {
				block->SetIndoorLightInfluence(highestIndoorInfluence);
				wasAnyLightValueCorrected = true;
			}

		}
		else {
			if (currentIndoorInfluence != 0) {
				block->SetIndoorLightInfluence(0);
				wasAnyLightValueCorrected = true;
			}
		}
	}


	return wasAnyLightValueCorrected;
}



void World::FlagSkyBlocks(Chunk* chunk)
{
	for (int y = 0; y < CHUNK_SIZE_Y; y++) {
		int blockIndex = (y << CHUNKSHIFT_Y);
		blockIndex |= (CHUNK_MASK_Z);
		for (int x = 0; x < CHUNK_SIZE_X; x++, blockIndex++) {
			BlockIterator blockIter(chunk, blockIndex);
			Block* block = blockIter.GetBlock();
			if (!block) continue;
			BlockDefinition const* blockDef = BlockDefinition::GetDefById(block->m_typeIndex);

			while (!blockDef->m_isOpaque) {
				block->SetIsSky(true);
				blockIter = blockIter.GetBottomNeighbor();
				block = blockIter.GetBlock();
				blockDef = BlockDefinition::GetDefById(block->m_typeIndex);

			}
		}
	}

	for (int y = 0; y < CHUNK_SIZE_Y; y++) {
		int blockIndex = (y << CHUNKSHIFT_Y);
		blockIndex |= (CHUNK_MASK_Z);
		for (int x = 0; x < CHUNK_SIZE_X; x++, blockIndex++) {
			BlockIterator blockIter(chunk, blockIndex);
			Block* block = blockIter.GetBlock();
			if (!block) continue;
			BlockDefinition const* blockDef = BlockDefinition::GetDefById(block->m_typeIndex);

			m_lightQueueMutex.lock();
			while (!blockDef->m_isOpaque) {
				BlockIterator northIter = blockIter.GetNorthNeighbor();
				BlockIterator southIter = blockIter.GetSouthNeighbor();
				BlockIterator westIter = blockIter.GetWestNeighbor();
				BlockIterator eastIter = blockIter.GetEastNeighbor();
				if (block->IsSky()) block->SetOutdoorLightInfluence(15);

				if (northIter.GetBlock()) {
					BlockDefinition const* northDef = BlockDefinition::GetDefById(northIter.GetBlock()->m_typeIndex);
					if (!northDef->m_isOpaque && !northIter.GetBlock()->IsSky()) {
						m_queueForMarkingAsDirty.push_back(northIter);
					}
				}

				if (southIter.GetBlock()) {
					BlockDefinition const* southDef = BlockDefinition::GetDefById(southIter.GetBlock()->m_typeIndex);
					if (!southDef->m_isOpaque && !southIter.GetBlock()->IsSky()) {
						m_queueForMarkingAsDirty.push_back(southIter);
					}
				}

				if (westIter.GetBlock()) {
					BlockDefinition const* westDef = BlockDefinition::GetDefById(westIter.GetBlock()->m_typeIndex);

					if (!westDef->m_isOpaque && !westIter.GetBlock()->IsSky()) {
						m_queueForMarkingAsDirty.push_back(westIter);
					}
				}

				if (eastIter.GetBlock()) {
					BlockDefinition const* eastDef = BlockDefinition::GetDefById(eastIter.GetBlock()->m_typeIndex);
					if (!eastDef->m_isOpaque && !eastIter.GetBlock()->IsSky()) {
						m_queueForMarkingAsDirty.push_back(eastIter);
					}
				}

				blockIter = blockIter.GetBottomNeighbor();
				block = blockIter.GetBlock();
				blockDef = BlockDefinition::GetDefById(block->m_typeIndex);
			}
			m_lightQueueMutex.unlock();

		}
	}
}

void World::MarkLightEmittingBlocksAsDirty(Chunk* chunk)
{
	for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_SIZE; blockIndex++) {
		BlockIterator blockIter(chunk, blockIndex);
		BlockDefinition const* blockDef = BlockDefinition::GetDefById(blockIter.GetBlock()->m_typeIndex);
		if (blockDef->m_indoorLightInfluence > 0 || blockDef->m_outdoorLightInfluence > 0) { // If either indoor or outdoor light influence is >0 then the whole number will be >0
			MarkLightingDirty(blockIter);
		}
	}
}

void World::MarkLightingDirtyOnChunkBorders(Chunk* chunk)
{
	for (int heightBlockIndex = 0; heightBlockIndex < CHUNK_SIZE_Z; heightBlockIndex++) {
		int southBlockIndex = (heightBlockIndex << CHUNKSHIFT_Z);
		int northBlockIndex = southBlockIndex | CHUNK_MASK_Y;

		BlockIterator southBorderIter(chunk, southBlockIndex);
		BlockIterator northBorderIter(chunk, northBlockIndex);
		m_lightQueueMutex.lock();

		for (int borderBlockIndex = 0; borderBlockIndex < CHUNK_SIZE_X; borderBlockIndex++) {
			m_queueForMarkingAsDirty.push_back(southBorderIter);
			m_queueForMarkingAsDirty.push_back(northBorderIter);
			southBorderIter = southBorderIter.GetEastNeighbor();
			northBorderIter = northBorderIter.GetEastNeighbor();
		}

		int westBlockIndex = (heightBlockIndex << CHUNKSHIFT_Z);
		int eastBlockIndex = westBlockIndex | CHUNK_MASK_X;

		BlockIterator westBorderIter(chunk, westBlockIndex);
		BlockIterator eastBorderIter(chunk, eastBlockIndex);
		for (int borderBlockIndex = 0; borderBlockIndex < CHUNK_SIZE_Y; borderBlockIndex++) {
			m_queueForMarkingAsDirty.push_back(westBorderIter);
			m_queueForMarkingAsDirty.push_back(eastBorderIter);

			westBorderIter = westBorderIter.GetNorthNeighbor();
			eastBorderIter = eastBorderIter.GetNorthNeighbor();
		}

		m_lightQueueMutex.unlock();
	}

}

void World::ToggleFog() const
{
	if (g_gameConstants.FogEndDistance == FLT_MAX) {
		float fogEndDistance = (m_activationRange - 16.0f);
		g_gameConstants.FogEndDistance = fogEndDistance;
	}
	else {
		g_gameConstants.FogEndDistance = FLT_MAX;
	}
}

void World::MarkLightingForDirtyNeighborBlocks(BlockIterator& blockIter)
{
	BlockIterator north = blockIter.GetNorthNeighbor();
	BlockIterator south = blockIter.GetSouthNeighbor();
	BlockIterator east = blockIter.GetEastNeighbor();
	BlockIterator west = blockIter.GetWestNeighbor();
	BlockIterator top = blockIter.GetTopNeighbor();
	BlockIterator bottom = blockIter.GetBottomNeighbor();

	if (north.GetChunk()) north.GetChunk()->m_isDirty = true;
	if (south.GetChunk()) south.GetChunk()->m_isDirty = true;
	if (east.GetChunk()) east.GetChunk()->m_isDirty = true;
	if (west.GetChunk()) west.GetChunk()->m_isDirty = true;
	if (top.GetChunk()) top.GetChunk()->m_isDirty = true;
	if (bottom.GetChunk()) bottom.GetChunk()->m_isDirty = true;


	static unsigned char waterId = BlockDefinition::GetDefByName("water")->m_id;
	static unsigned char iceId = BlockDefinition::GetDefByName("ice")->m_id;

	bool isNorthOpaque = north.GetBlock() && BlockDefinition::GetDefById(north.GetBlock()->m_typeIndex)->m_isOpaque;
	bool isSouthOpaque = south.GetBlock() && BlockDefinition::GetDefById(south.GetBlock()->m_typeIndex)->m_isOpaque;
	bool isEastOpaque = east.GetBlock() && BlockDefinition::GetDefById(east.GetBlock()->m_typeIndex)->m_isOpaque;
	bool isWestOpaque = west.GetBlock() && BlockDefinition::GetDefById(west.GetBlock()->m_typeIndex)->m_isOpaque;
	bool isTopOpaque = top.GetBlock() && BlockDefinition::GetDefById(top.GetBlock()->m_typeIndex)->m_isOpaque;
	bool isBottomOpaque = bottom.GetBlock() && BlockDefinition::GetDefById(bottom.GetBlock()->m_typeIndex)->m_isOpaque;

	bool isNorthWater = north.GetBlock() && ((north.GetBlock()->m_typeIndex == waterId) || (north.GetBlock()->m_typeIndex == iceId));
	bool isSouthWater = south.GetBlock() && ((south.GetBlock()->m_typeIndex == waterId) || (south.GetBlock()->m_typeIndex == iceId));
	bool isEastWater = east.GetBlock() && ((east.GetBlock()->m_typeIndex == waterId) || (east.GetBlock()->m_typeIndex == iceId));
	bool isWestWater = west.GetBlock() && ((west.GetBlock()->m_typeIndex == waterId) || (west.GetBlock()->m_typeIndex == iceId));
	bool isTopWater = top.GetBlock() && ((top.GetBlock()->m_typeIndex == waterId) || (top.GetBlock()->m_typeIndex == iceId));
	bool isBottomWater = bottom.GetBlock() && ((bottom.GetBlock()->m_typeIndex == waterId) || (bottom.GetBlock()->m_typeIndex == iceId));

	if (!isNorthOpaque || isNorthWater) {
		MarkLightingDirty(north);
	}

	if (!isSouthOpaque || isSouthWater) {
		MarkLightingDirty(south);
	}

	if (!isEastOpaque || isEastWater) {
		MarkLightingDirty(east);
	}
	if (!isWestOpaque || isWestWater) {
		MarkLightingDirty(west);
	}
	if (!isTopOpaque || isTopWater) {
		MarkLightingDirty(top);
	}
	if (!isBottomOpaque || isBottomWater) {
		MarkLightingDirty(bottom);
	}



}

void World::MarkLightingDirty(BlockIterator& blockIter)
{
	Block* block = blockIter.GetBlock();
	if (block) {
		if (block->IsLightDirty()) return;

		block->SetIsLightDirty(true);
		m_dirtyLightBlocks.push_back(blockIter);
		blockIter.GetChunk()->m_isDirty = true;
		if (m_isLightingStepEnabled && m_canUseDebugLightingDraw) {
			DebugAddWorldPoint(blockIter.GetBlockCenter(), 0.05f, -1.0f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::ALWAYS, 2, 3);
		}
	}
}

int World::GetHighestNeighborLightValue(BlockIterator& blockIter, bool isIndoorLighting)
{

	int highestValue = INT_MIN;
	BlockIterator north = blockIter.GetNorthNeighbor();
	BlockIterator south = blockIter.GetSouthNeighbor();
	BlockIterator east = blockIter.GetEastNeighbor();
	BlockIterator west = blockIter.GetWestNeighbor();
	BlockIterator top = blockIter.GetTopNeighbor();
	BlockIterator bottom = blockIter.GetBottomNeighbor();

	if (isIndoorLighting) {
		if (north.GetBlock() && north.GetBlock()->GetIndoorLightInfluence() > highestValue) {
			highestValue = north.GetBlock()->GetIndoorLightInfluence();
		}

		if (south.GetBlock() && south.GetBlock()->GetIndoorLightInfluence() > highestValue) {
			highestValue = south.GetBlock()->GetIndoorLightInfluence();
		}

		if (east.GetBlock() && east.GetBlock()->GetIndoorLightInfluence() > highestValue) {
			highestValue = east.GetBlock()->GetIndoorLightInfluence();
		}

		if (west.GetBlock() && west.GetBlock()->GetIndoorLightInfluence() > highestValue) {
			highestValue = west.GetBlock()->GetIndoorLightInfluence();
		}

		if (bottom.GetBlock() && bottom.GetBlock()->GetIndoorLightInfluence() > highestValue) {
			highestValue = bottom.GetBlock()->GetIndoorLightInfluence();
		}

		if (top.GetBlock() && top.GetBlock()->GetIndoorLightInfluence() > highestValue) {
			highestValue = top.GetBlock()->GetIndoorLightInfluence();
		}
	}
	else {
		if (north.GetBlock() && north.GetBlock()->GetOutdoorLightInfluence() > highestValue) {
			highestValue = north.GetBlock()->GetOutdoorLightInfluence();
		}

		if (south.GetBlock() && south.GetBlock()->GetOutdoorLightInfluence() > highestValue) {
			highestValue = south.GetBlock()->GetOutdoorLightInfluence();
		}

		if (east.GetBlock() && east.GetBlock()->GetOutdoorLightInfluence() > highestValue) {
			highestValue = east.GetBlock()->GetOutdoorLightInfluence();
		}

		if (west.GetBlock() && west.GetBlock()->GetOutdoorLightInfluence() > highestValue) {
			highestValue = west.GetBlock()->GetOutdoorLightInfluence();
		}

		if (bottom.GetBlock() && bottom.GetBlock()->GetOutdoorLightInfluence() > highestValue) {
			highestValue = bottom.GetBlock()->GetOutdoorLightInfluence();
		}

		if (top.GetBlock() && top.GetBlock()->GetOutdoorLightInfluence() > highestValue) {
			highestValue = top.GetBlock()->GetOutdoorLightInfluence();
		}
	}

	return highestValue;
}

void World::UndirtyAllBlocksInChunk(Chunk* chunk)
{
	for (int blockIndex = 0; blockIndex < m_dirtyLightBlocks.size(); blockIndex++) {
		BlockIterator& blockIter = m_dirtyLightBlocks[blockIndex];
		if (blockIter.GetChunk() == chunk) {
			m_dirtyLightBlocks.erase(m_dirtyLightBlocks.begin() + blockIndex);
			blockIndex--;
		}
	}
}

bool World::IsPointInSolid(Vec3 const& refPoint) const
{
	IntVec2 chunkCoords = Chunk::GetChunkCoordsForWorldPos(refPoint);
	Chunk* chunk = GetChunk(chunkCoords);
	if (!chunk) return false;
	IntVec3 localCoords = Chunk::GetLocalCoordsForWorldPos(refPoint);

	int blockIndex = Chunk::GetIndexForLocalCoords(localCoords);

	BlockDefinition const* blockDef = nullptr;
	if (blockIndex >= 0 && blockIndex < CHUNK_TOTAL_SIZE) {
		blockDef = BlockDefinition::GetDefById(chunk->m_blocks[blockIndex].m_typeIndex);
	}

	if (!blockDef) return false;

	return blockDef->m_isSolid;
}

void World::AddNeighborBlocksToPhysicsCheck(std::vector<BlockIterator>& cardinalBlocksToCheck, std::vector<BlockIterator>& diagonalBlocksToCheck, Entity const& entity, BlockIterator& block) const
{
	Vec3 blockCenter = block.GetBlockCenter();
	if (entity.m_position.x >= blockCenter.x) {
		BlockIterator east = block.GetEastNeighbor();
		diagonalBlocksToCheck.push_back(east.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(east);
		diagonalBlocksToCheck.push_back(east.GetTopNeighbor());
	}
	else {
		BlockIterator west = block.GetWestNeighbor();
		diagonalBlocksToCheck.push_back(west.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(west);
		diagonalBlocksToCheck.push_back(west.GetTopNeighbor());
	}

	if (entity.m_position.y >= blockCenter.y) {
		BlockIterator north = block.GetNorthNeighbor();
		diagonalBlocksToCheck.push_back(north.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(north);
		diagonalBlocksToCheck.push_back(north.GetTopNeighbor());
	}
	else {
		BlockIterator north = block.GetSouthNeighbor();
		diagonalBlocksToCheck.push_back(north.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(north);
		diagonalBlocksToCheck.push_back(north.GetTopNeighbor());
	}

	if ((entity.m_position.x >= blockCenter.x) && (entity.m_position.y >= blockCenter.y)) { // NorthEast
		BlockIterator northeast = block.GetNorthNeighbor().GetEastNeighbor();
		diagonalBlocksToCheck.push_back(northeast.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(northeast);
		diagonalBlocksToCheck.push_back(northeast.GetTopNeighbor());
	}

	if ((entity.m_position.x < blockCenter.x) && (entity.m_position.y >= blockCenter.y)) { // NorthWest
		BlockIterator northWest = block.GetNorthNeighbor().GetWestNeighbor();
		diagonalBlocksToCheck.push_back(northWest.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(northWest);
		diagonalBlocksToCheck.push_back(northWest.GetTopNeighbor());
	}

	if ((entity.m_position.x >= blockCenter.x) && (entity.m_position.y < blockCenter.y)) { // SouthEast
		BlockIterator southEast = block.GetSouthNeighbor().GetEastNeighbor();
		diagonalBlocksToCheck.push_back(southEast.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(southEast);
		diagonalBlocksToCheck.push_back(southEast.GetTopNeighbor());
	}

	if ((entity.m_position.x < blockCenter.x) && (entity.m_position.y < blockCenter.y)) { // SouthWest
		BlockIterator southWest = block.GetSouthNeighbor().GetWestNeighbor();
		diagonalBlocksToCheck.push_back(southWest.GetBottomNeighbor());
		cardinalBlocksToCheck.push_back(southWest);
		diagonalBlocksToCheck.push_back(southWest.GetTopNeighbor());
	}

}

void World::CalulateRenderingOrder()
{
	int maxChunkRadiusX = 1 + int(m_activationRange) / CHUNK_SIZE_X;
	int maxChunkRadiusY = 1 + int(m_activationRange) / CHUNK_SIZE_Y;

	for (int xOffset = -maxChunkRadiusX; xOffset < maxChunkRadiusX; xOffset++) {
		for (int yOffset = -maxChunkRadiusY; yOffset < maxChunkRadiusY; yOffset++) {
			IntVec2 offset(xOffset, yOffset);
			m_orderedRenderingOffsets.push_back(offset);
		}
	}

	std::sort(m_orderedRenderingOffsets.begin(), m_orderedRenderingOffsets.end(), [](IntVec2 const& left, IntVec2 const& right) {
		int leftSum = abs(left.x) + abs(left.y);
		int rightSum = abs(right.x) + abs(right.y);

		return leftSum > rightSum;
		});

}

SimpleMinerRaycast World::RaycastVsTiles(Vec3 const& rayStart, Vec3 const& forwardNormal, float maxLength)
{
	SimpleMinerRaycast hitInfo;
	hitInfo.m_startPosition = rayStart;
	hitInfo.m_forwardNormal = forwardNormal;
	hitInfo.m_maxDistance = maxLength;

	if (rayStart.z > (float)CHUNK_SIZE_Z) return hitInfo;
	if (IsPointInSolid(rayStart)) {
		hitInfo.m_didImpact = true;
		hitInfo.m_impactDist = 0.0f;
		hitInfo.m_impactPos = rayStart;

		return hitInfo;
	}

	IntVec2 chunkCoords = Chunk::GetChunkCoordsForWorldPos(rayStart);

	Chunk* chunk = GetChunk(chunkCoords);

	// LOCAL COORDS WILL RUIN FWD DIST CALCULATIONS. MUST USE REAL FLOOR FOR RAYSTART
	IntVec3 tilePosition((int)floorf(rayStart.x), (int)floorf(rayStart.y), (int)floorf(rayStart.z));
	IntVec3 localCoords = chunk->GetLocalCoordsForWorldPos(rayStart);
	int blockIndex = chunk->GetIndexForLocalCoords(localCoords);

	float fwdDistPerXCrossing = 0.0f;
	float fwdDistPerYCrossing = 0.0f;
	float fwdDistPerZCrossing = 0.0f;

	int tileStepDirectionX = 1;
	int tileStepDirectionY = 1;
	int tileStepDirectionZ = 1;

	if (forwardNormal.x < 0) {
		tileStepDirectionX = -1;
	}

	if (forwardNormal.y < 0) {
		tileStepDirectionY = -1;
	}

	if (forwardNormal.z < 0) {
		tileStepDirectionZ = -1;
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

	if (forwardNormal.z == 0) {
		fwdDistPerZCrossing = ARBITRARILY_LARGE_VALUE;
	}
	else {
		fwdDistPerZCrossing = 1.0f / fabsf(forwardNormal.z);
	}


	float xAtFirstCrossing = tilePosition.x + static_cast<float>((tileStepDirectionX + 1) / 2);
	float xDistToFirstCrossing = xAtFirstCrossing - rayStart.x;

	float fwdDistAtNextXCrossing = fabsf(xDistToFirstCrossing) * fwdDistPerXCrossing;

	float yAtFirstCrossing = tilePosition.y + static_cast<float>((tileStepDirectionY + 1) / 2);
	float yDistToFirstCrossing = yAtFirstCrossing - rayStart.y;

	float fwdDistAtNextYCrossing = fabsf(yDistToFirstCrossing) * fwdDistPerYCrossing;

	float zAtFirstCrossing = tilePosition.z + static_cast<float>((tileStepDirectionZ + 1) / 2);
	float zDistToFirstCrossing = zAtFirstCrossing - rayStart.z;

	float fwdDistAtNextZCrossing = fabsf(zDistToFirstCrossing) * fwdDistPerZCrossing;

	BlockIterator blockIter(chunk, blockIndex);

	while (!hitInfo.m_didImpact) {

		float minDistCrossing = (fwdDistAtNextXCrossing < fwdDistAtNextYCrossing) ? fwdDistAtNextXCrossing : fwdDistAtNextYCrossing;
		minDistCrossing = (fwdDistAtNextZCrossing < minDistCrossing) ? fwdDistAtNextZCrossing : minDistCrossing;

		bool isXMin = minDistCrossing == fwdDistAtNextXCrossing;
		bool isYMin = minDistCrossing == fwdDistAtNextYCrossing;
		bool isZMin = minDistCrossing == fwdDistAtNextZCrossing;

		if (isXMin) {
			if (tileStepDirectionX < 0) {
				blockIter = blockIter.GetWestNeighbor();
			}
			else {
				blockIter = blockIter.GetEastNeighbor();
			}
		}

		if (isYMin) {
			if (tileStepDirectionY < 0) {
				blockIter = blockIter.GetSouthNeighbor();
			}
			else {
				blockIter = blockIter.GetNorthNeighbor();
			}
		}

		if (isZMin) {
			if (tileStepDirectionZ < 0) {
				blockIter = blockIter.GetBottomNeighbor();
			}
			else {
				blockIter = blockIter.GetTopNeighbor();
			}
		}

		if (minDistCrossing > maxLength) {
			hitInfo.m_maxDistanceReached = true;
			return hitInfo;
		}


		Block* block = blockIter.GetBlock();
		if (!block) return hitInfo;
		BlockDefinition const* blockDef = BlockDefinition::GetDefById(block->m_typeIndex);

		bool isTileSolid = blockDef && blockDef->m_isSolid;
		if (isTileSolid) {
			hitInfo.m_didImpact = true;
			hitInfo.m_impactDist = minDistCrossing;
			hitInfo.m_impactPos = rayStart + (forwardNormal * hitInfo.m_impactDist);

			if (isXMin) {
				hitInfo.m_impactNormal = Vec3(static_cast<float>(-tileStepDirectionX), 0.0f, 0.0f);
			}

			if (isYMin) {
				hitInfo.m_impactNormal = Vec3(0.0f, static_cast<float>(-tileStepDirectionY), 0.0f);
			}

			if (isZMin) {
				hitInfo.m_impactNormal = Vec3(0.0f, 0.0f, static_cast<float>(-tileStepDirectionZ));
			}
		}
		else {
			if (isXMin)fwdDistAtNextXCrossing += fwdDistPerXCrossing;
			if (isYMin)fwdDistAtNextYCrossing += fwdDistPerYCrossing;
			if (isZMin)fwdDistAtNextZCrossing += fwdDistPerZCrossing;
		}

	}

	hitInfo.m_blockIter = blockIter;

	return hitInfo;
}

void World::SetCameraPositionConstant(Vec3 const& cameraPos) const
{
	g_gameConstants.CameraWorldPosition = Vec4(cameraPos);
}

Rgba8 const World::GetSkyColor() const
{
	return Rgba8(g_gameConstants.SkyColor);
}

void World::PushEntityOutOfWorld(Entity& entity) const
{
	IntVec2 chunkCoords = Chunk::GetChunkCoordsForWorldPos(entity.m_position);
	IntVec3 localCoords = Chunk::GetLocalCoordsForWorldPos(entity.m_position);
	int blockIndex = Chunk::GetIndexForLocalCoords(localCoords);

	Chunk* chunk = GetChunk(chunkCoords);
	BlockIterator block(chunk, blockIndex);

	std::vector<BlockIterator> cardinalBlocksToCheck;
	std::vector<BlockIterator> diagnoalBlocksToCheck;

	//cardinalBlocksToCheck.push_back(block);
	cardinalBlocksToCheck.push_back(block.GetBottomNeighbor());
	cardinalBlocksToCheck.push_back(block.GetTopNeighbor());

	AddNeighborBlocksToPhysicsCheck(cardinalBlocksToCheck, diagnoalBlocksToCheck, entity, block);

	cardinalBlocksToCheck.insert(cardinalBlocksToCheck.end(), diagnoalBlocksToCheck.begin(), diagnoalBlocksToCheck.end());
	IntVec3 entityLocalCoords = Chunk::GetLocalCoordsForWorldPos(entity.m_position);

	for (int neighborIndex = 0; neighborIndex < cardinalBlocksToCheck.size(); neighborIndex++) {
		BlockIterator const& blockIter = cardinalBlocksToCheck[neighborIndex];
		Block* currentNeighbor = blockIter.GetBlock();
		if (currentNeighbor) {
			BlockDefinition const* currentNeighborDef = BlockDefinition::GetDefById(currentNeighbor->m_typeIndex);
			if (!currentNeighborDef || !currentNeighborDef->m_isSolid) continue;

			AABB3 entityBounds = entity.GetBounds();
			AABB3 blockBounds = blockIter.GetBlockBounds();


			bool wasPushed = PushAABB3OutOfAABB3(blockBounds, entityBounds);
			if (wasPushed) {

				Vec3 const& newCenter = entityBounds.GetCenter();
				Vec3 pushDir = (entity.m_position - newCenter);

				if (pushDir.GetLengthSquared() > 0.0f) {
					Vec3 unsignedPushDir = pushDir;
					unsignedPushDir.x = fabsf(pushDir.x);
					unsignedPushDir.y = fabsf(pushDir.y);
					unsignedPushDir.z = fabsf(pushDir.z);

					float dotWithX = DotProduct3D(unsignedPushDir, Vec3(1.0f, 0.0f, 0.0f));
					float dotWithY = DotProduct3D(unsignedPushDir, Vec3(0.0f, 1.0f, 0.0f));
					float dotWithZ = DotProduct3D(unsignedPushDir, Vec3(0.0f, 0.0f, 1.0f));

					if ((dotWithX >= dotWithY) && (dotWithX >= dotWithZ)) {
						entity.m_velocity.x = 0.0f;
					}

					if ((dotWithY >= dotWithX) && (dotWithY >= dotWithZ)) {
						entity.m_velocity.y = 0.0f;
					}

					if ((dotWithZ >= dotWithY) && (dotWithZ >= dotWithX)) {
						entity.m_velocity.z = 0.0f;
						if (pushDir.z > 0.0f) {
							entity.m_pushedFromBottom = true;
						}
					}
				}

				entity.m_position = newCenter;
			}
		}
	}
}

void World::CarveBlocksInRadius(Vec3 const& refPoint, float radius)
{
	Vec3 radiusVec = Vec3(radius, radius, radius);

	IntVec2 chunkcoords = Chunk::GetChunkCoordsForWorldPos(refPoint);
	auto chunkIt = m_activeChunks.find(chunkcoords);

	if (chunkIt != m_activeChunks.end()) {
		Chunk* chunk = chunkIt->second;

		IntVec3 localCoords = Chunk::GetLocalCoordsForWorldPos(refPoint);

		DebugAddWorldWireSphere(refPoint - radiusVec, 0.1f, 20.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::ALWAYS);
		DebugAddWorldWireSphere(refPoint + radiusVec, 0.1f, 20.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::ALWAYS);
		chunk->CarveBlockInRadius(refPoint, localCoords, radius);
	}
}

bool operator<(IntVec2 const& coords, IntVec2 const& compareTo)
{
	if (coords.y < compareTo.y) {
		return true;
	}
	else if (coords.y > compareTo.y) {
		return false;
	}
	else if (coords.x < compareTo.x) {
		return true;
	}
	return false;
}
