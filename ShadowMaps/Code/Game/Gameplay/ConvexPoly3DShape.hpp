#pragma once
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/ConvexPoly3D.hpp"
#include "Engine/Math/RaycastUtils.hpp"

class ConvexPoly3DShape : public Entity {
public:
	ConvexPoly3DShape(Game* game, Vec3 position, ConvexPoly3D const& convexPoly);
	
	void Update(float deltaSeconds) override;
	void Render() const override;
	void RenderDiffuse() const override;
	void RenderHighlight() const;
	void RenderHighlightDiffuse() const;
	void RandomizeFaceColors();

	ConvexPoly3D m_convexPoly;
protected:
	std::vector<Rgba8> m_faceColors;

};