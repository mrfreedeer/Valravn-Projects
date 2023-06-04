#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Game/Framework/GameCommon.hpp"

class Shape2D {
protected:
	Shape2D(Vec2 const& position, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity = 1.0f);
public:
	virtual ~Shape2D(){}
	virtual void Update(float deltaSeconds);
	virtual void Render() const = 0;
	virtual void RenderHighlighted() const = 0;
	virtual Vec2 GetNearestPoint(Vec2 const& refPoint) const = 0;
	virtual bool IsPointInside(Vec2 const& refPoint) const = 0;

public:
	Vec2 m_position = Vec2::ZERO;
	Vec2 m_velocity = Vec2::ZERO;
	Rgba8 m_color = Rgba8::WHITE;
	Rgba8 m_highlightedColor = Rgba8::WHITE;
	float m_elasticity = 1.0f;

};