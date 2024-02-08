#include "Game/Gameplay/Tile.hpp"
#include "Game/Gameplay/TileDefinition.hpp"

Tile::Tile(AABB3 bounds, IntVec2 const& coords, TileDefinition const* definition):
	m_bounds(bounds),
	m_definition(definition),
	m_coords(coords)
{
}

bool Tile::IsAir() const
{
	bool noCeiling = m_definition->m_ceilingMaterialDefinition == nullptr;
	bool noFloor = m_definition->m_floorMaterialDefinition == nullptr;
	bool noWall = m_definition->m_wallMaterialDefinition == nullptr;
	return noCeiling && noFloor && noWall;
}

bool Tile::IsSolid() const
{
	if (!m_definition) return false;
	return m_definition->m_isSolid;
}

bool Tile::HasFloor() const
{
	return m_definition->m_floorMaterialDefinition;
}

