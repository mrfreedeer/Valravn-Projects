#pragma once
#include "Common.hpp"
#include "Engine/Core/HeatMaps.hpp"

class ColonyJob {
public:
	virtual ~ColonyJob(){}
	ColonyJob(Colony* colony) : m_colony(colony) {}
	virtual void Execute() = 0;

public:
	Colony* m_colony = nullptr;
	std::atomic<bool> m_isFinished = false;
};


class HeatmapUpdateJob : public ColonyJob {
public:
	HeatmapUpdateJob(Colony* colony, IntVec2 const& heatmapDims, std::vector<IntVec2> const& goals, eAgentType agentType, bool lookingForQueen);

	void Execute() override;
public:
	TileHeatMap m_heatmapCopy;
	eAgentType m_agentType;
	std::vector<IntVec2> m_goals;
	bool m_lookingForQueen = false;
};


class OrderProcessingJob : public ColonyJob {
public:
	OrderProcessingJob(Colony* colony): ColonyJob(colony){}

	void Execute() override;
};


class ProcessTurnInfoJob : public ColonyJob {
public:
	ProcessTurnInfoJob(Colony* colony) : ColonyJob(colony) {}

	void Execute() override;
};
