#include "Ant.hpp"
#include "Colony.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include <Engine/Math/RandomNumberGenerator.hpp>

// 0b111 Corpse Water Stone Dirt Air
unsigned char SCOUT_SOLIDNESS = (0b11100100);
unsigned char WORKER_SOLIDNESS = (0b11100100);
unsigned char SOLDIER_SOLIDNESS = (0b11100110);
unsigned char QUEEN_SOLIDNESS = (0b11101100);

eOrderCode GetMoveOrderForDir(IntVec2 const direction) {
	if (direction.x == -1) {
		return ORDER_MOVE_WEST;
	}
	if (direction.x == 1) {
		return ORDER_MOVE_EAST;
	}
	if (direction.y == 1) {
		return ORDER_MOVE_NORTH;
	}
	if (direction.y == -1) {
		return ORDER_MOVE_SOUTH;
	}

	return ORDER_HOLD;
}

eOrderCode Ant::GetOrder()
{
	if (m_exhaustion > 0) {
		return ORDER_HOLD;
	}

	switch (m_type)
	{
	case AGENT_TYPE_SCOUT:
		return GetOrderScout();
	case AGENT_TYPE_WORKER:
		return GetOrderWorker();
	case AGENT_TYPE_SOLDIER:
		return GetOrderSoldier();
	case AGENT_TYPE_QUEEN:
		return GetOrderQueen();
	default:
		return GetOrderSoldier();
	}
}

void Ant::AppointGuard(Ant* guard)
{
	m_guards.push_back(guard);
}

void Ant::RemoveGuard(Ant* guard)
{
	int guardIndex = 0;
	bool foundGuard = false;

	for (; (guardIndex < m_guards.size()) && !foundGuard;) {
		Ant const* currentGuard = m_guards[guardIndex];

		foundGuard = (currentGuard->m_agentID == guard->m_agentID);
		if (!foundGuard) {
			guardIndex++;
		}
	}

	if (foundGuard) {
		m_guards.erase(m_guards.begin() + guardIndex);
	}

}

void Ant::GuardAnt(Ant* ant)
{
	m_isGuardian = true;
	m_isChasingEnemy = false;
	m_guardedAnt = ant;
}

void Ant::RelieveFromGuardDuty()
{
	m_isGuardian = false;
	m_guardedAnt = nullptr;
}

