#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Game/Gameplay/3DShapesMode.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Shape3D.hpp"
#include "Game/Gameplay/CylinderShape3D.hpp"
#include "Game/Gameplay/AABB3Shape3D.hpp"
#include "Game/Gameplay/SphereShape3D.hpp"

std::string originalHelperText = "F8 to randomize. WASD to move around. EQ [Up/Down]. T = slow. Hold space to lock ray into place";
std::string originalUnlockHelperText = "F8 to randomize. WASD to move around. EQ [Up/Down]. T = slow. Hold space to unlock ray";


Shapes3DMode::Shapes3DMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
	m_gameModeName = "3D Shapes";
	m_helperText = originalHelperText;

	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);

	m_worldCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system
}

Shapes3DMode::~Shapes3DMode()
{
}

void Shapes3DMode::Startup()
{

	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	Player* player = new Player(m_game, Vec3(m_lastPosition), &m_worldCamera);
	m_player = player;
	m_player->m_orientation = m_lastOrientation;

	m_allEntities.push_back(m_player);
	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);
	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	AABB3 worldSpawnBounds(Vec3(-10.0, -10.0f, -10.0f), Vec3(10.0f, 10.0f, 10.0f));

	int amountOfShapesToSpawn = rng.GetRandomIntInRange(m_minShapeAmount, m_maxShapeAmount);

	int amountOfAABB3 = rng.GetRandomIntInRange(2, 6);
	int amountOfSpheres = rng.GetRandomIntInRange(2, amountOfShapesToSpawn - amountOfAABB3 - 2);
	int amountofCylinders = amountOfShapesToSpawn - amountOfAABB3 - amountOfSpheres;


	float aabb3SizeX = rng.GetRandomFloatInRange(3, 5);
	float aabb3SizeY = rng.GetRandomFloatInRange(3, 5);
	float aabb3SizeZ = rng.GetRandomFloatInRange(3, 5);
	Vec3 aabb3Size(aabb3SizeX, aabb3SizeY, aabb3SizeZ);

	bool wire = false;

	Vec3 spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
	Shape3D* aabb3 = new AABB3Shape3D(AABB3(spawnPos, spawnPos + aabb3Size), true, nullptr, Rgba8::WHITE, Rgba8::WHITE);
	m_allShapes.push_back(aabb3);
	m_shapesByType[(int)ShapeType::Box].push_back(aabb3);

	aabb3SizeX = rng.GetRandomFloatInRange(3, 5);
	aabb3SizeY = rng.GetRandomFloatInRange(3, 5);
	aabb3SizeZ = rng.GetRandomFloatInRange(3, 5);
	aabb3Size = Vec3(aabb3SizeX, aabb3SizeY, aabb3SizeZ);


	spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
	aabb3 = new AABB3Shape3D(AABB3(spawnPos, spawnPos + aabb3Size), false, testTexture, Rgba8::WHITE, Rgba8::WHITE);
	m_allShapes.push_back(aabb3);
	m_shapesByType[(int)ShapeType::Box].push_back(aabb3);

	float radius = rng.GetRandomFloatInRange(1.0f, 3.0f);
	float height = rng.GetRandomFloatInRange(3.0f, 7.0f);


	spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
	Shape3D* cylinder = new CylinderShape3D(spawnPos, radius, height, true, nullptr, Rgba8::WHITE, Rgba8::WHITE);
	m_allShapes.push_back(cylinder);
	m_shapesByType[(int)ShapeType::ZCylinder].push_back(cylinder);

	radius = rng.GetRandomFloatInRange(1.0f, 3.0f);
	height = rng.GetRandomFloatInRange(3.0f, 7.0f);

	spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
	cylinder = new CylinderShape3D(spawnPos, radius, height, false, testTexture, Rgba8::WHITE, Rgba8::WHITE);
	m_allShapes.push_back(cylinder);
	m_shapesByType[(int)ShapeType::ZCylinder].push_back(cylinder);


	radius = rng.GetRandomFloatInRange(1.0f, 3.0f);

	spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
	Shape3D* sphere = new SphereShape3D(spawnPos, radius, true, nullptr, Rgba8::WHITE, Rgba8::WHITE);
	m_allShapes.push_back(sphere);
	m_shapesByType[(int)ShapeType::Sphere].push_back(sphere);

	radius = rng.GetRandomFloatInRange(1.0f, 3.0f);

	wire = (rng.GetRandomIntInRange(0, 1) == 1);

	spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
	sphere = new SphereShape3D(spawnPos, radius, false, testTexture, Rgba8::WHITE, Rgba8::WHITE);
	m_allShapes.push_back(sphere);
	m_shapesByType[(int)ShapeType::Sphere].push_back(sphere);

	for (int shapeIndex = 2; shapeIndex < amountOfAABB3; shapeIndex++) {
		aabb3SizeX = rng.GetRandomFloatInRange(3, 5);
		aabb3SizeY = rng.GetRandomFloatInRange(3, 5);
		aabb3SizeZ = rng.GetRandomFloatInRange(3, 5);
		aabb3Size = Vec3(aabb3SizeX, aabb3SizeY, aabb3SizeZ);

		wire = (rng.GetRandomIntInRange(0, 1) == 1);
		Texture* usedTexture = (wire) ? nullptr : testTexture;

		spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
		aabb3 = new AABB3Shape3D(AABB3(spawnPos, spawnPos + aabb3Size), wire, usedTexture, Rgba8::WHITE, Rgba8::WHITE);
		m_allShapes.push_back(aabb3);
		m_shapesByType[(int)ShapeType::Box].push_back(aabb3);

	}

	for (int shapeIndex = 0; shapeIndex < amountOfSpheres; shapeIndex++) {
		radius = rng.GetRandomFloatInRange(1.0f, 3.0f);

		wire = (rng.GetRandomIntInRange(0, 1) == 1);
		Texture* usedTexture = (wire) ? nullptr : testTexture;

		spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
		sphere = new SphereShape3D(spawnPos, radius, wire, usedTexture, Rgba8::WHITE, Rgba8::WHITE);
		m_allShapes.push_back(sphere);
		m_shapesByType[(int)ShapeType::Sphere].push_back(sphere);

	}


	for (int shapeIndex = 2; shapeIndex < amountofCylinders; shapeIndex++) {
		radius = rng.GetRandomFloatInRange(1.0f, 3.0f);
		height = rng.GetRandomFloatInRange(3.0f, 7.0f);

		wire = (rng.GetRandomIntInRange(0, 1) == 1);
		Texture* usedTexture = (wire) ? nullptr : testTexture;

		spawnPos = GetRandomPositionWithinAABB3(worldSpawnBounds);
		cylinder = new CylinderShape3D(spawnPos, radius, height, wire, usedTexture, Rgba8::WHITE, Rgba8::WHITE);
		m_allShapes.push_back(cylinder);
		m_shapesByType[(int)ShapeType::ZCylinder].push_back(cylinder);

	}
}

