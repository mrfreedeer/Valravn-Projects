#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Easing.hpp"
#include "Game/Gameplay/SplinesMode.hpp"
#include "Game/Gameplay/CapsuleShape2D.hpp"
#include "Game/Gameplay/OBB2Shape2D.hpp"
#include "Game/Gameplay/AABB2Shape2D.hpp"
#include "Game/Gameplay/CapsuleShape2D.hpp"
#include "Game/Gameplay/DiscShape2D.hpp"

SplinesMode::SplinesMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = worldSize;
	m_gameModeName = "Easing, Curves, Splines (2D)";
	m_helperText = "F8 to randomize. W/E prev/next easing function. N/M decrease/increase amount of subdivisions(";
}

SplinesMode::~SplinesMode()
{
	delete m_bezierCurve;
	m_bezierCurve = nullptr;

	delete m_spline;
	m_spline = nullptr;
}

void SplinesMode::Startup()
{
	m_worldAABB2 = AABB2(Vec2::ZERO, m_worldSize);

	m_graphsPane = m_worldAABB2.GetBoxWithin(m_graphsPaneUVs);

	m_curvesPane = m_graphsPane.GetBoxWithin(m_curvesPaneUVs);
	m_easingPane = m_graphsPane.GetBoxWithin(m_easingPaneUVs);
	m_easingPaneGraph = m_easingPane.GetBoxWithin(m_easingPaneGraphUVs);
	m_easingPaneGraph.m_maxs = m_easingPaneGraph.m_mins + Vec2(m_easingGraphSize, m_easingGraphSize);
	m_splinesPane = m_graphsPane.GetBoxWithin(m_splinesPaneUVs);

	std::vector<Vec2> randomBezierPoints;
	for (int pointIndex = 0; pointIndex < m_bezierAmountGuidePoints; pointIndex++) {
		float randU = rng.GetRandomFloatZeroUpToOne();
		float randV = rng.GetRandomFloatZeroUpToOne();

		Vec2 randPoint = m_curvesPane.GetPointAtUV(randU, randV);
		randomBezierPoints.push_back(randPoint);
	}

	m_bezierCurve = new BezierCurve(randomBezierPoints);
	m_curveApproximateLength = m_bezierCurve->GetApproximateLength(m_amountOfSubdivisions);

	std::vector<Vec2> randomSplinePoints;
	m_splinesBounds = m_splinesPane.GetBoxWithin(m_splinesPaneBoundsUVs);

	m_splineAmountOfPoints = rng.GetRandomIntInRange(m_splineAmountOfPointsRange);
	float uStep = 1.0f / (float)m_splineAmountOfPoints;
	for (int pointIndex = 0; pointIndex < m_splineAmountOfPoints; pointIndex++) {
		float randU = rng.GetRandomFloatInRange(uStep * pointIndex, uStep * (pointIndex + 1));
		float randUVariation = (rng.GetRandomIntInRange(0, 1) == 1) ? 0.05f : -0.05f;
		randU += randUVariation;
		float randV = rng.GetRandomFloatZeroUpToOne();

		Vec2 randPoint = m_splinesBounds.GetPointAtUV(randU, randV);
		randomSplinePoints.push_back(randPoint);
	}

	m_spline = new Spline2D(randomSplinePoints);
	m_splineAproximateLength = m_spline->GetApproximateLength(m_amountOfSubdivisions);

	m_easingFunctions.push_back(SmoothStart2);
	m_easingFunctions.push_back(SmoothStart3);
	m_easingFunctions.push_back(SmoothStart4);
	m_easingFunctions.push_back(SmoothStart5);
	m_easingFunctions.push_back(SmoothStart6);

	m_easingFunctions.push_back(SmoothStop2);
	m_easingFunctions.push_back(SmoothStop3);
	m_easingFunctions.push_back(SmoothStop4);
	m_easingFunctions.push_back(SmoothStop5);
	m_easingFunctions.push_back(SmoothStop6);

	m_easingFunctions.push_back(SmoothStep3);
	m_easingFunctions.push_back(SmoothStep5);

	m_easingFunctions.push_back(Hesitate3);
	m_easingFunctions.push_back(Hesitate5);
	m_easingFunctions.push_back(CustomEasingFunction);

	m_easingFunctionsNames.push_back("SmoothStart2");
	m_easingFunctionsNames.push_back("SmoothStart3");
	m_easingFunctionsNames.push_back("SmoothStart4");
	m_easingFunctionsNames.push_back("SmoothStart5");
	m_easingFunctionsNames.push_back("SmoothStart6");

	m_easingFunctionsNames.push_back("SmoothStop2");
	m_easingFunctionsNames.push_back("SmoothStop3");
	m_easingFunctionsNames.push_back("SmoothStop4");
	m_easingFunctionsNames.push_back("SmoothStop5");
	m_easingFunctionsNames.push_back("SmoothStop6");

	m_easingFunctionsNames.push_back("SmoothStep3");
	m_easingFunctionsNames.push_back("SmoothStep5");

	m_easingFunctionsNames.push_back("Hesitate3");
	m_easingFunctionsNames.push_back("Hesitate5");
	m_easingFunctionsNames.push_back("CustomEasingFunction");
}

