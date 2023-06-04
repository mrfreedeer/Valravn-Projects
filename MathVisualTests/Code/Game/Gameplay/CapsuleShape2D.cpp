#include "Game/Gameplay/CapsuleShape2D.hpp"

CapsuleShape2D::CapsuleShape2D(Vec2 const& lineStart, Vec2 const& lineEnd, float capsuleRadius, Rgba8 const& color, Rgba8 const& highlighedColor):
	Shape2D((lineStart + lineEnd) * 0.5f, color, highlighedColor)
{

	m_capsule.m_bone.m_end = lineStart;
	m_capsule.m_bone.m_start = lineEnd;
	m_capsule.m_radius = capsuleRadius;
}

void CapsuleShape2D::Render() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForCapsule2D(worldVerts, m_capsule, m_color);

	g_theRenderer->DrawVertexArray(worldVerts);
}

void CapsuleShape2D::RenderHighlighted() const
{
	std::vector<Vertex_PCU> worldVerts;
	AddVertsForCapsule2D(worldVerts, m_capsule, m_highlightedColor);

	g_theRenderer->DrawVertexArray(worldVerts);
}

Vec2 CapsuleShape2D::GetNearestPoint(Vec2 const& refPoint) const
{
	return GetNearestPointOnCapsule2D(refPoint, m_capsule);
}

bool CapsuleShape2D::IsPointInside(Vec2 const& refPoint) const
{
	return IsPointInsideCapsule2D(refPoint, m_capsule);
}
