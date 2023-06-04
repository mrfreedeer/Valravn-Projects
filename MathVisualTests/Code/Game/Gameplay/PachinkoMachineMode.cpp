#include "Engine/Math/IntRange.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Gameplay/PachinkoMachineMode.hpp"
#include "Game/Gameplay/DiscShape2D.hpp"
#include "Game/Gameplay/AABB2Shape2D.hpp"
#include "Game/Gameplay/OBB2Shape2D.hpp"


PachinkoMachineMode::PachinkoMachineMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize, Vec2 const& worldSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_worldSize = worldSize;
	m_gameModeName = "Pachinko Machine 2D";
	m_rayStart = Vec2(20.0f, 20.0f);
	m_rayEnd = Vec2(100.0f, 40.0f);
	m_allBalls.reserve(1000);
}

PachinkoMachineMode::~PachinkoMachineMode()
{
}

void PachinkoMachineMode::Startup()
{
	Rgba8 transparentBlue(0, 0, 255, 120);
	Rgba8 solidBlue(0, 0, 255, 180);

	IntRange rangeAmountOfDiscBumpers = g_gameConfigBlackboard.GetValue("PACHINKO_RAND_RANGE_DISC_BUMPERS", IntRange::ZERO_TO_ONE);
	IntRange rangeAmountOfCapsuleBumpers = g_gameConfigBlackboard.GetValue("PACHINKO_RAND_RANGE_CAPSULE_BUMPERS", IntRange::ZERO_TO_ONE);
	IntRange rangeAmountOfOBB2Bumpers = g_gameConfigBlackboard.GetValue("PACHINKO_RAND_RANGE_OBB2_BUMPERS", IntRange::ZERO_TO_ONE);

	int amountOfDiscBumpers = rng.GetRandomIntInRange(rangeAmountOfDiscBumpers);
	int amountOfCapsuleBumpers = rng.GetRandomIntInRange(rangeAmountOfCapsuleBumpers);
	int amountOfOBB2Bumpers = rng.GetRandomIntInRange(rangeAmountOfOBB2Bumpers);

	for (int discIndex = 0; discIndex < amountOfDiscBumpers; discIndex++) {
		float randRadius = rng.GetRandomFloatInRange(5.0f, 10.0f);
		PachinkoBumper discBumper(BumperType::Disc, GetClampedRandomPositionInWorld(randRadius), Vec2(randRadius, 0.0f));
		m_bumpers.push_back(discBumper);
		m_discBumpers.push_back(discBumper);
		discBumper.AddVertsForDrawing(m_bumperVerts);

	}

	for (int capsuleIndex = 0; capsuleIndex < amountOfCapsuleBumpers; capsuleIndex++) {
		float randBoneLength = rng.GetRandomFloatInRange(5.0f, 10.0f);
		float randRadius = rng.GetRandomFloatInRange(2.0f, 6.0f);
		float randRotation = rng.GetRandomFloatInRange(0.0f, 180.0f);

		PachinkoBumper capsuleBumper(BumperType::Capsule, GetClampedRandomPositionInWorld(randBoneLength), Vec2(randBoneLength, randRadius), randRotation);
		m_bumpers.push_back(capsuleBumper);
		m_capsuleBumpers.push_back(capsuleBumper);
		capsuleBumper.AddVertsForDrawing(m_bumperVerts);
	}

	for (int obb2Index = 0; obb2Index < amountOfOBB2Bumpers; obb2Index++) {
		float randDimX = rng.GetRandomFloatInRange(3.0f, 6.0f);
		float randDimY = rng.GetRandomFloatInRange(3.0f, 6.0f);
		float biggestOBB2Dim = (randDimX > randDimY) ? randDimX : randDimY;
		float randRotation = rng.GetRandomFloatInRange(0.0f, 180.0f);

		PachinkoBumper obb2Bumper(BumperType::Box, GetClampedRandomPositionInWorld(biggestOBB2Dim), Vec2(randDimX, randDimY), randRotation);

		m_bumpers.push_back(obb2Bumper);
		m_obb2Bumpers.push_back(obb2Bumper);

		obb2Bumper.AddVertsForDrawing(m_bumperVerts);
	}


}

void PachinkoMachineMode::Shutdown()
{
	m_bumpers.clear();
	m_bumperVerts.clear();
	m_allBalls.clear();
	m_ballsVerts.clear();
	m_highlightedBalls.clear();

	m_discBumpers.clear();
	m_capsuleBumpers.clear();
	m_obb2Bumpers.clear();
}

