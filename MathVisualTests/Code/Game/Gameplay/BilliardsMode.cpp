#include "Game/Gameplay/BilliardsMode.hpp"
#include "Game/Gameplay/DiscShape2D.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"

BilliardsMode::BilliardsMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = cameraSize;
	m_gameModeName = "Billiards";
	m_helperText = "F8 to randomize. Hold T = slow";
	m_rayStart = Vec2(20.0f, 20.0f);
	m_rayEnd = Vec2(100.0f, 40.0f);
}

BilliardsMode::~BilliardsMode()
{
}

void BilliardsMode::Startup()
{
	int amountOfBumpers = rng.GetRandomIntInRange(m_minBilliardBumpers, m_maxBilliardBumpers);
	m_allShapes.reserve((size_t)amountOfBumpers);

	for (int bumperIndex = 0; bumperIndex < amountOfBumpers; bumperIndex++) {
		float radius = rng.GetRandomFloatInRange(3.0f, 8.0f);
		Vec2 position = GetClampedRandomPositionInWorld(radius);
		float elasticity = rng.GetRandomFloatInRange(0.1f, 0.9f);
		Rgba8 color = Rgba8::InterpolateColors(Rgba8::RED, Rgba8::GREEN, elasticity);
		Rgba8 transparentColor = color;
		transparentColor.a = 100;

		DiscShape2D* newShape = new DiscShape2D(position, radius, transparentColor, color, elasticity);
		newShape->m_canBeMoved = false;

		m_allShapes.push_back(newShape);
	}

}

void BilliardsMode::Shutdown()
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

void BilliardsMode::Update(float deltaSeconds)
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		if (shape) {
			shape->Update(deltaSeconds);
		}
	}

	CheckBilliardsCollisionsWithWalls();
	CheckBilliardsCollisions();

	AddVertsForRaycastVsDiscs2D();

	UpdateInput(deltaSeconds);
	GameMode::Update(deltaSeconds);

	float fps = 1.0f / (float)Clock::GetSystemClock().GetDeltaTime();
	DebugAddMessage(Stringf("FPS: %.2f", fps), 0, Rgba8::WHITE, Rgba8::WHITE);
}

void BilliardsMode::AddVertsForRaycastVsDiscs2D()
{
	m_normalColorShapes.clear();
	m_highlightedShapes.clear();
	m_raycastVsDiscCollisionVerts.clear();

	float rayDistance = GetDistance2D(m_rayStart, m_rayEnd);

	m_impactedDisc = false;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDist = ARBITRARILY_LARGE_VALUE;
	int closestImpactedShapeIndex = -1;

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		DiscShape2D const* shapeAsDisc = dynamic_cast<DiscShape2D*>(shape);
		if (!shape)continue;

		RaycastResult2D raycastResult;
		raycastResult = RaycastVsDisc(m_rayStart, (m_rayEnd - m_rayStart).GetNormalized(), rayDistance, shapeAsDisc->m_position, shapeAsDisc->m_radius);

		if (raycastResult.m_didImpact) {
			if (raycastResult.m_impactDist < closestRaycast.m_impactDist) {
				closestRaycast = raycastResult;
				closestImpactedShapeIndex = shapeIndex;
			}
			m_impactedDisc = true;
		}

	}

	m_raycastResult = closestRaycast;
	if (m_impactedDisc) {
		AddVertsForRaycastImpactOnDisc(*m_allShapes[closestImpactedShapeIndex], m_raycastResult);
	}
}

void BilliardsMode::AddVertsForRaycastImpactOnDisc(Shape2D& shape, RaycastResult2D& raycastResult)
{
	m_highlightedShapes.push_back(&shape);
	Vec2 const& impactPos = raycastResult.m_impactPos;
	Vec2 impactPosArrowEnd = impactPos + raycastResult.m_impactNormal * 5.0f;

	AddVertsForDisc2D(m_raycastVsDiscCollisionVerts, impactPos, 0.5f, Rgba8::WHITE);
	AddVertsForArrow2D(m_raycastVsDiscCollisionVerts, impactPos, impactPosArrowEnd, Rgba8::YELLOW, 0.5f, 1.0f);
}

void BilliardsMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(nullptr);

	RenderNormalColorShapes();
	RenderHighlightedColorShapes();

	g_theRenderer->DrawVertexArray(m_raycastVsDiscCollisionVerts);

	std::vector<Vertex_PCU> arrowVerts;

	if (m_impactedDisc) {
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

void BilliardsMode::RenderNormalColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_allShapes[shapeIndex];
		if (!shape) continue;
		shape->Render();
	}
}

