#include "ColonyJob.hpp"
#include "Colony.hpp"

HeatmapUpdateJob::HeatmapUpdateJob(Colony* colony, IntVec2 const& heatmapDims, std::vector<IntVec2> const& goals, eAgentType agentType, bool lookingForQueen) :
	m_heatmapCopy(heatmapDims),
	m_agentType(agentType),
	m_lookingForQueen(lookingForQueen),
	m_goals(goals),
	ColonyJob(colony)
{

}

void HeatmapUpdateJob::Execute()
{
	m_heatmapCopy.SetAllValues(FLT_MAX);
	for (IntVec2 const& goal : m_goals) {
		m_heatmapCopy.SetValue(goal, 0.0f);
	}
	m_colony->RecalculateHeatMap(m_heatmapCopy, m_agentType);
	m_colony->UpdateHeatmap(m_heatmapCopy, m_agentType, m_lookingForQueen);
	m_isFinished = true;
}

void OrderProcessingJob::Execute()
{
	Ant* colony = m_colony->GetColony();

	PlayerTurnOrders newOrders = {};
	newOrders.numberOfOrders = (int)m_colony->m_liveAnts;

	int orderIndex = 0;
	int liveAnts = (int)m_colony->m_liveAnts;

	for (int antIndex = 0, liveAntIndex = 0; liveAntIndex < liveAnts; antIndex++) {
		Ant& ant = colony[antIndex];
		if (ant.m_state == STATE_DEAD) continue;
		liveAntIndex++;
		if (ant.m_type == AGENT_TYPE_SOLDIER) continue;

		AgentOrder& currentOrder = newOrders.orders[orderIndex];
		currentOrder.agentID = ant.m_agentID;
		currentOrder.order = ant.GetOrder();
		orderIndex++;

	}


	for (Ant* guard : m_colony->m_soldierAnts) {
		if (!guard) continue;
		AgentOrder& currentOrder = newOrders.orders[orderIndex];
		currentOrder.agentID = guard->m_agentID;
		orderIndex++;
		if (guard->m_isGuardian) {
			IntVec2 guardPos = guard->GetPosition();
			IntVec2 guardedAntPos = guard->m_guardedAnt->GetPosition();
			if (guardPos == guardedAntPos) {
				currentOrder.order = guard->m_guardedAnt->m_nextMove;
				continue;
			}

		}

		currentOrder.order = guard->GetOrder();

	}

	//for (int antIndex = 0, liveAntIndex = 0; liveAntIndex < liveAnts; antIndex++) {
	//	Ant& ant = colony[antIndex];

	//	if ((ant.m_state != STATE_DEAD)) {
	//		liveAntIndex++;
	//		if (ant.m_isGuardian) continue;

	//		AgentOrder& currentOrder = newOrders.orders[orderIndex];
	//		currentOrder.agentID = ant.m_agentID;
	//		currentOrder.order = ant.GetOrder();
	//		orderIndex++;

	//	}
	//}

	/*for (Ant* guard : m_colony->m_soldierAnts) {
		if (!guard) continue;
		if (!guard->m_isGuardian) continue;

		AgentOrder& currentOrder = newOrders.orders[orderIndex];
		currentOrder.agentID = guard->m_agentID;
		orderIndex++;

		IntVec2 guardPos = guard->GetPosition();
		IntVec2 guardedAntPos = guard->m_guardedAnt->GetPosition();
		if (guardPos == guardedAntPos) {
			currentOrder.order = guard->m_guardedAnt->m_nextMove;
		}
		else {
			currentOrder.order = guard->GetOrder();
		}

	}*/

	m_colony->AppointAllPendingGuards();
	m_colony->ReleaseAllPendingGuards();
	m_colony->UpdateOrders(newOrders);

}

void ProcessTurnInfoJob::Execute()
{
	m_colony->ProcessTurnInfo();
}
