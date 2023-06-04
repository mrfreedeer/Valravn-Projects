#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/World.hpp"
#include "Game/Gameplay/BlockDefinition.hpp"
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Controller.hpp"
#include "Game/Gameplay/GameCamera.hpp"
#include "Game/Gameplay/PlayerController.hpp"
#include "Game/Gameplay/BlockTemplate.hpp"

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
	case GameState::Play:
		StartupPlay();
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

void Game::StartupPlay()
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Entity* player = new Entity(this, Vec3(0.0f, 0.0f, 90.0f), Vec3::ZERO, EulerAngles::ZERO);
	m_player = player;

	m_allEntities.push_back(m_player);

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

	BlockDefinition::InitializeDefinitions();
	BlockTemplate::InitializeDefinitions();

	m_world = new World(this);

	m_gameCamera = new GameCamera(this, player);
	m_gameCamera->SetCameraMode(CameraMode::SPECTATOR);

	m_playerController = new PlayerController(this);
	m_playerController->Possess(*m_gameCamera);

	m_controllers.push_back(m_playerController);

	/*float axisLabelTextHeight = 0.25f;
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
	float zLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Z - Up");
	zLabelTransformMatrix.SetTranslation3D(Vec3(0.0f, axisLabelTextHeight, zLabelWidth * 0.7f));

	DebugAddWorldText("Z - Up", zLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);*/


	m_worstPerlinNoiseChunkGenTime = 0.0;
	m_worstChunkLoadIncludeRLETime = 0.0;
	m_worstChunkSaveIncludeRLETime = 0.0;
	m_worstChunkMeshRegen = 0.0;
	m_worstLightResolveTime = 0.0;

	m_totalPerlinNoiseChunkGenTime = 0.0;
	m_totalChunkLoadIncludeRLETime = 0.0;
	m_totalChunkSaveIncludeRLETime = 0.0;
	m_totalChunkMeshRegen = 0.0;
	m_totalLightResolveTime = 0.0;

	m_countPerlinNoiseChunkGenTime = 0;
	m_countChunkLoadIncludeRLETime = 0;
	m_countChunkSaveIncludeRLETime = 0;
	m_countChunkMeshRegen = 0;
	m_countLightResolveTime = 0;
}

void Game::UpdateGameState()
{
	if (m_currentState != m_nextState) {
		switch (m_currentState)
		{
		case GameState::AttractScreen:
			ShutDown();
			break;
		case GameState::Play:
			ShutDown();
			break;
		}

		m_currentState = m_nextState;
		Startup();
	}
}

void Game::UpdateEntities(float deltaSeconds)
{
	for (int controllerIndex = 0; controllerIndex < m_controllers.size(); controllerIndex++) {
		Controller* controller = m_controllers[controllerIndex];
		if (controller) {
			controller->Update(deltaSeconds);
		}
	}

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (entity) {
			entity->m_pushedFromBottom = false;
			if (entity->GetPhysicsMode() != MovementMode::NOCLIP) {
				m_world->PushEntityOutOfWorld(*entity);
			}
			entity->Update(deltaSeconds);
		}
	}

	if (m_gameCamera->GetPhysicsMode() != MovementMode::NOCLIP) { // Camera is updated apart specifically to control update order
		m_world->PushEntityOutOfWorld(*m_gameCamera);
	}
	m_gameCamera->Update(deltaSeconds);

	m_worldCamera.SetTransform(m_gameCamera->m_position, m_gameCamera->m_orientation);
}

