#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Game/Gameplay/Shape2D.hpp"

class AABB2Shape2D : public Shape2D {
public:
	AABB2Shape2D(Vec2 const& dimensions, Vec2 const& center, Rgba8 const& color, Rgba8 const& highlighedColor);

	virtual void Render() const override;
	virtual void RenderHighlighted() const override;
	virtual Vec2 GetNearestPoint(Vec2 const& refPoint) const override;
	virtual bool IsPointInside(Vec2 const& refPoint) const override;

public:
	AABB2 m_AABB2;
};