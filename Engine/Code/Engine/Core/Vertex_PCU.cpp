#include "Engine/Core/Vertex_PCU.hpp"

Vertex_PCU::Vertex_PCU()
{
}

Vertex_PCU::Vertex_PCU(Vec3 const& position, Rgba8 const& tint, Vec2 const& uvTexCoords):
	m_position(position),
	m_color(tint),
	m_uvTexCoords(uvTexCoords)
{
}

Vertex_PCU::Vertex_PCU(float x, float y, float z, Rgba8 const& tint, float u, float v):
	m_position(x,y,z),
	m_color(tint),
	m_uvTexCoords(u,v)
{
}