void Game::ShutDown()
{
	for (int soundIndexPlaybackID = 0; soundIndexPlaybackID < GAME_SOUND::NUM_SOUNDS; soundIndexPlaybackID++) {
		SoundPlaybackID soundPlayback = g_soundPlaybackIDs[soundIndexPlaybackID];
		if (soundPlayback != -1) {
			g_theAudio->StopSound(soundPlayback);
		}
	}


	m_useTextAnimation = false;
	m_currentText = "";
	m_transitionTextColor = false;
	m_timeTextAnimation = 0.0f;
	m_player = nullptr;
	m_deltaTimeSample = nullptr;



	if (m_world) {
		delete m_world;
		m_world = nullptr;

		BlockTemplate::DestroyDefinitions();
		BlockDefinition::DestroyDefinitions();
	}

	if (m_gameCamera) {
		delete m_gameCamera;
		m_gameCamera = nullptr;
	}

	for (int controllerIndex = 0; controllerIndex < m_allEntities.size(); controllerIndex++) {
		Controller*& controller = m_controllers[controllerIndex];
		if (controller) {
			delete controller;
			controller = nullptr;
		}
	}

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}

	m_player = nullptr;
	m_playerController = nullptr;

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
	g_textures[(int)GAME_TEXTURE::SimpleMinerSprites] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/BasicSprites_64x64.png");

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
	case GameState::Play:
		UpdatePlay(gameDeltaSeconds);
		break;
	default:
		break;
	}


	if (m_printStatsToConsole) {
		g_theConsole->Clear();

		if (m_currentFrameCount >= 10) {
			for (int perfIndex = 0; perfIndex < 10; perfIndex++) {
				g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Frame %d: %f", perfIndex + 1, m_performanceFirstTenFrames[perfIndex]));
			}
		}

		double avgPerlin = m_totalPerlinNoiseChunkGenTime / (double)m_countPerlinNoiseChunkGenTime;
		double avgChunkLoad = m_totalChunkLoadIncludeRLETime / (double)m_countChunkLoadIncludeRLETime;
		double avgSaveChunk = m_totalChunkSaveIncludeRLETime / (double)m_countChunkSaveIncludeRLETime;
		double avgCPUMesh = m_totalChunkMeshRegen / (double)m_countChunkMeshRegen;
		double avgDirtyLighting = m_totalLightResolveTime / (double)m_countLightResolveTime;

		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Worst Perlin noise chunk gen time:\t %f", m_worstPerlinNoiseChunkGenTime));
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Avg Perlin noise chunk gen time:\t\t\t %f", avgPerlin));

		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Worst load chunk from Disk time:\t\t\t %f", m_worstChunkLoadIncludeRLETime));
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Avg load chunk from Disk time:\t\t\t\t\t %f", avgChunkLoad));

		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Worst save chunk time:\t\t\t\t\t\t\t\t\t\t\t\t\t %f", m_worstChunkSaveIncludeRLETime));
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Avg save chunk time:\t\t\t\t\t\t\t\t\t\t\t\t\t %f", avgSaveChunk));

		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Worst mesh rebuild time:\t\t\t\t\t\t\t\t\t\t\t %f", m_worstChunkMeshRegen));
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Avg mesh rebuild time:\t\t\t\t\t\t\t\t\t\t\t\t\t %f", avgCPUMesh));

		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Worst time resolving light:\t\t\t\t\t\t\t\t %f", m_worstLightResolveTime));
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, Stringf("Avg time resolving light:\t\t\t\t\t\t\t\t\t\t %f", avgDirtyLighting));
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

void Game::UpdatePlay(float deltaSeconds)
{
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}

	UpdateInputPlay(deltaSeconds);

	//std::string playerPosInfo = Stringf("Player position: %s ", m_player->m_position.ToString());
	//DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);
	//std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);
	//DebugAddMessage(gameInfoStr, 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	UpdateDeveloperCheatCodes(deltaSeconds);
	UpdateEntities(deltaSeconds);

	m_world->SetCameraPositionConstant(m_player->m_position);
	m_world->Update(deltaSeconds);

	if (m_skipPerformanceFirstFrame) {
		m_skipPerformanceFirstFrame = false;
		return;
	}

	if (m_currentFrameCount < 10) {
		m_performanceFirstTenFrames[m_currentFrameCount] = deltaSeconds * 1000.0f;
		m_currentFrameCount++;
	}
	//DisplayClocksInfo();


}

