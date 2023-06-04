#pragma once

#include "Engine/Math/Curves.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Game/Gameplay/GameMode.hpp"


class SplinesMode : public GameMode {

	typedef float (*EasingFunction)(float tZeroToOne);
public:
	SplinesMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize);
	~SplinesMode();

	virtual void Startup() override;
	virtual void Shutdown() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

private:
	virtual void UpdateInput(float deltaSeconds) override;

	virtual void UpdateInputFromKeyboard();
	virtual void UpdateInputFromController();

	void AddVertsForBezierCurve(std::vector<Vertex_PCU>& verts) const;
	void AddVertsForEasingFunction(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const;
	void AddVertsForSpline(std::vector<Vertex_PCU>& verts) const;
	AABB2 m_graphsPaneUVs = g_gameConfigBlackboard.GetValue("SPLINES_GRAPHS_AABB2", AABB2::ZERO_TO_ONE);

	AABB2 m_easingPaneUVs = g_gameConfigBlackboard.GetValue("SPLINES_EASING_AABB2", AABB2::ZERO_TO_ONE);
	AABB2 m_easingPaneGraphUVs = g_gameConfigBlackboard.GetValue("SPLINES_EASING_GRAPH_AABB2", AABB2::ZERO_TO_ONE);
	AABB2 m_curvesPaneUVs = g_gameConfigBlackboard.GetValue("SPLINES_CURVES_AABB2", AABB2::ZERO_TO_ONE);
	AABB2 m_splinesPaneUVs = g_gameConfigBlackboard.GetValue("SPLINES_SPLINES_AABB2", AABB2::ZERO_TO_ONE);
	AABB2 m_splinesPaneBoundsUVs = g_gameConfigBlackboard.GetValue("SPLINES_SPLINES_BOUNDS_AABB2", AABB2::ZERO_TO_ONE);
	AABB2 m_worldAABB2 = AABB2::ZERO_TO_ONE;
	AABB2 m_graphsPane = AABB2::ZERO_TO_ONE;
	AABB2 m_curvesPane = AABB2::ZERO_TO_ONE;
	AABB2 m_easingPane = AABB2::ZERO_TO_ONE;
	AABB2 m_easingPaneGraph = AABB2::ZERO_TO_ONE;
	AABB2 m_splinesPane = AABB2::ZERO_TO_ONE;
	AABB2 m_splinesBounds = AABB2::ZERO_TO_ONE;

	BezierCurve* m_bezierCurve = nullptr;
	Spline2D* m_spline = nullptr;

	int m_bezierAmountGuidePoints = g_gameConfigBlackboard.GetValue("SPLINES_BEZIER_AMOUNT_GUIDE_POINTS", 3);
	int m_amountOfSubdivisions = g_gameConfigBlackboard.GetValue("SPLINES_AMOUNT_OF_SUBDIVISIONS", 16);
	int m_splineAmountOfPoints = 0;
	
	IntRange m_splineAmountOfPointsRange = g_gameConfigBlackboard.GetValue("SPLINES_AMOUNT_OF_SPLINE_POINTS_RANGE", IntRange::ZERO_TO_ONE);
	float m_curveApproximateLength = 0.0f;
	float m_splineAproximateLength = 0.0f;
	float m_easingGraphSize = g_gameConfigBlackboard.GetValue("SPLINES_EASING_GRAPH_SIZE", 30.0f);
	float m_currenT = 0.0f;
	float m_stepPerDivision = 0.0f;
	float m_discSize = g_gameConfigBlackboard.GetValue("SPLINES_MOVING_DISC_SIZE", 0.5f);

	std::vector<EasingFunction> m_easingFunctions;
	std::vector<std::string> m_easingFunctionsNames;
	int m_easingFunctionIndex = 0;
	bool m_recalculateLengths = false;
};




