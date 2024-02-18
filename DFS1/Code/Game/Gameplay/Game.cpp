#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Prop.hpp"
#include "Game/Gameplay/Triangulation.hpp"
#include "Game/Gameplay/Triangulation3D.hpp"
#include "Game/Gameplay/ConvexPoly3D.hpp"
#include "Game/Gameplay/DelaunayShape2D.hpp"
#include "Game/Gameplay/DelaunayShape3D.hpp"


extern bool g_drawDebug;
extern App* g_theApp;
extern AudioSystem* g_theAudio;
Game const* pointerToSelf = nullptr;


Game::Game(App* appPointer) :
	m_theApp(appPointer)
{
	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);

	m_worldCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system
	pointerToSelf = this;

	SubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	SubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	SubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	SubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	SubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	SubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	SubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	SubscribeEventCallbackFunction("Controls", GetControls);
}

Game::~Game()
{

}

void Game::Startup()
{
	LoadAssets();
	if (m_deltaTimeSample) {
		delete m_deltaTimeSample;
		m_deltaTimeSample = nullptr;
	}

	m_deltaTimeSample = new double[m_fpsSampleSize];
	m_storedDeltaTimes = 0;
	m_totalDeltaTimeSample = 0.0f;

	g_squirrelFont = g_theRenderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");

	switch (m_currentState)
	{
	case GameState::AttractScreen:
		StartupAttractScreen();
		break;
	case GameState::Delaunay2D:
		StartupDelaunay2D();
		break;
	case GameState::Delaunay3D:
		StartupDelaunay3D();
		break;
	default:
		break;
	}
	DebugAddMessage("Hello", 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void Game::StartupAttractScreen()
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;


	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}

	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	DebugRenderClear();
}

void Game::StartupDelaunay2D()
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	g_theInput->ResetMouseClientDelta();

	AABB2 UIBounds(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));
	AABB2 pointsBounds = UIBounds.GetBoxWithin(0.1f, 0.1f, 0.9f, 0.9f);
	for (int randPoint = 0; randPoint < 10; randPoint++) {
		float x = rng.GetRandomFloatInRange(pointsBounds.GetXRange());
		float y = rng.GetRandomFloatInRange(pointsBounds.GetYRange());
		m_eventPoints.emplace_back(x, y);
	}

	m_drawTriangleMesh = false;

	m_gameModeName = "Delaunay 2D";
	m_helperText = "LMB = Add new point, LMB Hold = Move point around, RMB = Remove point\n";
	m_helperText += "C= Clear Points, F1 = Debug Draw, F2 = Toggle Draw Super Triangle, Shift + F2 = Draw Triangles \n";
	m_helperText += "F3 = Toggle Drawing Voronoi, Space = Split Convex Polygon, Arrows = Cycle through all polygons, [comma,dot] -/+ Triangulation Step\n";
	m_helperText += "[EXPERIMENTAL] Shift + RMB = Add Interest point, Ctrl + RMB = Remove Interest Point";

	std::vector<Vec2> convexPolyVertexes = {
	Vec2(445.0f, 430.0f),
	Vec2(582.0f, 575.0f),
	Vec2(781.0f, 684.0f),
	Vec2(940.0f, 594.0f),
	Vec2(1118.0f, 498.0f),
	Vec2(1079.0f, 352.0f),
	Vec2(927.0f, 166.7f),
	Vec2(583.0f, 206.0f)
	};

	//std::vector<Vec2> convexPolyVertexes = {
	//Vec2(245.0f, 230.0f),
	//Vec2(245.0f, 630.0f),
	//Vec2(645.0f, 630.0f),
	//Vec2(645.0f, 230.0f),
	//};

	m_convexPoly = new DelaunayShape2D(convexPolyVertexes);

	m_allPolygons.push_back(m_convexPoly);
	m_allPolygonsColors.push_back(GetRandomColor());

	std::string superTriangleText = "Super triangle that contains all polygons' vertexes";
	std::string triangleText = "Triangles crated using Delaunay's Triangulation";
	std::string voronoiText = "Voronoi edges, created by connecting triangles' circumcenters";

	DebugAddScreenText(superTriangleText, Vec2(0.0f, m_UISizeY * 0.87f), -1.0f, Vec2::ZERO, m_textCellHeight, Rgba8::BLUE, Rgba8::BLUE);
	DebugAddScreenText(triangleText, Vec2(0.0f, m_UISizeY * 0.84f), -1.0f, Vec2::ZERO, m_textCellHeight, Rgba8::RED, Rgba8::RED);
	DebugAddScreenText(voronoiText, Vec2(0.0f, m_UISizeY * 0.81f), -1.0f, Vec2::ZERO, m_textCellHeight, Rgba8::YELLOW, Rgba8::YELLOW);

}

