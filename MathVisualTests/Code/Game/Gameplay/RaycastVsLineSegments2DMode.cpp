#include "Game/Gameplay/RaycastVsLineSegments2DMode.hpp"
#include "Game/Gameplay/AABB2Shape2D.hpp"

RaycastVsLineSegments2DMode::RaycastVsLineSegments2DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = cameraSize;
	m_gameModeName = "Raycasts vs Line Segments 2D";
	m_helperText = "F8 to randomize. LMB/RMB set ray start/end; WASD moves start, IJKL moves end, arrows move ray. Hold T = slow";
	m_rayStart = Vec2(20.0f, 20.0f);
	m_rayEnd = Vec2(100.0f, 40.0f);
}

RaycastVsLineSegments2DMode::~RaycastVsLineSegments2DMode()
{
}

void RaycastVsLineSegments2DMode::Startup()
{
	for (int lineIndex = 0; lineIndex < m_amountOfLineSegments; lineIndex++) {

		float randomSize = rng.GetRandomFloatInRange(5.0f, 40.0f);
		Vec2 randPosStart = GetRandomPositionInWorld();
		float randomOrientation = rng.GetRandomFloatInRange(0.0f, 180.0f);
		Vec2 randDir(1.0f, 0.0f);
		randDir.SetOrientationDegrees(randomOrientation);

		LineSegment2* newLineSegment = new LineSegment2(randPosStart, randPosStart + (randDir * randomSize));

		if (newLineSegment->m_start.x < 0.0f) {
			newLineSegment->Translate(Vec2(newLineSegment->m_start.x + 1.0f, 0.0f));
		}

		if (newLineSegment->m_start.x > m_worldSize.x) {
			newLineSegment->Translate(Vec2(m_worldSize.x - newLineSegment->m_start.x - 1.0f, 0.0f));
		}

		if (newLineSegment->m_start.y < 0.0f) {
			newLineSegment->Translate(Vec2(0.0f, newLineSegment->m_start.y + 1.0f));
		}

		if (newLineSegment->m_start.y > m_worldSize.y) {
			newLineSegment->Translate(Vec2(0.0f, m_worldSize.y - newLineSegment->m_start.y - 1.0f));
		}

		if (newLineSegment->m_end.x < 0.0f) {
			newLineSegment->Translate(Vec2(newLineSegment->m_end.x + 1.0f, 0.0f));
		}

		if (newLineSegment->m_end.x > m_worldSize.x) {
			newLineSegment->Translate(Vec2(m_worldSize.x - newLineSegment->m_end.x - 1.0f, 0.0f));
		}

		if (newLineSegment->m_end.y < 0.0f) {
			newLineSegment->Translate(Vec2(0.0f, newLineSegment->m_end.y + 1.0f));
		}

		if (newLineSegment->m_end.y > m_worldSize.y) {
			newLineSegment->Translate(Vec2(0.0f, m_worldSize.y - newLineSegment->m_end.y - 1.0f));
		}

		m_allShapes.push_back(newLineSegment);
	}


}

void RaycastVsLineSegments2DMode::Shutdown()
{
	for (int lineSegmentIndex = 0; lineSegmentIndex < m_allShapes.size(); lineSegmentIndex++) {
		LineSegment2*& lineSegment = m_allShapes[lineSegmentIndex];
		if (lineSegment) {
			delete lineSegment;
			lineSegment = nullptr;
		}
	}

	m_allShapes.clear();
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
}

void RaycastVsLineSegments2DMode::Update(float deltaSeconds)
{
	AddVertsForRaycastVsLines2D();

	AddVertsForNormalColorLineSegments();
	AddVertsForHighlightedColorLineSegments();

	UpdateInput(deltaSeconds);
	GameMode::Update(deltaSeconds);
}

void RaycastVsLineSegments2DMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);

	g_theRenderer->BindTexture(nullptr);


	RenderNormalColorShapes();
	RenderHighlightedColorShapes();

	g_theRenderer->DrawVertexArray(m_raycastVsDiscCollisionVerts);

	std::vector<Vertex_PCU> arrowVerts;

	if (m_impactedBox) {
		if (m_raycastResult.m_impactDist >= 1.0f) {
			AddVertsForArrow2D(arrowVerts, m_rayStart, m_raycastResult.m_impactPos, Rgba8::RED, 0.5f);
		}
		AddVertsForArrow2D(arrowVerts, m_raycastResult.m_impactPos, m_rayEnd, Rgba8::GRAY, 0.5f);
	}
	else {
		AddVertsForArrow2D(arrowVerts, m_rayStart, m_rayEnd, Rgba8::GREEN, 0.5f);
	}

	g_theRenderer->DrawVertexArray(arrowVerts);
	g_theRenderer->EndCamera(m_worldCamera);

	RenderUI();

	DebugRenderWorld(m_worldCamera);
	DebugRenderScreen(m_UICamera);
}

void RaycastVsLineSegments2DMode::RenderNormalColorShapes() const
{
	g_theRenderer->DrawVertexArray(m_normalColoredVerts);
}

void RaycastVsLineSegments2DMode::RenderHighlightedColorShapes() const
{
	g_theRenderer->DrawVertexArray(m_highlightedColoredVerts);
}


