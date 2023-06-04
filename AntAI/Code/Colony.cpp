#include "Colony.hpp"
#include "ColonyJob.hpp"
#include "ArenaPlayerInterface.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Colony::Colony(StartupInfo const& startupInfo) :
	m_startupInfo(startupInfo),
	m_queenFoodMap(IntVec2(startupInfo.matchInfo.mapWidth, startupInfo.matchInfo.mapWidth)),
	m_heatmapToQueens(IntVec2(startupInfo.matchInfo.mapWidth, startupInfo.matchInfo.mapWidth)),
	m_solidMap(IntVec2(startupInfo.matchInfo.mapWidth, startupInfo.matchInfo.mapWidth)),
	m_foodDensityMap(IntVec2(startupInfo.matchInfo.mapWidth, startupInfo.matchInfo.mapWidth)),
	m_enemyHistoryMap(IntVec2(startupInfo.matchInfo.mapWidth, startupInfo.matchInfo.mapWidth)),
	m_soldierAttackMap(IntVec2(startupInfo.matchInfo.mapWidth, startupInfo.matchInfo.mapWidth))
{
	m_queenFoodMap.SetAllValues(FLT_MAX);
	m_solidMap.SetAllValues(1.0f);
	m_tiles.resize(startupInfo.matchInfo.mapWidth * startupInfo.matchInfo.mapWidth);
	m_foodDensityMap.SetAllValues(0.0f);
}

void Colony::Update()
{
	m_aStarsRequested = 0;
	m_aStarsRequestedSoldiers = 0;
	eTurnStatus turnStatus = g_threadSafe_turnStatus;
	if (turnStatus == TURN_STATUS_PROCESSING_UPDATE) {
		g_threadSafe_turnInfo.CopyTo(m_currentTurnInfo);
		g_threadSafe_turnStatus = TURN_STATUS_WORKING_ON_ORDERS;

		//ProcessTurnInfoJob* processTurnInfoJob = new ProcessTurnInfoJob(this);
		m_areCalculationsDone = false;
		//QueueJobForExecution(processTurnInfoJob);

		ProcessTurnInfo();


	}
	else if (turnStatus == TURN_STATUS_WORKING_ON_ORDERS) {
		if (!m_areCalculationsDone) {
			ColonyJob* jobToExecute = ClaimJobForExecution();
			if (jobToExecute) {
				jobToExecute->Execute();
				delete jobToExecute;
			}
			return;
		}

		m_ordersMutex.lock();
		g_threadSafe_turnOrders.CopyFrom(m_turnOrders);
		m_ordersMutex.unlock();


		if (g_debug->IsDebugging()) {
			g_debug->SetMoodText("Debugging");

			std::vector<VertexPC> debugVerts;
			AddDebugVisibilityVerts(debugVerts);
			//AddHeatmapDebugVerts(debugVerts, m_heatmapToQueens, 25.0f);

			int liveAntsNum = m_liveAnts;
			for (int antIndex = 0; antIndex < liveAntsNum; antIndex++) {
				Ant const& ant = m_colony[antIndex];
				if (ant.m_state == STATE_DEAD) continue;
				if (ant.m_type != AGENT_TYPE_SOLDIER)continue;
				AddAStarPathVerts(debugVerts, m_colony[antIndex].m_currentPath, ant.m_type);
			}



			g_debug->QueueDrawVertexArray((int)debugVerts.size(), debugVerts.data());
			g_debug->FlushQueuedDraws();
		}
		g_threadSafe_turnStatus = TURN_STATUS_ORDERS_READY;
	}

}



void Colony::AsyncUpdate()
{
	ColonyJob* jobToExecute = ClaimJobForExecution();
	if (jobToExecute) {
		jobToExecute->Execute();
		delete jobToExecute;
	}
}

TileHeatMap Colony::GetHeatmap(eAgentType agentType, bool lookingForQueen)
{

	TileHeatMap heatmapToReturn = TileHeatMap(IntVec2(1, 1));
	m_heatmapsMutex.lock();

	if (lookingForQueen) {
		heatmapToReturn = m_heatmapToQueens;
	}
	else {
		switch (agentType)
		{
		case AGENT_TYPE_QUEEN:
			heatmapToReturn = m_queenFoodMap;
			break;
		case AGENT_TYPE_SOLDIER:
			heatmapToReturn = m_soldierAttackMap;
		default:
			m_queenFoodMap = m_queenFoodMap;
			break;
		}
	}
	m_heatmapsMutex.unlock();
	return heatmapToReturn;

}
void Colony::UpdateHeatmap(TileHeatMap const& updatedHeatmap, eAgentType agentType, bool lookingForQueen)
{
	m_heatmapsMutex.lock();
	if (lookingForQueen) {
		m_heatmapToQueens = updatedHeatmap;
	}
	else {
		switch (agentType)
		{
		case AGENT_TYPE_QUEEN:
			m_queenFoodMap = updatedHeatmap;
			break;
		case AGENT_TYPE_SOLDIER:
			m_soldierAttackMap = updatedHeatmap;
			break;
		default:
			m_queenFoodMap = updatedHeatmap;
			break;
		}
	}

	m_heatmapsMutex.unlock();
}

void Colony::UpdateOrders(PlayerTurnOrders const& newOrders)
{
	m_ordersMutex.lock();
	m_turnOrders = newOrders;
	m_ordersMutex.unlock();

	m_areCalculationsDone = true;
}

int Colony::GetNutrients() const
{
	return m_currentTurnInfo.currentNutrients;
}