void Game::StartupDelaunay3D()
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Player* player = new Player(this, Vec3::ZERO, &m_worldCamera);
	m_player = player;

	Prop* cubeProp = new Prop(this, Vec3(-2.0f, 2.0f, 0.0f));
	cubeProp->m_angularVelocity.m_yawDegrees = 45.0f;

	Prop* gridProp = new Prop(this, Vec3::ZERO, PropRenderType::GRID);

	Prop* sphereProp = new Prop(this, Vec3(10.0f, -5.0f, 1.0f), 1.0f, PropRenderType::SPHERE);
	sphereProp->m_angularVelocity.m_pitchDegrees = 20.0f;
	sphereProp->m_angularVelocity.m_yawDegrees = 20.0f;

	m_allEntities.push_back(player);
	m_allEntities.push_back(cubeProp);
	m_allEntities.push_back(gridProp);
	m_allEntities.push_back(sphereProp);

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

	float axisLabelTextHeight = 0.25f;
	Mat44 xLabelTransformMatrix(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3::ZERO);
	float xLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "X - Forward");
	xLabelTransformMatrix.SetTranslation3D(Vec3(xLabelWidth * 0.7f, 0.0f, axisLabelTextHeight));

	DebugAddWorldText("X - Forward", xLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

	Mat44 yLabelTransformMatrix(Vec3(0.0f, 1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3::ZERO);
	float yLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Y - Left");
	yLabelTransformMatrix.SetTranslation3D(Vec3(-axisLabelTextHeight, yLabelWidth * 0.7f, 0.0f));

	DebugAddWorldText("Y - Left", yLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH);

	Mat44 zLabelTransformMatrix(Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3::ZERO);
	float zLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Z - Up");
	zLabelTransformMatrix.SetTranslation3D(Vec3(0.0f, axisLabelTextHeight, zLabelWidth * 0.7f));

	DebugAddWorldText("Z - Up", zLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);

	m_gameModeName = "Delaunay 3D";
	m_helperText += "F1 = Debug Draw, F2 = Toggle Draw Super Triangle, Shift + F2 = Draw Triangles\n";
	m_helperText += "F3 = Toggle Voronoi Edge Projections Visibility, Shift + F3 = Toggle Drawing Voronoi\n";
	m_helperText += "F9 = Toggle Draw Mode Polygon, [comma,dot] -/+ Triangulation Step";

	std::vector<Vec3> polyVerts = {
		// Clockwise Bottoms
		Vec3(1.0f, 2.0f, 1.0f), // 0
		Vec3(2.0f, 1.5f, 1.0f), // 1
		Vec3(3.0f, 2.0f, 1.0f), // 2
		Vec3(3.0f, 3.0f, 1.0f), // 3
		Vec3(2.0f, 3.5f, 1.0f), // 4
		Vec3(1.0f, 3.0f, 1.0f), // 5

		// Clockwise Top
		Vec3(1.0f, 2.0f, 2.0f), // 6
		Vec3(2.0f, 1.5f, 2.0f), // 7
		Vec3(3.0f, 2.0f, 2.0f), // 8
		Vec3(3.0f, 3.0f, 2.0f), // 9
		Vec3(2.0f, 3.5f, 2.0f), // 10
		Vec3(1.0f, 3.0f, 2.0f), // 11

	};

	std::vector<Face> polyFaces = {
		// Bottom
		Face{std::vector<int>({5,4,3,2,1,0})},
		// Top
		Face{std::vector<int>({6,7,8,9,10,11})},

		// Sides
		Face{std::vector<int>({0,1,7,6})},
		Face{std::vector<int>({1,2,8,7})},
		Face{std::vector<int>({2,3,9,8})},
		Face{std::vector<int>({3,4,10,9})},
		Face{std::vector<int>({4,5,11,10})},
		Face{std::vector<int>({5,0,6,11})}

	};

	m_convexPoly3D = new DelaunayShape3D(polyVerts, polyFaces);


	std::string superTriangleText = "Super Tetrahedron that contains all polygons' vertexes";
	std::string triangleText = "Tetrahedrons crated using Delaunay's Triangulation";
	std::string voronoiText = "Voronoi edges created with Tetrahedrons";

	DebugAddScreenText(superTriangleText, Vec2(0.0f, m_UISizeY * 0.87f), -1.0f, Vec2::ZERO, m_textCellHeight, Rgba8::BLUE, Rgba8::BLUE);
	DebugAddScreenText(triangleText, Vec2(0.0f, m_UISizeY * 0.84f), -1.0f, Vec2::ZERO, m_textCellHeight, Rgba8::RED, Rgba8::RED);
	DebugAddScreenText(voronoiText, Vec2(0.0f, m_UISizeY * 0.81f), -1.0f, Vec2::ZERO, m_textCellHeight, Rgba8::YELLOW, Rgba8::YELLOW);
}

void Game::UpdateGameState()
{
	if (m_currentState != m_nextState) {
		switch (m_currentState)
		{
		case GameState::AttractScreen:
			ShutDown();
			break;
		case GameState::Delaunay2D:
			ShutDown();
			break;
		case GameState::Delaunay3D:
			ShutDown();
			break;
		}

		m_currentState = m_nextState;
		Startup();
	}
}

void Game::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (entity) {
			entity->Update(deltaSeconds);
		}
	}
}

void Game::UpdateDelaunayShapes2D(float deltaSeconds)
{
	// Honestly, the easiest way to push of of walls is to create a poly from the walls and push them out of those
	std::vector<Vec2> screenBottomVertexes = {
		Vec2::ZERO,
		Vec2(m_UISizeX, 0.0f),
		Vec2(m_UISizeX * 0.5f, -m_UISizeY)
	};

	DelaunayConvexPoly2D screenBottomPoly(screenBottomVertexes);

	std::vector<Vec2> leftWallVertexes = {
	Vec2::ZERO,
	Vec2(-m_UISizeX, m_UISizeY * 0.5f),
	Vec2(0.0f, m_UISizeY),
	};

	DelaunayConvexPoly2D leftWallPoly(leftWallVertexes);

	std::vector<Vec2> rightWallVertexes = {
	Vec2(m_UISizeX, 0.0f),
	Vec2(m_UISizeX, m_UISizeY),
	Vec2(m_UISizeX * 1.5f, m_UISizeY * 0.5f),
	};

	DelaunayConvexPoly2D rightWallPoly(rightWallVertexes);


	for (int shapeIndex = 0; shapeIndex < m_allPolygons.size(); shapeIndex++) {
		DelaunayShape2D* shape = m_allPolygons[shapeIndex];
		if (shape) {

			shape->m_velocity.y -= 10.0f * deltaSeconds;
			shape->Update(deltaSeconds);
		}
	}


	//Collision check

	for (int shapeIndex = 0; shapeIndex < m_allPolygons.size(); shapeIndex++) {
		DelaunayShape2D* shapeA = m_allPolygons[shapeIndex];

		if (!shapeA) continue;

		for (int otherShapeIndex = shapeIndex + 1; otherShapeIndex < m_allPolygons.size(); otherShapeIndex++) {
			DelaunayShape2D* shapeB = m_allPolygons[otherShapeIndex];
			if (!shapeB) continue;

			Vec2 shapeAPrevPos = shapeA->m_position;
			Vec2 shapeBPrevPos = shapeB->m_position;

			bool gotPushed = PushConvexPolysOutOfEachOther(shapeA->GetPoly(), shapeB->GetPoly());

			if (gotPushed) {
				Vec2& shapeANewPos = shapeA->GetPoly().m_middlePoint;
				if ((shapeANewPos.x < 0.0f || shapeANewPos.x > m_UISizeX) || (shapeANewPos.y < 0.0f || shapeANewPos.y > m_UISizeY)) {
					shapeANewPos = shapeAPrevPos;
				}

				Vec2& shapeBNewPos = shapeB->GetPoly().m_middlePoint;
				if ((shapeBNewPos.x < 0.0f || shapeBNewPos.x > m_UISizeX) || (shapeBNewPos.y < 0.0f || shapeBNewPos.y > m_UISizeY)) {
					shapeBNewPos = shapeBPrevPos;
				}

				shapeA->m_position = shapeA->GetPoly().m_middlePoint;
				shapeB->m_position = shapeB->GetPoly().m_middlePoint;

				Vec2 dispA = shapeA->m_position - shapeAPrevPos;
				Vec2 dispB = shapeB->m_position - shapeBPrevPos;
				if (dispA.y > 0) shapeA->m_velocity.y = 0.0f;
				if (dispB.y > 0) shapeB->m_velocity.y = 0.0f;

				shapeB->m_velocity.x = 0.0f;
				shapeA->m_velocity.x = 0.0f;
			}

		}
	}

	// Collision With walls
	for (int shapeIndex = 0; shapeIndex < m_allPolygons.size(); shapeIndex++) {
		DelaunayShape2D* shapeA = m_allPolygons[shapeIndex];

		if (!shapeA) continue;
		bool gotPushed = PushConvexPolyOutOfOtherPoly(screenBottomPoly, shapeA->GetPoly());
		if (gotPushed) {
			shapeA->m_velocity = Vec2::ZERO;
			shapeA->m_position = shapeA->GetPoly().m_middlePoint;
		}

		gotPushed = PushConvexPolyOutOfOtherPoly(leftWallPoly, shapeA->GetPoly());

		if (gotPushed) {
			shapeA->m_velocity.x *= -1.0f;
			shapeA->m_position = shapeA->GetPoly().m_middlePoint;
		}

		gotPushed = PushConvexPolyOutOfOtherPoly(rightWallPoly, shapeA->GetPoly());

		if (gotPushed) {
			shapeA->m_velocity.x *= -1.0f;
			shapeA->m_position = shapeA->GetPoly().m_middlePoint;
		}

	}
}