void PachinkoMachineMode::Update(float deltaSeconds)
{

	UpdateInput(deltaSeconds);
	m_helperText = m_originalHelperText;
	if (m_useTimeStep) {
		float usedTotalDeltaTime = deltaSeconds + m_owedPhysicsTime;
		float elapsedDeltaTime = 0.0f;
		for (float currentDeltaTime = m_defaultPhysicsTimeStepMiliseconds; currentDeltaTime < usedTotalDeltaTime; currentDeltaTime += m_defaultPhysicsTimeStepMiliseconds, elapsedDeltaTime += m_defaultPhysicsTimeStepMiliseconds) {
			UpdatePachinkoBalls(m_defaultPhysicsTimeStepMiliseconds);

			CheckBallsCollisionWithEachOther();
			CheckBallsCollisionsWithBumpers();
			CheckBallsCollisionsWithWalls();
		}
		m_owedPhysicsTime = usedTotalDeltaTime - elapsedDeltaTime;
		float timeStepInMs = m_defaultPhysicsTimeStepMiliseconds * 1000.0f;
		m_helperText += Stringf(" %.2f ms", timeStepInMs);
		m_helperText += " (Numpad -/+ increase / decrease 1ms)";
	}
	else {
		UpdatePachinkoBalls(deltaSeconds);

		CheckBallsCollisionWithEachOther();
		CheckBallsCollisionsWithBumpers();
		CheckBallsCollisionsWithWalls();
	}



	AddVertsForPachinkoBallsAndRaycast();

	DebugAddMessage("", 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddMessage("", 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	GameMode::Update(deltaSeconds);
	Vec2 fpsPosition = m_UICamera.GetOrthoTopRight();
	fpsPosition.x *= 0.83f;
	fpsPosition.y *= 0.97f;

	Vec2 ballAmountPosition = m_UICamera.GetOrthoTopRight();
	ballAmountPosition.x *= 0.83f;
	ballAmountPosition.y *= 0.95f;
	DebugAddScreenText(Stringf("FPS: %0.2f", 1.0f / deltaSeconds), fpsPosition, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(Stringf("Ball Amount: %d", m_allBalls.size()), ballAmountPosition, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
}

void PachinkoMachineMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->DrawVertexArray(m_ballsVerts);

	RenderBumpers();

	//g_theRenderer->DrawVertexArray(m_raycastVsDiscCollisionVerts);

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

	DebugDrawRing(m_rayStart, m_ballSizeRange.m_min, 0.25f, Rgba8::WHITE);
	DebugDrawRing(m_rayStart, m_ballSizeRange.m_max, 0.25f, Rgba8::WHITE);

	g_theRenderer->DrawVertexArray(arrowVerts);
	g_theRenderer->EndCamera(m_worldCamera);

	RenderUI();
}

void PachinkoMachineMode::RenderBumpers() const
{
	g_theRenderer->DrawVertexArray(m_bumperVerts);
}

void PachinkoMachineMode::SpawnNewBall(Vec2 const& pos, Vec2 const& velocity)
{
	AABB2 worldBounds(Vec2::ZERO, m_worldSize);
	if (!worldBounds.IsPointInside(pos)) return;
	Rgba8 color = Rgba8::YELLOW;
	float elasticity = m_defaultBallElasticity;
	float size = rng.GetRandomFloatInRange(m_ballSizeRange);

	m_allBalls.emplace_back(pos, velocity, size, elasticity);
}


void PachinkoMachineMode::UpdateInput(float deltaSeconds)
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

	if (g_theInput->WasKeyJustPressed('L')) {
		m_useTimeStep = !m_useTimeStep;
	}

	if (g_theInput->IsKeyDown(107)) {
		if (m_useTimeStep) {
			m_defaultPhysicsTimeStepMiliseconds += 0.0001f;
		}
	}
	if (g_theInput->IsKeyDown(109)) {
		if (m_useTimeStep) {
			m_defaultPhysicsTimeStepMiliseconds -= 0.0001f;
		}
	}

	if (m_defaultPhysicsTimeStepMiliseconds <= 0.001f) m_defaultPhysicsTimeStepMiliseconds = 0.001f;

	m_rayStart += rayStartMoveVelocity * m_dotSpeed * deltaSeconds;
	m_rayEnd += rayEndMoveVelocity * m_dotSpeed * deltaSeconds;

	AABB2 worldBoundingBox(Vec2::ZERO, m_worldSize);

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) {
		m_rayStart = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) {
		m_rayEnd = worldBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
	}

	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->IsKeyDown('N')) {
		SpawnNewBall(m_rayStart, (m_rayEnd - m_rayStart));
	}

	if (g_theInput->WasKeyJustPressed('B')) {
		m_collapseFloor = !m_collapseFloor;
	}

}

