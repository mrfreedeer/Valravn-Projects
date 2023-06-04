#include "Common.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Tile.hpp"
#include "Ant.hpp"
struct StartupInfo;

class ColonyJob;

class Colony {
public:
	Colony(StartupInfo const& startupInfo);
	void Update();
	void AsyncUpdate();
	TileHeatMap GetHeatmap(eAgentType agentType, bool lookingForQueen = false);
	int GetTurnNumber() const { return m_currentTurnInfo.turnNumber; }
	int GetNutrients() const;
	void RecalculateHeatMap(TileHeatMap& heatmap, eAgentType agentType);
	void QueueJobForExecution(ColonyJob* newJob);
	ColonyJob* ClaimJobForExecution();
	void UpdateHeatmap(TileHeatMap const& updatedHeatmap, eAgentType agentType, bool lookingForQueen = false);
	void UpdateOrders(PlayerTurnOrders const& newOrders);
	void ProcessTurnInfo();
	void GetUnexploredTiles(std::vector<IntVec2>& tileCoordsContainer) const;
	
	bool IsTileSolid(int tileIndex, eAgentType agentType) const;
	bool IsTileSolid(IntVec2 const& tileCoords, eAgentType agentType) const;
	bool IsTileDirt(int tileIndex) const;
	bool IsTileDirt(IntVec2 const& tileCoords) const;

	int GetCostToBirth(eAgentType agentType) const;
	int GetSightRadius(eAgentType agentType) const;
	int GetMapWidth() const { return m_startupInfo.matchInfo.mapWidth; }
	int GetFoodDensityInRadius(IntVec2 const& startPos, int radius) const;
	void ScheduleForBirth(eAgentType agentType);
	int HowManyWillBeBorn(eAgentType agentType) const;

	void ScheduleForDeath(eAgentType agentType);
	bool IsScheduledForDeath(eAgentType agentType) const;
	IntVec2 GetRandNonSolidCoords() const;

	std::vector<IntVec2>& GetTilesWithFood();
	void FlagFoodAsTaken(IntVec2 const& coords);
	void ResetFoodDensity(IntVec2 const& coords);

	void RegisterSoldierAStartRequest();
	void GetAStarPath(std::vector<IntVec2>& resultPath, IntVec2 const& start, std::vector<IntVec2> const& goals, eAgentType agentType, int maxLoops, float heuristicWeight = 1.0f, bool seekingFood = false);

	void TryAppointingGuard(Ant* requestingAnt);
	void QueueGuardForRelease(Ant* requestingAnt);
	void AppointAllPendingGuards();
	void ReleaseAllPendingGuards();
	bool CanCalculateAStar() const;
	bool CanCalculateAStarSoldiers() const;
	int GetNumPlayers() const;

	IntVec2 GetHighestFoodDensityCoords() const;

	ObservedAgent const* GetClosestEnemy(IntVec2 const& coords) const;
	ObservedAgent const* GetClosestEnemyQueen(IntVec2 const& coords) const;
	ObservedAgent const* GetEnemyIfVisible(AgentID enemyId) const;

	bool IsAnyQueenBeingAttacked() const;
	bool IsEnemyAtPos(IntVec2 const& coords) const;
	bool IsFoodAtPos(IntVec2 const& coords) const;

	unsigned int m_liveAnts = 0;
	unsigned int m_liveWorkers = 0;
	unsigned int m_liveQueens = 0;
	unsigned int m_liveScouts = 0;
	unsigned int m_liveSoldiers = 0;

	unsigned int m_workersToBirth = 0;
	unsigned int m_queensToBirth = 0;
	unsigned int m_scoutsToBirth = 0;
	unsigned int m_soldiersToBirth = 0;
	int m_aStarsRequested = 0;
	int m_aStarsRequestedSoldiers = 0;

	std::atomic<unsigned char> m_scheduledForDeath = 0;
	std::atomic<unsigned int> m_foodCount = 0;
	Ant* GetColony() { return m_colony; }
	std::vector<Ant*> m_soldierAnts;
	std::vector<IntVec2> m_tilesWithFood;
	std::vector<IntVec2> m_unexploredTiles;
	std::vector<IntVec2> m_enemyPositions;

private:
	int GetTileIndex(int x, int y) const;
	IntVec2 GetTileCoords(int tileIndex) const;