void Game::ShutDown()
{
	for (int soundIndexPlaybackID = 0; soundIndexPlaybackID < GAME_SOUND::NUM_SOUNDS; soundIndexPlaybackID++) {
		SoundPlaybackID soundPlayback = g_soundPlaybackIDs[soundIndexPlaybackID];
		if (soundPlayback != -1) {
			g_theAudio->StopSound(soundPlayback);
		}
	}

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_eventPoints.clear();

	m_useTextAnimation = false;
	m_currentText = "";
	m_transitionTextColor = false;
	m_timeTextAnimation = 0.0f;
	m_player = nullptr;
	m_deltaTimeSample = nullptr;
	DebugRenderClear();
	m_gameModeName = "";
	m_helperText = "";

	if (m_convexPoly) {
		for (int polyIndex = 0; polyIndex < m_allPolygons.size(); polyIndex++) {
			DelaunayShape2D*& poly = m_allPolygons[polyIndex];
			if (poly) {
				delete poly;
				poly = nullptr;
			}
		}

		m_convexPoly = nullptr;
		m_allPolygonsColors.clear();
		m_allPolygonsColors.resize(0);
		m_allPolygons.resize(0);
		m_maxVoronoiStep2D = -1;
		m_maxDelaunay2DStep = -1;
		m_artificialPoints.clear();
	}

	if (m_convexPoly3D) {
		delete m_convexPoly3D;
		m_convexPoly3D = nullptr;
		m_maxVoronoiStep = -1;
	}
}

void Game::LoadAssets()
{
	if (m_loadedAssets) return;
	LoadTextures();
	LoadSoundFiles();

	m_loadedAssets = true;
}

void Game::LoadTextures()
{
	g_textures[(int)GAME_TEXTURE::TestUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
	g_textures[(int)GAME_TEXTURE::CompanionCube] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/CompanionCube.png");

	for (int textureIndex = 0; textureIndex < (int)GAME_TEXTURE::NUM_TEXTURES; textureIndex++) {
		if (!g_textures[textureIndex]) {
			ERROR_RECOVERABLE(Stringf("FORGOT TO LOAD TEXTURE %d", textureIndex));
		}
	}
}

void Game::LoadSoundFiles()
{
	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		g_sounds[soundIndex] = (SoundID)-1;
	}

	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		g_soundPlaybackIDs[soundIndex] = (SoundPlaybackID)-1;
	}
	//winGameSound = g_theAudio->CreateOrGetSound("Data/Audio/WinGameSound.wav");
	//loseGameSound = g_theAudio->CreateOrGetSound("Data/Audio/LoseGameSound.wav");

	// Royalty free version of Clair De Lune found here:
	// https://soundcloud.com/pianomusicgirl/claire-de-lune
	g_soundSources[GAME_SOUND::CLAIRE_DE_LUNE] = "Data/Audio/ClaireDeLuneRoyaltyFreeFirstMinute.wav";
	g_soundSources[GAME_SOUND::DOOR_KNOCKING_SOUND] = "Data/Audio/KnockingDoor.wav";

	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		SoundID soundID = g_theAudio->CreateOrGetSound(g_soundSources[soundIndex]);
		g_sounds[soundIndex] = soundID;
		g_soundEnumIDBySource[g_soundSources[soundIndex]] = (GAME_SOUND)soundIndex;
	}
}

void Game::Update()
{
	AddDeltaToFPSCounter();
	CheckIfWindowHasFocus();

	float gameDeltaSeconds = static_cast<float>(m_clock.GetDeltaTime());

	UpdateCameras(gameDeltaSeconds);
	UpdateGameState();
	switch (m_currentState)
	{
	case GameState::AttractScreen:
		UpdateAttractScreen(gameDeltaSeconds);
		break;
	case GameState::Delaunay2D:
		UpdateDelaunay2D(gameDeltaSeconds);
		break;
	case GameState::Delaunay3D:
		UpdateDelaunay3D(gameDeltaSeconds);
		break;
	default:
		break;
	}

}

void Game::CheckIfWindowHasFocus()
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
}

void Game::UpdateDelaunay2D(float deltaSeconds)
{
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}

	UpdateInputDelaunay2D(deltaSeconds);
	UpdateDelaunayShapes2D(deltaSeconds);


	AABB2 UIBoundingBox(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));
	Vec2 clickPos = UIBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());

	m_nearestPointDelaunay2D = GetNearestPointOnConvexPoly2D(clickPos, m_convexPoly->GetPoly());

	if (g_drawDebug) {
		DebugAddScreenText(Stringf("CursorPos: %s", clickPos.ToString().c_str()), Vec2(0.0f, 0.10f * m_UISizeY), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	}
	if (m_selectedEventPoint) {
		*m_selectedEventPoint = clickPos;
	}
	if (m_isDraggingConvexPoly) {
		Vec2 distToTranslate = clickPos - m_convexPoly->GetPoly().m_middlePoint;
		m_convexPoly->Translate(distToTranslate);
	}

	UpdateDeveloperCheatCodes(deltaSeconds);
	CheckForConvexPoly2DOverlap();

	//DisplayClocksInfo();


}

