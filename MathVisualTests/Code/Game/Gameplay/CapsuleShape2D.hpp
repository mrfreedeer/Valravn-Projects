#pragma once
#include "Engine/Math/Capsule2.hpp"
#include "Game/Gameplay/Shape2D.hpp"

class CapsuleShape2D : public Shape2D {
public:
	CapsuleShape2D(Vec2 const& lineStart, Vec2 const& lineEnd, float capsuleRadius, Rgba8 const& color, Rgba8 const& highlighedColor);

	virtual void Render() const override;
	virtual void RenderHighlighted() const override;
	virtual Vec2 GetNearestPoint(Vec2 const& refPoint) const override;
	virtual bool IsPointInside(Vec2 const& refPoint) const override;

private:
	Capsule2 m_capsule;
};