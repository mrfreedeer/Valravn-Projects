#pragma once
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"

//------------------------------------------------------------------------------------------------
class TileDefinition;

//------------------------------------------------------------------------------------------------
struct Tile
{
public:
	Tile( AABB3 bounds, IntVec2 const& coords, TileDefinition const* definition = nullptr );

	bool IsAir() const;
	bool IsSolid() const;
	bool HasFloor() const;

public:
	AABB3 m_bounds;
	IntVec2 m_coords = IntVec2::ZERO;
	TileDefinition const* m_definition;
};
