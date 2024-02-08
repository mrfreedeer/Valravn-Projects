#include "Game/Gameplay/RaycastVsOBB2Mode.hpp"
#include "Game/Gameplay/OBB2Shape2D.hpp"

RaycastVsOBB2Mode::RaycastVsOBB2Mode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = cameraSize;
	m_gameModeName = "Raycasts vs OBB2 2D";
	m_helperText = "F8 to randomize. LMB/RMB set ray start/end; WASD moves start, IJKL moves end, arrows move ray. Hold T = slow";
	m_rayStart = Vec2(20.0f, 20.0f);
	m_rayEnd = Vec2(100.0f, 40.0f);
}

RaycastVsOBB2Mode::~RaycastVsOBB2Mode()
{
}

void RaycastVsOBB2Mode::Startup()
{
	Rgba8 transparentBlue(0, 0, 255, 120);
	Rgba8 solidBlue(0, 0, 255, 220);
	for (int obb2Index = 0; obb2Index < m_amountOfBoxes; obb2Index++) {
		float randDimX = rng.GetRandomFloatInRange(10.0f, 20.0f);
		float randDimY = rng.GetRandomFloatInRange(10.0f, 20.0f);
		float randOrientation = rng.GetRandomFloatInRange(0.0f, 90.0f);

		float maxDim = (randDimX > randDimY) ? randDimX : randDimY;

		OBB2Shape2D* newBox = new OBB2Shape2D(Vec2(randDimX, randDimY), GetClampedRandomPositionInWorld(maxDim), randOrientation, transparentBlue, solidBlue);
		m_allShapes.push_back(newBox);
	}
}

void RaycastVsOBB2Mode::Shutdown()
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D*& shape = m_allShapes[shapeIndex];
		if (shape) {
			delete shape;
			shape = nullptr;
		}
	}

	m_allShapes.clear();
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
}

void RaycastVsOBB2Mode::Update(float deltaSeconds)
{
	AddVertsForRaycastVsBoxes2D();

	UpdateInput(deltaSeconds);
	GameMode::Update(deltaSeconds);
}

void RaycastVsOBB2Mode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->BindTexture(nullptr);


	RenderNormalColorShapes();
	RenderHighlightedColorShapes();

	g_theRenderer->DrawVertexArray(m_raycastVsBoxCollisionVerts);

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
}

void RaycastVsOBB2Mode::RenderNormalColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_normalColorShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_normalColorShapes[shapeIndex];
		if (!shape) continue;
		shape->Render();
	}
}

void RaycastVsOBB2Mode::RenderHighlightedColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_highlightedShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_highlightedShapes[shapeIndex];
		if (!shape) continue;
		shape->RenderHighlighted();
	}
}


void RaycastVsOBB2Mode::UpdateInput(float deltaSeconds)
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

void RaycastVsOBB2Mode::AddVertsForRaycastVsBoxes2D()
{
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
	m_raycastVsBoxCollisionVerts.clear();

	float rayDistance = GetDistance2D(m_rayStart, m_rayEnd);

	m_impactedBox = false;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDist = ARBITRARILY_LARGE_VALUE;
	int closestImpactedShapeIndex = -1;

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		OBB2Shape2D const* shapeAsOBB2 = dynamic_cast<OBB2Shape2D*>(shape);
		if (!shape)continue;

		RaycastResult2D raycastResult;
		raycastResult = RaycastVsOBB2D(m_rayStart, (m_rayEnd - m_rayStart).GetNormalized(), rayDistance, shapeAsOBB2->m_OBB2);


		if (raycastResult.m_didImpact) {
			if (raycastResult.m_impactDist < closestRaycast.m_impactDist) {
				closestRaycast = raycastResult;
				closestImpactedShapeIndex = shapeIndex;
			}
			m_impactedBox = true;
		}

	}

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		if (shapeIndex != closestImpactedShapeIndex) {
			m_normalColorShapes.push_back(shape);
		}

	}

	m_raycastResult = closestRaycast;
	if (m_impactedBox) {
		AddVertsForRaycastImpactOnBox(*m_allShapes[closestImpactedShapeIndex], m_raycastResult);
	}
}

void RaycastVsOBB2Mode::AddVertsForRaycastImpactOnBox(Shape2D& shape, RaycastResult2D& raycastResult)
{
	m_highlightedShapes.push_back(&shape);
	Vec2 const& impactPos = raycastResult.m_impactPos;
	Vec2 impactPosArrowEnd = impactPos + raycastResult.m_impactNormal * 5.0f;

	AddVertsForDisc2D(m_raycastVsBoxCollisionVerts, impactPos, 0.5f, Rgba8::WHITE);
	AddVertsForArrow2D(m_raycastVsBoxCollisionVerts, impactPos, impactPosArrowEnd, Rgba8::YELLOW, 0.5f, 1.0f);
}

void RaycastVsOBB2Mode::UpdateInputFromKeyboard()
{
}

void RaycastVsOBB2Mode::UpdateInputFromController()
{
}