void RaycastVsLineSegments2DMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	Vec2 rayStartMoveVelocity = Vec2::ZERO;
	Vec2 rayEndMoveVelocity = Vec2::ZERO;

	if (g_theInput->IsKeyDown('W') || g_theInput->IsKeyDown(KEYCODE_UPARROW)) {
		rayStartMoveVelocity = Vec2(0.0f, 1.0f);
	}

	if (g_theInput->IsKeyDown('A') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW)) {
		rayStartMoveVelocity = Vec2(-1.0f, 0.0f);
	}

	if (g_theInput->IsKeyDown('S') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW)) {
		rayStartMoveVelocity = Vec2(0.0f, -1.0f);
	}

	if (g_theInput->IsKeyDown('D') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW)) {
		rayStartMoveVelocity = Vec2(1.0f, 0.0f);
	}

	if (g_theInput->IsKeyDown('I') || g_theInput->IsKeyDown(KEYCODE_UPARROW)) {
		rayEndMoveVelocity = Vec2(0.0f, 1.0f);
	}

	if (g_theInput->IsKeyDown('J') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW)) {
		rayEndMoveVelocity = Vec2(-1.0f, 0.0f);
	}

	if (g_theInput->IsKeyDown('K') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW)) {
		rayEndMoveVelocity = Vec2(0.0f, -1.0f);
	}

	if (g_theInput->IsKeyDown('L') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW)) {
		rayEndMoveVelocity = Vec2(1.0f, 0.0f);
	}


	m_rayStart += rayStartMoveVelocity * m_dotSpeed * deltaSeconds;
	m_rayEnd += rayEndMoveVelocity * m_dotSpeed * deltaSeconds;

	AABB2 worldBoundingBox(Vec2::ZERO, m_worldSize);

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) {
		m_rayStart = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) {
		m_rayEnd = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}
}

void RaycastVsLineSegments2DMode::AddVertsForRaycastVsLines2D()
{
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
	m_raycastVsDiscCollisionVerts.clear();

	float rayDistance = GetDistance2D(m_rayStart, m_rayEnd);

	m_impactedBox = false;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDist = ARBITRARILY_LARGE_VALUE;
	int closestImpactedShapeIndex = -1;

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		LineSegment2* shape = m_allShapes[shapeIndex];
		if (!shape)continue;

		RaycastResult2D raycastResult;
		raycastResult = RaycastVsLineSegment2D(m_rayStart, (m_rayEnd - m_rayStart).GetNormalized(), rayDistance, *shape);


		if (raycastResult.m_didImpact) {
			if (raycastResult.m_impactDist < closestRaycast.m_impactDist) {
				closestRaycast = raycastResult;
				closestImpactedShapeIndex = shapeIndex;
			}
			m_impactedBox = true;
		}

	}

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		LineSegment2* shape = m_allShapes[shapeIndex];
		if (shapeIndex != closestImpactedShapeIndex) {
			m_normalColorShapes.push_back(shape);
		}

	}

	m_raycastResult = closestRaycast;
	if (m_impactedBox) {
		AddVertsForRaycastImpactOnLine(*m_allShapes[closestImpactedShapeIndex], m_raycastResult);
	}
}

void RaycastVsLineSegments2DMode::AddVertsForRaycastImpactOnLine(LineSegment2& shape, RaycastResult2D& raycastResult)
{
	m_highlightedShapes.push_back(&shape);
	Vec2 const& impactPos = raycastResult.m_impactPos;
	Vec2 impactPosArrowEnd = impactPos + raycastResult.m_impactNormal * 5.0f;

	AddVertsForDisc2D(m_raycastVsDiscCollisionVerts, impactPos, 0.5f, Rgba8::WHITE);
	AddVertsForArrow2D(m_raycastVsDiscCollisionVerts, impactPos, impactPosArrowEnd, Rgba8::YELLOW, 0.5f, 1.0f);
}

void RaycastVsLineSegments2DMode::AddVertsForNormalColorLineSegments()
{
	static Rgba8 const transparentBlue(0, 0, 255, 120);

	m_normalColoredVerts.clear();
	for (int lineSegmentIndex = 0; lineSegmentIndex < m_normalColorShapes.size(); lineSegmentIndex++) {
		LineSegment2* lineSegment = m_normalColorShapes[lineSegmentIndex];
		if (!lineSegment) continue;
		AddVertsForLineSegment2D(m_normalColoredVerts, *lineSegment, transparentBlue, 0.65f);
	}

}

void RaycastVsLineSegments2DMode::AddVertsForHighlightedColorLineSegments()
{
	static Rgba8 const solidBlue(0, 0, 255, 220);

	m_highlightedColoredVerts.clear();
	for (int lineSegmentIndex = 0; lineSegmentIndex < m_highlightedShapes.size(); lineSegmentIndex++) {
		LineSegment2* lineSegment = m_highlightedShapes[lineSegmentIndex];
		if (!lineSegment) continue;
		AddVertsForLineSegment2D(m_highlightedColoredVerts, *lineSegment, solidBlue, 0.65f);
	}

}

void RaycastVsLineSegments2DMode::UpdateInputFromKeyboard()
{
}

void RaycastVsLineSegments2DMode::UpdateInputFromController()
{
}