void Colony::GetAStarPath(std::vector<IntVec2>& resultPath, IntVec2 const& start, std::vector<IntVec2> const& goals, eAgentType agentType, int maxLoops, float heuristicWeight, bool seekingFood)
{
	m_aStarsRequested++;
	IntVec2 mapDimensions = IntVec2(m_startupInfo.matchInfo.mapWidth, m_startupInfo.matchInfo.mapWidth);

	TileHeatMap tileScores(mapDimensions);
	tileScores.SetAllValues(FLT_MAX); // All tiles unexplored
	TileHeatMap tileFScores(mapDimensions);
	tileFScores.SetAllValues(FLT_MAX);

	std::vector<IntVec2> openTiles;
	openTiles.reserve(1000);
	openTiles.push_back(start);
	tileScores.SetValue(start, 0.0f);

	IntVec2 closestGoal;
	if (seekingFood) {
		closestGoal = GetClosestFoodGoal(start, goals);
	}
	else {
		closestGoal = GetClosestGoal(start, goals);
	}

	float currentHeuristic = GetAStarHeuristic(start, closestGoal, seekingFood);
	tileFScores.SetValue(start, currentHeuristic);

	// Tile, Came From
	std::map<IntVec2, IntVec2> tilePaths;
	tilePaths[start] = IntVec2(-1, -1); // considered invalid or null as coords cannot be negative
	int currentLoop = 0;
	IntVec2 closestToEnd = start;
	int closestDistToEnd = GetTaxicabDistance2D(start, closestGoal);
	while ((openTiles.size() > 0) && (currentLoop <= maxLoops)) {

		float lowestFScore = FLT_MAX;
		IntVec2 currentTile = openTiles.front();
		int tileInd = 0;

		// Get the tile with lowest F Score
		for (int openTileInd = 0; openTileInd < openTiles.size(); openTileInd++) {
			IntVec2 const& candidateCoords = openTiles[openTileInd];
			float candidateFScore = tileFScores.GetValue(candidateCoords);

			if (candidateFScore < lowestFScore) {
				lowestFScore = candidateFScore;
				currentTile = candidateCoords;
				tileInd = openTileInd;
			}
		}

		float currentScore = tileScores.GetValue(currentTile);

		if (currentTile == closestGoal) {
			ConstructPathForAStar(resultPath, tilePaths, currentTile);
			return;
		}

		// Erase currently explored tile
		openTiles.erase(openTiles.begin() + tileInd);
		int distToEnd = GetTaxicabDistance2D(currentTile, closestGoal);
		if (closestDistToEnd > distToEnd) {
			distToEnd = closestDistToEnd;
			closestToEnd = currentTile;
		}

		IntVec2 tileNeighbors[] = {
		currentTile + IntVec2(1,0),
		currentTile + IntVec2(0,1),
		currentTile + IntVec2(-1,0),
		currentTile + IntVec2(0,-1)
		};

		// The tile must exist in scores to be explored
		for (int neighborIndex = 0; neighborIndex < 4; neighborIndex++) {
			IntVec2 const& neighborTile = tileNeighbors[neighborIndex];
			if (IsTileSolid(neighborTile, agentType)) continue;

			float const& neighborScore = tileScores.GetValue(neighborTile);
			float const& neighborFScore = tileFScores.GetValue(neighborTile);

			float tentativeScore = currentScore + GetTileCost(neighborTile, agentType);
			float tentativeFScore = tentativeScore + (GetAStarHeuristic(neighborTile, closestGoal) * heuristicWeight, seekingFood);

			if (tentativeScore < neighborScore) {
				bool isOpenNode = (neighborFScore == FLT_MAX);

				tileScores.SetValue(neighborTile, tentativeScore);
				tilePaths[neighborTile] = currentTile;

				if (tentativeFScore < neighborFScore) {
					if (isOpenNode) {
						tileFScores.SetValue(neighborTile, tentativeFScore);
						openTiles.push_back(neighborTile);
					}
				}

			}

		}

		currentLoop++;
	}

	// Didn't reach, but returning partial path
	ConstructPathForAStar(resultPath, tilePaths, closestToEnd);

}

void Colony::TryAppointingGuard(Ant* requestingAnt)
{
	for (Ant* soldierAnt : m_soldierAnts) {
		if (!soldierAnt) continue;
		if (!soldierAnt->m_isGuardian) {
			auto const it = m_guardAppointments.find(soldierAnt);
			if (it != m_guardAppointments.end()) continue;

			m_guardAppointments[soldierAnt] = requestingAnt;
			//soldierAnt->GuardAnt(requestingAnt);
			//requestingAnt->AppointGuard(soldierAnt);
			return;
		}
	}

}

void Colony::QueueGuardForRelease(Ant* requestingAnt)
{
	m_guardsToRelease.push_back(requestingAnt);
}

void Colony::AppointAllPendingGuards()
{
	for (std::map<Ant*, Ant*>::const_iterator it = m_guardAppointments.begin(); it != m_guardAppointments.end(); it++) {
		Ant* guard = it->first;
		Ant* guardedAnt = it->second;

		guardedAnt->AppointGuard(guard);
		guard->GuardAnt(guardedAnt);
	}

	m_guardAppointments.clear();
}

void Colony::ReleaseAllPendingGuards()
{
	for (Ant* guardToRelease : m_guardsToRelease) {
		if (guardToRelease->m_guardedAnt) {
			guardToRelease->m_guardedAnt->RemoveGuard(guardToRelease);
		}
		guardToRelease->RelieveFromGuardDuty();
	}
	m_guardsToRelease.clear();
}

bool Colony::CanCalculateAStar() const
{
	return (m_aStarsRequested < 8);
}

bool Colony::CanCalculateAStarSoldiers() const
{
	return (m_aStarsRequestedSoldiers < 8);
}

int Colony::GetNumPlayers() const
{
	return m_startupInfo.matchInfo.numPlayers;
}

ObservedAgent const* Colony::GetClosestEnemy(IntVec2 const& coords) const
{
	int bestDistance = INT_MAX;
	ObservedAgent const* closestEnemy = nullptr;

	for (int observedAgentCount = 0; observedAgentCount < m_currentTurnInfo.numObservedAgents; observedAgentCount++) {
		ObservedAgent const& enemyAgent = m_currentTurnInfo.observedAgents[observedAgentCount];
		IntVec2 enemyCoords = IntVec2(enemyAgent.tileX, enemyAgent.tileY);
		if (IsTileSolid(enemyCoords, AGENT_TYPE_SOLDIER)) continue;

		int distanceToEnemy = GetTaxicabDistance2D(coords, enemyCoords);

		if (closestEnemy) {
			if ((enemyAgent.type > closestEnemy->type)) {
				closestEnemy = &enemyAgent;
				bestDistance = distanceToEnemy;
			}
			else if (enemyAgent.type == closestEnemy->type) {
				if (distanceToEnemy < bestDistance) {
					bestDistance = distanceToEnemy;
					closestEnemy = &enemyAgent;
				}
			}
			continue;
		}
		else {
			closestEnemy = &enemyAgent;
			bestDistance = distanceToEnemy;
		}
	}

	return closestEnemy;
}

