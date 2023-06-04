#pragma once
#include "Common.hpp"
#include "Engine/Core/HeatMaps.hpp"

extern unsigned char SCOUT_SOLIDNESS;
extern unsigned char WORKER_SOLIDNESS;
extern unsigned char SOLDIER_SOLIDNESS;
extern unsigned char QUEEN_SOLIDNESS;


class Ant {

public:
	Ant() = default;
	eOrderCode GetOrder();
	void AppointGuard(Ant* guard);
	void RemoveGuard(Ant* guard);
	void GuardAnt(Ant* ant);
	void RelieveFromGuardDuty();
	IntVec2 GetPosition() const { return IntVec2(m_tileX, m_tileY); }
private:
	eOrderCode GetOrderQueen();
	eOrderCode GetOrderSoldier();
	eOrderCode GetOrderGuardianSoldier();
	eOrderCode GetOrderAttackSoldier();
	eOrderCode GetOrderScout();
	eOrderCode GetOrderWorker();
	void ReleaseAllGuards();
public:
	AgentID				m_agentID;		// This agent's unique ID #
	short				m_tileX;			// Current tile coordinates in map; (0,0) is bottom-left
	short				m_tileY;
	short				m_exhaustion;

	short				m_combatDamage;		// Amount of damage received this turn 
	short				m_suffocationDamage;	// Suffocation damage received this turn (1 if you suffocated)

	eAgentType			m_type;			// Type of agent (permanent/unique per agent),	e.g. AGENT_TYPE_WORKER
	eAgentState			m_state = STATE_DEAD;			// Special status of agent,						e.g. STATE_HOLDING_FOOD
	eOrderCode m_nextMove = ORDER_HOLD;

	Colony* m_colony = nullptr;
	bool m_isGuardian = false;
	bool m_canDig = false;
	bool m_isChasingEnemy = false;

	std::vector<IntVec2> m_currentPath;
	std::vector<Ant*> m_guards;

	IntVec2 m_currentGoal = IntVec2(-1, -1);
	Ant* m_guardedAnt = nullptr;
	AgentID m_chasedEnemy;
	int m_lastCheckedEnemy = 0; 
	int m_currentPathIndex = 0;
	int m_turnsOnHold = 0;
	int m_movedPreviousTurn = 0;
	int m_lastGaveBirth = 0;
};


