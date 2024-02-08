#pragma once
#include "Game/Gameplay/Shape3D.hpp"

class SphereShape3D : public Shape3D {
public:
	SphereShape3D(Vec3 const& spherePos, float radius, bool wireSphere, Texture* texture, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity = 1.0f);

	void Update(float deltaSeconds);
	void Render() const override;
	void RenderHighlighted() const override;

	Vec3 m_center = Vec3::ZERO;
	float m_radius = 0.0f;

};