ObservedAgent const* Colony::GetClosestEnemyQueen(IntVec2 const& coords) const
{
	if (!m_foundQueen) return nullptr;

	int bestDistance = INT_MAX;
	ObservedAgent const* closestEnemy = nullptr;

	for (int observedAgentCount = 0; observedAgentCount < m_currentTurnInfo.numObservedAgents; observedAgentCount++) {
		ObservedAgent const& enemyAgent = m_currentTurnInfo.observedAgents[observedAgentCount];

		if (enemyAgent.type != AGENT_TYPE_QUEEN) continue;

		IntVec2 enemyCoords = IntVec2(enemyAgent.tileX, enemyAgent.tileY);
		if (IsTileSolid(enemyCoords, AGENT_TYPE_SCOUT)) continue;

		int distanceToEnemy = GetTaxicabDistance2D(coords, enemyCoords);

		if (distanceToEnemy < bestDistance) {
			bestDistance = distanceToEnemy;
			closestEnemy = &enemyAgent;
		}

	}

	return closestEnemy;
}

ObservedAgent const* Colony::GetEnemyIfVisible(AgentID enemyId) const
{
	for (int observedAgentCount = 0; observedAgentCount < m_currentTurnInfo.numObservedAgents; observedAgentCount++) {
		ObservedAgent const& enemyAgent = m_currentTurnInfo.observedAgents[observedAgentCount];
		if (enemyAgent.agentID == enemyId) return &enemyAgent;
	}

	return nullptr;
}


bool Colony::IsAnyQueenBeingAttacked() const
{
	return (m_currentTurnInfo.nutrientsLostDueToQueenDamage > 0) || (m_currentTurnInfo.nutrientsLostDueToQueenSuffocation > 0);
}

bool Colony::IsEnemyAtPos(IntVec2 const& coords) const
{
	float heatmapVal = m_enemyHistoryMap.GetValue(coords);
	return heatmapVal > 3.0f;
}


bool Colony::IsFoodAtPos(IntVec2 const& coords) const
{
	float heatmapVal = m_foodDensityMap.GetValue(coords);
	return heatmapVal > 3.0f;
}

int Colony::GetTileIndex(int x, int y) const
{
	short const& mapWidth = m_startupInfo.matchInfo.mapWidth;
	if (x < 0 || x >= mapWidth) return -1;
	if (y < 0 || y >= mapWidth) return -1;
	return y * mapWidth + x;
}

IntVec2 Colony::GetTileCoords(int tileIndex) const
{
	int tileY = tileIndex / m_startupInfo.matchInfo.mapWidth;
	return IntVec2(tileIndex - tileY * m_startupInfo.matchInfo.mapWidth, tileY);
}

void Colony::ProcessEnemyInfo()
{
	int mapWidth = GetMapWidth();
	int tileSize = mapWidth * mapWidth;
	m_enemyPositions.clear();

	for (int tileIndex = 0; tileIndex < tileSize; tileIndex++) {
		float currentValue = m_enemyHistoryMap.GetValue(tileIndex);
		currentValue--;
		if (currentValue < 0.0f) currentValue = 0.0f;
		m_enemyHistoryMap.SetValue(tileIndex, currentValue);
		if (currentValue > 0.0f) {
			IntVec2 enemyPos = GetTileCoords(tileIndex);
			m_enemyPositions.push_back(enemyPos);
		}
	}

	for (int enemyIndex = 0; enemyIndex < m_currentTurnInfo.numObservedAgents; enemyIndex++) {
		ObservedAgent const& enemy = m_currentTurnInfo.observedAgents[enemyIndex];
		IntVec2 enemyPos = IntVec2(enemy.tileX, enemy.tileY);

		float currentValue = m_enemyHistoryMap.GetValue(enemyPos);
		if (currentValue == 0.0f) {
			m_enemyPositions.push_back(enemyPos);
			currentValue += 4.0f;
		}

		m_enemyHistoryMap.SetValue(enemyPos, currentValue);
	}
}

void Colony::ProcessTurnInfo()
{
	m_heatmapScheduled = false;
	m_workersToBirth = 0;
	m_queensToBirth = 0;
	m_scoutsToBirth = 0;
	m_soldiersToBirth = 0;
	m_scheduledForDeath = 0;
	m_foundQueen = false;

	ProcessEnemyInfo();
	ProcessTilesInfo(m_currentTurnInfo.observedTiles, m_currentTurnInfo.tilesThatHaveFood);

	for (int observedAgentCount = 0; (observedAgentCount < m_currentTurnInfo.numObservedAgents) && !m_foundQueen; observedAgentCount++) {
		ObservedAgent const& enemyAgent = m_currentTurnInfo.observedAgents[observedAgentCount];
		if (enemyAgent.type == AGENT_TYPE_QUEEN) m_foundQueen = true;
	}

	for (int reportIndex = 0; reportIndex < m_currentTurnInfo.numReports; reportIndex++) {
		AgentReport const& currentReport = m_currentTurnInfo.agentReports[reportIndex];
		ProcessReport(currentReport);
	}



	if (!m_heatmapScheduled && m_heatmapToQueensDirty) {
		RecalculateHeatmapToQueens();
	}

	if (!m_heatmapScheduled /*&& (m_lastSoldierUpdate > 3)*/) {
		RecalculateSoldierAttackmap();
	}
	else {
		m_lastSoldierUpdate++;
	}

	/*if (!m_heatmapScheduled && (m_queenFoodMapDirty || (m_lastQueenFoodUpdate >= 10))) {
		RecalculateQueenFoodmap();
	}
	else {
		m_lastQueenFoodUpdate++;
	}*/

	OrderProcessingJob* orderProcessJob = new OrderProcessingJob(this);
	QueueJobForExecution(orderProcessJob);
}