void BilliardsMode::CheckBilliardsCollisions()
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		DiscShape2D* shape = reinterpret_cast<DiscShape2D*>(m_allShapes[shapeIndex]);
		if (!shape) continue;

		for (int otherShapeIndex = shapeIndex + 1; otherShapeIndex < m_allShapes.size(); otherShapeIndex++) {
			DiscShape2D* otherShape = reinterpret_cast<DiscShape2D*>(m_allShapes[otherShapeIndex]);
			if (!otherShape) continue;
			bool neitherCanMove = (!shape->m_canBeMoved) && (!otherShape->m_canBeMoved);
			if(neitherCanMove) continue;

			float combinedElasticity = shape->m_elasticity * otherShape->m_elasticity;
			if (shape->m_canBeMoved && !otherShape->m_canBeMoved) {
				BounceDiscOffDisc2D(shape->m_position, shape->m_velocity, shape->m_radius, otherShape->m_position, otherShape->m_radius, combinedElasticity);
			}
			else if (!shape->m_canBeMoved && otherShape->m_canBeMoved) {
				BounceDiscOffDisc2D(otherShape->m_position, otherShape->m_velocity, otherShape->m_radius, shape->m_position, shape->m_radius,  combinedElasticity);
			}
			else {
				BounceDiscOffEachOther2D(shape->m_position, shape->m_velocity, shape->m_radius, otherShape->m_position, otherShape->m_velocity, otherShape->m_radius, combinedElasticity);
			}
		}
	}
}

void BilliardsMode::CheckBilliardsCollisionsWithWalls()
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		DiscShape2D* shape = reinterpret_cast<DiscShape2D*>(m_allShapes[shapeIndex]);
		if (!shape) continue;

		// Touches Left Wall
		if ((shape->m_position.x - shape->m_radius) <= 0.0f) {
			BounceDiscOffPoint2D(shape->m_position, shape->m_velocity, shape->m_radius, Vec2(0.0f, shape->m_position.y), 0.9f);
		}

		// Touches Right Wall
		if ((shape->m_position.x + shape->m_radius) >= m_worldSize.x) {
			BounceDiscOffPoint2D(shape->m_position, shape->m_velocity, shape->m_radius, Vec2(m_worldSize.x, shape->m_position.y), 0.9f);
		}

		// Touches Top Wall
		if ((shape->m_position.y + shape->m_radius) >= m_worldSize.y) {
			BounceDiscOffPoint2D(shape->m_position, shape->m_velocity, shape->m_radius, Vec2(shape->m_position.x, m_worldSize.y), 0.9f);
		}

		// Touches Bottom Wall
		if ((shape->m_position.y - shape->m_radius) <= 0.0f) {
			BounceDiscOffPoint2D(shape->m_position, shape->m_velocity, shape->m_radius, Vec2(shape->m_position.x, 0.0f), 0.9f);
		}
	}
}

void BilliardsMode::SpawnNewBilliard(Vec2 const& pos, Vec2 const& velocity)
{
	AABB2 worldBounds(Vec2::ZERO, m_worldSize);
	if (!worldBounds.IsPointInside(pos)) return;
	Rgba8 color = Rgba8::BLUE;
	Rgba8 transparentColor = color;
	transparentColor.a = 140;
	Shape2D* newBilliard = new DiscShape2D(pos, m_billiardsSize, transparentColor, color, 0.9f);
	newBilliard->m_velocity = velocity;

	m_allShapes.push_back(newBilliard);
}

void BilliardsMode::RenderHighlightedColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_highlightedShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_highlightedShapes[shapeIndex];
		if (!shape) continue;
		shape->RenderHighlighted();
	}
}

void BilliardsMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	UpdateInputFromKeyboard();
	UpdateInputFromController();
}

void BilliardsMode::UpdateInputFromKeyboard()
{
	Vec2 rayStartMoveVelocity = Vec2::ZERO;
	Vec2 rayEndMoveVelocity = Vec2::ZERO;

	AABB2 worldBoundingBox(Vec2::ZERO, m_worldSize);

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) {
		m_rayStart = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) {
		m_rayEnd = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->WasKeyJustPressed(' ')) {
		SpawnNewBilliard(m_rayStart, (m_rayEnd - m_rayStart));
	}
}

void BilliardsMode::UpdateInputFromController()
{
}
