#pragma once
#include "Engine/Math/OBB2.hpp"
#include "Game/Gameplay/Shape2D.hpp"

class OBB2Shape2D : public Shape2D {
public:
	OBB2Shape2D(Vec2 const& halfDimensions, Vec2 const& position, float orientation, Rgba8 const& color, Rgba8 const& highlighedColor);

	virtual void Render() const override;
	virtual void RenderHighlighted() const override;
	virtual Vec2 GetNearestPoint(Vec2 const& refPoint) const override;
	virtual bool IsPointInside(Vec2 const& refPoint) const override;

public:
	OBB2 m_OBB2;
};