	void ProcessEnemyInfo();
	void ProcessTilesInfo(eTileType const* observedTiles, bool const* tilesWithFood);
	void ProcessReport(AgentReport const& newReport);
	void ProcessAgentCreation(AgentReport const& newReport);
	void ProcessAgentHold(AgentReport const& newReport);
	void ProcessAgentMoved(AgentReport const& newReport);
	void ProcessAgentDeath(AgentReport const& newReport);
	void ProcessAgentPickup(AgentReport const& newReport);
	void ProcessAgentDug(AgentReport const& newReport);
	void ProcessAndCopyInfo(AgentReport const& newReport);
	void CopyReportInfo(Ant& ant, AgentReport const& agentReport);

	void AddDebugVisibilityVerts(std::vector<VertexPC>& verts);
	void AddHeatmapDebugVerts(std::vector<VertexPC>& verts, TileHeatMap const& heatMap, float maxVisibleValue);
	void AddAStarPathVerts(std::vector<VertexPC>& verts, std::vector<IntVec2> const& aStarPath, eAgentType agentType);
	void AddVertsForSquare(std::vector<VertexPC>& verts, float size, int x, int y, Color8 color);

	void RecalculateHeatmapToQueens();
	void RecalculateQueenFoodmap();
	void RecalculateSoldierAttackmap();

	bool SetTileHeatmapValue(TileHeatMap& heatmap, eAgentType agentType, int tileIndex, float currentValue);

	float GetTileCost(int tileIndex, eAgentType agentType) const;
	float GetTileCost(IntVec2 const& tileCoords, eAgentType agentType) const;

	void ConstructPathForAStar(std::vector<IntVec2>& pathContainer, std::map<IntVec2, IntVec2> const& cameFromMap, IntVec2 const& goalCoords) const;
	float GetAStarHeuristic(IntVec2 const& tileCoords, std::vector<IntVec2> const& goals, bool avoidEnemies = false);
	float GetAStarHeuristic(IntVec2 const& tileCoords, IntVec2 const& goal, bool avoidEnemies = false);
	IntVec2 GetClosestGoal(IntVec2 const& tilecoords, std::vector<IntVec2> const& goals);
	IntVec2 GetClosestFoodGoal(IntVec2 const& tilecoords, std::vector<IntVec2> const& goals);
	void AddSoldierAnt(Ant* soldierAnt);
	void RemoveSoldierAnt(Ant* antToRemove);

	Ant& GetFreeAntSlot();
	Ant& GetAnt(AgentID agentId, bool& out_wasSuccessful);
	StartupInfo m_startupInfo = {};
	ArenaTurnStateForPlayer m_currentTurnInfo = {};
	Ant m_colony[MAX_AGENTS_PER_PLAYER] = {};

	std::mutex m_heatmapsMutex;

	TileHeatMap m_heatmapToQueens;
	TileHeatMap m_queenFoodMap;
	TileHeatMap m_solidMap;
	TileHeatMap m_foodDensityMap;
	TileHeatMap m_enemyHistoryMap;
	TileHeatMap m_soldierAttackMap;
	std::vector<int> m_foodDensityMipMap;
	bool m_heatmapToQueensDirty = true;
	bool m_workerFoodmapDirty = true;
	bool m_queenFoodMapDirty = true;
	int m_lastQueenFoodUpdate = 0;
	int m_lastSoldierUpdate = 0;

	std::vector<Tile> m_tiles;
	std::atomic<bool> m_areCalculationsDone = false;

	mutable std::mutex m_queuedJobsMutex;
	std::deque<ColonyJob*> m_queuedJobs;
	std::atomic<int> m_queuedJobsAmount;

	std::mutex m_ordersMutex;
	PlayerTurnOrders m_turnOrders = {};
	int m_numKnownTiles = 0;
	std::map<Ant*, Ant*> m_guardAppointments;
	std::vector<Ant*> m_guardsToRelease;
	bool m_heatmapScheduled = false;
	bool m_foundQueen = false;

};

bool operator<(IntVec2 const& coords, IntVec2 const& compareTo);