#pragma once
#include "Game/Gameplay/GameMode.hpp"
#include "Engine/Math/RaycastUtils.hpp"


class BilliardsMode : public GameMode {
public:
	BilliardsMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize);
	~BilliardsMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	virtual void UpdateInput(float deltaSeconds) override;

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();
	void AddVertsForRaycastVsDiscs2D();
	void AddVertsForRaycastImpactOnDisc(Shape2D& shape, RaycastResult2D& raycastResult);
	void RenderHighlightedColorShapes() const;
	void RenderNormalColorShapes() const;
	
	void CheckBilliardsCollisions();
	void CheckBilliardsCollisionsWithWalls();
	void SpawnNewBilliard(Vec2 const& pos, Vec2 const& velocity);

private:
	std::vector<Shape2D*> m_allShapes;
	std::vector<Shape2D*> m_highlightedShapes;
	std::vector<Shape2D*> m_normalColorShapes;
	std::vector<Vertex_PCU> m_raycastVsDiscCollisionVerts;

	Vec2 m_rayStart = Vec2::ZERO;
	Vec2 m_rayEnd = Vec2::ZERO;

	RaycastResult2D m_raycastResult;
	bool m_impactedDisc = false;

	float m_billiardsSize = g_gameConfigBlackboard.GetValue("BILLIARD_SIZE", 3.0f);

	int m_minBilliardBumpers = g_gameConfigBlackboard.GetValue("BILLIARDS_MIN_BUMPERS", 10);
	int m_maxBilliardBumpers = g_gameConfigBlackboard.GetValue("BILLIARDS_MAX_BUMPERS", 12);
};