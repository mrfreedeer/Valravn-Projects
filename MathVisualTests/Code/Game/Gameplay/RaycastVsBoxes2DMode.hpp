#pragma once
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/GameMode.hpp"

class RaycastVsBoxes2DMode : public GameMode {
public:
	RaycastVsBoxes2DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);
	~RaycastVsBoxes2DMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	virtual void UpdateInput(float deltaSeconds) override;
	void AddVertsForRaycastVsBoxes2D();
	void AddVertsForRaycastImpactOnBox(Shape2D& shape, RaycastResult2D& raycastResult);

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderNormalColorShapes() const;
	void RenderHighlightedColorShapes() const;
private:
	std::vector<Shape2D*> m_allShapes;
	std::vector<Shape2D*> m_highlightedShapes;
	std::vector<Shape2D*> m_normalColorShapes;
	std::vector<Vertex_PCU> m_raycastVsBoxesCollisionVerts;

	Vec2 m_rayStart = Vec2::ZERO;
	Vec2 m_rayEnd = Vec2::ZERO;

	RaycastResult2D m_raycastResult;
	bool m_impactedBox = false;
	float m_dotSpeed = g_gameConfigBlackboard.GetValue("DOT_SPEED", 40.0f);

	int m_amountOfBoxes = g_gameConfigBlackboard.GetValue("AMOUNT_OF_2D_BOXES", 10);

};