void Game::UpdateInputDelaunay2D(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}

	if (g_theInput->WasKeyJustPressed('C')) {
		m_eventPoints.clear();
	}

	if (g_theInput->WasKeyJustReleased(KEYCODE_SHIFT)) {
		m_isDraggingConvexPoly = false;
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) {

		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			m_isDraggingConvexPoly = true;
		}
		else {
			if (!m_isDraggingEventPoint) {
				AABB2 UIBoundingBox(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));
				Vec2 clickPos = UIBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
				m_selectedEventPoint = GetEventPoint2D(clickPos);
				if (m_selectedEventPoint) {
					m_isDraggingEventPoint = true;
				}
			}

		}
	}

	if (g_theInput->WasKeyJustReleased(KEYCODE_LEFT_MOUSE)) {
		if (!m_isDraggingEventPoint && !m_isDraggingConvexPoly) {
			AABB2 UIBoundingBox(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));
			Vec2 clickPos = UIBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
			m_convexPoly->GetPoly().m_vertexes.push_back(clickPos);
		}
		m_selectedEventPoint = nullptr;
		m_isDraggingEventPoint = false;
		m_isDraggingConvexPoly = false;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE)) {
		AABB2 UIBoundingBox(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));
		Vec2 clickPos = UIBoundingBox.GetPointAtUV(g_theWindow->GetNormalizedCursorPos());
		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			m_artificialPoints.push_back(clickPos);
			m_maxVoronoiStep2D = -1;
			m_maxDelaunay2DStep = -1;
		}
		else if(g_theInput->IsKeyDown(KEYCODE_CTRL)){
			m_artificialPoints.erase(m_artificialPoints.end() - 1);
			m_maxVoronoiStep2D = -1;
			m_maxDelaunay2DStep = -1;
		}
		else{
			RemoveEventPoint2D(clickPos);
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2)) {
		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			m_drawTriangleMesh = !m_drawTriangleMesh;
		}
		else {
			m_drawSuperTriangle = !m_drawSuperTriangle;
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F3)) {
		m_drawVoronoiRegions = !m_drawVoronoiRegions;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW)) {
		if (m_allPolygons.size() > 0) {
			auto iterToCurrent = std::find(m_allPolygons.begin(), m_allPolygons.end(), m_convexPoly);
			iterToCurrent++;

			if (iterToCurrent == m_allPolygons.end()) {
				iterToCurrent = m_allPolygons.begin();
			}

			m_convexPoly = *iterToCurrent;
			m_maxVoronoiStep2D = -1;
			m_maxDelaunay2DStep = -1;

		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW)) {
		if (m_allPolygons.size() > 0) {
			auto iterToCurrent = std::find(m_allPolygons.begin(), m_allPolygons.end(), m_convexPoly);

			if (iterToCurrent == m_allPolygons.begin()) {
				m_convexPoly = m_allPolygons[m_allPolygons.size() - 1];
			}
			else {

				iterToCurrent--;
				m_convexPoly = *iterToCurrent;
				m_maxVoronoiStep2D = -1;
				m_maxDelaunay2DStep = -1;

			}
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE)) {
		auto vecIterator = std::find(m_allPolygons.begin(), m_allPolygons.end(), m_convexPoly);
		int index = int(vecIterator - m_allPolygons.begin());

		m_allPolygons.erase(vecIterator);
		m_allPolygonsColors.erase(m_allPolygonsColors.begin() + index);

		std::vector<DelaunayTriangle> triangulatedPoly = TriangulateConvexPoly2D(m_convexPoly->GetPoly(),m_artificialPoints, true);
		std::vector<DelaunayEdge> voronoiRegionEdges = GetVoronoiDiagramFromConvexPoly(triangulatedPoly, m_convexPoly->GetPoly(), m_artificialPoints);

		Vec2 prevMiddlePos = m_convexPoly->GetPoly().m_middlePoint;

		std::vector<DelaunayConvexPoly2D> subPolys = SplitConvexPoly(voronoiRegionEdges, m_convexPoly->GetPoly());


		for (int subPolyInd = 0; subPolyInd < subPolys.size(); subPolyInd++) {
			DelaunayConvexPoly2D& newPoly = subPolys[subPolyInd];
			DelaunayShape2D* newShape = new DelaunayShape2D(newPoly.m_middlePoint, newPoly);

			m_allPolygons.push_back(newShape);
			Vec2 const& newMiddle = newShape->GetPoly().m_middlePoint;
			newShape->m_velocity = (newMiddle - prevMiddlePos) * 0.5f;
			if (newShape->m_velocity.y >= 0) newShape->m_velocity.y = 0.0f;


			m_allPolygonsColors.push_back(GetRandomColor());
		}

		if (m_allPolygons.size() > 0) {
			m_maxVoronoiStep2D = -1;
			m_maxDelaunay2DStep = -1;

			delete m_convexPoly;
			m_convexPoly = m_allPolygons[0];
			m_artificialPoints.clear();

		}
	}

	if (g_theInput->WasKeyJustPressed(190)) { // ,
		if (m_maxDelaunay2DStep == -1) m_maxDelaunay2DStep++;

		m_maxDelaunay2DStep++;

		if (m_maxDelaunay2DStep > m_convexPoly->GetPoly().m_vertexes.size()) {
			m_maxDelaunay2DStep = m_maxDelaunay2DStep % m_convexPoly->GetPoly().m_vertexes.size();
		}
	}

	if (g_theInput->WasKeyJustPressed(188)) { // .

		m_maxDelaunay2DStep--;

		if (m_maxDelaunay2DStep < 0) m_maxDelaunay2DStep = (int)m_convexPoly->GetPoly().m_vertexes.size();
	}

	if (g_theInput->WasKeyJustPressed('M')) {
		if (m_maxVoronoiStep2D == -1) m_maxVoronoiStep2D++;
		m_maxVoronoiStep2D++;

	}

	if (g_theInput->WasKeyJustPressed('N')) {

		m_maxVoronoiStep2D--;

	}
}

void Game::UpdateDelaunay3D(float deltaSeconds)
{
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}

	UpdateInputDelaunay3D(deltaSeconds);

	std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	DebugAddScreenText(playerPosInfo, Vec2(0.0f, m_UISizeY * 0.74f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);

	std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);
	DebugAddScreenText(gameInfoStr, Vec2(0.0f, m_UISizeY * 0.71f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);


	UpdateDeveloperCheatCodes(deltaSeconds);
	UpdateEntities(deltaSeconds);



	//DisplayClocksInfo();
}

void Game::UpdateInputDelaunay3D(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}

	if (g_theInput->WasKeyJustPressed('R')) {
		m_frozenRaycast = !m_frozenRaycast;
		if (m_frozenRaycast) {
			m_rayStart = m_player->m_position;
			m_rayFwd = m_player->m_orientation.GetXForward();
		}
	}

	if (g_theInput->WasKeyJustPressed(190)) {
		if (m_maxDelaunay3DStep == -1) m_maxDelaunay3DStep++;

		m_maxDelaunay3DStep++;

		if (m_maxDelaunay3DStep > m_convexPoly3D->GetPoly().m_vertexes.size()) {
			m_maxDelaunay3DStep = m_maxDelaunay3DStep % m_convexPoly3D->GetPoly().m_vertexes.size();
		}
	}

	if (g_theInput->WasKeyJustPressed(188)) {

		m_maxDelaunay3DStep--;

		if (m_maxDelaunay3DStep < 0) m_maxDelaunay3DStep = (int)m_convexPoly3D->GetPoly().m_vertexes.size();
	}

	if (g_theInput->WasKeyJustPressed('M')) {
		if (m_maxVoronoiStep == -1) m_maxVoronoiStep++;

		m_maxVoronoiStep++;

	}

	if (g_theInput->WasKeyJustPressed('N')) {

		m_maxVoronoiStep--;

	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F9)) {
		m_draw3DPoly = !m_draw3DPoly;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2)) {
		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			m_drawTriangleMesh = !m_drawTriangleMesh;
		}
		else {
			m_drawSuperTetra = !m_drawSuperTetra;
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F3)) {
		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			m_drawVoronoiRegions = !m_drawVoronoiRegions;
		}
		else {
			m_drawVoronoiEdgeProjections = !m_drawVoronoiEdgeProjections;
		}
	}

}

