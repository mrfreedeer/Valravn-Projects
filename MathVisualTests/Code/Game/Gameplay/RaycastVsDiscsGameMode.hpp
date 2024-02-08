#pragma once
#include "Game/Gameplay/GameMode.hpp"

class RaycastVsDiscMode : public GameMode {
public:
	RaycastVsDiscMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize);
	~RaycastVsDiscMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	virtual void UpdateInput(float deltaSeconds) override;
	void AddVertsForRaycastVsDiscs2D();
	void AddVertsForRaycastImpactOnDisc(Shape2D& shape, RaycastResult2D& raycastResult);

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderNormalColorShapes() const;
	void RenderHighlightedColorShapes() const;
private:
	std::vector<Shape2D*> m_allShapes;
	std::vector<Shape2D*> m_highlightedShapes;
	std::vector<Shape2D*> m_normalColorShapes;
	std::vector<Vertex_PCU> m_raycastVsDiscCollisionVerts;

	Vec2 m_rayStart = Vec2::ZERO;
	Vec2 m_rayEnd = Vec2::ZERO;

	RaycastResult2D m_raycastResult;
	bool m_impactedDisc = false;
	float m_dotSpeed = g_gameConfigBlackboard.GetValue("DOT_SPEED", 40.0f);

};