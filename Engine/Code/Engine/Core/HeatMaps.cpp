#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"


TileHeatMap::TileHeatMap(IntVec2 const& dimensions):
	m_dimensions(dimensions)
{
	size_t vecSize = (size_t)dimensions.x * (size_t)dimensions.y;
	m_values.reserve(vecSize);
	m_values.resize(vecSize);
}

void TileHeatMap::SetAllValues(float newValue)
{
	for (int valuesIndex = 0; valuesIndex < m_values.capacity(); valuesIndex++) {
		m_values[valuesIndex] = newValue;
	}
}

float TileHeatMap::GetValue(int index) const
{
	if (index >= m_values.size() || index < 0) return 99999999.0f;
	return m_values[index];
}
float TileHeatMap::GetValue(IntVec2 const& coords) const
{
	int index = coords.y * m_dimensions.x + coords.x;
	if (index >= m_values.size() || index < 0) return 99999999.0f;
	return m_values[index];
}

void TileHeatMap::SetValue(int index, float newValue)
{
	m_values[index] = newValue;
}

void TileHeatMap::SetValue(IntVec2 const& coords, float newValue)
{
	int index = coords.y * m_dimensions.x + coords.x;
	m_values[index] = newValue;
}

void TileHeatMap::AddValue(float newValue)
{
	m_values.emplace_back(newValue);
}

IntVec2 TileHeatMap::GetCoordsForNextLowestValue(IntVec2 const& coords) const
{
	IntVec2 stepNorth = coords + IntVec2(0, 1);
	IntVec2 stepSouth = coords + IntVec2(0, -1);
	IntVec2 stepEast = coords + IntVec2(1, 0);
	IntVec2 stepWest = coords + IntVec2(-1, 0 );

	float currValue = GetValue(coords);

	float lowestValue = currValue;

	float valueNorth = GetValue(stepNorth);
	float valueSouth = GetValue(stepSouth);
	float valueEast = GetValue(stepEast);
	float valueWest = GetValue(stepWest);

	if(valueNorth < lowestValue) lowestValue = valueNorth; 
	if(valueSouth < lowestValue) lowestValue = valueSouth;
	if(valueEast < lowestValue) lowestValue = valueEast;
	if(valueWest < lowestValue) lowestValue = valueWest;
	
	

	if (valueNorth == lowestValue) {
		return stepNorth;
	}
	if (valueSouth == lowestValue) {
		return stepSouth;
	}
	if (valueEast == lowestValue) {
		return stepEast;
	}
	if (valueWest == lowestValue) {
		return stepWest;
	}

	return coords;
}

IntVec2 TileHeatMap::GetCoordsForNextHighestValue(IntVec2 const& coords) const
{
	IntVec2 stepNorth = coords + IntVec2(0, 1);
	IntVec2 stepSouth = coords + IntVec2(0, -1);
	IntVec2 stepEast = coords + IntVec2(-1, 0);
	IntVec2 stepWest = coords + IntVec2(1, 0);

	float valueNorth = GetValue(stepNorth);
	float valueSouth = GetValue(stepSouth);
	float valueEast = GetValue(stepEast);
	float valueWest = GetValue(stepWest);

	float currValue = GetValue(coords);

	if (valueNorth >= currValue) {
		return stepNorth;
	}
	if (valueSouth >= currValue) {
		return stepSouth;
	}
	if (valueEast >= currValue) {
		return stepEast;
	}
	if (valueWest >= currValue) {
		return stepWest;
	}

	return coords;
}

float TileHeatMap::GetMaxValue(float maxValueToLookUnder) const
{
	float maxValue = -1.0f;
	for (int valueIndex = 0; valueIndex < m_values.size(); valueIndex++) {
		if (m_values[valueIndex] > maxValue && m_values[valueIndex] < maxValueToLookUnder) {
			maxValue = m_values[valueIndex];
		}
	}
	return maxValue;
}

IntVec2 TileHeatMap::GetRandomValue(float valueLowerThanExclusive) const
{
	RandomNumberGenerator randNumGen;
	int possibleCandidates = 0;
	float chanceToSwitch = 0.0f;
	IntVec2 chosenCandidate = IntVec2(-1, -1);


	for (int valueIndex = 0; valueIndex < m_values.size(); valueIndex++) {
		if (m_values[valueIndex] < valueLowerThanExclusive) {
			possibleCandidates++;
			chanceToSwitch = 1.0f / static_cast<float>(possibleCandidates);
			float randNum = randNumGen.GetRandomFloatInRange(0.0f, 1.0f);
			if (randNum < chanceToSwitch) {
				chosenCandidate = IntVec2(valueIndex % m_dimensions.x, valueIndex / m_dimensions.x);
			}
		}
	}
	return chosenCandidate;
}

std::vector<IntVec2> TileHeatMap::GeneratePathToCoords(IntVec2 const& currentCoords, IntVec2 const& goalCoords)
{

	std::vector<IntVec2> pathToGoal;
	pathToGoal.reserve(m_dimensions.x + m_dimensions.y);

	float currentCoordsValue = GetValue(currentCoords);
	float goalCoordsValue = GetValue(goalCoords);

	if (goalCoordsValue > currentCoordsValue) {
		ERROR_RECOVERABLE(Stringf("TileHeatMap::GeneratePathToCoords -> THE GOAL HAS A HIGHER VALUE IN HEATMAP, ARE YOU SURE YOU WANT THIS?"));
	}

	IntVec2 nextLowerCoords = currentCoords;

	while (nextLowerCoords != goalCoords) {
		nextLowerCoords = GetCoordsForNextLowestValue(nextLowerCoords);
		pathToGoal.push_back(nextLowerCoords);
	}

	pathToGoal.push_back(goalCoords);

	std::vector<IntVec2> reversePathToGoal;
	reversePathToGoal.reserve(pathToGoal.size());

	for (int pathPointIndex = (int)pathToGoal.size() - 1; pathPointIndex >= 0; pathPointIndex--) {
		reversePathToGoal.push_back(pathToGoal[pathPointIndex]);
	}


	return reversePathToGoal;
}