void SplinesMode::Shutdown()
{

}

void SplinesMode::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);

	if (m_recalculateLengths) {
		m_curveApproximateLength = m_bezierCurve->GetApproximateLength(m_amountOfSubdivisions);
		m_splineAproximateLength = m_spline->GetApproximateLength(m_amountOfSubdivisions);
	}


	GameMode::Update(deltaSeconds);
	m_currenT = fmodf((float)m_clock.GetTotalTime() * 0.25f, 1.0f);
	m_stepPerDivision = 1.0f / m_amountOfSubdivisions;
	m_helperText = "F8 to randomize. W/E prev/next easing function. N/M decrease/increase amount of subdivisions(";
	m_helperText += Stringf("%d). Hold T = slow ", m_amountOfSubdivisions);
}

void SplinesMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	g_theRenderer->BindTexture(nullptr);

	std::vector<Vertex_PCU> verts;
	std::vector<Vertex_PCU> textVerts;

	AddVertsForBezierCurve(verts);
	AddVertsForEasingFunction(verts, textVerts);
	AddVertsForSpline(verts);
	g_theRenderer->DrawVertexArray(verts);

	g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);
	g_theRenderer->EndCamera(m_worldCamera);

	RenderUI();


}

void SplinesMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	UpdateInputFromKeyboard();
}



void SplinesMode::UpdateInputFromKeyboard()
{
	m_recalculateLengths = false;
	if (g_theInput->WasKeyJustPressed('E')) {
		m_easingFunctionIndex++;
	}
	if (g_theInput->WasKeyJustPressed('W')) {
		m_easingFunctionIndex--;
	}

	if (g_theInput->WasKeyJustPressed('N')) {
		m_amountOfSubdivisions /= 2;
		m_recalculateLengths = true;
	}

	if (g_theInput->WasKeyJustPressed('M')) {
		m_amountOfSubdivisions *= 2;
		m_recalculateLengths = true;
	}

	if (m_easingFunctionIndex < 0) m_easingFunctionIndex = (int)m_easingFunctions.size() - 1;
	if (m_easingFunctionIndex == m_easingFunctions.size()) m_easingFunctionIndex = 0;
	if (m_amountOfSubdivisions == 0) m_amountOfSubdivisions = 1;
}

void SplinesMode::UpdateInputFromController()
{
}

void SplinesMode::AddVertsForBezierCurve(std::vector<Vertex_PCU>& verts) const
{
	m_bezierCurve->AddVertsForControlPoints(verts, Rgba8::GRAY, 0.8f);
	Vec2 previousPoint = m_bezierCurve->EvaluateAtParametric(0.0f);

	for (int divisionIndex = 1; divisionIndex <= m_amountOfSubdivisions; divisionIndex++) {
		Vec2 currentPoint = m_bezierCurve->EvaluateAtParametric(m_stepPerDivision * divisionIndex);
		LineSegment2 currentLine(previousPoint, currentPoint);
		AddVertsForLineSegment2D(verts, currentLine, Rgba8::GREEN, 0.64f, false);
		previousPoint = currentPoint;
	}

	float distanceForCurve = m_curveApproximateLength * m_currenT;
	Vec2 parametricPoint = m_bezierCurve->EvaluateAtParametric(m_currenT);
	Vec2 distancePoint = m_bezierCurve->EvaluateAtApproximateDistance(distanceForCurve, m_amountOfSubdivisions);

	AddVertsForDisc2D(verts, parametricPoint, m_discSize, Rgba8::WHITE);
	AddVertsForDisc2D(verts, distancePoint, m_discSize, Rgba8::ORANGE);

}