void PachinkoMachineMode::AddVertsForPachinkoBallsAndRaycast()
{
	m_ballsVerts.clear();
	m_highlightedBalls.clear();
	//m_raycastVsDiscCollisionVerts.clear();

	m_ballsVerts.reserve(48 * m_allBalls.size());
	//m_raycastVsDiscCollisionVerts.reserve(50);


	//float rayDistance = GetDistance2D(m_rayStart, m_rayEnd);


	m_impactedDisc = false;
	//RaycastResult2D closestRaycast;
	//closestRaycast.m_impactDist = ARBITRARILY_LARGE_VALUE;
	//int closestImpactedShapeIndex = -1;

	//Vec2 rayFwd = (m_rayEnd - m_rayStart).GetNormalized();

	for (int shapeIndex = 0; shapeIndex < m_allBalls.size(); shapeIndex++) {
		PachinkoBall const& shape = m_allBalls[shapeIndex];
		/*	RaycastResult2D raycastResult;
			raycastResult = RaycastVsDisc(m_rayStart, rayFwd, rayDistance, shape.m_position, shape.m_radius);

			if (raycastResult.m_didImpact) {
				if (raycastResult.m_impactDist < closestRaycast.m_impactDist) {
					closestRaycast = raycastResult;
					closestImpactedShapeIndex = shapeIndex;
				}
				m_impactedDisc = true;
			}*/

		shape.AddVertsForRendering(m_ballsVerts);
	}

	//m_raycastResult = closestRaycast;
	//if (m_impactedDisc) {
	//	AddVertsForRaycastImpactOnDisc(m_allBalls[closestImpactedShapeIndex], m_raycastResult);
	//}
}

void PachinkoMachineMode::AddVertsForRaycastImpactOnDisc(PachinkoBall& shape, RaycastResult2D& raycastResult)
{
	shape.AddVertsForRendering(m_ballsVerts);
	Vec2 const& impactPos = raycastResult.m_impactPos;
	Vec2 impactPosArrowEnd = impactPos + raycastResult.m_impactNormal * 5.0f;

	AddVertsForDisc2D(m_raycastVsDiscCollisionVerts, impactPos, 0.5f, Rgba8::WHITE, 16);
	AddVertsForArrow2D(m_raycastVsDiscCollisionVerts, impactPos, impactPosArrowEnd, Rgba8::YELLOW, 0.5f, 1.0f);
}

void PachinkoMachineMode::UpdatePachinkoBalls(float deltaSeconds)
{
	for (int ballIndex = 0; ballIndex < m_allBalls.size(); ballIndex++) {
		PachinkoBall& ball = m_allBalls[ballIndex];
		ball.Update(deltaSeconds);
	}
}


void PachinkoMachineMode::UpdateInputFromKeyboard()
{


}

void PachinkoMachineMode::UpdateInputFromController()
{
}

void PachinkoMachineMode::CheckBallsCollisionWithEachOther()
{
	for (int ballIndex = 0; ballIndex < m_allBalls.size(); ballIndex++) {
		PachinkoBall& ball = m_allBalls[ballIndex];
		for (int otherBallIndex = ballIndex + 1; otherBallIndex < m_allBalls.size(); otherBallIndex++) {
			PachinkoBall& otherBall = m_allBalls[otherBallIndex];
			BounceBallsOfEachOtherSpecialPachinko(ball.m_position, ball.m_velocity, ball.m_radius, otherBall.m_position, otherBall.m_velocity, otherBall.m_radius, ball.m_elasticity * otherBall.m_elasticity);
		}
	}
}