void Game::UpdateAttractScreen(float deltaSeconds)
{
	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}
	m_AttractScreenCamera.SetOrthoView(Vec2(0.0f, 0.0f), Vec2(m_UISizeX, m_UISizeY));

	UpdateInputAttractScreen(deltaSeconds);
	m_timeAlive += deltaSeconds;

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}
}

void Game::UpdateInputAttractScreen(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed(KEYCODE_SPACE);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::A);

	if (exitAttractMode) {
		m_nextState = GameState::Delaunay2D;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}
	UpdateDeveloperCheatCodes(deltaSeconds);
}

void Game::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	XboxController controller = g_theInput->GetController(0);
	Clock& sysClock = Clock::GetSystemClock();

	if (g_theInput->WasKeyJustPressed('T')) {
		sysClock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('O')) {
		Clock::GetSystemClock().StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('P')) {
		sysClock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
		g_drawDebug = !g_drawDebug;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6)) {
		m_nextState = GameState(((int)m_currentState - 1) % (int)GameState::NumGameStates);
		if ((int)m_nextState < 0) m_nextState = (GameState)((int)GameState::NumGameStates - 1);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7)) {
		m_nextState = GameState(((int)m_currentState + 1) % (int)GameState::NumGameStates);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) {
		ShutDown();
		Startup();
	}

	if (g_theInput->WasKeyJustReleased('T')) {
		sysClock.SetTimeDilation(1);
	}

	if (g_theInput->WasKeyJustPressed('K')) {
		g_soundPlaybackIDs[GAME_SOUND::DOOR_KNOCKING_SOUND] = g_theAudio->StartSound(g_sounds[DOOR_KNOCKING_SOUND]);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
		g_theInput->ResetMouseClientDelta();
	}

	if (g_theInput->WasKeyJustPressed('1')) {
		FireEvent("DebugAddWorldWireSphere");
	}

	if (g_theInput->WasKeyJustPressed('2')) {
		FireEvent("DebugAddWorldLine");
	}

	if (g_theInput->WasKeyJustPressed('3')) {
		FireEvent("DebugAddBasis");
	}

	if (g_theInput->WasKeyJustPressed('4')) {
		FireEvent("DebugAddBillboardText");
	}

	if (g_theInput->WasKeyJustPressed('5')) {
		FireEvent("DebugAddWorldWireCylinder");
	}

	if (g_theInput->WasKeyJustPressed('6')) {
		EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
		std::string cameraOrientationInfo = Stringf("Camera Orientation: (%.2f, %.2f, %.2f)", cameraOrientation.m_yawDegrees, cameraOrientation.m_pitchDegrees, cameraOrientation.m_rollDegrees);
		DebugAddMessage(cameraOrientationInfo, 5.0f, Rgba8::WHITE, Rgba8::RED);
	}

	if (g_theInput->WasKeyJustPressed('9')) {
		Clock const& debugClock = DebugRenderGetClock();
		double newTimeDilation = debugClock.GetTimeDilation();
		newTimeDilation -= 0.1;
		if (newTimeDilation < 0.1) newTimeDilation = 0.1;
		DebugRenderSetTimeDilation(newTimeDilation);
	}

	if (g_theInput->WasKeyJustPressed('0')) {
		Clock const& debugClock = DebugRenderGetClock();
		double newTimeDilation = debugClock.GetTimeDilation();
		newTimeDilation += 0.1;
		if (newTimeDilation > 10.0) newTimeDilation = 10.0;
		DebugRenderSetTimeDilation(newTimeDilation);
	}
}

void Game::UpdateCameras(float deltaSeconds)
{
	UpdateWorldCamera(deltaSeconds);

	UpdateUICamera(deltaSeconds);

}

void Game::UpdateWorldCamera(float deltaSeconds)
{
	// #FixBeforeSubmitting
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);

	if (m_screenShake) {
		float easingInScreenShake = m_timeShakingScreen / m_screenShakeDuration;
		easingInScreenShake *= (m_screenShakeTranslation * 0.95f);

		float randAmountX = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);
		float randAmountY = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);

		m_worldCamera.TranslateCamera(randAmountX, randAmountY);

		m_timeShakingScreen += deltaSeconds;

		if (m_timeShakingScreen > m_screenShakeDuration) {
			m_screenShake = false;
			m_timeShakingScreen = 0.0f;;
		}
	}
}

void Game::UpdateUICamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	m_UICamera.SetOrthoView(Vec2(), Vec2(m_UISizeX, m_UISizeY));
}


void Game::UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation)
{
	m_timeTextAnimation += deltaTime;
	m_textAnimTriangles.clear();

	Rgba8 usedTextColor = m_textAnimationColor;
	if (m_transitionTextColor) {

		float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0, m_textAnimationTime);

		usedTextColor.r = static_cast<unsigned char>(Interpolate(m_startTextColor.r, m_endTextColor.r, timeAsFraction));
		usedTextColor.g = static_cast<unsigned char>(Interpolate(m_startTextColor.g, m_endTextColor.g, timeAsFraction));
		usedTextColor.b = static_cast<unsigned char>(Interpolate(m_startTextColor.b, m_endTextColor.b, timeAsFraction));
		usedTextColor.a = static_cast<unsigned char>(Interpolate(m_startTextColor.a, m_endTextColor.a, timeAsFraction));

	}

	g_squirrelFont->AddVertsForText2D(m_textAnimTriangles, Vec2::ZERO, m_textCellHeightAttractScreen, text, usedTextColor, 0.6f);

	float fullTextWidth = g_squirrelFont->GetTextWidth(m_textCellHeightAttractScreen, text, 0.6f);
	float textWidth = GetTextWidthForAnim(fullTextWidth);

	Vec2 iBasis = GetIBasisForTextAnim();
	Vec2 jBasis = GetJBasisForTextAnim();

	TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

	if (m_timeTextAnimation >= m_textAnimationTime) {
		m_useTextAnimation = false;
		m_timeTextAnimation = 0.0f;
	}

}

void Game::KillAllEnemies()
{

}

void Game::TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation)
{
	for (int vertIndex = 0; vertIndex < int(m_textAnimTriangles.size()); vertIndex++) {
		Vec3& vertPosition = m_textAnimTriangles[vertIndex].m_position;
		TransformPositionXY3D(vertPosition, iBasis, jBasis, translation);
	}
}

float Game::GetTextWidthForAnim(float fullTextWidth) {

	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float textWidth = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			textWidth = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, fullTextWidth);
		}
		else {
			return fullTextWidth;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		return fullTextWidth;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			return fullTextWidth;
		}
		else {
			textWidth = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, fullTextWidth, 0.0f);
		}
	}
	return textWidth;

}

Vec2 const Game::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float iScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, 1.0f);
		}
		else {
			iScale = 1.0f;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		iScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = 1.0f;
		}
		else {
			iScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 1.0f, 0.0f);
		}
	}

	return Vec2(iScale, 0.0f);
}

Vec2 const Game::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float jScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = 0.05f;
		}
		else {
			jScale = RangeMapClamped(timeAsFraction, m_textMovementPhaseTimePercentage * 0.5f, m_textMovementPhaseTimePercentage, 0.05f, 1.0f);

		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		jScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 0.05f);
		}
		else {
			jScale = 0.05f;
		}
	}

	return Vec2(0.0f, jScale);
}

