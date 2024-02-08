#include "Engine/Math/LineSegment2.hpp"
#include "Game/Gameplay/NearestPointGameMode.hpp"
#include "Game/Gameplay/CapsuleShape2D.hpp"
#include "Game/Gameplay/OBB2Shape2D.hpp"
#include "Game/Gameplay/AABB2Shape2D.hpp"
#include "Game/Gameplay/CapsuleShape2D.hpp"
#include "Game/Gameplay/DiscShape2D.hpp"

NearestPointGameMode::NearestPointGameMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = worldSize;
	m_gameModeName = "Nearest Point 2D";
	m_helperText = "F8 to randomize. WASD or Arrows to move the dot. Hold T = slow";
}

NearestPointGameMode::~NearestPointGameMode()
{
}

void NearestPointGameMode::Startup()
{
	m_allShapes.clear();
	Rgba8 transparentBlue(0, 0, 255, 120);
	Rgba8 solidBlue(0, 0, 255, 180);

	float randomCapsuleOrientation = rng.GetRandomFloatInRange(0.0f, 360.0f);
	float randomCapsuleLength = rng.GetRandomFloatInRange(10.0f, 20.0f);
	float randomCapsuleRadius = rng.GetRandomFloatInRange(2.0f, 8.0f);

	Vec2 capsuleLineStart = GetClampedRandomPositionInWorld();
	Vec2 rightDir(1.0f, 0.0f);
	rightDir.RotateDegrees(randomCapsuleOrientation);
	rightDir.SetLength(randomCapsuleLength);

	CapsuleShape2D* capsuleShape = new CapsuleShape2D(capsuleLineStart, capsuleLineStart + rightDir, randomCapsuleRadius, transparentBlue, solidBlue);

	float boundsRandMin = 7.5f;
	float boundsRandMax = 15.5f;

	float obb2XDim = rng.GetRandomFloatInRange(boundsRandMin, boundsRandMax);
	float obb2YDim = rng.GetRandomFloatInRange(boundsRandMin, boundsRandMax);

	float biggestOBB2Dim = (obb2XDim > obb2YDim) ? obb2XDim : obb2YDim;

	OBB2Shape2D* OBB2Shape = new OBB2Shape2D(Vec2(obb2XDim, obb2YDim), GetClampedRandomPositionInWorld(biggestOBB2Dim), rng.GetRandomFloatInRange(0.0f, 360.0f), transparentBlue, solidBlue);

	float abb2XDim = rng.GetRandomFloatInRange(boundsRandMin, boundsRandMax);
	float abb2YDim = rng.GetRandomFloatInRange(boundsRandMin, boundsRandMax);

	AABB2Shape2D* ABB2Shape = new AABB2Shape2D(Vec2(abb2XDim, abb2YDim), GetClampedRandomPositionInWorld(abb2XDim), transparentBlue, solidBlue);

	m_lineSegment.m_start = GetClampedRandomPositionInWorld();
	m_lineSegment.m_end = GetClampedRandomPositionInWorld();

	float randDiscRadius = rng.GetRandomFloatInRange(7.0f, 10.0f);

	DiscShape2D* discShape = new DiscShape2D(GetClampedRandomPositionInWorld(randDiscRadius), randDiscRadius, transparentBlue, solidBlue);

	Vec2 infiniteLineStart = GetClampedRandomPositionInWorld();
	Vec2 infiniteLineEnd = GetClampedRandomPositionInWorld();

	Vec2 infLineDisp = (infiniteLineEnd - infiniteLineStart).GetNormalized();

	float arbitrarilyLargeNumber = 1500.0f;
	infiniteLineStart -= infLineDisp * arbitrarilyLargeNumber;
	infiniteLineEnd += infLineDisp * arbitrarilyLargeNumber;

	m_infiniteLine.m_start = infiniteLineStart;
	m_infiniteLine.m_end = infiniteLineEnd;

	m_allShapes.push_back(capsuleShape);
	m_allShapes.push_back(OBB2Shape);
	m_allShapes.push_back(ABB2Shape);
	m_allShapes.push_back(discShape);
}