void Shapes3DMode::Shutdown()
{
	m_lastPosition = m_player->m_position;
	m_lastOrientation = m_player->m_orientation;

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}

	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape3D*& shape = m_allShapes[shapeIndex];
		delete shape;
		shape = nullptr;
	}
	m_allShapes.clear();
	
	for (int shapeListIndex = 0; shapeListIndex < (int) ShapeType::NUM_SHAPE_TYPES; shapeListIndex++) {
		m_shapesByType[shapeListIndex].clear();
	}

	m_closestHitShape = nullptr;
	m_lockedShape = nullptr;

	m_allEntities.resize(0);
}

void Shapes3DMode::Update(float deltaSeconds)
{
	bool isDevConsoleShown = g_theConsole->GetMode() == DevConsoleMode::SHOWN;
	if (isDevConsoleShown) {
		g_theInput->ConsumeAllKeysJustPressed();
	}

	if (g_theWindow->HasFocus()) {
		if (m_lostFocusBefore) {
			g_theInput->ResetMouseClientDelta();
			m_lostFocusBefore = false;
		}
		if (isDevConsoleShown) {
			g_theInput->SetMouseMode(false, false, false);
		}
		else {
			g_theInput->SetMouseMode(m_isCursorHidden, m_isCursorClipped, m_isCursorRelative);
		}
	}
	else {
		m_lostFocusBefore = true;
	}

	UpdateEntities(deltaSeconds);
	UpdateShapes();

	UpdateRaycast();

	UpdateInput(deltaSeconds);


	GameMode::Update(deltaSeconds);
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);

	std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	//DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);

	if (m_isRaycastLocked) {
		m_helperText = originalUnlockHelperText;
	}
	else {
		m_helperText = originalHelperText;
	}

	if (m_closestHitShape) {
		if (m_lockedShape) {
			m_helperText += " LMB = Release Object";
		}
		else {
			m_helperText += " LMB = Grab Object";
		}
	}
	

}