void Game::UpdateInputPlay(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) {
		m_world->DigBlockAtRaycastHit();
	}

	if (g_theInput->WasKeyJustReleased(KEYCODE_LEFT_MOUSE)) {
		m_world->StopDigging();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE)) {
		m_world->PlaceBlockAtRaycastHit(m_currentBlockType);
	}

	if (g_theInput->WasKeyJustPressed('1')) {
		m_currentBlockType = 1;
	}

	if (g_theInput->WasKeyJustPressed('2')) {
		m_currentBlockType = 2;
	}

	if (g_theInput->WasKeyJustPressed('3')) {
		m_currentBlockType = 3;
	}

	if (g_theInput->WasKeyJustPressed('4')) {
		m_currentBlockType = 4;
	}

	if (g_theInput->WasKeyJustPressed('5')) {
		m_currentBlockType = 5;
	}

	if (g_theInput->WasKeyJustPressed('6')) {
		m_currentBlockType = 6;
	}

	if (g_theInput->WasKeyJustPressed('7')) {
		m_currentBlockType = 7;
	}

	if (g_theInput->WasKeyJustPressed('8')) {
		m_currentBlockType = 8;
	}

	if (g_theInput->WasKeyJustPressed('9')) {
		m_currentBlockType = 9;
	}

	if (g_theInput->WasKeyJustPressed('0')) {
		m_currentBlockType = 0;
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
		m_nextState = GameState::Play;
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

	if (g_theInput->WasKeyJustPressed('R')) {
		m_world->m_freezeRaycast = !m_world->m_freezeRaycast;
		if (m_world->m_freezeRaycast) {
			m_world->m_storedRaycast = m_world->m_latestRaycast;
		}
	}

	if (g_theInput->WasKeyJustPressed('L')) {
		m_world->m_stepDebugLighting = true;
		DebugRenderClear();
	}

	if (g_theInput->WasKeyJustPressed('T')) {
		sysClock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('Y')) {
		m_world->m_worldTimeScale = g_gameConfigBlackboard.GetValue("WORLD_TIME_SCALE", 200.0f) * 50.0f;
	}

	if (g_theInput->WasKeyJustReleased('Y')) {
		m_world->m_worldTimeScale = g_gameConfigBlackboard.GetValue("WORLD_TIME_SCALE", 200.0f);
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
	if (g_theInput->WasKeyJustPressed(KEYCODE_F2)) {
		if (m_gameCamera->GetMode() == CameraMode::SPECTATOR) {
			m_playerController->Unpossess();
			m_playerController->Possess(*m_player);
		}
		m_gameCamera->SetNextCameraMode();
		if (m_gameCamera->GetMode() == CameraMode::SPECTATOR) {
			m_playerController->Unpossess();
			m_playerController->Possess(*m_gameCamera);
		}
	}

	if (g_theInput->WasKeyJustReleased(KEYCODE_F3)) {
		m_player->SetNextPhysicsMode();
		m_gameCamera->SetNextPhysicsMode();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6)) {
		KillAllEnemies();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) {
		Vec3 playerPos = m_player->m_position;
		EulerAngles playerOrientation = m_player->m_orientation;
		ShutDown();
		Startup();
		m_player->m_position = playerPos;
		m_player->m_orientation = playerOrientation;
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

	if (g_theInput->WasKeyJustPressed('J')) {
		m_world->CarveBlocksInRadius(m_player->m_position, 5.0f);
	}

	if (g_theInput->WasKeyJustPressed('F')) {
		m_world->ToggleFog();
	}

	//if (g_theInput->WasKeyJustPressed('1')) {
	//	FireEvent("DebugAddWorldWireSphere");
	//}

	//if (g_theInput->WasKeyJustPressed('2')) {
	//	FireEvent("DebugAddWorldLine");
	//}

	//if (g_theInput->WasKeyJustPressed('3')) {
	//	FireEvent("DebugAddBasis");
	//}

	//if (g_theInput->WasKeyJustPressed('4')) {
	//	FireEvent("DebugAddBillboardText");
	//}

	//if (g_theInput->WasKeyJustPressed('5')) {
	//	FireEvent("DebugAddWorldWireCylinder");
	//}

	//if (g_theInput->WasKeyJustPressed('6')) {
	//	EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
	//	std::string cameraOrientationInfo = Stringf("Camera Orientation: (%.2f, %.2f, %.2f)", cameraOrientation.m_yawDegrees, cameraOrientation.m_pitchDegrees, cameraOrientation.m_rollDegrees);
	//	DebugAddMessage(cameraOrientationInfo, 5.0f, Rgba8::WHITE, Rgba8::RED);
	//}

	//if (g_theInput->WasKeyJustPressed('9')) {
	//	Clock const& debugClock = DebugRenderGetClock();
	//	double newTimeDilation = debugClock.GetTimeDilation();
	//	newTimeDilation -= 0.1;
	//	if (newTimeDilation < 0.1) newTimeDilation = 0.1;
	//	DebugRenderSetTimeDilation(newTimeDilation);
	//}

	//if (g_theInput->WasKeyJustPressed('0')) {
	//	Clock const& debugClock = DebugRenderGetClock();
	//	double newTimeDilation = debugClock.GetTimeDilation();
	//	newTimeDilation += 0.1;
	//	if (newTimeDilation > 10.0) newTimeDilation = 10.0;
	//	DebugRenderSetTimeDilation(newTimeDilation);
	//}
}

void Game::UpdateCameras(float deltaSeconds)
{
	UpdateWorldCamera(deltaSeconds);

	UpdateUICamera(deltaSeconds);

}

void Game::UpdateWorldCamera(float deltaSeconds)
{
	// #FixBeforeSubmitting
	//m_worldCamera.SetOrthoView(Vec2(-1, -1 ), Vec2(1, 1));
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 1000.0f);

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
	//Clock& devClock = g_theConsole->m_clock;
	//Clock const& debugClock = DebugRenderGetClock();

	//double devClockFPS = 1.0 / devClock.GetDeltaTime();
	//double debugClockFPS = 1.0 / debugClock.GetDeltaTime();
	double gameFPS = GetFPS();

	//double devClockTotalTime = devClock.GetTotalTime();
	//double debugTotalTime = debugClock.GetTotalTime();
	double gameTotalTime = m_clock.GetTotalTime();

	//double devClockScale = devClock.GetTimeDilation();
	//double debugScale = debugClock.GetTimeDilation();
	double gameScale = m_clock.GetTimeDilation();


	//std::string devClockInfo = Stringf("Dev Console:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", devClockTotalTime, devClockFPS, devClockScale);
	//std::string debugClockInfo = Stringf("Debug Render:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", debugTotalTime, debugClockFPS, debugScale);
	std::string gameClockInfo = Stringf("Game:\t | Time: %.2f  AvgFPS: %.2f  Scale: %.2f", gameTotalTime, gameFPS, gameScale);

	//float topPadding = 50.0f;

	//Vec2 devClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, devClockInfo), m_UISizeY - m_textCellHeight - topPadding);
	//Vec2 gameClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, gameClockInfo), m_UISizeY - (m_textCellHeight * 2.0f) - topPadding);
	Vec2 gameClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, gameClockInfo), m_UISizeY - (m_textCellHeight));
	//Vec2 debugClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, debugClockInfo), m_UISizeY - (m_textCellHeight * 3.0f) - topPadding);

	//DebugAddScreenText(devClockInfo, devClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	//DebugAddScreenText(debugClockInfo, debugClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(gameClockInfo, gameClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
}


