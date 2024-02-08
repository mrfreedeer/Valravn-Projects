#pragma once
#include "Game/Gameplay/Shape2D.hpp"

class DiscShape2D : public Shape2D {
public:
	DiscShape2D(Vec2 const& center, float radius, Rgba8 const& color, Rgba8 const& highlighedColor, float elasticity = 1.0f);

	virtual void Render() const override; 
	virtual void RenderHighlighted() const override;
	virtual Vec2 GetNearestPoint(Vec2 const& refPoint) const override;
	virtual bool IsPointInside(Vec2 const& refPoint) const override;

	float m_radius = 0.0f;
	bool m_canBeMoved = true;
private:
};