eOrderCode Ant::GetOrderQueen()
{
	m_nextMove = ORDER_HOLD;
	if ((m_colony->GetTurnNumber() % 100) == 0) return ORDER_EMOTE_HAPPY;
	eOrderCode orderToReturn = ORDER_HOLD;
	IntVec2 currentPos = IntVec2(m_tileX, m_tileY);


	int guardsPerQueen = 3;
	int numAnts = m_colony->m_liveAnts;
	int nutrients = m_colony->GetNutrients();

	int numWorkers = m_colony->m_liveWorkers;
	int thresholdBirthWorkers = (m_colony->GetCostToBirth(AGENT_TYPE_WORKER) * 3) / 2;

	int numScouts = m_colony->m_liveScouts;
	int thresholdBirthScouts = (m_colony->GetCostToBirth(AGENT_TYPE_SCOUT) * 3) / 2;

	int numQueens = m_colony->m_liveQueens;
	int thresholdBirthQueens = (m_colony->GetCostToBirth(AGENT_TYPE_QUEEN) * 3) / 2;

	int numSoldiers = m_colony->m_liveSoldiers;
	int thresholdBirthSoldiers = (m_colony->GetCostToBirth(AGENT_TYPE_SOLDIER) * 3) / 2;

	float nutrientsCoeff = (float)nutrients / (float)numAnts;
	float workersNutrientCoeff = (float)nutrients / (float)numWorkers;
	float queensCoeff = (float)nutrients / (float)numQueens;
	float maxQueensCoeff = (float)nutrients / ((float)m_colony->GetCostToBirth(AGENT_TYPE_QUEEN) * 1.5f);


	int maxWorkers = (numQueens * 12);
	//int maxWorkers = 1;
	//int maxWorkers = 10;
	int maxScouts = 5;
	int maxQueens = RoundDownToInt(maxQueensCoeff);
	if (maxQueens > 15) maxQueens = 15;
	int maxSoldiers = (guardsPerQueen * numQueens) + numWorkers;
	int numPlayers = m_colony->GetNumPlayers();
	if (numPlayers < 2) maxSoldiers = 0;

	if (m_guards.size() < guardsPerQueen) {
		m_colony->TryAppointingGuard(this);;
	}


	if (m_lastGaveBirth > 2) {

		if (numSoldiers < (maxSoldiers) && (nutrientsCoeff >= thresholdBirthSoldiers)) {
			int soldiersToBirth = m_colony->HowManyWillBeBorn(AGENT_TYPE_SOLDIER);
			if ((numSoldiers + soldiersToBirth) < maxSoldiers) {
				orderToReturn = ORDER_BIRTH_SOLDIER;
				m_colony->ScheduleForBirth(AGENT_TYPE_SOLDIER);

				if (numSoldiers == 0) {
					m_lastGaveBirth = 0;
					return orderToReturn;
				}

			}
		}

		if ((numQueens < maxQueens) && (queensCoeff >= thresholdBirthQueens)) {
			int queensToBirth = m_colony->HowManyWillBeBorn(AGENT_TYPE_QUEEN);
			if ((queensToBirth + numQueens) < maxQueens) {
				orderToReturn = ORDER_BIRTH_QUEEN;
				m_colony->ScheduleForBirth(AGENT_TYPE_QUEEN);
				m_lastGaveBirth = 0;
			}
		}

		if ((numScouts < maxScouts) && (nutrientsCoeff >= thresholdBirthScouts)) {
			int scoutsToBirth = m_colony->HowManyWillBeBorn(AGENT_TYPE_SCOUT);
			if ((numScouts + scoutsToBirth) < maxScouts) {
				orderToReturn = ORDER_BIRTH_SCOUT;
				m_colony->ScheduleForBirth(AGENT_TYPE_SCOUT);
				m_lastGaveBirth = 0;
			}
		}

		if ((numWorkers == 0) || ((numWorkers < maxWorkers) && (workersNutrientCoeff >= thresholdBirthWorkers))) {
			if (numWorkers < ((int)m_colony->m_foodCount * 2)) {
				int workersToBirth = m_colony->HowManyWillBeBorn(AGENT_TYPE_WORKER);
				if ((numWorkers + workersToBirth) < maxWorkers) {
					orderToReturn = ORDER_BIRTH_WORKER;
					m_colony->ScheduleForBirth(AGENT_TYPE_WORKER);

					if (numWorkers == 0) {
						m_lastGaveBirth = 0;
						return orderToReturn;
					}
				}
			}
		}


	}
	else {
		m_lastGaveBirth++;
	}

	if (orderToReturn <= 4) {

		IntVec2 highestFoodDensityCoord = m_colony->GetHighestFoodDensityCoords();
		int mapWidth = m_colony->GetMapWidth();
		int minDist = mapWidth / 8;

		int distToGoal = GetTaxicabDistance2D(currentPos, highestFoodDensityCoord);
		if (distToGoal < minDist) {
			orderToReturn = eOrderCode::ORDER_HOLD;
			m_currentPathIndex = 0;
			m_currentGoal = IntVec2(-1, -1);
		}
		else {
			if ((m_currentPathIndex == 0) || (m_currentPath.size() == 0)) {
				std::vector<IntVec2> goals = { highestFoodDensityCoord };
				m_currentPath.clear();
				m_colony->GetAStarPath(m_currentPath, currentPos, goals, eAgentType::AGENT_TYPE_QUEEN, mapWidth * 10);
				if (m_currentPath.size() == 0) {
					RandomNumberGenerator randOff;
					IntVec2 randOffset = currentPos;
					randOffset.x += randOff.GetRandomIntInRange(-8, 8);
					randOffset.y += randOff.GetRandomIntInRange(-8, 8);

					std::vector<IntVec2> randGoal = { randOffset };
					m_colony->GetAStarPath(m_currentPath, currentPos, randGoal, eAgentType::AGENT_TYPE_QUEEN, mapWidth * 5, 1.5f);

				}

				if (m_currentPath.size() > 0) {
					m_currentPathIndex = (int)m_currentPath.size() - 1;
				}

			}
			if (m_movedPreviousTurn > 3) {
				if (m_currentPathIndex > 0) {
					m_currentPathIndex--;
					IntVec2 nextMove = m_currentPath[m_currentPathIndex];
					IntVec2 dirToMove = nextMove - currentPos;
					orderToReturn = GetMoveOrderForDir(dirToMove);
					m_nextMove = orderToReturn;
					m_movedPreviousTurn = 0;
				}
			}
			else {
				m_movedPreviousTurn++;
			}
		}
	}




	return orderToReturn;
}

