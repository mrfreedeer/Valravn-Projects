#include "Engine/Math/Sampling.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

PoissonDisc2D::PoissonDisc2D(Vec2 const& spaceSize, float radius) :
	m_spaceSize(spaceSize),
	m_radius(radius)
{
	m_cellSize = radius / sqrtf(2.0f);
	int gridWidth = int(ceilf(spaceSize.x / m_cellSize)) + 1;
	int gridHeight = int(ceilf(spaceSize.y / m_cellSize)) + 1;

	m_gridSize = IntVec2(gridWidth, gridHeight);
}

void PoissonDisc2D::GetSamplingPoints(std::vector<Vec2>& samplePoints, int triesFindingSample /*= 32*/)
{
	int totalCells = m_gridSize.x * m_gridSize.y;

	samplePoints.reserve(totalCells);

	m_grid.clear();
	m_grid.reserve(totalCells);

	for (int fillIndex = 0; fillIndex < (totalCells); fillIndex++) {
		m_grid.push_back(Vec2(-1.0f, -1.0f));
	}

	RandomNumberGenerator randGen;

	float startX = randGen.GetRandomFloatInRange(0.0f, m_spaceSize.x);
	float startY = randGen.GetRandomFloatInRange(0.0f, m_spaceSize.y);

	Vec2 startingPoint = Vec2(startX, startY);
	InsertPointInGrid(startingPoint);
	samplePoints.push_back(startingPoint);

	std::vector<Vec2> activePoints;
	activePoints.push_back(startingPoint);
	float diameter = 2.0f * m_radius;

	while (activePoints.size() > 0) {
		int randInd = randGen.GetRandomIntInRange(0, int(activePoints.size() - 1));
		Vec2 const& point = activePoints[randInd];

		bool foundCandidate = false;

		for (int tryInd = 0; (tryInd < triesFindingSample) && !foundCandidate; tryInd++) {
			float randAngle = randGen.GetRandomFloatZeroUpToOne() * 360.0f;
			float randRadius = randGen.GetRandomFloatInRange(m_radius, diameter);

			Vec2 candidatePoint = point + Vec2::MakeFromPolarDegrees(randAngle, randRadius);
			if (IsPointValid(candidatePoint)) {
				foundCandidate = true;
				activePoints.push_back(candidatePoint);
				InsertPointInGrid(candidatePoint);
				samplePoints.push_back(candidatePoint);

			}

		}

		if (!foundCandidate) {
			activePoints.erase(activePoints.begin() + randInd);
		}
	}

}

bool PoissonDisc2D::IsPointValid(Vec2 const& point) const
{
	// Point is out of bounds
	if (IsOutOfGrid(point)) return false;

	IntVec2 pointCoords = GetGridCoords(point);

	// Algorithm suggests checking 2Rs away
	for (int y = pointCoords.y - 2; y < (pointCoords.y + 2); y++) {
		for (int x = pointCoords.x - 2; x < (pointCoords.x + 2); x++) {
			if (IsOutOfGrid(x, y)) continue;

			int gridIndex = GetIndexFromCoords(x, y);
			Vec2 const& gridPoint = m_grid[gridIndex];

			if (gridPoint == Vec2(-1.0f, -1.0f)) continue;

			float distToGridPoint = GetDistance2D(gridPoint, point);

			if (distToGridPoint < m_radius) return false;

		}
	}

	return true;

}

IntVec2 PoissonDisc2D::GetGridCoords(Vec2 const& point) const
{
	int x = (int)floorf(point.x / m_cellSize);
	int y = (int)floorf(point.y / m_cellSize);

	return IntVec2(x, y);

}

bool PoissonDisc2D::IsOutOfGrid(Vec2 const& point) const
{
	if ((point.x < 0) || (point.x >= (float)m_spaceSize.x) || (point.y < 0) || (point.y >= (float)m_spaceSize.y))
		return true;

	return false;
}

bool PoissonDisc2D::IsOutOfGrid(IntVec2 const& coords) const
{
	if ((coords.x < 0) || (coords.x >= m_spaceSize.x) || (coords.y < 0) || (coords.y >= m_spaceSize.y))
		return true;

	return false;
}

bool PoissonDisc2D::IsOutOfGrid(int x, int y) const
{
	if ((x < 0) || (x >= m_spaceSize.x) || (y < 0) || (y >= m_spaceSize.y))
		return true;

	return false;
}


int PoissonDisc2D::GetIndexFromCoords(IntVec2 const& coords) const
{
	return (coords.y * m_gridSize.x) + coords.x;
}


int PoissonDisc2D::GetIndexFromCoords(int x, int y) const
{
	return (y * m_gridSize.x) + x;
}

void PoissonDisc2D::InsertPointInGrid(Vec2 const& point)
{
	IntVec2 const& coords = GetGridCoords(point);
	int index = GetIndexFromCoords(coords);

	m_grid[index] = point;
}

