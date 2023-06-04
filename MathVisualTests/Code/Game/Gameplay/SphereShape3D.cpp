#include "Game/Gameplay/SphereShape3D.hpp"

SphereShape3D::SphereShape3D(Vec3 const& spherePos, float radius, bool wireSphere, Texture* texture, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity) :
	m_center(spherePos),
	m_radius(radius),
	Shape3D(spherePos, color, highLightedColor, elasticity)
{
	m_texture = texture;

	if (!wireSphere) {
		AddVertsForSphere(m_verts, radius, 16, 32, color);
		AddVertsForSphere(m_verts, radius, 16, 32, highLightedColor);
	}
	else {
		AddVertsForWireSphere(m_verts, radius, 16, 32, color);
		AddVertsForWireSphere(m_verts, radius, 16, 32, highLightedColor);
	}
}


void SphereShape3D::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Shape3D::Update();
}

void SphereShape3D::Render() const
{
	g_theRenderer->SetModelMatrix(GetModelMatrix());
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(m_verts);
}

void SphereShape3D::RenderHighlighted() const
{
	g_theRenderer->SetModelMatrix(GetModelMatrix());
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->DrawVertexArray(m_highlightedVerts);
}