void Shapes3DMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthFunc::LESSEQUAL, true);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	RenderShapes();

	g_theRenderer->EndCamera(m_worldCamera);


	

	RenderUI();
	DebugRenderWorld(m_worldCamera);
	DebugRenderScreen(m_UICamera);

}

void Shapes3DMode::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (entity) {
			entity->Update(deltaSeconds);
		}
	}

	if (m_lockedShape) {
		Vec3 playerDisp = m_player->GetDeltaDisplacement();
		m_lockedShape->m_position += playerDisp;

		Vec3 dispToPlayer = (m_lockedShape->m_position - m_player->m_position);
		EulerAngles deltaOrientation = m_player->GetDeltaOrientation();
		deltaOrientation.m_pitchDegrees = 0.0f;

		Mat44 rotationAroundPlayer;
		rotationAroundPlayer.Append(deltaOrientation.GetMatrix_XFwd_YLeft_ZUp());
		rotationAroundPlayer.AppendTranslation3D(dispToPlayer);

		Vec3 translationDueToRotation = rotationAroundPlayer.GetTranslation3D();
		translationDueToRotation.z = 0.0f;
		float prevShapeZ = m_lockedShape->m_position.z;

		AABB3Shape3D* shapeAsBox = dynamic_cast<AABB3Shape3D*>(m_lockedShape);
		if (!shapeAsBox) {
			m_lockedShape->m_position = m_player->m_position + translationDueToRotation;
			m_lockedShape->m_position.z = prevShapeZ;
			m_lockedShape->m_orientation.m_yawDegrees += deltaOrientation.m_yawDegrees;
		}
	}

}

void Shapes3DMode::UpdateShapes()
{
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape3D* shape = m_allShapes[shapeIndex];
		if (shape) {
			shape->Update();
		}
	}

	CheckShapeOverlap();

}

void Shapes3DMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	UpdateInputFromKeyboard();
	UpdateInputFromController();
}

void Shapes3DMode::UpdateInputFromKeyboard()
{
	if (g_theInput->WasKeyJustPressed(' ')) {
		m_isRaycastLocked = !m_isRaycastLocked;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE)) {
		if (m_lockedShape) {
			m_lockedShape = nullptr;
		}
		else if (m_closestHitShape) {
			m_lockedShape = m_closestHitShape;
		}
	}
}

void Shapes3DMode::UpdateInputFromController()
{
}

void Shapes3DMode::RenderShapes() const
{
	if (m_lockedShape) {
		g_theRenderer->SetModelColor(Rgba8::RED);
		m_lockedShape->Render();
	}


	float colorLerpValue = RangeMapClamped(sinf(m_timeAlive * 8.0f), -1.0f, 1.0f, 0.0f, 1.0f);
	Rgba8 overlapColor = Rgba8::InterpolateColors(Rgba8(0, 0, 128), Rgba8::BLUE, colorLerpValue);

	g_theRenderer->SetModelColor(overlapColor);
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape3D* shape = m_allShapes[shapeIndex];
		if (shape->m_isOverlapping && !(m_lockedShape == shape)) {
			shape->Render();
		}
	}

	g_theRenderer->SetModelColor(Rgba8::WHITE);
	for (int shapeIndex = 0; shapeIndex < m_allShapes.size(); shapeIndex++) {
		Shape3D* shape = m_allShapes[shapeIndex];
		if (!shape->m_isOverlapping && !(m_lockedShape == shape)) {
			shape->Render();
		}
	}


	g_theRenderer->SetModelMatrix(Mat44());
	std::vector<Vertex_PCU> basisVerts;
	Mat44 playerModel = m_player->GetModelMatrix();
	Vec3 playerFwd = playerModel.GetIBasis3D() * 0.25f;
	Vec3 basisDisp = m_worldCamera.GetViewPosition() + playerFwd;

	Vec3 iBasis(1.0f, 0.0f, 0.0f);
	Vec3 jBasis(0.0f, 1.0f, 0.0f);
	Vec3 kBasis(0.0f, 0.0f, 1.0f);

	AddVertsForArrow3D(basisVerts, basisDisp, basisDisp + (iBasis * 0.015f), 0.0005f, 8, Rgba8::RED);
	AddVertsForArrow3D(basisVerts, basisDisp, basisDisp + (jBasis * 0.015f), 0.0005f, 8, Rgba8::GREEN);
	AddVertsForArrow3D(basisVerts, basisDisp, basisDisp + (kBasis * 0.015f), 0.0005f, 8, Rgba8::BLUE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(basisVerts);
}