double Game::GetFPS() const
{
	if (m_storedDeltaTimes < m_fpsSampleSize) return 1 / Clock::GetSystemClock().GetDeltaTime();

	double fps = m_fpsSampleSize / m_totalDeltaTimeSample;

	return fps;
}

void Game::AddDeltaToFPSCounter()
{
	int prevIndex = m_currentFPSAvIndex;
	m_currentFPSAvIndex++;
	if (m_currentFPSAvIndex >= m_fpsSampleSize) m_currentFPSAvIndex = 0;
	m_deltaTimeSample[m_currentFPSAvIndex] = Clock::GetSystemClock().GetDeltaTime();

	m_totalDeltaTimeSample += Clock::GetSystemClock().GetDeltaTime();
	m_storedDeltaTimes++;

	if (m_storedDeltaTimes > m_fpsSampleSize) {
		m_totalDeltaTimeSample -= m_deltaTimeSample[prevIndex];
	}

}

void Game::DisplayClocksInfo() const
{
	Clock& devClock = g_theConsole->m_clock;
	Clock const& debugClock = DebugRenderGetClock();

	double devClockFPS = 1.0 / devClock.GetDeltaTime();
	double gameFPS = GetFPS();
	double debugClockFPS = 1.0 / debugClock.GetDeltaTime();

	double devClockTotalTime = devClock.GetTotalTime();
	double gameTotalTime = m_clock.GetTotalTime();
	double debugTotalTime = debugClock.GetTotalTime();

	double devClockScale = devClock.GetTimeDilation();
	double gameScale = m_clock.GetTimeDilation();
	double debugScale = debugClock.GetTimeDilation();


	std::string devClockInfo = Stringf("Dev Console:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", devClockTotalTime, devClockFPS, devClockScale);
	std::string debugClockInfo = Stringf("Debug Render:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", debugTotalTime, debugClockFPS, debugScale);
	std::string gameClockInfo = Stringf("Game:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", gameTotalTime, gameFPS, gameScale);

	Vec2 devClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, devClockInfo), m_UISizeY - m_textCellHeight);
	Vec2 gameClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, gameClockInfo), m_UISizeY - (m_textCellHeight * 2.0f));
	Vec2 debugClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, debugClockInfo), m_UISizeY - (m_textCellHeight * 3.0f));

	DebugAddScreenText(devClockInfo, devClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(debugClockInfo, debugClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(gameClockInfo, gameClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
}


void Game::Render() const
{
	switch (m_currentState)
	{
	case GameState::AttractScreen:
		RenderAttractScreen();
		break;
	case GameState::Delaunay2D:
		RenderDelaunay2D();
		break;
	case GameState::Delaunay3D:
		RenderDelaunay3D();
		break;
	default:
		break;
	}

	DebugRenderWorld(m_worldCamera);
	RenderUI();
	DebugRenderScreen(m_UICamera);

}



void Game::RenderEntities() const
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity const* entity = m_allEntities[entityIndex];
		entity->Render();
	}
}

void Game::RenderDelaunay2D() const
{
	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthFunc::ALWAYS, false);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);


	std::vector<Vertex_PCU> verts;

	std::vector<DelaunayTriangle> triangulatedPoly = TriangulateConvexPoly2D(m_convexPoly->GetPoly(), m_artificialPoints, true, 0.1f, m_maxDelaunay2DStep);
	std::vector<DelaunayTriangle> triangulatedPolyExcludeSuper = TriangulateConvexPoly2D(m_convexPoly->GetPoly(), m_artificialPoints, false, 0.1f, m_maxDelaunay2DStep);
	DelaunayTriangle const& superTriangle = GetASuperTriangleFromPoly(m_convexPoly->GetPoly());

	g_theRenderer->BindTexture(nullptr);

	DebugAddScreenText(Stringf("Vertex amount: %d", m_convexPoly->GetPoly().m_vertexes.size()), Vec2(0.0f, 0.15f * m_UISizeY), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);


	for (int polyIndex = 0; polyIndex < m_allPolygons.size(); polyIndex++) {
		DelaunayShape2D const* convexPoly = m_allPolygons[polyIndex];
		if (convexPoly) {
			convexPoly->AddVertsForShape(verts, m_allPolygonsColors[polyIndex]);
		}
	}


	if (m_drawVoronoiRegions) {

		std::vector<DelaunayEdge> voronoi = GetVoronoiDiagramFromConvexPoly(triangulatedPoly, m_convexPoly->GetPoly(), m_artificialPoints);

		if (m_maxVoronoiStep2D <= -1) m_maxVoronoiStep2D = (int)voronoi.size();
		if (m_maxVoronoiStep2D > (int)voronoi.size()) m_maxVoronoiStep2D = 1;

		if (voronoi.size() > 0) {

			for (int edgeIndex = 0; edgeIndex < m_maxVoronoiStep2D; edgeIndex++) {
				DelaunayEdge const& edge = voronoi[edgeIndex];
				LineSegment2 linesegment(edge.m_pointA, edge.m_pointB);
				AddVertsForLineSegment2D(verts, linesegment, Rgba8::YELLOW, 2.5f);
			}
		}



	}

	if (m_drawTriangleMesh) {
		std::vector<DelaunayTriangle>const& drawingTriangles = (m_drawSuperTriangle) ? triangulatedPoly : triangulatedPolyExcludeSuper;

		for (int triangleIndex = 0; triangleIndex < drawingTriangles.size(); triangleIndex++) {
			DelaunayTriangle const& triangle = drawingTriangles[triangleIndex];
			triangle.AddVertsForTriangleMesh(verts, Rgba8::RED, 2.5f);
			if (g_drawDebug) {
				DebugDrawRing(triangle.GetCircumcenter(), triangle.GetCircumcenterRadius(), 1.25f, Rgba8::GREEN);
			}
		}

		superTriangle.AddVertsForTriangleMesh(verts, Rgba8::BLUE, 3.0f);
	}

	for (int pointIndex = 0; pointIndex < m_convexPoly->GetPoly().m_vertexes.size(); pointIndex++) {
		Vec2 const& point = m_convexPoly->GetPoly().m_vertexes[pointIndex];
		Rgba8 usedColor = (m_selectedEventPoint && (&point == m_selectedEventPoint)) ? Rgba8::GREEN : Rgba8::RED;
		AddVertsForDisc2D(verts, point, 5.0f, usedColor);
	}




	AddVertsForDisc2D(verts, m_nearestPointDelaunay2D, 10.0f, Rgba8::YELLOW, 30);

	g_theRenderer->DrawVertexArray(verts);
	g_theRenderer->EndCamera(m_UICamera);
}

