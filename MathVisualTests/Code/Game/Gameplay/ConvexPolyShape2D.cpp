#include "Game/Gameplay/ConvexPolyShape2D.hpp"
#include "Engine/Math/ConvexHull2D.hpp"

ConvexPolyShape2D::ConvexPolyShape2D(std::vector<Vec2> ccwPoints, Rgba8 const& color, Rgba8 const& highlighedColor) :
	Shape2D(Vec2::ZERO, color, highlighedColor)
{
	m_discCenter = m_position;
	m_convexPoly2D = ConvexPoly2D(ccwPoints);
	UpdateBoundingDisc();
}

void ConvexPolyShape2D::AddVertsForEdges(std::vector<Vertex_PCU>& vertexes, Rgba8 const& borderColor) const
{	
	AddVertsForWireConvexPoly2D(vertexes, m_convexPoly2D, borderColor, 1.0f);
}

void ConvexPolyShape2D::AddVertsForFillings(std::vector<Vertex_PCU>& vertexes) const
{
	AddVertsForConvexPoly2D(vertexes, m_convexPoly2D, m_color);
}

void ConvexPolyShape2D::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForWireConvexPoly2D(worldVerts, m_convexPoly2D,  Rgba8::WHITE);
	AddVertsForConvexPoly2D(worldVerts, m_convexPoly2D,  m_color);
	//AddVertsForConvexHull2D(worldVerts, m_convexPoly2D.GetConvexHull(), Rgba8::WHITE, 1000.0f);

	g_theRenderer->DrawVertexArray(worldVerts);
}

void ConvexPolyShape2D::RenderHighlighted() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForWireConvexPoly2D(worldVerts, m_convexPoly2D, Rgba8::WHITE);
	AddVertsForConvexPoly2D(worldVerts, m_convexPoly2D, m_highlightedColor);
	//AddVertsForConvexHull2D(worldVerts, m_convexPoly2D.GetConvexHull(), Rgba8::WHITE, 1000.0f);
	g_theRenderer->DrawVertexArray(worldVerts);
}

Vec2 ConvexPolyShape2D::GetNearestPoint(Vec2 const& refPoint) const
{
	UNUSED(refPoint);
	return Vec2::ZERO;
}

bool ConvexPolyShape2D::IsPointInside(Vec2 const& refPoint) const
{
	return IsPointInsideConvexPoly2D(refPoint, m_convexPoly2D);
}

void ConvexPolyShape2D::UpdateBoundingDisc()
{
	m_radius = m_convexPoly2D.GetBoudingDisc(m_discCenter);
}