void Shapes3DMode::CheckShapeOverlap() const
{
	CheckSpheresVsSpheres();
	CheckSpheresVsBoxes();
	CheckSpheresVsZCylinders();

	CheckBoxesVsBoxes();
	CheckBoxesVsZCylinders();

	CheckZCylindersVsZCylinders();

}

void Shapes3DMode::CheckSpheresVsSpheres() const
{
	for (int sphereIndex = 0; sphereIndex < m_shapesByType[(int)ShapeType::Sphere].size(); sphereIndex++) {
		SphereShape3D* sphereA = reinterpret_cast<SphereShape3D*>(m_shapesByType[(int)ShapeType::Sphere][sphereIndex]);
		for (int otherSphereIndex = sphereIndex + 1; otherSphereIndex < m_shapesByType[(int)ShapeType::Sphere].size(); otherSphereIndex++) {
			SphereShape3D* sphereB = reinterpret_cast<SphereShape3D*>(m_shapesByType[(int)ShapeType::Sphere][otherSphereIndex]);

			if (DoSpheresOverlap(sphereA->m_position, sphereA->m_radius, sphereB->m_position, sphereB->m_radius)) {
				sphereA->m_isOverlapping = true;
				sphereB->m_isOverlapping = true;
			}

		}
	}
}

void Shapes3DMode::CheckSpheresVsBoxes() const
{
	for (int sphereIndex = 0; sphereIndex < m_shapesByType[(int)ShapeType::Sphere].size(); sphereIndex++) {
		SphereShape3D* sphere = reinterpret_cast<SphereShape3D*>(m_shapesByType[(int)ShapeType::Sphere][sphereIndex]);
		for (int boxIndex = 0; boxIndex < m_shapesByType[(int)ShapeType::Box].size(); boxIndex++) {
			AABB3Shape3D* box = reinterpret_cast<AABB3Shape3D*>(m_shapesByType[(int)ShapeType::Box][boxIndex]);
			if (DoSphereAndAABB3Overlap(sphere->m_position, sphere->m_radius, box->GetBounds())) {
				sphere->m_isOverlapping = true;
				box->m_isOverlapping = true;
			}

		}
	}
}

void Shapes3DMode::CheckSpheresVsZCylinders() const
{
	for (int sphereIndex = 0; sphereIndex < m_shapesByType[(int)ShapeType::Sphere].size(); sphereIndex++) {
		SphereShape3D* sphere = reinterpret_cast<SphereShape3D*>(m_shapesByType[(int)ShapeType::Sphere][sphereIndex]);
		for (int cylinderIndex = 0; cylinderIndex < m_shapesByType[(int)ShapeType::ZCylinder].size(); cylinderIndex++) {
			CylinderShape3D* cylinder = reinterpret_cast<CylinderShape3D*>(m_shapesByType[(int)ShapeType::ZCylinder][cylinderIndex]);
			Vec2 cylinderXYCenter(cylinder->m_position.x, cylinder->m_position.y);
			FloatRange heightRange(cylinder->m_position.z - (cylinder->m_height * 0.5f), cylinder->m_position.z + (cylinder->m_height * 0.5f));
			if (DoSphereAndZCylinderOverlap(sphere->m_position, sphere->m_radius, cylinderXYCenter, cylinder->m_radius, heightRange)) {
				sphere->m_isOverlapping = true;
				cylinder->m_isOverlapping = true;
			}
		}
	}
}