void Game::RenderDelaunay3D() const
{

	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthFunc::LESSEQUAL, true);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	//RenderEntities();

	g_theRenderer->BindMaterial(nullptr);
	g_theRenderer->BindTexture(nullptr);

	std::vector<Vertex_PCU> verts;
	g_theRenderer->SetModelMatrix(Mat44());


	DelaunayTetrahedron superTetra = GetASuperTetrahedronPoly(m_convexPoly3D->GetPoly());

	std::vector<DelaunayTetrahedron> triangulatedPoly3D = TriangulateConvexPoly3D(m_convexPoly3D->GetPoly(), true, 0.1f, m_maxDelaunay3DStep);
	std::vector<DelaunayTetrahedron> triangulatedExludingSuperTetra = TriangulateConvexPoly3D(m_convexPoly3D->GetPoly(), false, 0.1f, m_maxDelaunay3DStep);

	if (m_drawTriangleMesh) {
		std::vector<DelaunayTetrahedron> const& drawTriangulation = (m_drawSuperTetra) ? triangulatedPoly3D : triangulatedExludingSuperTetra;
		for (int tetraInd = 0; tetraInd < drawTriangulation.size(); tetraInd++) {
			DelaunayTetrahedron const& tetrahedron = drawTriangulation[tetraInd];
			tetrahedron.AddVertsForWireframe(verts, Rgba8::RED, 0.005f);

			if (g_drawDebug) {
				DebugAddWorldPoint(tetrahedron.m_circumcenter, 0.1f, 0.0f, Rgba8::GRAY, Rgba8::GRAY, DebugRenderMode::USEDEPTH, 4, 8);
			}
		}

		DebugAddScreenText(Stringf("Tetrahedron amount: %d", drawTriangulation.size()), Vec2(0.0f, m_UISizeY * 0.67f), 0.0f, Vec2::ZERO, m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);

		if (m_drawSuperTetra) {
			superTetra.AddVertsForWireframe(verts, Rgba8::BLUE);
		}
	}




	if (m_drawVoronoiRegions) {
		std::vector<DelaunayEdge3D> voronoiEdges = GetVoronoiDiagramFromConvexPoly3D(triangulatedPoly3D, m_convexPoly3D->GetPoly());
		std::vector<DelaunayEdge3D> voronoiEdgesExcludeProjections = GetVoronoiDiagramFromConvexPoly3D(triangulatedPoly3D, m_convexPoly3D->GetPoly(), false);

		std::vector<DelaunayEdge3D> const& drawnVoronoiEdges = (m_drawVoronoiEdgeProjections) ? voronoiEdges : voronoiEdgesExcludeProjections;

		if (m_maxVoronoiStep > (int)drawnVoronoiEdges.size()) m_maxVoronoiStep = 1;
		if (m_maxVoronoiStep < -1) m_maxVoronoiStep = (int)drawnVoronoiEdges.size();

		int maxDrawnVoronoiEdges = (m_maxVoronoiStep == -1) ? (int)drawnVoronoiEdges.size() : m_maxVoronoiStep;



		for (int edgeIndex = 0; edgeIndex < maxDrawnVoronoiEdges; edgeIndex++) {
			DelaunayEdge3D const& edge = drawnVoronoiEdges[edgeIndex];
			AddVertsForLineSegment3D(verts, edge.m_pointA, edge.m_pointB, Rgba8::YELLOW, 0.01f);

			/*if (m_frozenRaycast) {
				RaycastResult3D raycastVsEdge = RaycastVsLineSegment3D(m_rayStart, m_rayFwd, 0.25f, edge.m_pointA, edge.m_pointB);
				if (raycastVsEdge.m_didImpact) {
					DebugAddWorldPoint(raycastVsEdge.m_impactPos, 0.05f, 0.0f, Rgba8::GRAY, Rgba8::GRAY, DebugRenderMode::USEDEPTH, 4, 8);

				}
			}*/
		}


	}
	else {
		if (m_draw3DPoly) {
			m_convexPoly3D->AddVertsForShape(verts);
		}
		else {
			m_convexPoly3D->AddVertsForWireShape(verts, Rgba8::CYAN);
		}
	}

	if (m_frozenRaycast) {
		DebugAddWorldLine(m_rayStart, m_rayStart + m_rayFwd * 0.25f, 0.025f, 0.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);
	}


	g_theRenderer->DrawVertexArray(verts);


	g_theRenderer->EndCamera(m_worldCamera);
}

void Game::RenderAttractScreen() const
{
	g_theRenderer->BeginCamera(m_AttractScreenCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthFunc::ALWAYS, false);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	if (m_useTextAnimation) {
		RenderTextAnimation();
	}

	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	std::vector<Vertex_PCU> testTextVerts;
	AABB2 testTextureAABB2(740.0f, 150.0f, 1040.0f, 450.f);
	AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8());
	g_theRenderer->BindTexture(testTexture);
	g_theRenderer->DrawVertexArray((int)testTextVerts.size(), testTextVerts.data());
	g_theRenderer->BindTexture(nullptr);

	/*g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	Vertex_PCU vertices[] =
	{
	Vertex_PCU(Vec3(600.0f, 400.0f, 0.0f), Rgba8::CYAN, Vec2(0.0f, 0.0f)),
	Vertex_PCU(Vec3(1000.0f,  400.0f, 0.0f), Rgba8::BLUE, Vec2(0.0f, 0.0f)),
	Vertex_PCU(Vec3(800.0f, 600.0f, 0.0f), Rgba8::ORANGE, Vec2(0.0f, 0.0f)),
	};*/

	//g_theRenderer->DrawVertexArray(3, vertices);

	g_theRenderer->EndCamera(m_AttractScreenCamera);

}


void Game::RenderUI() const
{

	g_theRenderer->BeginCamera(m_UICamera);

	if (m_useTextAnimation && m_currentState != GameState::AttractScreen) {

		RenderTextAnimation();
	}

	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	AABB2 screenBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());


	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);

	std::vector<Vertex_PCU> titleVerts;
	Vec2 titlePos(m_UISizeX * 0.01f, m_UISizeY * 0.98f);
	Vec2 helpTextPos(m_UISizeX * 0.01f, titlePos.y * 0.97f);

	std::string gameModeAsText = GetCurrentGameStateAsText();
	std::string gameModeHelperText = m_helperText;
	AABB2 topHelperBox = screenBounds.GetBoxWithin(0.0f, 0.5f, 1.0f, 1.0f);

	std::string completeText = gameModeAsText + "\n" + gameModeHelperText;

	if (m_currentState != GameState::AttractScreen) {
		g_squirrelFont->AddVertsForTextInBox2D(titleVerts, topHelperBox, m_textCellHeight, completeText, Rgba8::WHITE, 0.6f, Vec2(0.0f, 1.0f));
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(titleVerts);
	}

	g_theConsole->Render(devConsoleBounds);
	g_theRenderer->EndCamera(m_UICamera);
}

void Game::RenderTextAnimation() const
{
	if (m_textAnimTriangles.size() > 0) {
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(int(m_textAnimTriangles.size()), m_textAnimTriangles.data());
	}
}