void PachinkoMachineMode::CheckBallsCollisionsWithWalls()
{
	for (int ballIndex = 0; ballIndex < m_allBalls.size(); ballIndex++) {
		PachinkoBall& ball = m_allBalls[ballIndex];
		// Touches Left Wall
		if ((ball.m_position.x - ball.m_radius) <= 0.0f) {
			BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, Vec2(0.0f, ball.m_position.y), 0.9f);
		}

		// Touches Right Wall
		if ((ball.m_position.x + ball.m_radius) >= m_worldSize.x) {
			BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, Vec2(m_worldSize.x, ball.m_position.y), 0.9f);
		}

		// Touches Top Wall
		//if ((ball.m_position.y + ball.m_radius) >= m_worldSize.y) {
		//	BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, Vec2(ball.m_position.x, m_worldSize.y), 0.9f);
		//}

		// Touches Bottom Wall
		if (m_collapseFloor) {
			if ((ball.m_position.y + ball.m_radius) <= 0.0f) {
				ball.m_position.y = (m_worldSize.y * 1.1f) + ball.m_radius;
			}
		}
		else {
			if ((ball.m_position.y - ball.m_radius) <= 0.0f) {
				BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, Vec2(ball.m_position.x, 0.0f), 0.9f);
			}
		}

	}
}

void PachinkoMachineMode::CheckBallsCollisionsWithBumpers()
{
	for (int ballIndex = 0; ballIndex < m_allBalls.size(); ballIndex++) {
		PachinkoBall& ball = m_allBalls[ballIndex];
		for (int bumperIndex = 0; bumperIndex < m_discBumpers.size(); bumperIndex++) {
			PachinkoBumper const& bumper = m_discBumpers[bumperIndex];
			bumper.BounceBallOffDiscBumper(ball);
		}
		for (int bumperIndex = 0; bumperIndex < m_capsuleBumpers.size(); bumperIndex++) {
			PachinkoBumper const& bumper = m_capsuleBumpers[bumperIndex];
			bumper.BounceBallOffCapsuleBumper(ball);
		}
		for (int bumperIndex = 0; bumperIndex < m_obb2Bumpers.size(); bumperIndex++) {
			PachinkoBumper const& bumper = m_obb2Bumpers[bumperIndex];
			bumper.BounceBallOffOBB2Bumper(ball);
		}
	}

}

void PachinkoMachineMode::BounceBallsOfEachOtherSpecialPachinko(Vec2& aCenter, Vec2& aVelocity, float aRadius, Vec2& bCenter, Vec2& bVelocity, float bRadius, float combinedElasticity)
{
	float radiusSum = aRadius + bRadius;
	float distanceSquared = GetDistanceSquared2D(aCenter, bCenter);

	if (!(distanceSquared < (radiusSum * radiusSum))) return;

	float ideaDistanceToOtherCircle = aRadius + bRadius;
	float overlapDistance = ideaDistanceToOtherCircle - GetDistance2D(aCenter, bCenter);

	Vec2 pushDirectionA = (aCenter - bCenter).GetNormalized();

	aCenter += pushDirectionA * overlapDistance * 0.5f;
	bCenter -= pushDirectionA * overlapDistance * 0.5f;

	Vec2 normal = (bCenter - aCenter).GetNormalized();
	Vec2 bVelocityNormalComp = GetProjectedOnto2D(bVelocity, normal);
	Vec2 aVelocityNormalComp = GetProjectedOnto2D(aVelocity, normal);

	Vec2 bVelocityTangComp, aVelocityTangComp;

	bVelocityTangComp = bVelocity - bVelocityNormalComp;
	aVelocityTangComp = aVelocity - aVelocityNormalComp;

	if (DotProduct2D(normal, bVelocity) < DotProduct2D(normal, aVelocity)) {
		aVelocity = aVelocityTangComp + (bVelocityNormalComp * combinedElasticity);
		bVelocity = bVelocityTangComp + (aVelocityNormalComp * combinedElasticity);
	}
}