void Shapes3DMode::CheckBoxesVsBoxes() const
{
	for (int boxIndex = 0; boxIndex < m_shapesByType[(int)ShapeType::Box].size(); boxIndex++) {
		AABB3Shape3D* boxA = reinterpret_cast<AABB3Shape3D*>(m_shapesByType[(int)ShapeType::Box][boxIndex]);
		for (int otherBoxIndex = boxIndex + 1; otherBoxIndex < m_shapesByType[(int)ShapeType::Box].size(); otherBoxIndex++) {
			AABB3Shape3D* boxB = reinterpret_cast<AABB3Shape3D*>(m_shapesByType[(int)ShapeType::Box][otherBoxIndex]);

			if (DoAABB3sOverlap(boxA->GetBounds(), boxB->GetBounds())) {
				boxA->m_isOverlapping = true;
				boxB->m_isOverlapping = true;
			}

		}
	}
}

void Shapes3DMode::CheckBoxesVsZCylinders() const
{
	for (int boxIndex = 0; boxIndex < m_shapesByType[(int)ShapeType::Box].size(); boxIndex++) {
		AABB3Shape3D* boxA = reinterpret_cast<AABB3Shape3D*>(m_shapesByType[(int)ShapeType::Box][boxIndex]);
		for (int cylinderIndex = 0; cylinderIndex < m_shapesByType[(int)ShapeType::ZCylinder].size(); cylinderIndex++) {
			CylinderShape3D* cylinder = reinterpret_cast<CylinderShape3D*>(m_shapesByType[(int)ShapeType::ZCylinder][cylinderIndex]);
			Vec2 cylinderXYCenter(cylinder->m_position.x, cylinder->m_position.y);
			FloatRange heightRange(cylinder->m_position.z - (cylinder->m_height * 0.5f), cylinder->m_position.z + (cylinder->m_height * 0.5f));
			if (DoAABB3AndZCylinderOverlap(boxA->GetBounds(), cylinderXYCenter, cylinder->m_radius, heightRange)) {
				boxA->m_isOverlapping = true;
				cylinder->m_isOverlapping = true;
			}
		}
	}
}

void Shapes3DMode::CheckZCylindersVsZCylinders() const
{
	for (int cylinderIndex = 0; cylinderIndex < m_shapesByType[(int)ShapeType::ZCylinder].size(); cylinderIndex++) {
		CylinderShape3D* cylinderA = reinterpret_cast<CylinderShape3D*>(m_shapesByType[(int)ShapeType::ZCylinder][cylinderIndex]);
		Vec2 cylinderAXYCenter(cylinderA->m_position.x, cylinderA->m_position.y);
		FloatRange heightRangeA(cylinderA->m_position.z - (cylinderA->m_height * 0.5f), cylinderA->m_position.z + (cylinderA->m_height * 0.5f));

		for (int otherCylinderIndex = cylinderIndex + 1; otherCylinderIndex < m_shapesByType[(int)ShapeType::ZCylinder].size(); otherCylinderIndex++) {
			CylinderShape3D* cylinderB = reinterpret_cast<CylinderShape3D*>(m_shapesByType[(int)ShapeType::ZCylinder][otherCylinderIndex]);
			Vec2 cylinderBXYCenter(cylinderB->m_position.x, cylinderB->m_position.y);
			FloatRange heightRangeB(cylinderB->m_position.z - (cylinderB->m_height * 0.5f), cylinderB->m_position.z + (cylinderB->m_height * 0.5f));

			if (DoZCylindersOverlap(cylinderAXYCenter, cylinderA->m_radius, heightRangeA, cylinderBXYCenter, cylinderB->m_radius, heightRangeB)) {
				cylinderA->m_isOverlapping = true;
				cylinderB->m_isOverlapping = true;
			}

		}
	}

}