void SplinesMode::AddVertsForEasingFunction(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const
{
	Vec2 startEasingPoint = m_easingPaneGraph.m_mins;
	startEasingPoint.x += 0.0f;
	float distanceToEnd = m_easingGraphSize;

	Vec2 endEasingPoint = m_easingPaneGraph.m_maxs;

	AABB2 textAABB2 = m_easingPaneGraph;
	textAABB2.m_mins.y = m_easingPane.m_mins.y;
	textAABB2.m_maxs.y = m_easingPaneGraph.m_mins.y;

	AddVertsForAABB2D(verts, m_easingPaneGraph, Rgba8::LIGHTBLUE);
	Vec2 previousPoint = startEasingPoint;

	for (int divisionIndex = 1; divisionIndex <= m_amountOfSubdivisions; divisionIndex++) {
		float normalT = m_stepPerDivision * divisionIndex;
		float easedT = m_easingFunctions[m_easingFunctionIndex](m_stepPerDivision* divisionIndex);
		Vec2 currentPoint = startEasingPoint;
		currentPoint.x += distanceToEnd * normalT;
		currentPoint.y += distanceToEnd * easedT;
		LineSegment2 currentLine(previousPoint, currentPoint);
		AddVertsForLineSegment2D(verts, currentLine, Rgba8::GREEN, 0.6f, false);
		previousPoint = currentPoint;
	}

	float easedT = m_easingFunctions[m_easingFunctionIndex](m_currenT);
	Vec2 parametricPoint = startEasingPoint;
	parametricPoint.x += distanceToEnd * m_currenT;
	parametricPoint.y += distanceToEnd * easedT;
	AddVertsForDisc2D(verts, parametricPoint, m_discSize, Rgba8::WHITE);

	g_squirrelFont->AddVertsForTextInBox2D(textVerts, textAABB2, m_textCellHeight, m_easingFunctionsNames[m_easingFunctionIndex]);

}

void SplinesMode::AddVertsForSpline(std::vector<Vertex_PCU>& verts) const
{
	float currentT = fmodf((float)m_clock.GetTotalTime() * 0.25f, (float)(m_spline->GetAmountOfCurves()));
	float currentTForDriving = currentT / ((float)m_spline->GetAmountOfCurves());
	//AddVertsForAABB2D(verts, m_splinesPane, Rgba8::RED);
	m_spline->AddVertsForControlPoints(verts, Rgba8::GRAY, 0.8f);

	Vec2 previousPoint = m_spline->EvaluateAtParametric(0.0f);

	for (int divisionIndex = 1; divisionIndex <= (m_amountOfSubdivisions * (m_spline->GetAmountOfCurves())); divisionIndex++) {
		Vec2 currentPoint = m_spline->EvaluateAtParametric(m_stepPerDivision * divisionIndex);
		LineSegment2 currentLine(previousPoint, currentPoint);
		AddVertsForLineSegment2D(verts, currentLine, Rgba8::GREEN, 0.64f, false);
		previousPoint = currentPoint;
	}
	

	float distanceForCurve = m_splineAproximateLength * currentTForDriving;
	Vec2 parametricPoint = m_spline->EvaluateAtParametric(currentT);
	Vec2 distancePoint = m_spline->EvaluateAtApproximateDistance(distanceForCurve, m_amountOfSubdivisions);

	AddVertsForDisc2D(verts, parametricPoint, m_discSize, Rgba8::WHITE);
	AddVertsForDisc2D(verts, distancePoint, m_discSize, Rgba8::ORANGE);

}


