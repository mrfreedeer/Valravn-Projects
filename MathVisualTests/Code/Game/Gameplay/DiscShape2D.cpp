#include "Game/Gameplay/DiscShape2D.hpp"

DiscShape2D::DiscShape2D(Vec2 const& center, float radius, Rgba8 const& color, Rgba8 const& highlighedColor, float elasticity):
	Shape2D(center, color, highlighedColor, elasticity),
	m_radius(radius)
{
}


void DiscShape2D::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForDisc2D(worldVerts, m_position, m_radius, m_color);

	g_theRenderer->DrawVertexArray(worldVerts);
}

void DiscShape2D::RenderHighlighted() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForDisc2D(worldVerts, m_position, m_radius, m_highlightedColor);

	g_theRenderer->DrawVertexArray(worldVerts);
}

Vec2 DiscShape2D::GetNearestPoint(Vec2 const& refPoint) const
{
	return GetNearestPointOnDisc2D(refPoint, m_position, m_radius);
}

bool DiscShape2D::IsPointInside(Vec2 const& refPoint) const
{
	return IsPointInsideDisc2D(refPoint, m_position, m_radius);
}
