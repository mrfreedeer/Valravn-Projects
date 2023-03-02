#pragma once
#include "Game/Gameplay/Entity.hpp"

class Texture;

enum class PropRenderType {
	CUBE = 0,
	GRID,
	SPHERE,
	NUM_PROP_RENDER_TYPES
};

class Prop : public Entity {
public:
	Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, PropRenderType renderType = PropRenderType::CUBE, IntVec2 gridSize = IntVec2(100,100));
	Prop(Game* pointerToGame, Vec3 const& startingWorldPosition, float radius, PropRenderType renderType = PropRenderType::SPHERE);
	~Prop();

	void Update(float deltaSeconds) override;
	void Render() const override;

private:
	void InitializeLocalVerts();

	void InitiliazeLocalVertsCube();
	void InitiliazeLocalVertsGrid();
	void InitializeLocalVertsSphere();


	void RenderMultiColoredCube() const;
	void RenderGrid() const;
	void RenderSphere() const;

	Texture* m_texture = nullptr;
	PropRenderType m_type = PropRenderType::CUBE;
	IntVec2 m_gridSize = IntVec2::ZERO;
	float m_sphereRadius = 0.0f;

};