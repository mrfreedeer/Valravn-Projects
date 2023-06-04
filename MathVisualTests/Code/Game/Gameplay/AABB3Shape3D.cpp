#include "Game/Gameplay/AABB3Shape3D.hpp"

AABB3Shape3D::AABB3Shape3D(AABB3 const& bounds, bool wireAABB3, Texture* texture, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity) :
	m_bounds(bounds),
	Shape3D(Vec3::ZERO , color, highLightedColor, elasticity)
{
	m_bounds.Translate(Vec3::ZERO);
	m_texture = texture;
	
	if (!wireAABB3) {
		AddVertsForAABB3D(m_verts, m_bounds, color);
		AddVertsForAABB3D(m_highlightedVerts, m_bounds, color);
	}
	else {
		AddVertsForWireAABB3D(m_verts, m_bounds, color);
		AddVertsForWireAABB3D(m_highlightedVerts, m_bounds, color);
	}
}

AABB3 const AABB3Shape3D::GetBounds() const {
	AABB3 bounds(m_bounds);
	bounds.Translate(m_position);

	return bounds;
}

void AABB3Shape3D::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Shape3D::Update();
}

void AABB3Shape3D::Render() const
{
	g_theRenderer->SetModelMatrix(GetModelMatrix());
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(m_verts);
}

void AABB3Shape3D::RenderHighlighted() const
{
	g_theRenderer->SetModelMatrix(GetModelMatrix());
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(m_highlightedVerts);
}