void Shapes3DMode::GetNearestPointSpheres()
{
	for (int sphereIndex = 0; sphereIndex < m_shapesByType[(int)ShapeType::Sphere].size(); sphereIndex++) {
		SphereShape3D* sphere = reinterpret_cast<SphereShape3D*>(m_shapesByType[(int)ShapeType::Sphere][sphereIndex]);

		Vec3 nearPoint = GetNearestPointOnSphere(m_player->m_position, sphere->m_position, sphere->m_radius);
		m_allNearestPoints.push_back(nearPoint);
	}

}

void Shapes3DMode::GetNearestPointBoxes()
{
	for (int boxIndex = 0; boxIndex < m_shapesByType[(int)ShapeType::Box].size(); boxIndex++) {
		AABB3Shape3D* box = reinterpret_cast<AABB3Shape3D*>(m_shapesByType[(int)ShapeType::Box][boxIndex]);

		Vec3 nearPoint = GetNearestPointOnAABB3D(m_player->m_position, box->GetBounds());
		m_allNearestPoints.push_back(nearPoint);
	}

}

void Shapes3DMode::GetNearestPointCylinders()
{
	for (int cylinderIndex = 0; cylinderIndex < m_shapesByType[(int)ShapeType::ZCylinder].size(); cylinderIndex++) {
		CylinderShape3D* cylinder = reinterpret_cast<CylinderShape3D*>(m_shapesByType[(int)ShapeType::ZCylinder][cylinderIndex]);
		Vec2 cylinderXYCenter(cylinder->m_position.x, cylinder->m_position.y);
		FloatRange heightRange(cylinder->m_position.z - (cylinder->m_height * 0.5f), cylinder->m_position.z + (cylinder->m_height * 0.5f));

		Vec3 nearPoint = GetNearestPointOnZCylinder(m_player->m_position, cylinderXYCenter, cylinder->m_radius, heightRange);
		m_allNearestPoints.push_back(nearPoint);
	}

}

Vec3 const Shapes3DMode::GetRandomPositionWithinAABB3(AABB3 const& bounds) const
{
	float xPos = rng.GetRandomFloatInRange(bounds.m_mins.x, bounds.m_maxs.x);
	float yPos = rng.GetRandomFloatInRange(bounds.m_mins.y, bounds.m_maxs.y);
	float zPos = rng.GetRandomFloatInRange(bounds.m_mins.z, bounds.m_maxs.z);

	return Vec3(xPos, yPos, zPos);
}

Shape3DRaycastResult Shapes3DMode::GetNearestRaycastHit() const
{
	Shape3DRaycastResult closestSphereHit = RaycastVsSpheres();
	Shape3DRaycastResult closestBoxHit = RaycastVsBoxes();
	Shape3DRaycastResult closestCylinderHit = RaycastVsCylinders();

	Shape3DRaycastResult closestHit = {};
	closestHit.m_impactDist = FLT_MAX;

	if (closestSphereHit.m_didImpact) {
		if (closestHit.m_impactDist > closestSphereHit.m_impactDist) closestHit = closestSphereHit;
	}

	if (closestBoxHit.m_didImpact) {
		if (closestHit.m_impactDist > closestBoxHit.m_impactDist) closestHit = closestBoxHit;
	}

	if (closestCylinderHit.m_didImpact) {
		if (closestHit.m_impactDist > closestCylinderHit.m_impactDist) closestHit = closestCylinderHit;
	}


	return closestHit;
}

Shape3DRaycastResult Shapes3DMode::RaycastVsSpheres() const
{
	Shape3DRaycastResult closestRaycast = {};
	closestRaycast.m_impactDist = FLT_MAX;

	for (int sphereIndex = 0; sphereIndex < m_shapesByType[(int)ShapeType::Sphere].size(); sphereIndex++) {
		SphereShape3D* sphere = reinterpret_cast<SphereShape3D*>(m_shapesByType[(int)ShapeType::Sphere][sphereIndex]);
		RaycastResult3D rayVsSphereResult = RaycastVsSphere(m_rayStart, m_rayForward, m_raycastLength, sphere->m_position, sphere->m_radius);

		if (rayVsSphereResult.m_didImpact) {
			if (closestRaycast.m_impactDist > rayVsSphereResult.m_impactDist) {
				closestRaycast = rayVsSphereResult;
				closestRaycast.m_shapeHit = sphere;
			}
		}
	}

	return closestRaycast;
}

