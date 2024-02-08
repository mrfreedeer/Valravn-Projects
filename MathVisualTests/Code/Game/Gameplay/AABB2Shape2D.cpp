#include "Game/Gameplay/AABB2Shape2D.hpp"

AABB2Shape2D::AABB2Shape2D(Vec2 const& dimensions, Vec2 const& center, Rgba8 const& color, Rgba8 const& highlighedColor):
	Shape2D(center, color, highlighedColor)
{
	m_AABB2.SetCenter(center);
	m_AABB2.SetDimensions(dimensions);
}

void AABB2Shape2D::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForAABB2D(worldVerts, m_AABB2, m_color);

	g_theRenderer->DrawVertexArray(worldVerts);
}

void AABB2Shape2D::RenderHighlighted() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForAABB2D(worldVerts, m_AABB2, m_highlightedColor);

	g_theRenderer->DrawVertexArray(worldVerts);
}

Vec2 AABB2Shape2D::GetNearestPoint(Vec2 const& refPoint) const
{
	return GetNearestPointOnAABB2D(refPoint, m_AABB2);
}

bool AABB2Shape2D::IsPointInside(Vec2 const& refPoint) const
{
	return IsPointInsideAABB2D(refPoint, m_AABB2);
}