void Game::ShakeScreenCollision()
{
	m_screenShakeDuration = m_maxScreenShakeDuration;
	m_screenShakeTranslation = m_maxScreenShakeTranslation;
	m_screenShake = true;
}

void Game::ShakeScreenDeath()
{
	m_screenShake = true;
	m_screenShakeDuration = m_maxDeathScreenShakeDuration;
	m_screenShakeTranslation = m_maxDeathScreenShakeTranslation;
}

void Game::ShakeScreenPlayerDeath()
{
	m_screenShake = true;
	m_screenShakeDuration = m_maxPlayerDeathScreenShakeDuration;
	m_screenShakeTranslation = m_maxPlayerDeathScreenShakeTranslation;
}

void Game::StopScreenShake() {
	m_screenShake = false;
	m_screenShakeDuration = 0.0f;
	m_screenShakeTranslation = 0.0f;
}

Rgba8 const Game::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(128, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(190, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

bool Game::DebugSpawnWorldWireSphere(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldWireSphere(pointerToSelf->m_player->m_position, 1, 5, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnWorldLine3D(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldLine(pointerToSelf->m_player->m_position, Vec3::ZERO, 0.125f, 5.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::XRAY);
	return false;
}

bool Game::DebugClearShapes(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugRenderClear();
	return false;
}

bool Game::DebugToggleRenderMode(EventArgs& eventArgs)
{
	static bool isDebuggerRenderSystemVisible = true;
	UNUSED(eventArgs);

	isDebuggerRenderSystemVisible = !isDebuggerRenderSystemVisible;
	if (isDebuggerRenderSystemVisible) {
		DebugRenderSetVisible();
	}
	else {
		DebugRenderSetHidden();
	}

	return false;
}

bool Game::DebugSpawnPermanentBasis(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	Mat44 invertedView = pointerToSelf->m_worldCamera.GetViewMatrix();
	DebugAddWorldBasis(invertedView.GetOrthonormalInverse(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnWorldWireCylinder(EventArgs& eventArgs)
{
	UNUSED(eventArgs);

	float cylinderHeight = 2.0f;
	Vec3 cylbase = pointerToSelf->m_player->m_position;
	cylbase.z -= cylinderHeight * 0.5f;

	Vec3 cylTop = cylbase;
	cylTop.z += cylinderHeight;
	DebugAddWorldWireCylinder(cylbase, cylTop, 0.5f, 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnBillboardText(EventArgs& eventArgs)
{
	Player* const& player = pointerToSelf->m_player;
	std::string playerInfo = Stringf("Position: (%f, %f, %f)\nOrientation: (%f, %f, %f)", player->m_position.x, player->m_position.y, player->m_position.z, player->m_orientation.m_yawDegrees, player->m_orientation.m_pitchDegrees, player->m_orientation.m_rollDegrees);
	DebugAddWorldBillboardText(playerInfo, player->m_position, 0.25f, Vec2(0.5f, 0.5f), 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	UNUSED(eventArgs);
	return false;
}

bool Game::GetControls(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	std::string controlsStr = "";
	controlsStr += "~       - Toggle dev console\n";
	controlsStr += "ESC     - Exit to attract screen\n";
	controlsStr += "F8      - Reset game\n";
	controlsStr += "W/S/A/D - Move forward/backward/left/right\n";
	controlsStr += "Z/C     - Move upward/downward\n";
	controlsStr += "Q/E     - Roll to left/right\n";
	controlsStr += "Mouse   - Aim camera\n";
	controlsStr += "Shift   - Sprint\n";
	controlsStr += "T       - Slow mode\n";
	controlsStr += "Y       - Fast mode\n";
	controlsStr += "O       - Step frame\n";
	controlsStr += "P       - Toggle pause\n";
	controlsStr += "1       - Add debug wire sphere\n";
	controlsStr += "2       - Add debug world line\n";
	controlsStr += "3       - Add debug world basis\n";
	controlsStr += "4       - Add debug world billboard text\n";
	controlsStr += "5       - Add debug wire cylinder\n";
	controlsStr += "6       - Add debug camera message\n";
	controlsStr += "9       - Decrease debug clock speed\n";
	controlsStr += "0       - Increase debug clock speed";

	Strings controlStringsSplit = SplitStringOnDelimiter(controlsStr, '\n');

	for (int stringIndex = 0; stringIndex < controlStringsSplit.size(); stringIndex++) {
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, controlStringsSplit[stringIndex]);
	}

	return false;
}

std::string Game::GetCurrentGameStateAsText() const
{
	std::string fullTitleText("Mode (F6/F7 for prev/next): ");
	fullTitleText += m_gameModeName;
	return fullTitleText;
}

void Game::RemoveEventPoint2D(Vec2 const& clickPosition)
{
	std::vector<Vec2>& eventPoints = m_convexPoly->GetPoly().m_vertexes;

	float radius = 5.0f;
	for (int eventPointIndex = 0; eventPointIndex < eventPoints.size(); eventPointIndex++) {
		Vec2 const& eventPoint = eventPoints[eventPointIndex];
		if (DoDiscsOverlap(clickPosition, radius, eventPoint, radius)) {
			eventPoints.erase(eventPoints.begin() + eventPointIndex);
			eventPointIndex--;
		}
	}
}

Vec2* Game::GetEventPoint2D(Vec2 const& clickPosition)
{
	std::vector<Vec2>& eventPoints = m_convexPoly->GetPoly().m_vertexes;

	float radius = 5.0f;
	for (int eventPointIndex = 0; eventPointIndex < eventPoints.size(); eventPointIndex++) {
		Vec2& eventPoint = eventPoints[eventPointIndex];
		if (DoDiscsOverlap(clickPosition, radius, eventPoint, radius)) {
			return &eventPoint;
		}
	}

	return nullptr;
}

void Game::CheckForConvexPoly2DOverlap()
{
	m_normalColorPolys.clear();
	m_highlightedPolys.clear();

	for (int polyInd = 0; polyInd < m_allPolygons.size(); polyInd++) {
		DelaunayShape2D* polyA = m_allPolygons[polyInd];
		if (!polyA) continue;

		bool pushedA = false;
		for (int otherPolyInd = polyInd + 1; otherPolyInd < m_allPolygons.size(); otherPolyInd++) {
			DelaunayShape2D* polyB = m_allPolygons[otherPolyInd];
			if (!polyB) continue;

			if (DoConvexPolygons2DOverlap(polyA->GetPoly(), polyB->GetPoly())) {
				auto foundA = std::find(m_highlightedPolys.begin(), m_highlightedPolys.end(), polyA);

				if (foundA == m_highlightedPolys.end()) {
					m_highlightedPolys.push_back(polyA);
					pushedA = true;
				}

				auto foundB = std::find(m_highlightedPolys.begin(), m_highlightedPolys.end(), polyB);
				if (foundB == m_highlightedPolys.end()) {
					m_highlightedPolys.push_back(polyB);
				}
			}
		}
		if (!pushedA) {
			m_normalColorPolys.push_back(polyA);
			m_normalColorPolysMap.push_back(m_allPolygonsColors[polyInd]);
		}
	}

}
