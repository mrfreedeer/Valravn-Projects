#pragma once
#include "Game/Gameplay/Shape3D.hpp"

class CylinderShape3D : public Shape3D {
public:
	CylinderShape3D(Vec3 const& center, float radius, float height, bool wireCylinder, Texture* texture, Rgba8 const& color,Rgba8 const& highLightedColor, float elasticity = 1.0f);

	void Update(float deltaSeconds);
	void Render() const override;
	void RenderHighlighted() const override;

public:
	float m_radius = 0.0f;
	float m_height = 0.0f;
};