void Colony::GetUnexploredTiles(std::vector<IntVec2>& tileCoordsContainer) const
{
	int mapWidth = m_startupInfo.matchInfo.mapWidth;

	if (m_numKnownTiles == (mapWidth * mapWidth)) return;

	for (int tileY = 0, tileIndex = 0; tileY < mapWidth; tileY++) {
		for (int tileX = 0; tileX < mapWidth; tileX++, tileIndex++) {
			Tile const& tile = m_tiles[tileIndex];
			if (tile.m_type == TILE_TYPE_UNSEEN) {
				tileCoordsContainer.emplace_back(tileX, tileY);
			}
		}
	}

}

void Colony::ProcessTilesInfo(eTileType const* observedTiles, bool const* tilesWithFood)
{
	m_foodCount = 0;
	m_tilesWithFood.clear();
	m_tilesWithFood.reserve(m_solidMap.GetDimensions().x * 4);
	m_unexploredTiles.clear();
	m_foodDensityMipMap.clear();
	m_foodDensityMipMap.resize(16);


	int foodMipMapWidth = m_startupInfo.matchInfo.mapWidth / 4;

	for (int tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++) {
		eTileType const& observedType = observedTiles[tileIndex];
		Tile& tile = m_tiles[tileIndex];

		if (observedType != eTileType::TILE_TYPE_UNSEEN) {
			tile.m_hasFood = tilesWithFood[tileIndex];
		}
		IntVec2 tileCoords = GetTileCoords(tileIndex);

		float currentDensityValue = m_foodDensityMap.GetValue(tileCoords);
		if (tile.m_hasFood) {
			//if (m_solidMap.GetValue(tileCoords) == 0.0f) {
				m_foodCount++;
				m_tilesWithFood.push_back(tileCoords);

				if (currentDensityValue < 0.0f) currentDensityValue = 0.0f;
				currentDensityValue++;
				m_foodDensityMap.SetValue(tileCoords, currentDensityValue);

			//}
		}

		if (currentDensityValue > 0.0f) {
			int mipMapX = tileCoords.x / foodMipMapWidth;
			int mipMapY = tileCoords.y / foodMipMapWidth;

			int mipMapIndex = (mipMapY * 4) + mipMapX;
			m_foodDensityMipMap[mipMapIndex] += 1;
			m_foodCount++;
			m_tilesWithFood.push_back(tileCoords);
		}



		if (observedType == TILE_TYPE_UNSEEN) {
			if (tile.m_type == TILE_TYPE_UNSEEN) {
				m_unexploredTiles.emplace_back(tileCoords);
			}
			continue;
		}


		if (tile.m_type != TILE_TYPE_UNSEEN) continue;
		tile.m_type = observedTiles[tileIndex];
		m_heatmapToQueensDirty = true;
		m_numKnownTiles++;

		if (tile.m_type != TILE_TYPE_STONE) {
			m_solidMap.SetValue(tileCoords, 0.0f);
		}

	}

}

void Colony::ProcessReport(AgentReport const& newReport)
{
	switch (newReport.result)
	{
	default: {
		ProcessAndCopyInfo(newReport);
	}
		   break;
	case AGENT_ORDER_SUCCESS_DUG:
		ProcessAgentDug(newReport);
		break;
	case AGENT_ORDER_SUCCESS_PICKUP:
		ProcessAgentPickup(newReport);
		break;
	case AGENT_ORDER_SUCCESS_HELD:
		ProcessAgentHold(newReport);
		break;
	case AGENT_WAS_CREATED:
		ProcessAgentCreation(newReport);
		break;
	case AGENT_ORDER_SUCCESS_MOVED:
		ProcessAgentMoved(newReport);
		break;
	case AGENT_ORDER_SUCCESS_SUICIDE:
	case AGENT_KILLED_BY_ENEMY:
	case AGENT_KILLED_BY_PENALTY:
	case AGENT_KILLED_BY_STARVATION:
	case AGENT_KILLED_BY_SUFFOCATION:
	case AGENT_KILLED_BY_WATER:
		ProcessAgentDeath(newReport);
		break;
	}
}

void Colony::ProcessAgentCreation(AgentReport const& newReport)
{
	Ant& newAnt = GetFreeAntSlot();
	if (newAnt.m_state == STATE_DEAD) {
		newAnt.m_colony = this;

		CopyReportInfo(newAnt, newReport);
		m_liveAnts++;
		RandomNumberGenerator randomGen;

		switch (newAnt.m_type)
		{
		default:
			break;
		case AGENT_TYPE_SCOUT:
			m_liveScouts++;
			break;
		case AGENT_TYPE_WORKER:
			m_liveWorkers++;
			newAnt.m_canDig = (randomGen.GetRandomFloatZeroUpToOne() < 0.25f);
			break;
		case AGENT_TYPE_SOLDIER:
			m_liveSoldiers++;
			AddSoldierAnt(&newAnt);
			break;
		case AGENT_TYPE_QUEEN:
			m_liveQueens++;
			m_heatmapToQueensDirty = true;
			break;
		}

	}
}

void Colony::ProcessAgentHold(AgentReport const& newReport)
{
	bool retrievedCorrectAnt = false;

	Ant& ant = GetAnt(newReport.agentID, retrievedCorrectAnt);

	if (retrievedCorrectAnt) {
		CopyReportInfo(ant, newReport);
	}
}

void Colony::ProcessAgentMoved(AgentReport const& newReport)
{
	bool retrievedCorrectAnt = false;

	Ant& ant = GetAnt(newReport.agentID, retrievedCorrectAnt);

	if (retrievedCorrectAnt) {
		CopyReportInfo(ant, newReport);
		if (ant.m_type == AGENT_TYPE_QUEEN) {
			m_heatmapToQueensDirty = true;
			m_queenFoodMapDirty = true;
		}
	}
}

