#pragma once
#include "Common.hpp"

struct Tile {
	eTileType m_type = TILE_TYPE_UNSEEN;
	bool m_hasFood = false;
};