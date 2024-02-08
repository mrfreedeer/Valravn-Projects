#include "Game/Gameplay/OBB2Shape2D.hpp"

OBB2Shape2D::OBB2Shape2D(Vec2 const& halfDimensions, Vec2 const& position, float orientation, Rgba8 const& color, Rgba8 const& highlighedColor):
	Shape2D(position, color, highlighedColor)
{
	m_OBB2.m_center = position;
	m_OBB2.m_halfDimensions = halfDimensions;
	m_OBB2.RotateAboutCenter(orientation);
}


void OBB2Shape2D::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForOBB2D(worldVerts, m_OBB2, m_color);

	g_theRenderer->DrawVertexArray(worldVerts);
}

void OBB2Shape2D::RenderHighlighted() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForOBB2D(worldVerts, m_OBB2, m_highlightedColor);

	g_theRenderer->DrawVertexArray(worldVerts);
}

Vec2 OBB2Shape2D::GetNearestPoint(Vec2 const& refPoint) const
{
	return GetNearestPointOnOBB2D(refPoint, m_OBB2);
}

bool OBB2Shape2D::IsPointInside(Vec2 const& refPoint) const
{
	return IsPointInsideOBB2D(refPoint, m_OBB2);
}