void Colony::ProcessAgentDeath(AgentReport const& newReport)
{
	bool retrievedCorrectAnt = false;

	Ant& ant = GetAnt(newReport.agentID, retrievedCorrectAnt);
	if (retrievedCorrectAnt) {
		for (Ant* guard : ant.m_guards) {
			if (guard->m_guardedAnt) {
				guard->m_guardedAnt->RemoveGuard(guard);
			}
			guard->RelieveFromGuardDuty();
		}

		ant.m_state = STATE_DEAD;
		m_liveAnts--;

		switch (ant.m_type)
		{
		default:
			break;
		case AGENT_TYPE_SCOUT:
			m_liveScouts--;
			break;
		case AGENT_TYPE_WORKER:
			m_liveWorkers--;
			if (ant.m_currentPath.size() > 0) {
				m_foodDensityMap.SetValue(ant.m_currentPath[0], 1.0f);
			}
			break;
		case AGENT_TYPE_SOLDIER:
			m_liveSoldiers--;
			RemoveSoldierAnt(&ant);
			break;
		case AGENT_TYPE_QUEEN:
			m_liveQueens--;
			m_heatmapToQueensDirty = true;
			break;
		}
	}
}

void Colony::ProcessAgentPickup(AgentReport const& newReport)
{
	IntVec2 tileCoords = IntVec2(newReport.tileX, newReport.tileY);
	bool retrievedCorrectAnt = false;

	Ant& ant = GetAnt(newReport.agentID, retrievedCorrectAnt);
	if (retrievedCorrectAnt) {
		CopyReportInfo(ant, newReport);
	}

	/*auto node = m_tilesWithUnclaimedFood.extract(tileCoords);*/
	//if (!node.empty()) {
	m_workerFoodmapDirty = true;
	//}
}

void Colony::ProcessAgentDug(AgentReport const& newReport)
{
	IntVec2 tileCoords = IntVec2(newReport.tileX, newReport.tileY);
	bool retrievedCorrectAnt = false;

	Ant& ant = GetAnt(newReport.agentID, retrievedCorrectAnt);
	if (retrievedCorrectAnt) {
		CopyReportInfo(ant, newReport);
		int tileindex = GetTileIndex(newReport.tileX, newReport.tileY);
		Tile& tileDug = m_tiles[tileindex];
		tileDug.m_type = TILE_TYPE_AIR;
	}
	m_workerFoodmapDirty = true;

}

void Colony::ProcessAndCopyInfo(AgentReport const& newReport)
{
	bool retrievedCorrectAnt = false;

	Ant& ant = GetAnt(newReport.agentID, retrievedCorrectAnt);
	if (retrievedCorrectAnt) {
		CopyReportInfo(ant, newReport);
	}
}

void Colony::CopyReportInfo(Ant& ant, AgentReport const& agentReport)
{
	ant.m_agentID = agentReport.agentID;
	ant.m_combatDamage += agentReport.receivedCombatDamage;
	ant.m_suffocationDamage += agentReport.receivedSuffocationDamage;
	ant.m_state = agentReport.state;
	ant.m_tileX = agentReport.tileX;
	ant.m_tileY = agentReport.tileY;
	ant.m_type = agentReport.type;
	ant.m_exhaustion = agentReport.exhaustion;
}

void Colony::AddDebugVisibilityVerts(std::vector<VertexPC>& verts)
{
	float quadSize = 1.0f;
	for (int tileX = 0; tileX < m_startupInfo.matchInfo.mapWidth; tileX++) {
		for (int tileY = 0; tileY < m_startupInfo.matchInfo.mapWidth; tileY++) {

			int tileIndex = GetTileIndex(tileX, tileY);
			Tile& tile = m_tiles[tileIndex];

			if (tile.m_type != TILE_TYPE_UNSEEN) {
				AddVertsForSquare(verts, quadSize, tileX, tileY, Color8(255, 0, 0, 100));
			}

		}
	}
}

void Colony::AddHeatmapDebugVerts(std::vector<VertexPC>& verts, TileHeatMap const& heatMap, float maxVisibleValue)
{
	m_heatmapsMutex.lock();
	float quadSize = 1.0f;
	for (int tileX = 0; tileX < m_startupInfo.matchInfo.mapWidth; tileX++) {
		for (int tileY = 0; tileY < m_startupInfo.matchInfo.mapWidth; tileY++) {

			int tileIndex = GetTileIndex(tileX, tileY);
			float heatmapValue = heatMap.GetValue(tileIndex);
			if (heatmapValue == FLT_MAX) continue;
			float colorIntegrity = 1.0f;
			if (heatmapValue > 0) {
				colorIntegrity = maxVisibleValue - heatmapValue;
				colorIntegrity = (colorIntegrity < 0.0f) ? 0.0f : colorIntegrity;
				colorIntegrity /= maxVisibleValue;
			}

			unsigned char finalColor = DenormalizeByte(colorIntegrity);


			AddVertsForSquare(verts, quadSize, tileX, tileY, Color8(finalColor, finalColor, finalColor, 100));

		}
	}
	m_heatmapsMutex.unlock();
}



void Colony::AddAStarPathVerts(std::vector<VertexPC>& verts, std::vector<IntVec2> const& aStarPath, eAgentType agentType)
{
	Color8 color(0, 255, agentType * 30, 50);

	for (IntVec2 const& coords : aStarPath) {
		AddVertsForSquare(verts, 1.0f, coords.x, coords.y, color);
	}
}

void Colony::AddVertsForSquare(std::vector<VertexPC>& verts, float size, int x, int y, Color8 color)
{
	float halfSize = size * 0.5f;
	VertexPC bottomLeft = { x - halfSize, y - halfSize, color };
	VertexPC bottomRight = { x + halfSize, y - halfSize, color };
	VertexPC topLeft = { x - halfSize, y + halfSize, color };
	VertexPC topRight = { x + halfSize, y + halfSize, color };

	verts.push_back(bottomLeft); // bottom left
	verts.push_back(topRight); // top right 
	verts.push_back(topLeft); // top left 

	verts.push_back(bottomLeft); // bottom left
	verts.push_back(bottomRight); // bottom right 

	verts.push_back(topRight); // top right 
}

void Colony::RecalculateHeatmapToQueens()
{
	std::vector<IntVec2> goals;
	for (int liveAntIndex = 0, antIndex = 0; liveAntIndex < (int)m_liveAnts; antIndex++) {
		Ant const& ant = m_colony[antIndex];
		if (ant.m_state == STATE_DEAD) continue;
		else liveAntIndex++;

		if (ant.m_type == AGENT_TYPE_QUEEN) {
			IntVec2 queenPos = IntVec2(ant.m_tileX, ant.m_tileY);
			goals.push_back(queenPos);
		}
	}

	HeatmapUpdateJob* heatmapUpdateJob = new HeatmapUpdateJob(this, m_heatmapToQueens.GetDimensions(), goals, AGENT_TYPE_WORKER, true);
	QueueJobForExecution(heatmapUpdateJob);
	//RecalculateHeatMap(m_heatmapToQueens, AGENT_TYPE_WORKER);
	m_heatmapToQueensDirty = false;
	m_heatmapScheduled = true;
}