eOrderCode Ant::GetOrderSoldier()
{
	//if (m_colony->IsAnyQueenBeingAttacked()) {
	//	TileHeatMap const& heatmap = m_colony->GetHeatmap(AGENT_TYPE_SOLDIER, true);
	//	IntVec2 currentPos = IntVec2(m_tileX, m_tileY);
	//	IntVec2 nextLowest = heatmap.GetCoordsForNextLowestValue(currentPos);
	//	IntVec2 moveDir = nextLowest - currentPos;
	//	return GetMoveOrderForDir(moveDir);
	//}

	if (m_isGuardian) {
		return GetOrderGuardianSoldier();
	}

	return GetOrderAttackSoldier();
}

eOrderCode Ant::GetOrderGuardianSoldier()
{
	int mapWidth = m_colony->GetMapWidth();
	IntVec2 currentPos = IntVec2((int)m_tileX, (int)m_tileY);
	IntVec2 nextTile = currentPos;
	bool isGoalValid = !m_colony->IsTileSolid(m_currentGoal, AGENT_TYPE_SOLDIER);

	if (!isGoalValid || (currentPos == m_currentGoal)) {
		m_currentGoal = IntVec2(-1, -1);
		m_currentPath.resize(0);
		m_currentPathIndex = 0;
	}


	if (m_currentPathIndex == 0) {
		std::vector<IntVec2> goalTiles;
		IntVec2 guardedAntPos = IntVec2(m_guardedAnt->m_tileX, m_guardedAnt->m_tileY);
		goalTiles.emplace_back(guardedAntPos);
		m_currentPath.clear();

		m_colony->GetAStarPath(m_currentPath, currentPos, goalTiles, eAgentType::AGENT_TYPE_SOLDIER, mapWidth * 2, 2.0f);
		if (m_currentPath.size() > 0) {
			m_currentGoal = m_currentPath[0];
			m_currentPathIndex = (int)m_currentPath.size() - 1;
		}
	}

	if ((m_currentPath.size() > 0) && (m_currentPathIndex > 0)) {
		m_currentPathIndex--;
		nextTile = m_currentPath[m_currentPathIndex];
	}

	IntVec2 moveDir = nextTile - currentPos;

	return GetMoveOrderForDir(moveDir);
}

eOrderCode Ant::GetOrderAttackSoldier()
{
	IntVec2 currentPos = IntVec2((int)m_tileX, (int)m_tileY);

	int closeDistance = 15;
	bool recomputeAStar = false;
	bool canComputeAStar = m_colony->CanCalculateAStarSoldiers();

	recomputeAStar = !m_colony->IsEnemyAtPos(m_currentGoal);

	int distToGoal = GetTaxicabDistance2D(currentPos, m_currentGoal);
	if ((m_currentGoal == currentPos) || (m_currentPathIndex == 0) || (distToGoal < 10)) {
		recomputeAStar = true;
		m_isChasingEnemy = false;
		m_currentPath.clear();
	}

	if (canComputeAStar) {

		if (!m_isChasingEnemy)recomputeAStar = true;

		if (recomputeAStar) {
			m_currentPath.clear();
			m_colony->GetAStarPath(m_currentPath, currentPos, m_colony->m_enemyPositions, eAgentType::AGENT_TYPE_SOLDIER, m_colony->GetMapWidth() * 3, 1.75f);
			m_colony->RegisterSoldierAStartRequest();
			if (m_currentPath.size() > 0) {
				m_isChasingEnemy = true;
				m_currentGoal = m_currentPath[0];
				m_currentPathIndex = (int)m_currentPath.size() - 1;
			}

		}
	}

	IntVec2 nextTile = currentPos;


	if ((m_currentPath.size() > 0) && (m_currentPathIndex > 0)) {
		m_currentPathIndex--;
		nextTile = m_currentPath[m_currentPathIndex];
	}

	if (!m_isChasingEnemy) {
		TileHeatMap const& heatmap = m_colony->GetHeatmap(AGENT_TYPE_SOLDIER, true);
		nextTile = heatmap.GetCoordsForNextLowestValue(currentPos);
	}

	if (m_colony->IsTileSolid(nextTile, AGENT_TYPE_SOLDIER)) {
		m_isChasingEnemy = false;
	}
	//IntVec2 nextTile = currentPos;
	//if (m_colony->m_enemyPositions.size() > 0) {
	//	TileHeatMap attackMap = m_colony->GetHeatmap(eAgentType::AGENT_TYPE_SOLDIER);
	//	nextTile = attackMap.GetCoordsForNextLowestValue(currentPos);
	//}
	//else {
	//	TileHeatMap heatmapToQueen = m_colony->GetHeatmap(AGENT_TYPE_SOLDIER, true);
	//	nextTile = heatmapToQueen.GetCoordsForNextLowestValue(currentPos);
	//}

	IntVec2 moveDir = nextTile - currentPos;
	int length = moveDir.GetTaxicabLength();
	if ((length > 1) && m_isChasingEnemy) {
		m_isChasingEnemy = false;
	}

	int guardsPerQueen = 5;
	int numWorkers = m_colony->m_liveWorkers;
	int numSoldiers = m_colony->m_liveSoldiers;
	int numQueens = m_colony->m_liveQueens;
	int maxSoldiers = (guardsPerQueen * numQueens) + numWorkers;

	if (numSoldiers > maxSoldiers + 2) {
		if (!m_colony->IsScheduledForDeath(AGENT_TYPE_SOLDIER)) {
			m_colony->ScheduleForDeath(AGENT_TYPE_SOLDIER);
			return eOrderCode::ORDER_SUICIDE;
		}
	}

	eOrderCode orderToReturn = GetMoveOrderForDir(moveDir);
	return orderToReturn;
}

