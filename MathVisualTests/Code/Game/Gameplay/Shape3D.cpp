#include "Game/Gameplay/Shape3D.hpp"

Shape3D::Shape3D(Vec3 const& position, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity):
	m_color(color),
	m_highlightedColor(highLightedColor),
	m_elasticity(elasticity),
	Entity(nullptr, position)
{
}

void Shape3D::Update()
{
	m_isOverlapping = false;
}