//void Colony::RecalculateWorkerFoodmap()
//{
//	int mapWidth = m_startupInfo.matchInfo.mapWidth;
//	std::vector<IntVec2> workerGoals;
//	int reserveSize = mapWidth * 2;
//	workerGoals.reserve(reserveSize);
//
//	int tileIndex = 0;
//	for (int tileY = 0; tileY < mapWidth; tileY++) {
//		for (int tileX = 0; tileX < mapWidth; tileX++, tileIndex++) {
//			Tile const& tile = m_tiles[tileIndex];
//			if (tile.m_hasFood) {
//				workerGoals.emplace_back(tileX, tileY);
//			}
//		}
//	}
//
//	HeatmapUpdateJob* workerFoodmapUpate = new HeatmapUpdateJob(this, m_workerFoodMap.GetDimensions(), workerGoals, eAgentType::AGENT_TYPE_WORKER, false);
//	QueueJobForExecution(workerFoodmapUpate);
//	m_workerFoodmapDirty = false;
//	m_heatmapScheduled = true;
//
//
//}

void Colony::RecalculateQueenFoodmap()
{
	int mapWidth = m_startupInfo.matchInfo.mapWidth;
	std::vector<IntVec2> queenGoals;
	int reserveSize = mapWidth * 2;
	queenGoals.reserve(reserveSize);

	//int tileIndex = 0;
	//for (int tileY = 0; tileY < mapWidth; tileY++) {
	//	for (int tileX = 0; tileX < mapWidth; tileX++, tileIndex++) {
	//		Tile const& tile = m_tiles[tileIndex];
	//		if ((tile.m_hasFood) && (tile.m_type != TILE_TYPE_WATER)) {
	//			queenGoals.emplace_back(tileX, tileY);
	//		}
	//	}
	//}

	for (IntVec2 const& tileWithFood : m_tilesWithFood) {
		int tileIndex = GetTileIndex(tileWithFood.x, tileWithFood.y);
		Tile const& tile = m_tiles[tileIndex];

		if (tile.m_type != TILE_TYPE_WATER) {
			queenGoals.emplace_back(tileWithFood);
		}
	}

	HeatmapUpdateJob* queenFoodmapUpdate = new HeatmapUpdateJob(this, m_queenFoodMap.GetDimensions(), queenGoals, eAgentType::AGENT_TYPE_QUEEN, false);
	QueueJobForExecution(queenFoodmapUpdate);
	m_queenFoodMapDirty = false;
	m_lastQueenFoodUpdate = 0;
	m_heatmapScheduled = true;
}

void Colony::RecalculateSoldierAttackmap()
{
	HeatmapUpdateJob* soldierUpdate = new HeatmapUpdateJob(this, m_soldierAttackMap.GetDimensions(), m_enemyPositions, eAgentType::AGENT_TYPE_SOLDIER, false);
	QueueJobForExecution(soldierUpdate);

	m_heatmapScheduled = true;
	m_lastSoldierUpdate = 0;
}

void Colony::RecalculateHeatMap(TileHeatMap& heatmap, eAgentType agentType)
{
	int mapWidth = (int)m_startupInfo.matchInfo.mapWidth;

	bool modifiedAnyTile = true;
	float currentValue = 0.0f;
	while (modifiedAnyTile) {
		modifiedAnyTile = false;
		for (int tileY = 0; tileY < mapWidth; tileY++) {
			for (int tileX = 0; tileX < mapWidth; tileX++) {
				int tileIndex = GetTileIndex(tileX, tileY);

				float const& tileValue = heatmap.GetValue(tileIndex);
				if (tileValue != currentValue) continue;
				if (IsTileSolid(tileIndex, agentType)) continue;

				int northTileIndex = GetTileIndex(tileX, tileY + 1);
				int southTileIndex = GetTileIndex(tileX, tileY - 1);
				int eastTileIndex = GetTileIndex(tileX + 1, tileY);
				int westTileIndex = GetTileIndex(tileX - 1, tileY);

				bool modifiedNorth = (northTileIndex > -1) && (SetTileHeatmapValue(heatmap, agentType, northTileIndex, currentValue));
				bool modifiedSouth = (southTileIndex > -1) && (SetTileHeatmapValue(heatmap, agentType, southTileIndex, currentValue));
				bool modifiedEast = (eastTileIndex > -1) && (SetTileHeatmapValue(heatmap, agentType, eastTileIndex, currentValue));
				bool modifiedWest = (westTileIndex > -1) && (SetTileHeatmapValue(heatmap, agentType, westTileIndex, currentValue));

				modifiedAnyTile = modifiedAnyTile || modifiedNorth || modifiedSouth || modifiedEast || modifiedWest;

			}
		}

		currentValue++;
	}
}



void Colony::QueueJobForExecution(ColonyJob* newJob)
{
	m_queuedJobsMutex.lock();
	m_queuedJobs.push_back(newJob);
	m_queuedJobsMutex.unlock();
	m_queuedJobsAmount++;
}

ColonyJob* Colony::ClaimJobForExecution()
{
	ColonyJob* jobToExecute = nullptr;

	m_queuedJobsMutex.lock();
	if (!m_queuedJobs.empty()) {
		bool foundJob = false;
		std::deque<ColonyJob*>::iterator foundJobIt;
		for (std::deque<ColonyJob*>::iterator dequeIt = m_queuedJobs.begin(); dequeIt != m_queuedJobs.end() && !foundJob; dequeIt++) {
			if (*dequeIt) {
				jobToExecute = *dequeIt;
				foundJob = true;
				foundJobIt = dequeIt;
			}
		}
		if (foundJob) {
			m_queuedJobs.erase(foundJobIt);
		}
	}
	m_queuedJobsMutex.unlock();

	if (jobToExecute) {
		m_queuedJobsAmount--;
	}

	return jobToExecute;
}



