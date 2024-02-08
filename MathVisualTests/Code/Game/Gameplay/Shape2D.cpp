#include "Game/Gameplay/Shape2D.hpp"

Shape2D::Shape2D(Vec2 const& position, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity):
	m_position(position),
	m_color(color),
	m_highlightedColor(highLightedColor),
	m_elasticity(elasticity)
{
}

void Shape2D::Update(float deltaSeconds)
{
	m_position += m_velocity * deltaSeconds;
}