PachinkoBumper::PachinkoBumper(BumperType bumperType, Vec2 const& position, Vec2 const& dimensions, float orientation /*= 0.0f*/) :
	m_bumperType(bumperType),
	m_position(position),
	m_dimensions(dimensions),
	m_elasticity(rng.GetRandomFloatInRange(0.1f, 0.9f)),
	m_orientation(orientation)
{
	switch (m_bumperType)
	{
	case BumperType::Disc:
		m_boundsRadius = m_dimensions.x;
		break;
	case BumperType::Capsule:
	{
		m_capsule = Capsule2(Vec2::ZERO, Vec2(m_dimensions.x, 0.0f), m_dimensions.y);
		m_capsule.RotateAboutCenter(m_orientation);
		m_capsule.SetCenter(m_position);
		m_boundsRadius = m_dimensions.x + m_capsule.m_radius;
		break;
	}
	case BumperType::Box:
	{
		m_obb2.m_halfDimensions = m_dimensions;
		m_obb2.m_center = m_position;
		Vec2 basis(1.0f, 0.0f);
		basis.SetOrientationDegrees(m_orientation);
		m_obb2.m_iBasisNormal = basis;
		m_boundsRadius = m_obb2.m_halfDimensions.GetLength();
		break;
	}
	default:
		break;
	}
	m_color = (Rgba8::InterpolateColors(Rgba8::RED, Rgba8::GREEN, m_elasticity));

}

void PachinkoBumper::AddVertsForDrawing(std::vector<Vertex_PCU>& verts)
{
	switch (m_bumperType)
	{
	case BumperType::Disc:
		AddVertsForDisc2D(verts, m_position, m_dimensions.x, m_color, 60);
		break;
	case BumperType::Capsule:
	{
		AddVertsForCapsule2D(verts, m_capsule, m_color);
		break;
	}
	case BumperType::Box:
	{
		AddVertsForOBB2D(verts, m_obb2, m_color);
		break;
	}
	default:
		break;
	}
}

void PachinkoBumper::BounceBallOffBumper(PachinkoBall& ball) const
{
	if (!DoDiscsOverlap(m_position, m_boundsRadius, ball.m_position, ball.m_radius)) return;

	switch (m_bumperType)
	{
	case BumperType::Disc:
		BounceDiscOffDisc2D(ball.m_position, ball.m_velocity, ball.m_radius, m_position, m_boundsRadius, ball.m_elasticity * m_elasticity);
		break;
	case BumperType::Capsule:
	{
		Vec2 bouncePoint = GetNearestPointOnCapsule2D(ball.m_position, m_capsule);
		BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, bouncePoint, ball.m_elasticity * m_elasticity);
		break;
	}
	case BumperType::Box: {
		Vec2 bouncePoint = GetNearestPointOnOBB2D(ball.m_position, m_obb2);
		BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, bouncePoint, ball.m_elasticity * m_elasticity);
		break;
	}
	default:
		break;
	}

}

void PachinkoBumper::BounceBallOffDiscBumper(PachinkoBall& ball) const
{
	if (!DoDiscsOverlap(m_position, m_boundsRadius, ball.m_position, ball.m_radius)) return;

	BounceDiscOffDisc2D(ball.m_position, ball.m_velocity, ball.m_radius, m_position, m_boundsRadius, ball.m_elasticity * m_elasticity);
}

void PachinkoBumper::BounceBallOffCapsuleBumper(PachinkoBall& ball) const
{
	if (!DoDiscsOverlap(m_position, m_boundsRadius, ball.m_position, ball.m_radius)) return;

	Vec2 bouncePoint = GetNearestPointOnCapsule2D(ball.m_position, m_capsule);
	BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, bouncePoint, ball.m_elasticity * m_elasticity);
}

void PachinkoBumper::BounceBallOffOBB2Bumper(PachinkoBall& ball) const
{
	if (!DoDiscsOverlap(m_position, m_boundsRadius, ball.m_position, ball.m_radius)) return;

	Vec2 bouncePoint = GetNearestPointOnOBB2D(ball.m_position, m_obb2);
	BounceDiscOffPoint2D(ball.m_position, ball.m_velocity, ball.m_radius, bouncePoint, ball.m_elasticity * m_elasticity);
}

PachinkoBall::PachinkoBall(Vec2 const& position, Vec2 const& initialVelocity, float radius, float elasticity) :
	m_position(position),
	m_velocity(initialVelocity),
	m_radius(radius),
	m_elasticity(elasticity),
	m_color(Rgba8::InterpolateColors(Rgba8::LIGHTBLUE, Rgba8::BLUE, rng.GetRandomFloatZeroUpToOne()))
{
}

void PachinkoBall::Update(float deltaSeconds)
{
	m_velocity += Vec2(0.0f, -50.0f) * deltaSeconds;
	m_position += m_velocity * deltaSeconds;
}

void PachinkoBall::AddVertsForRendering(std::vector<Vertex_PCU>& verts) const
{
	AddVertsForDisc2D(verts, m_position, m_radius, m_color, 16);
}