bool Colony::SetTileHeatmapValue(TileHeatMap& heatmap, eAgentType agentType, int tileIndex, float currentValue)
{
	if (IsTileSolid(tileIndex, agentType)) return false;

	float const& tileValue = heatmap.GetValue(tileIndex);
	float tileCost = GetTileCost(tileIndex, agentType);
	float possibleValue = currentValue + tileCost;
	if (possibleValue < tileValue) {
		heatmap.SetValue(tileIndex, possibleValue);
		return true;
	}

	return false;
}

bool Colony::IsTileSolid(int tileIndex, eAgentType agentType) const
{
	Tile const& tile = m_tiles[tileIndex];

	unsigned char tileTypeMask = (1 << tile.m_type);
	if (tile.m_type == TILE_TYPE_UNSEEN) return false;

	unsigned char comparisonResult = 0;

	switch (agentType)
	{
	case AGENT_TYPE_SCOUT:
		comparisonResult = SCOUT_SOLIDNESS & tileTypeMask;
		break;
	case AGENT_TYPE_WORKER:
		comparisonResult = WORKER_SOLIDNESS & tileTypeMask;
		break;
	case AGENT_TYPE_SOLDIER:
		comparisonResult = SOLDIER_SOLIDNESS & tileTypeMask;
		break;
	case AGENT_TYPE_QUEEN:
		comparisonResult = QUEEN_SOLIDNESS & tileTypeMask;
		break;
	default:
		return false;
		break;
	}

	bool isSolid = (comparisonResult != 0);

	return isSolid;

}

bool Colony::IsTileSolid(IntVec2 const& tileCoords, eAgentType agentType) const
{
	int index = GetTileIndex(tileCoords.x, tileCoords.y);
	if (index == -1) return true;
	else return IsTileSolid(index, agentType);
}

bool Colony::IsTileDirt(int tileIndex) const
{
	Tile const& tile = m_tiles[tileIndex];
	return tile.m_type == TILE_TYPE_DIRT;
}

bool Colony::IsTileDirt(IntVec2 const& tileCoords) const
{
	int index = GetTileIndex(tileCoords.x, tileCoords.y);
	if (index == -1) return false;
	else return IsTileDirt(index);
}

int Colony::GetCostToBirth(eAgentType agentType) const
{
	return m_startupInfo.matchInfo.agentTypeInfos[agentType].costToBirth;
}

int Colony::GetSightRadius(eAgentType agentType) const
{
	return m_startupInfo.matchInfo.agentTypeInfos[agentType].visibilityRange;
}

int Colony::GetFoodDensityInRadius(IntVec2 const& startPos, int radius) const
{
	int foodDensity = 0;
	for (int tileX = 0; tileX < radius; tileX++) {
		for (int tileY = 0; tileY < radius; tileY++) {
			IntVec2 resultingCoords = startPos + IntVec2(tileX, tileY);
			int tileIndex = GetTileIndex(resultingCoords.x, resultingCoords.y);

			if (tileIndex == -1) continue;
			Tile const& tile = m_tiles[tileIndex];
			if (tile.m_hasFood) {
				foodDensity++;
			}
		}
	}

	return foodDensity;
}

void Colony::ScheduleForBirth(eAgentType agentType)
{
	switch (agentType)
	{
	case AGENT_TYPE_SCOUT:
		m_scoutsToBirth++;
		break;
	case AGENT_TYPE_WORKER:
		m_workersToBirth++;
		break;
	case AGENT_TYPE_SOLDIER:
		m_soldiersToBirth++;
		break;
	case AGENT_TYPE_QUEEN:
		m_queensToBirth++;
		break;
	}
}

void Colony::ScheduleForDeath(eAgentType agentType)
{
	m_scheduledForDeath |= (1 << agentType);
}

int Colony::HowManyWillBeBorn(eAgentType agentType) const
{
	switch (agentType)
	{
	case AGENT_TYPE_SCOUT:
		return m_scoutsToBirth;
	case AGENT_TYPE_WORKER:
		return m_workersToBirth;
	case AGENT_TYPE_SOLDIER:
		return m_soldiersToBirth;
	case AGENT_TYPE_QUEEN:
		return m_queensToBirth;
	}

	return 0;
}

bool Colony::IsScheduledForDeath(eAgentType agentType) const
{
	unsigned char isScheduled = (m_scheduledForDeath & (1 << agentType));

	return isScheduled > 0;
}

IntVec2 Colony::GetRandNonSolidCoords() const
{
	return m_solidMap.GetRandomValue(1.0f);
}

std::vector<IntVec2>& Colony::GetTilesWithFood()
{
	return m_tilesWithFood;
}


void Colony::FlagFoodAsTaken(IntVec2 const& coords)
{
	int tileIndex = GetTileIndex(coords.x, coords.y);
	if (tileIndex == -1) return;
	//m_tiles[tileIndex].m_hasFood = false;

	m_foodDensityMap.SetValue(tileIndex, -1.0f);
	/*for (IntVec2& tileCoords : m_tilesWithFood) {
		if (tileCoords == coords) {
			tileCoords = IntVec2(-1, -1);
		}
	}*/
}

void Colony::ResetFoodDensity(IntVec2 const& coords)
{
	int tileIndex = GetTileIndex(coords.x, coords.y);
	if (tileIndex == -1) return;
	m_foodDensityMap.SetValue(tileIndex, 0.0f);
}

void Colony::RegisterSoldierAStartRequest()
{
	m_aStarsRequestedSoldiers++;
}

float Colony::GetTileCost(int tileIndex, eAgentType agentType) const
{
	if (IsTileSolid(tileIndex, agentType)) return FLT_MAX;

	Tile const& tile = m_tiles[tileIndex];

	if (tile.m_type == TILE_TYPE_WATER) return 6.0f;
	if (tile.m_type == TILE_TYPE_DIRT) {
		if (agentType == AGENT_TYPE_SCOUT) {
			return 1.0f;
		}
		return 2.0f;
	}

	return 1.0f;
}

float Colony::GetTileCost(IntVec2 const& tileCoords, eAgentType agentType) const
{
	int tileIndex = GetTileIndex(tileCoords.x, tileCoords.y);

	if (tileIndex == -1) return FLT_MAX;
	else return GetTileCost(tileIndex, agentType);
}


