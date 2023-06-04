#pragma once
#include "Game/Gameplay/Shape3D.hpp"

class AABB3Shape3D : public Shape3D {
public:
	AABB3Shape3D(AABB3 const& bounds, bool wireAABB3, Texture* texture, Rgba8 const& color, Rgba8 const& highLightedColor, float elasticity = 1.0f);

	void Update(float deltaSeconds);
	void Render() const override;
	void RenderHighlighted() const override;
	AABB3 const GetBounds() const;

	AABB3 m_bounds;
};
