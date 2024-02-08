#pragma once
#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Game/Gameplay/Controller.hpp"

class Map;

class AI : public Controller
{
public:
	AI(Map* pointerToMap);
	virtual ~AI();

	virtual void Update( float deltaSeconds ) override;

	virtual void UpdateNormal(float deltaSeconds, Actor& closestEnemy);
	virtual void UpdatePathing(float deltaSeconds);

	bool m_hasReachedGoal = true;
	bool m_hasSightOfPlayer = false;
	std::vector<IntVec2> m_pathPoints; 
	Vec3 m_nextWaypoint = Vec3::ZERO;
	Vec3 m_goalPosition = Vec3::ZERO;
	Vec3 m_lastKnownPlayerPosition = Vec3::ZERO;


	Stopwatch m_meleeStopwatch;
	TileHeatMap m_heatmap;
};