void Colony::ConstructPathForAStar(std::vector<IntVec2>& pathContainer, std::map<IntVec2, IntVec2> const& cameFromMap, IntVec2 const& goalCoords) const
{
	pathContainer.push_back(goalCoords);

	IntVec2 currentCoords = goalCoords;

	// There should definitely be a goal in the map
	while (currentCoords != IntVec2(-1, -1)) { // Considered null
		pathContainer.push_back(currentCoords);
		currentCoords = cameFromMap.at(currentCoords);
	}
}

float Colony::GetAStarHeuristic(IntVec2 const& tileCoords, std::vector<IntVec2> const& goals, bool avoidEnemies)
{
	int bestHeuristicValue = INT_MAX;
	IntVec2 bestCoords;
	for (IntVec2 const& goalCoords : goals) {
		if (goalCoords == IntVec2(-1, -1)) continue;
		int heuristic = GetTaxicabDistance2D(tileCoords, goalCoords);
		if (heuristic < bestHeuristicValue) {
			bestHeuristicValue = heuristic;
			bestCoords = goalCoords;
		}

	}

	float enemyValue = (avoidEnemies) ? m_enemyHistoryMap.GetValue(bestCoords) : 0.0f;

	return (float)bestHeuristicValue + enemyValue;
}

float Colony::GetAStarHeuristic(IntVec2 const& tileCoords, IntVec2 const& goal, bool avoidEnemies)
{
	float enemyValue = (avoidEnemies) ? m_enemyHistoryMap.GetValue(tileCoords) : 0.0f;
	return (float)GetTaxicabDistance2D(tileCoords, goal) + enemyValue;
}

IntVec2 Colony::GetClosestGoal(IntVec2 const& tilecoords, std::vector<IntVec2> const& goals)
{
	int bestDistance = INT_MAX;
	IntVec2 closestGoal = IntVec2(-1, -1);
	for (IntVec2 const& goalCoords : goals) {
		if (goalCoords == IntVec2(-1, -1)) continue;
		int distToGoal = GetTaxicabDistance2D(tilecoords, goalCoords);
		if (distToGoal < bestDistance) {
			bestDistance = distToGoal;
			closestGoal = goalCoords;
		}
	}

	return closestGoal;
}

IntVec2 Colony::GetClosestFoodGoal(IntVec2 const& tilecoords, std::vector<IntVec2> const& goals)
{
	int bestDistance = INT_MAX;
	IntVec2 closestGoal = IntVec2(-1, -1);
	for (IntVec2 const& goalCoords : goals) {
		if (m_foodDensityMap.GetValue(goalCoords) < 0.0f) continue;
		int distToGoal = (int)GetAStarHeuristic(tilecoords, goalCoords);
		if (distToGoal < bestDistance) {
			bestDistance = distToGoal;
			closestGoal = goalCoords;
		}
	}

	return closestGoal;
}

void Colony::AddSoldierAnt(Ant* soldierAnt)
{
	for (Ant*& ant : m_soldierAnts) {
		if (!ant) {
			ant = soldierAnt;
			return;
		}
	}

	m_soldierAnts.push_back(soldierAnt);
}

void Colony::RemoveSoldierAnt(Ant* antToRemove)
{
	for (Ant*& ant : m_soldierAnts) {
		if (!ant) continue;
		if (ant->m_agentID == antToRemove->m_agentID) {
			ant = nullptr;
		}
	}
}

Ant& Colony::GetFreeAntSlot()
{
	for (int antIndex = 0; antIndex < MAX_AGENTS_PER_PLAYER; antIndex++) {
		Ant& currentSlot = m_colony[antIndex];
		if (currentSlot.m_state == STATE_DEAD) {
			return currentSlot;
		}
	}

	return m_colony[0];
}


Ant& Colony::GetAnt(AgentID agentId, bool& out_wasSuccessful)
{
	for (int antIndex = 0; antIndex < MAX_AGENTS_PER_PLAYER; antIndex++) {
		Ant& currentAnt = m_colony[antIndex];
		if (currentAnt.m_agentID == agentId) {
			out_wasSuccessful = true;
			return currentAnt;
		}
	}

	return m_colony[0];
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

IntVec2 Colony::GetHighestFoodDensityCoords() const
{

	int highestDensityInd = 0;
	int highestDensity = 0;
	int foodMipMapWidth = m_startupInfo.matchInfo.mapWidth / 4;

	for (int densityIndex = 0; densityIndex < 16; densityIndex++) {
		int const& density = m_foodDensityMipMap[densityIndex];

		if (highestDensity < density) {
			highestDensity = density;
			highestDensityInd = densityIndex;
		}
	}


	int coordY = highestDensityInd / 4;
	int coordX = highestDensityInd % 4;


	int halfMipMapWidth = foodMipMapWidth / 2;
	IntVec2 resultingCoords = IntVec2(coordX, coordY) * foodMipMapWidth;
	resultingCoords += IntVec2(halfMipMapWidth, halfMipMapWidth);


	bool isCenterSolid = IsTileSolid(resultingCoords, AGENT_TYPE_QUEEN);

	if (isCenterSolid) {
		for (int radiusX = 0; radiusX < halfMipMapWidth; radiusX++) {
			for (int radiusY = 0; radiusY < halfMipMapWidth; radiusY++) {
				IntVec2 altResult1 = resultingCoords + IntVec2(radiusX, radiusY);
				IntVec2 altResult2 = resultingCoords + IntVec2(-radiusX, radiusY);
				IntVec2 altResult3 = resultingCoords + IntVec2(radiusX, -radiusY);
				IntVec2 altResult4 = resultingCoords + IntVec2(-radiusX, -radiusY);

				if (!IsTileSolid(altResult1, AGENT_TYPE_QUEEN)) return altResult1;
				if (!IsTileSolid(altResult2, AGENT_TYPE_QUEEN)) return altResult2;
				if (!IsTileSolid(altResult3, AGENT_TYPE_QUEEN)) return altResult3;
				if (!IsTileSolid(altResult4, AGENT_TYPE_QUEEN)) return altResult4;
			}
		}
	}

	return resultingCoords;
}