eOrderCode Ant::GetOrderScout()
{
	//return eOrderCode::ORDER_HOLD;
	int mapWidth = m_colony->GetMapWidth();
	IntVec2 currentPos = IntVec2((int)m_tileX, (int)m_tileY);
	IntVec2 nextTile = currentPos;
	bool isGoalValid = !m_colony->IsTileSolid(m_currentGoal, AGENT_TYPE_SCOUT);


	if (!isGoalValid || (currentPos == m_currentGoal)) {
		m_currentGoal = IntVec2(-1, -1);
		m_currentPathIndex = 0;
		m_currentPath.resize(0);
		m_isChasingEnemy = false;
	}

	ObservedAgent const* closestQueen = m_colony->GetClosestEnemyQueen(currentPos);
	if (closestQueen && !m_isChasingEnemy) {
		m_isChasingEnemy = true;
		std::vector<IntVec2> goalTiles = { IntVec2(closestQueen->tileX, closestQueen->tileY) };
		m_currentPath.clear();
		m_colony->GetAStarPath(m_currentPath, currentPos, goalTiles, eAgentType::AGENT_TYPE_SCOUT, mapWidth * 3, 2.0f);
		if (m_currentPath.size() > 0) {
			m_currentGoal = m_currentPath[0];
			m_currentPathIndex = (int)m_currentPath.size() - 1;
		}
	}


	if ((m_currentPathIndex == 0) || (m_currentPath.size() == 0)) {
		std::vector<IntVec2>& goalTiles = m_colony->m_unexploredTiles;

		if (goalTiles.size() < 10) {
			goalTiles.push_back(m_colony->GetRandNonSolidCoords());
		}

		m_currentPath.clear();
		m_colony->GetAStarPath(m_currentPath, currentPos, goalTiles, eAgentType::AGENT_TYPE_SCOUT, mapWidth * 5, 1.5f);
		if (m_currentPath.size() == 0) {
			RandomNumberGenerator randOff;
			IntVec2 randOffset = currentPos;
			randOffset.x += randOff.GetRandomIntInRange(-8, 8);
			randOffset.y += randOff.GetRandomIntInRange(-8, 8);

			std::vector<IntVec2> randGoal = { randOffset };
			m_colony->GetAStarPath(m_currentPath, currentPos, randGoal, eAgentType::AGENT_TYPE_SCOUT, mapWidth * 5, 1.5f);

		}

		if (m_currentPath.size() > 0) {
			m_currentGoal = m_currentPath[0];
			m_currentPathIndex = (int)m_currentPath.size() - 1;
		}
	}
	//TileHeatMap const& heatmap = m_colony->GetHeatmap(*this);

	//nextTile = heatmap.GetCoordsForNextLowestValue(currentPos);
	if ((m_currentPath.size() > 0) && (m_currentPathIndex > 0)) {
		m_currentPathIndex--;
		nextTile = m_currentPath[m_currentPathIndex];
	}

	IntVec2 moveDir = nextTile - currentPos;

	return GetMoveOrderForDir(moveDir);
}