void Game::Render() const
{
	switch (m_currentState)
	{
	case GameState::AttractScreen:
		RenderAttractScreen();
		break;
	case GameState::Play:
		RenderPlay();
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
	g_theRenderer->BindShader(nullptr);
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (entity) {
			entity->Render();
		}
	}

	if (m_gameCamera->GetMode() != CameraMode::FIRST_PERSON) {
		std::vector<Vertex_PCU> velocityVerts;
		velocityVerts.reserve(6);
		AddVertsForLineSegment3D(velocityVerts, m_player->m_position, m_player->m_position + m_player->m_velocity, Rgba8::GREEN, 0.0025f);

		g_theRenderer->DrawVertexArray(velocityVerts);
	}

}

void Game::RenderPlay() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(m_world->GetSkyColor());

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	m_world->Render();


	RenderEntities();

	double frameTime = m_clock.GetDeltaTime();
	float fps = 1.0f / (float)frameTime;

	frameTime *= 1000.0;

	BlockDefinition const* currentBlockDef = BlockDefinition::GetDefById(m_currentBlockType);

	std::string movementDebugInfo = "WASD=Movement, QE=Vertical, Space/Shift=Fast, F1=Debug, F2=Camera, F8=Regenerate, ";

	std::string addiotionalDebugInfo = "Block= ";
	addiotionalDebugInfo += currentBlockDef->m_name;
	addiotionalDebugInfo += ", CameraView=";
	addiotionalDebugInfo += m_gameCamera->GetCurrentCameraModeAsText();

	addiotionalDebugInfo += ", Physics=";
	addiotionalDebugInfo += m_gameCamera->GetPhysicsModeAsString();

	std::string worldDebugInfo = Stringf("Chunks: %d Vertex: %d Index: %d XYZ: %s YPR: %s FPS: %.2f(%.1f)", m_world->m_activeChunks.size(), m_world->m_vertexAmount, m_world->m_indexAmount, m_player->m_position.ToString().c_str(), m_player->m_orientation.ToString().c_str(), fps, frameTime);


	DebugAddScreenText(movementDebugInfo, Vec2(0.0f, m_UISizeY * 0.97f), 0.0f, Vec2::ZERO, m_textCellHeight * 0.8f, Rgba8::YELLOW, Rgba8::YELLOW);
	DebugAddScreenText(addiotionalDebugInfo, Vec2(0.0f, m_UISizeY * 0.94f), 0.0f, Vec2::ZERO, m_textCellHeight * 0.8f, Rgba8::YELLOW, Rgba8::YELLOW);
	DebugAddScreenText(worldDebugInfo, Vec2(0.0f, m_UISizeY * 0.91f), 0.0f, Vec2::ZERO, m_textCellHeight * 0.8f, Rgba8::BLUE, Rgba8::BLUE);
	g_theRenderer->EndCamera(m_worldCamera);
}

void Game::RenderAttractScreen() const
{
	g_theRenderer->BeginCamera(m_AttractScreenCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::ALWAYS, false);
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

	std::vector<Vertex_PCU> gameInfoVerts;

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

void Game::RefreshLoadStats()
{
	m_printStatsToConsole = true;
}

Rgba8 const Game::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
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
	Entity* const& player = pointerToSelf->m_player;
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
