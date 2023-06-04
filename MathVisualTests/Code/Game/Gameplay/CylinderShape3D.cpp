#include "Game/Gameplay/CylinderShape3D.hpp"

CylinderShape3D::CylinderShape3D(Vec3 const& center, float radius, float height, bool wireCylinder, Texture* texture, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity) :
	m_radius(radius),
	m_height(height),
	Shape3D(center, color, highLightedColor, elasticity)
{
	m_texture = texture;
	Vec3 base = Vec3::ZERO;
	base.z -= height * 0.5f;
	Vec3 top = base + Vec3(0.0f, 0.0f, height);
	if (!wireCylinder) {
		AddVertsForCylinder(m_verts, base, top, radius, 16, color);
		AddVertsForCylinder(m_highlightedVerts, base, top, radius, 16, highLightedColor);
	}
	else {
		AddVertsForWireCylinder(m_verts, base, top, radius, 16, color);
		AddVertsForWireCylinder(m_highlightedVerts, base, top, radius, 16, highLightedColor);
	}

}

void CylinderShape3D::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Shape3D::Update();
}

void CylinderShape3D::Render() const
{
	g_theRenderer->SetModelMatrix(GetModelMatrix());
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(m_verts);
}

void CylinderShape3D::RenderHighlighted() const
{
	g_theRenderer->SetModelMatrix(GetModelMatrix());
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(m_highlightedVerts);
}
