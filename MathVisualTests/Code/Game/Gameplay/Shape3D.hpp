#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Game/Framework/GameCommon.hpp"

class Shape3D: public Entity {
protected:
	Shape3D(Vec3 const& position, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity = 1.0f);
public:
	virtual ~Shape3D() {}
	virtual void Render() const = 0;
	virtual void RenderHighlighted() const = 0;
	virtual void Update();

public:
	Rgba8 m_color = Rgba8::WHITE;
	Rgba8 m_highlightedColor = Rgba8::WHITE;
	float m_elasticity = 1.0f;
	bool m_isOverlapping = false;

	std::vector<Vertex_PCU> m_highlightedVerts;
};