#include "Engine/Core/VertexUtils.hpp"
#include "Game/Gameplay/DelaunayShape2D.hpp"

DelaunayShape2D::DelaunayShape2D(std::vector<Vec2> vertexes):
	m_polygon(vertexes)
{
	m_position = m_polygon.m_middlePoint;
}

DelaunayShape2D::DelaunayShape2D(Vec2 const& startingPosition, std::vector<Vec2> vertexes) :
	m_position(startingPosition),
	m_polygon(vertexes)
{
	if (m_polygon.m_middlePoint != m_position) {
		Vec2 dispToCenter = m_position - m_polygon.m_middlePoint;
		m_polygon.Translate(dispToCenter);
	}
}

DelaunayShape2D::DelaunayShape2D(Vec2 const& startingPosition, DelaunayConvexPoly2D const& convexPoly) :
	m_position(startingPosition),
	m_polygon(convexPoly)
{
	if (m_polygon.m_middlePoint != m_position) {
		Vec2 dispToCenter = m_position - m_polygon.m_middlePoint;
		m_polygon.Translate(dispToCenter);
	}
}

void DelaunayShape2D::Update(float deltaSeconds)
{
	Vec2 prevPos = m_position;
	m_position += m_velocity * deltaSeconds;
	Vec2 dispToNewPos = m_position - prevPos;
	m_polygon.Translate(dispToNewPos);
}

void DelaunayShape2D::Translate(Vec2 const& disp)
{
	m_position += disp;
	m_polygon.Translate(disp);
}

void DelaunayShape2D::AddVertsForShape(std::vector<Vertex_PCU>& verts, Rgba8 const& color) const
{
	AddVertsForDelaunayConvexPoly2D(verts, m_polygon, color);
}


