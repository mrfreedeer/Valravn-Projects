#pragma once
#include <vector>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Core/EngineCommon.hpp"

class TileHeatMap {
public:
	TileHeatMap(IntVec2 const& dimensions);
	void SetAllValues(float newValue);
	float GetValue(int index) const;
	float GetValue(IntVec2 const& coords) const;
	void SetValue(int index, float newValue);
	void SetValue(IntVec2 const& coords, float newValue);
	void AddValue(float newValue);
	IntVec2 GetDimensions() const { return m_dimensions; }

	IntVec2 GetCoordsForNextLowestValue(IntVec2 const& coords) const;
	IntVec2 GetCoordsForNextHighestValue(IntVec2 const& coords) const;

	float GetMaxValue(float maxValueToLookUnder = ARBITRARILY_LARGE_VALUE) const;
	IntVec2 GetRandomValue(float valueLowerThan) const;
	
	std::vector<IntVec2> GeneratePathToCoords(IntVec2 const& currentCoords, IntVec2 const& goalCoords);

private:
	std::vector<float> m_values = { 0 };
	IntVec2 m_dimensions = IntVec2::ZERO;
};