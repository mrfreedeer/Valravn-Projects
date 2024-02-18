#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>


class PoissonDisc2D {
public:
	PoissonDisc2D(Vec2 const& spaceSize, float radius);
	void GetSamplingPoints(std::vector<Vec2>& samplePoints, int triesFindingSample = 32);
private:
	bool IsPointValid(Vec2 const& point) const;
	IntVec2 GetGridCoords(Vec2 const& point) const;
	bool IsOutOfGrid(Vec2 const& point) const;
	bool IsOutOfGrid(IntVec2 const& point) const;
	bool IsOutOfGrid(int x, int y) const;
	int GetIndexFromCoords(IntVec2 const& coords) const;
	int GetIndexFromCoords(int x, int y) const;
	void InsertPointInGrid(Vec2 const& point);

private:
	IntVec2 m_gridSize = Vec2::ZERO;
	Vec2 m_spaceSize = Vec2::ZERO;
	float m_radius = 0.0f;
	float m_cellSize = 0.0f;
	std::vector<Vec2> m_grid;
};