void NearestPointGameMode::Shutdown()
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

void NearestPointGameMode::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);

	UpdateShapesColors();
	UpdateNearestPointsOnShapes();

	GameMode::Update(deltaSeconds);
}

void NearestPointGameMode::Render() const
{

	std::vector<Vertex_PCU> worldVerts;
	Rgba8 gold(255, 215, 0, 255);
	Rgba8 blue(0, 0, 255, 255);

	AddVertsForLineSegment2D(worldVerts, m_lineSegment, blue, 0.9f);
	AddVertsForLineSegment2D(worldVerts, m_infiniteLine, blue, 0.9f);


	for (int pointIndex = 0; pointIndex < m_nearestPointsOnShapes.size(); pointIndex++) {
		Vec2 const& nearPoint = m_nearestPointsOnShapes[pointIndex];
		AddVertsForDisc2D(worldVerts, nearPoint, m_nearestPointRadius, gold);
		LineSegment2 lineToNearestPoint(m_cursorDiscPos, nearPoint);

		AddVertsForLineSegment2D(worldVerts, lineToNearestPoint, Rgba8(255, 255, 255, 30), 0.6f);
	}

	AddVertsForDisc2D(worldVerts, m_cursorDiscPos, m_cursorDiscRadius, Rgba8());

	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	g_theRenderer->BindTexture(nullptr);

	RenderNormalColorShapes();
	RenderHighlightedColorShapes();

	g_theRenderer->DrawVertexArray((int)worldVerts.size(), worldVerts.data());
	worldVerts.clear();
	g_theRenderer->EndCamera(m_worldCamera);

	RenderUI();
}

void NearestPointGameMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->IsKeyDown('W') || g_theInput->IsKeyDown(KEYCODE_UPARROW)) {
		m_cursorDiscPos.y += m_dotSpeed * deltaSeconds;
	}

	if (g_theInput->IsKeyDown('S') || g_theInput->IsKeyDown(KEYCODE_DOWNARROW)) {
		m_cursorDiscPos.y -= m_dotSpeed * deltaSeconds;
	}

	if (g_theInput->IsKeyDown('A') || g_theInput->IsKeyDown(KEYCODE_LEFTARROW)) {
		m_cursorDiscPos.x -= m_dotSpeed * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('D') || g_theInput->IsKeyDown(KEYCODE_RIGHTARROW)) {
		m_cursorDiscPos.x += m_dotSpeed * deltaSeconds;
	}

}

void NearestPointGameMode::UpdateShapesColors()
{
	m_highlightedShapes.clear();
	m_normalColorShapes.clear();
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		if (shape->IsPointInside(m_cursorDiscPos)) {
			m_highlightedShapes.push_back(shape);
		}
		else {
			m_normalColorShapes.push_back(shape);
		}
	}
}

void NearestPointGameMode::UpdateNearestPointsOnShapes()
{
	m_nearestPointsOnShapes.clear();

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape2D* shape = m_allShapes[shapeIndex];
		m_nearestPointsOnShapes.push_back(shape->GetNearestPoint(m_cursorDiscPos));
	}

	m_nearestPointsOnShapes.push_back(GetNearestPointOnLineSegment2D(m_cursorDiscPos, m_lineSegment));
	m_nearestPointsOnShapes.push_back(GetNearestPointOnLineSegment2D(m_cursorDiscPos, m_infiniteLine));
}

void NearestPointGameMode::UpdateInputFromKeyboard()
{
}

void NearestPointGameMode::UpdateInputFromController()
{
}

void NearestPointGameMode::RenderNormalColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_normalColorShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_normalColorShapes[shapeIndex];
		if (!shape) continue;
		shape->Render();
	}
}

void NearestPointGameMode::RenderHighlightedColorShapes() const
{
	for (int shapeIndex = 0; shapeIndex < m_highlightedShapes.size(); shapeIndex++) {
		Shape2D const* shape = m_highlightedShapes[shapeIndex];
		if (!shape) continue;
		shape->RenderHighlighted();
	}
}
