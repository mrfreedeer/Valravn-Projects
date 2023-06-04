#pragma once
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/GameMode.hpp"

class RaycastVsLineSegments2DMode : public GameMode {
public:
	RaycastVsLineSegments2DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);
	~RaycastVsLineSegments2DMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	virtual void UpdateInput(float deltaSeconds) override;
	void AddVertsForRaycastVsLines2D();
	void AddVertsForRaycastImpactOnLine(LineSegment2& shape, RaycastResult2D& raycastResult);

	void AddVertsForNormalColorLineSegments();
	void AddVertsForHighlightedColorLineSegments();

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void RenderNormalColorShapes() const;
	void RenderHighlightedColorShapes() const;
private:
	std::vector<LineSegment2*> m_allShapes;
	std::vector<LineSegment2*> m_highlightedShapes;
	std::vector<LineSegment2*> m_normalColorShapes;
	std::vector<Vertex_PCU> m_raycastVsDiscCollisionVerts;

	std::vector<Vertex_PCU> m_normalColoredVerts;
	std::vector<Vertex_PCU> m_highlightedColoredVerts;

	Vec2 m_rayStart = Vec2::ZERO;
	Vec2 m_rayEnd = Vec2::ZERO;

	RaycastResult2D m_raycastResult;
	bool m_impactedBox = false;
	float m_dotSpeed = g_gameConfigBlackboard.GetValue("DOT_SPEED", 40.0f);

	int m_amountOfLineSegments = g_gameConfigBlackboard.GetValue("AMOUNT_OF_2D_LINESEGMENTS", 15);

};