eOrderCode Ant::GetOrderWorker()
{
	IntVec2 currentPos = IntVec2((int)m_tileX, (int)m_tileY);
	IntVec2 nextLowest = currentPos;

	int mapWidth = m_colony->GetMapWidth();
	m_nextMove = ORDER_HOLD;

	int guardsPerWorker = 1;

	/*if (m_guards.size() < guardsPerWorker) {
		m_colony->TryAppointingGuard(this);;
	}*/

	// Must take food to queen!
	if (m_state == STATE_HOLDING_FOOD) {
		TileHeatMap const& heatmap = m_colony->GetHeatmap(AGENT_TYPE_WORKER, true);
		float currentValue = heatmap.GetValue(currentPos);

		if (currentValue == 0.0f) {
			g_debug->QueueDrawWorldText((float)currentPos.x, (float)currentPos.y, 0.0f, 0.0f, 1.0f, Color8(255, 255, 0, 255), "Feed your highness");
			//ReleaseAllGuards();
			return ORDER_DROP_CARRIED_OBJECT;
		}

		nextLowest = heatmap.GetCoordsForNextLowestValue(currentPos);
	}
	else {
		if (m_canDig) {
			if (m_colony->IsTileDirt(currentPos)) {
				m_nextMove = ORDER_DIG_HERE;
				return m_nextMove;
			}
		}


		if (currentPos == m_currentGoal) {
			g_debug->LogText("Worker picked up food");
			m_colony->ResetFoodDensity(m_currentGoal);
			m_currentGoal = IntVec2(-1, -1);
			m_currentPathIndex = 0;
			m_currentPath.clear();
			return ORDER_PICK_UP_FOOD;
		}

		int distToGoal = GetTaxicabDistance2D(currentPos, m_currentGoal);

		if (distToGoal > 18) {
			if (((m_colony->m_foodCount * 3) / 2) < m_colony->m_liveWorkers) {
				if (!m_colony->IsScheduledForDeath(AGENT_TYPE_WORKER)) {
					m_colony->ScheduleForDeath(AGENT_TYPE_WORKER);
					return ORDER_SUICIDE;
				}
			}
		}

		bool isFoodAtGoal = m_colony->IsFoodAtPos(m_currentGoal);
		if (!isFoodAtGoal) {
			m_colony->ResetFoodDensity(m_currentGoal);
		}

		

		if ((!isFoodAtGoal && distToGoal > 10) || ((m_currentPathIndex == 0) || (m_currentPath.size() == 0))) {
			if (m_colony->CanCalculateAStar()) {
				//std::vector<IntVec2> goals;
				//goals.reserve(1000);
				std::vector<IntVec2>& tilesWithFood = m_colony->GetTilesWithFood();
				if (tilesWithFood.size() > 0) {
					m_currentPath.clear();
					m_colony->GetAStarPath(m_currentPath, currentPos, tilesWithFood, m_type, (mapWidth * 3) / 2, 1.5f, true);
				}

				if (m_currentPath.size() > 0) {
					m_currentGoal = m_currentPath[0];
					m_colony->FlagFoodAsTaken(m_currentGoal);
					m_currentPathIndex = (int)m_currentPath.size() - 1;
				}
			}
		}

		if ((m_currentPath.size() > 0) && (m_currentPathIndex > 0)) {
			m_currentPathIndex--;
			nextLowest = m_currentPath[m_currentPathIndex];
		}
	}
	bool isNextTileSolid = m_colony->IsTileSolid(nextLowest, AGENT_TYPE_WORKER);
	if (isNextTileSolid) {
		m_currentPathIndex = 0;
		m_currentPath.clear();
	}

	IntVec2 direction = nextLowest - currentPos;
	m_nextMove = GetMoveOrderForDir(direction);

	if (m_nextMove == ORDER_HOLD) {
		if (m_turnsOnHold > 10) {
			m_turnsOnHold = 0;
			IntVec2 highestFoodDensityCoord = m_colony->GetHighestFoodDensityCoords();
			std::vector<IntVec2> densityGoal = { highestFoodDensityCoord };
			m_currentPath.clear();
			m_colony->GetAStarPath(m_currentPath, currentPos, densityGoal, eAgentType::AGENT_TYPE_QUEEN, (mapWidth * 3) / 2, 1.25f);

			if ((m_currentPath.size() > 0)) {
				m_currentPathIndex = (int)m_currentPath.size() - 2;
				m_currentGoal = m_currentPath[0];
				nextLowest = m_currentPath[m_currentPathIndex];
			}

			direction = nextLowest - currentPos;
			if (direction.GetLengthSquared() > 0) {
				m_nextMove = GetMoveOrderForDir(direction);
			}
		}
		else {
			m_turnsOnHold++;
		}
	}
	else {
		m_turnsOnHold = 0;
	}


	return m_nextMove;
}

void Ant::ReleaseAllGuards()
{
	for (Ant*& guard : m_guards) {
		m_colony->QueueGuardForRelease(guard);
	}
}