Shape3DRaycastResult Shapes3DMode::RaycastVsBoxes() const
{
	Shape3DRaycastResult closestRaycast = {};
	closestRaycast.m_impactDist = FLT_MAX;

	for (int boxIndex = 0; boxIndex < m_shapesByType[(int)ShapeType::Box].size(); boxIndex++) {
		AABB3Shape3D* box = reinterpret_cast<AABB3Shape3D*>(m_shapesByType[(int)ShapeType::Box][boxIndex]);
		RaycastResult3D rayVsBox = RaycastVsBox3D(m_rayStart, m_rayForward, m_raycastLength, box->GetBounds());

		if (rayVsBox.m_didImpact) {
			if (closestRaycast.m_impactDist > rayVsBox.m_impactDist) {
				closestRaycast = rayVsBox;
				closestRaycast.m_shapeHit = box;
			}
		}
	}

	return closestRaycast;
}

Shape3DRaycastResult Shapes3DMode::RaycastVsCylinders() const
{
	Shape3DRaycastResult closestRaycast = {};
	closestRaycast.m_impactDist = FLT_MAX;

	for (int cylinderIndex = 0; cylinderIndex < m_shapesByType[(int)ShapeType::ZCylinder].size(); cylinderIndex++) {
		CylinderShape3D* cylinder = reinterpret_cast<CylinderShape3D*>(m_shapesByType[(int)ShapeType::ZCylinder][cylinderIndex]);
		Vec3 cylinderBase(cylinder->m_position.x, cylinder->m_position.y, cylinder->m_position.z - (cylinder->m_height * 0.5f));

		RaycastResult3D rayVsCylinder = RaycastVsZCylinder(m_rayStart, m_rayForward, m_raycastLength, cylinderBase, cylinder->m_radius, cylinder->m_height);

		if (rayVsCylinder.m_didImpact) {
			if (closestRaycast.m_impactDist > rayVsCylinder.m_impactDist) {
				closestRaycast = rayVsCylinder;
				closestRaycast.m_shapeHit = cylinder;
			}
		}
	}

	return closestRaycast;
}

void Shapes3DMode::UpdateRaycast()
{
	if (!m_isRaycastLocked) {
		m_rayStart = m_player->m_position;
		m_rayForward = m_player->GetForward();

		m_allNearestPoints.clear();

		GetNearestPointSpheres();
		GetNearestPointBoxes();
		GetNearestPointCylinders();
	}

	Shape3DRaycastResult closestImpact = GetNearestRaycastHit();
	m_closestHitShape = nullptr;

	if (closestImpact.m_didImpact) {
		DebugAddWorldPoint(closestImpact.m_impactPos, 0.06f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
		DebugAddWorldArrow(closestImpact.m_impactPos, closestImpact.m_impactPos + closestImpact.m_impactNormal, 0.03f, 0.0f, Rgba8::YELLOW, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::USEDEPTH);
		m_closestHitShape = closestImpact.m_shapeHit;
	}

	if (m_isRaycastLocked) {
		DebugAddWorldArrow(m_rayStart, m_rayStart + (m_rayForward * m_raycastLength), 0.03f, 0.0f, Rgba8::GREEN, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH);
	}

	float closestDistToNearPoint = FLT_MAX;
	Vec3 nearestPoint = Vec3::ZERO;
	for (int nearPointIndex = 0; nearPointIndex < m_allNearestPoints.size(); nearPointIndex++) {
		Vec3 const& nearPoint = m_allNearestPoints[nearPointIndex];

		DebugAddWorldPoint(nearPoint, 0.1f, 0.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

		float distToNearPoint = GetDistanceSquared3D(m_rayStart, nearPoint);
		if (closestDistToNearPoint > distToNearPoint) {
			closestDistToNearPoint = distToNearPoint;
			nearestPoint = nearPoint;
		}

		if (m_isRaycastLocked) {
			DebugAddWorldLine(m_rayStart, nearPoint, 0.03f, 0.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

		}
	}
	DebugAddWorldPoint(nearestPoint, 0.1f, 0.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH);

}
