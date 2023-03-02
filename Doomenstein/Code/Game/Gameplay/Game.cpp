#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/TileMaterialDefinition.hpp"
#include "Game/Gameplay/TileSetDefinition.hpp"
#include "Game/Gameplay/TileDefinition.hpp"
#include "Game/Gameplay/MapDefinition.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"
#include "Game/Gameplay/WeaponDefinition.hpp"
#include "Game/Gameplay/Map.hpp"

extern bool g_drawDebug;
extern App* g_theApp;
extern AudioSystem* g_theAudio;
Game* pointerToSelf = nullptr;


Game::Game(App* appPointer) :
	m_theApp(appPointer)
{
	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);

	for (int worldCameraIndex = 0; worldCameraIndex < 4; worldCameraIndex++) {
		m_playerWorldCameras[worldCameraIndex].SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system
	}
	pointerToSelf = this;

	SubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	SubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	SubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	SubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	SubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	SubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	SubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	SubscribeEventCallbackFunction("Controls", GetControls);
	SubscribeEventCallbackFunction("RaycastDebugToggle", RaycastDebugToggle);
	g_masterVolume = g_gameConfigBlackboard.GetValue("MASTER_VOLUME", 1.0f);

}

Game::~Game()
{
	MapDefinition::DestroyDefinitions();
	WeaponDefinition::DestroyDefinitions();
	ActorDefinition::ClearDefinitions();

	TileSetDefinition::DestroyDefinitions();
	TileDefinition::DestroyDefinitions();
	TileMaterialDefinition::DestroyDefinitions();
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
	case GameState::Lobby:
		StartupLobby();
		break;
	case GameState::GamemodeLobby:
		StartupGamemodeLobby();
		break;
	case GameState::VictoryScreen:
		StartupVictory();
		break;
	}


}

void Game::StartupAttractScreen()
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;


	/*if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}*/

	if (g_soundPlaybackIDs[GAME_SOUND::MAIN_MENU_IN_THE_DARK] == -1) {
		PlaySound(GAME_SOUND::MAIN_MENU_IN_THE_DARK, m_musicVolume, true);
	}

	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	DebugRenderClear();
}

void Game::StartupPlay()
{

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
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


	SetViewportsForPlayers();
	m_map = new Map(this, m_maptoLoad, m_joinedPlayers);
	g_theAudio->SetNumListeners((int)m_joinedPlayers.size());

	if (g_soundPlaybackIDs[GAME_SOUND::AT_DOOMS_GATE] == -1) {
		PlaySound(GAME_SOUND::AT_DOOMS_GATE, m_musicVolume, true);
	}
}

void Game::StartupLobby()
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);


	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;
	DebugRenderClear();
	m_joinedPlayers.clear();

}

void Game::StartupGamemodeLobby()
{
	m_currentGamemodeOption = 0;
}

void Game::StartupVictory()
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	if (g_soundPlaybackIDs[GAME_SOUND::AT_DOOMS_GATE] == -1) {
		PlaySound(GAME_SOUND::AT_DOOMS_GATE, m_musicVolume, true);
	}
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
		case GameState::Lobby:
			ShutDown();
			break;
		default:
			break;
		case GameState::GamemodeLobby:
			ShutDown();
			break;
		case GameState::VictoryScreen:
			ShutDown();
			break;
		}

		m_currentState = m_nextState;
		Startup();
	}
}

void Game::ShutDown()
{
	delete m_map;
	m_map = nullptr;

	for (int soundIndexPlaybackID = 0; soundIndexPlaybackID < GAME_SOUND::NUM_SOUNDS; soundIndexPlaybackID++) {
		SoundPlaybackID soundPlayback = g_soundPlaybackIDs[soundIndexPlaybackID];
		if (soundPlayback != -1) {
			if (soundIndexPlaybackID == GAME_SOUND::CLICK) continue;
			if ((soundIndexPlaybackID == GAME_SOUND::MAIN_MENU_IN_THE_DARK) && (m_nextState == GameState::Lobby || m_nextState == GameState::AttractScreen)) continue;
			g_theAudio->StopSound(soundPlayback);
			g_soundPlaybackIDs[(SoundPlaybackID)soundIndexPlaybackID] = (SoundPlaybackID)-1;
		}
	}

	m_useTextAnimation = false;
	m_currentText = "";
	m_transitionTextColor = false;
	m_timeTextAnimation = 0.0f;
	m_deltaTimeSample = nullptr;
}

void Game::LoadAssets()
{
	if (m_loadedAssets) return;
	LoadTextures();
	LoadSoundFiles();
	TileMaterialDefinition::InitializeDefinitions();
	TileDefinition::InitializeDefinitions();
	TileSetDefinition::InitializeDefinitions();

	ActorDefinition::InitializeDefinitions("Data/Definitions/ProjectileActorDefinitions.xml");
	WeaponDefinition::InitializeDefinitions("Data/Definitions/WeaponDefinitions.xml");
	ActorDefinition::InitializeDefinitions("Data/Definitions/ActorDefinitions.xml");
	MapDefinition::InitializeDefinitions();

	m_loadedAssets = true;
}

void Game::LoadTextures()
{
	g_textures[(int)GAME_TEXTURE::TestUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
	g_textures[(int)GAME_TEXTURE::CompanionCube] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/CompanionCube.png");
	g_textures[(int)GAME_TEXTURE::Terrain] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");
	g_textures[(int)GAME_TEXTURE::Infected] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/infectedTexture.jpg");
	g_textures[(int)GAME_TEXTURE::Victory] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EndScreenImage.jpg");
	g_textures[(int)GAME_TEXTURE::Died] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/UDed.png");
	g_textures[(int)GAME_TEXTURE::Doom_Face] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/DoomGuyFace.png");

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

	g_soundSources[GAME_SOUND::CLAIRE_DE_LUNE] = "Data/Audio/ClaireDeLuneRoyaltyFreeFirstMinute.wav";
	g_soundSources[GAME_SOUND::DOOR_KNOCKING_SOUND] = "Data/Audio/KnockingDoor.wav";
	g_soundSources[GAME_SOUND::AT_DOOMS_GATE] = "Data/Audio/Music/E1M1_AtDoomsGate.mp2";
	g_soundSources[GAME_SOUND::MAIN_MENU_IN_THE_DARK] = "Data/Audio/Music/MainMenu_InTheDark.mp2";
	g_soundSources[GAME_SOUND::CLICK] = "Data/Audio/Click.mp3";
	g_soundSources[GAME_SOUND::DEMON_ACTIVE] = "Data/Audio/DemonActive.wav";
	g_soundSources[GAME_SOUND::DEMON_ATTACK] = "Data/Audio/DemonAttack.wav";
	g_soundSources[GAME_SOUND::DEMON_DEATH] = "Data/Audio/DemonDeath.wav";
	g_soundSources[GAME_SOUND::DEMON_HURT] = "Data/Audio/DemonHurt.wav";
	g_soundSources[GAME_SOUND::DISCOVERED] = "Data/Audio/Discovered.mp3";
	g_soundSources[GAME_SOUND::PISTOL_FIRE] = "Data/Audio/PistolFire.wav";
	g_soundSources[GAME_SOUND::PLASMA_FIRE] = "Data/Audio/PlasmaFire.wav";
	g_soundSources[GAME_SOUND::PLASMA_HIT] = "Data/Audio/PlasmaHit.wav";
	g_soundSources[GAME_SOUND::PLAYER_DEATH_1] = "Data/Audio/PlayerDeath1.wav";
	g_soundSources[GAME_SOUND::PLAYER_DEATH_2] = "Data/Audio/PlayerDeath2.wav";
	g_soundSources[GAME_SOUND::PLAYER_GIBBED] = "Data/Audio/PlayerGibbed.wav";
	g_soundSources[GAME_SOUND::PLAYER_HIT] = "Data/Audio/PlayerHit.wav";
	g_soundSources[GAME_SOUND::PLAYER_HURT] = "Data/Audio/PlayerHurt.wav";
	g_soundSources[GAME_SOUND::TELEPORTER] = "Data/Audio/Teleporter.wav";
	g_soundSources[GAME_SOUND::SPIDER_HURT] = "Data/Audio/SpiderHurt.wav";
	g_soundSources[GAME_SOUND::SPIDER_DEATH] = "Data/Audio/SpiderDeath.wav";
	g_soundSources[GAME_SOUND::SPIDER_WALK] = "Data/Audio/SpiderWalk.wav";
	g_soundSources[GAME_SOUND::SPIDER_ATTACK] = "Data/Audio/SpiderAttack.wav";
	g_soundSources[GAME_SOUND::GRAVITY_FIRE] = "Data/Audio/GravityGunShooting.wav";
	g_soundSources[GAME_SOUND::GRAVITY_PULSE] = "Data/Audio/PulseSound.wav";


	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		SoundID soundID = g_theAudio->CreateOrGetSound(g_soundSources[soundIndex]);
		g_sounds[soundIndex] = soundID;
		g_soundEnumIDBySource[g_soundSources[soundIndex]] = (GAME_SOUND)soundIndex;
	}
	// Royalty free version of Claire De Lune found here:
	// https://soundcloud.com/pianomusicgirl/claire-de-lune

}

void Game::GoToVictoryScreen()
{
	m_nextState = GameState::VictoryScreen;
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
	case GameState::Lobby:
		UpdateLobby(gameDeltaSeconds);
		break;
	case GameState::GamemodeLobby:
		UpdateGamemodeLobby(gameDeltaSeconds);
		break;
	case GameState::VictoryScreen:
		UpdateVictory(gameDeltaSeconds);
		break;
	default:
		break;
	}

	UpdateDeveloperCheatCodes(gameDeltaSeconds);
	//DisplayClocksInfo();
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
		g_theInput->ResetKeyStates();
	}
}

void Game::UpdatePlay(float deltaSeconds)
{
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}

	UpdateInputPlay(deltaSeconds);
	//UpdateDeveloperCheatCodes(deltaSeconds);
	m_map->Update(deltaSeconds);

	UpdateListeners();

	//Player* player = m_map->GetPlayer(0);

	//std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", player->m_position.x, player->m_position.y, player->m_position.z);
	//DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);
	//std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);
	//DebugAddMessage(gameInfoStr, 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	//DisplayClocksInfo();
}

void Game::UpdateInputPlay(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}

	bool shouldShootRaycast = g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) || g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE);
	shouldShootRaycast = shouldShootRaycast || controller.WasButtonJustPressed(XboxButtonID::LeftThumb);
	shouldShootRaycast = shouldShootRaycast || controller.WasButtonJustPressed(XboxButtonID::RightThumb);

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) || g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE)) {
		if (pointerToSelf->m_map) {
			Player* player = m_map->GetPlayer(0);
			Mat44 playerMat = player->m_orientation.GetMatrix_XFwd_YLeft_ZUp();
			Vec3 const& forward = playerMat.GetIBasis3D();
			Vec3 rayStart = player->m_position;
			float rayLength = (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE)) ? m_debugRayLength : m_debugShortRayLength;
			float rayLengthController = (controller.WasButtonJustPressed(XboxButtonID::LeftThumb)) ? m_debugRayLength : m_debugShortRayLength;

			float usedRayLength = (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE)) ? rayLength : rayLengthController;

			if (m_isRaycastDebugEnabled) {
				DebugAddWorldLine(rayStart, rayStart + forward * usedRayLength, 0.01f, 10.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
				m_map->RaycastVsMap(rayStart, forward, usedRayLength, player);
			}
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
	bool buttonPressed = false;

	if (exitAttractMode) {
		m_nextState = GameState::Lobby;
		buttonPressed = true;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
		buttonPressed = true;
	}

	if (buttonPressed) {
		PlaySound(GAME_SOUND::CLICK);
	}
	//UpdateDeveloperCheatCodes(deltaSeconds);
}

void Game::UpdateLobby(float deltaSeconds)
{
	UpdateInputLobby(deltaSeconds);
}

void Game::UpdateInputLobby(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	bool buttonPressed = false;

	bool erasedPlayer = false;
	bool hasPlayerJoined[5] = { false };
	int keyboardPlayerIndex = -1;
	for (int playersInfoIndex = 0; playersInfoIndex < m_joinedPlayers.size(); playersInfoIndex++) {
		if (m_joinedPlayers[playersInfoIndex].m_controller == -1) {
			hasPlayerJoined[4] = m_joinedPlayers[playersInfoIndex].m_playerIndex != -1;
			if (hasPlayerJoined[4]) {
				keyboardPlayerIndex = playersInfoIndex;
			}
		}
		else {
			hasPlayerJoined[m_joinedPlayers[playersInfoIndex].m_controller] = m_joinedPlayers[playersInfoIndex].m_playerIndex != -1;
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) {
		buttonPressed = true;
		if (m_joinedPlayers.size() == 0) {
			m_nextState = GameState::AttractScreen;
		}
		else {
			if (hasPlayerJoined[4]) {
				m_joinedPlayers.erase(m_joinedPlayers.begin() + keyboardPlayerIndex);
				erasedPlayer = true;
			}
		}
	}

	if (g_theInput->WasKeyJustPressed(' ')) {
		buttonPressed = true;
		if (keyboardPlayerIndex == -1) {
			PlayerInfo newPlayer = { (int)m_joinedPlayers.size(), -1 };
			m_joinedPlayers.push_back(newPlayer);
		}
	}

	bool pressedStart = false;

	for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; controllerIndex++) {
		XboxController const& controller = g_theInput->GetController(controllerIndex);
		if (controller.IsConnected()) {
			if (!hasPlayerJoined[controllerIndex]) {
				if (controller.WasButtonJustPressed(XboxButtonID::A) || controller.WasButtonJustPressed(XboxButtonID::Start)) {
					buttonPressed = true;
					PlayerInfo newPlayer = { (int)m_joinedPlayers.size(), controllerIndex };
					m_joinedPlayers.push_back(newPlayer);
				}
				if (controller.WasButtonJustPressed(XboxButtonID::Back) || controller.WasButtonJustPressed(XboxButtonID::B)) {
					buttonPressed = true;
					if (m_joinedPlayers.size() == 0) {
						m_nextState = GameState::AttractScreen;
					}
				}
			}
			else {
				if (controller.WasButtonJustPressed(XboxButtonID::A) || controller.WasButtonJustPressed(XboxButtonID::Start)) {
					buttonPressed = true;
					pressedStart = true;
				}

				if (controller.WasButtonJustPressed(XboxButtonID::Back) || controller.WasButtonJustPressed(XboxButtonID::B)) {
					buttonPressed = true;
					int playerIndex = GetPlayerIndexForController(controllerIndex);
					if (m_joinedPlayers.size() == 0) {
						m_nextState = GameState::AttractScreen;
					}
					if (playerIndex != -1) {
						m_joinedPlayers.erase(m_joinedPlayers.begin() + playerIndex);
						erasedPlayer = true;
					}
				}
			}
		}
	}

	if (erasedPlayer) {
		for (int playerIndex = 0; playerIndex < m_joinedPlayers.size(); playerIndex++) {
			PlayerInfo& playerInfo = m_joinedPlayers[playerIndex];
			playerInfo.m_playerIndex = playerIndex;
		}
	}

	if (m_joinedPlayers.size() > 0) {
		bool startGamePC = g_theInput->WasKeyJustPressed(KEYCODE_ENTER) && hasPlayerJoined[4];
		startGamePC = startGamePC || (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) && hasPlayerJoined[4]);
		if (startGamePC || pressedStart) { // PC Player joined and pressed start Or Gamepad player joined and pressed Start
			m_nextState = GameState::GamemodeLobby;
		}
	}

	if (buttonPressed) {
		PlaySound(GAME_SOUND::CLICK);
	}
}

void Game::UpdateGamemodeLobby(float deltaSeconds)
{
	UpdateInputGamemodeLobby(deltaSeconds);
}

void Game::UpdateInputGamemodeLobby(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	AnalogJoystick const& leftStick = controller.GetLeftStick();

	if (g_theInput->WasKeyJustPressed(KEYCODE_DOWNARROW) || g_theInput->WasKeyJustPressed('S') || controller.WasButtonJustPressed(XboxButtonID::Down) || (leftStick.GetPosition().y < 0.0f)) {
		m_currentGamemodeOption++;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_UPARROW) || g_theInput->WasKeyJustPressed('W') || controller.WasButtonJustPressed(XboxButtonID::Up) || (leftStick.GetPosition().y > 0.0f)) {
		m_currentGamemodeOption--;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::B)) {
		m_nextState = GameState::AttractScreen;
	}

	m_currentGamemodeOption = m_currentGamemodeOption % 2;
	if (m_currentGamemodeOption < 0) m_currentGamemodeOption = 1;


	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) || controller.WasButtonJustPressed(XboxButtonID::A) || controller.WasButtonJustPressed(XboxButtonID::Start)) {
		m_nextState = GameState::Play;
		std::string defaultMap = g_gameConfigBlackboard.GetValue("DEFAULT_MAP", "TestMap");
		std::string defaultHordeMap = g_gameConfigBlackboard.GetValue("DEFAULT_HORDE_MAP", "HordeMap");

		switch (m_currentGamemodeOption)
		{
		case 0:
			m_maptoLoad = MapDefinition::GetByName(defaultMap);
			break;
		case 1:
			m_maptoLoad = MapDefinition::GetByName(defaultHordeMap);
			break;
		default:
			break;
		}
	}

}

void Game::UpdateVictory(float deltaSeconds)
{
	UpdateInputVictory(deltaSeconds);
}

void Game::UpdateInputVictory(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool pressedValidButton = controller.WasButtonJustPressed(XboxButtonID::B) || controller.WasButtonJustPressed(XboxButtonID::Start);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || pressedValidButton) {
		m_nextState = GameState::AttractScreen;
	}
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

	if (g_theInput->WasKeyJustPressed(KEYCODE_F9)) {
		g_drawDebug = !g_drawDebug;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6) && g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
		KillAllEnemies();
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

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed('1')) {
		FireEvent("DebugAddWorldWireSphere");
	}

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed('2')) {
		FireEvent("DebugAddWorldLine");
	}

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed('3')) {
		FireEvent("DebugAddBasis");
	}

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed('4')) {
		FireEvent("DebugAddBillboardText");
	}

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed('5')) {
		FireEvent("DebugAddWorldWireCylinder");
	}

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed('6')) {
		EulerAngles cameraOrientation = m_playerWorldCameras[0].GetViewOrientation();
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

	if (g_theInput->WasKeyJustPressed('N') || controller.WasButtonJustPressed(XboxButtonID::RightShoulder)) {
		if (m_map && (m_map->GetPlayerAmount() == 1)) {
			Player* playerController = m_map->GetPlayer(0);
			Actor* actor = playerController->GetActor();
			Actor* nextPossessableActor = m_map->GetNextPossessableActor(actor);
			if (nextPossessableActor) {
				playerController->Possess(nullptr);
				playerController->Possess(nextPossessableActor);
			}
		}
	}

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) && g_theInput->WasKeyJustPressed(KEYCODE_F2)) {
		if (m_map) {
			m_map->GetPlayer(0)->ToggleInvincibility();
		}
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
	//m_worldCamera.SetOrthoView(Vec2(-1, -1 ), Vec2(1, 1));

	if (m_screenShake) {
		float easingInScreenShake = m_timeShakingScreen / m_screenShakeDuration;
		easingInScreenShake *= (m_screenShakeTranslation * 0.95f);

		float randAmountX = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);
		float randAmountY = rng.GetRandomFloatInRange(-1.0f, 1.0f) * (m_screenShakeTranslation - easingInScreenShake);

		m_playerWorldCameras[0].TranslateCamera(randAmountX, randAmountY);

		m_timeShakingScreen += deltaSeconds;

		if (m_timeShakingScreen > m_screenShakeDuration) {
			m_screenShake = false;
			m_timeShakingScreen = 0.0f;;
		}
	}
}

void Game::UpdateListeners()
{
	if (!m_map) return;

	std::vector<Player*> players = m_map->GetPlayers();

	for (int playerIndex = 0; playerIndex < players.size(); playerIndex++) {

		Camera const* playerCamera = players[playerIndex]->m_worldCamera;

		Vec3 playerForward = Vec3::ZERO;
		Vec3 playerUp = Vec3::ZERO;
		Vec3 playerLeft = Vec3::ZERO;
		playerCamera->GetViewOrientation().GetVectors_XFwd_YLeft_ZUp(playerForward, playerLeft, playerUp);

		g_theAudio->UpdateListener(playerIndex, playerCamera->GetViewPosition(), Vec3::ZERO, playerForward, playerUp);
	}
}

void Game::UpdateUICamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	m_UICamera.SetOrthoView(Vec2(), Vec2(m_UISizeX, m_UISizeY));
	for (int playerUICameraIndex = 0; playerUICameraIndex < 4; playerUICameraIndex++) {
		m_playerUICameras[playerUICameraIndex].SetOrthoView(Vec2(), Vec2(m_UISizeX, m_UISizeY));
	}
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
	m_map->KillAllDemons();
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
	case GameState::Play:
		RenderPlay();
		break;
	case GameState::Lobby:
		RenderLobby();
		break;
	case GameState::GamemodeLobby:
		RenderGamemodeLobby();
		break;
	case GameState::VictoryScreen:
		RenderVictory();
		break;
	default:
		break;
	}

	DebugRenderWorld(m_playerWorldCameras[0]);
	RenderUI();
	DebugRenderScreen(m_UICamera);

}


void Game::RenderPlay() const
{
	if (m_map) {
		std::vector<Player*> const& players = m_map->GetPlayers();

		for (int playerIndex = 0; playerIndex < players.size(); playerIndex++) {
			Player const* player = players[playerIndex];
			Camera const& worldCamera = *player->m_worldCamera;

			g_theRenderer->BeginCamera(worldCamera);
			g_theRenderer->ClearScreen(Rgba8::BLACK);

			g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
			g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
			g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
			g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

			m_map->Render(worldCamera, playerIndex);
			g_theRenderer->EndCamera(worldCamera);
		}
	}


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

void Game::RenderLobby() const
{
	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::ALWAYS, false);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	AABB2 cameraBounds = m_UICamera.GetCameraBounds();
	AABB2 joinInfoAABB2 = cameraBounds.GetBoxWithin(0.1f, 0.1f, 0.9f, 0.25f);


	std::string joinText = "Press SPACE to join with mouse and keyboard\n";
	joinText += "Press START to join with controller\n";
	joinText += "Press ESCAPE or BACK to exit to Attract Screen";

	std::vector<Vertex_PCU> textVerts;

	g_squirrelFont->AddVertsForTextInBox2D(textVerts, joinInfoAABB2, m_textCellHeight, joinText);


	Vec2 paneSize = Vec2(0.8f, 0.8f);
	Vec2 playerPaneSize(0.3f, 0.3f);
	float paneSpacing = 0.1f;

	if (m_joinedPlayers.size() > 0) {
		int playersPerRow = (m_joinedPlayers.size() >= 2) ? 2 : 1;
		int playerPerCol = (m_joinedPlayers.size() >= 3) ? 2 : 1;

		float xSize = paneSize.x / (static_cast<float>(playersPerRow));
		float ySize = paneSize.y / (static_cast<float>(playerPerCol));
		playerPaneSize = Vec2(xSize, ySize);
	}



	for (int playerIndex = 0; playerIndex < m_joinedPlayers.size(); playerIndex++) {
		int playerHeight = playerIndex / 2;
		int playerWidth = playerIndex % 2;



		float minX = (playerWidth * playerPaneSize.x) + (paneSpacing * (playerIndex + 1));
		float minY = 0.2f + (playerHeight * playerPaneSize.y);

		Vec2 minUVs = Vec2(minX, minY);
		Vec2 maxUVs = minUVs;
		maxUVs += playerPaneSize;

		RenderPlayerJoinedInfo(textVerts, m_joinedPlayers[playerIndex], cameraBounds.GetBoxWithin(minUVs, maxUVs));
	}


	g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->EndCamera(m_UICamera);

}

void Game::RenderGamemodeLobby() const
{
	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::ALWAYS, false);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	AABB2 cameraBounds = m_UICamera.GetCameraBounds();
	AABB2 optionOneBounds = cameraBounds.GetBoxWithin(0.25f, 0.6f, 0.75f, 0.8f);
	AABB2 optionTwoBounds = cameraBounds.GetBoxWithin(0.25f, 0.3f, 0.75f, 0.5f);

	std::string optionOneText = "Deathmatch";
	std::string optionTwoText = "Hordes";

	AABB2 const& chosenOption = (m_currentGamemodeOption == 0) ? optionOneBounds : optionTwoBounds;

	std::vector<Vertex_PCU> quadVerts;
	std::vector<Vertex_PCU> textVerts;
	textVerts.reserve(3000);

	AddVertsForHollowAABB2D(quadVerts, chosenOption, 1.0f, Rgba8::RED);

	g_squirrelFont->AddVertsForTextInBox2D(textVerts, optionOneBounds, m_textCellHeight, optionOneText);
	g_squirrelFont->AddVertsForTextInBox2D(textVerts, optionTwoBounds, m_textCellHeight, optionTwoText);

	g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
	g_theRenderer->DrawVertexArray(textVerts);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(quadVerts);

	g_theRenderer->EndCamera(m_UICamera);

}

void Game::RenderVictory() const
{

	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::ALWAYS, false);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	AABB2 cameraBounds = m_UICamera.GetCameraBounds();
	std::vector<Vertex_PCU> victoryQuad;
	victoryQuad.reserve(6);

	AddVertsForAABB2D(victoryQuad, cameraBounds, Rgba8::WHITE);

	g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::Victory]);
	g_theRenderer->DrawVertexArray(victoryQuad);

	g_theRenderer->EndCamera(m_UICamera);
}


void Game::RenderUI() const
{

	g_theRenderer->BeginCamera(m_UICamera);

	if (m_useTextAnimation && m_currentState != GameState::AttractScreen) {

		RenderTextAnimation();
	}

	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());


	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);

	std::vector<Vertex_PCU> gameInfoVerts;

	std::vector<Vertex_PCU> blackOverlay;

	// #ToDo
	if (m_map) {
		std::vector<Player*>const& players = m_map->GetPlayers();
		for (int playerIndex = 0; playerIndex < players.size(); playerIndex++) {
			Player const* player = players[playerIndex];
			Camera const* playerCamera = player->m_screenCamera;
			AABB2 cameraBounds(playerCamera->GetOrthoBottomLeft(), playerCamera->GetOrthoTopRight());
			if (!m_map->IsActorAlive(player->GetActor())) {
				AABB2 screenBounds = cameraBounds.GetBoxWithin(playerCamera->GetViewport());
				AddVertsForAABB2D(blackOverlay, screenBounds, Rgba8::TRANSPARENT_BLACK);
			}
			player->RenderHUD(*playerCamera);

		}

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(blackOverlay);

		if (m_map->IsHordeMode()) {
			AABB2 hordeEnemiesInfo = m_UICamera.GetCameraBounds().GetBoxWithin(0.05f, 0.9f, 0.4f, 0.94f);
			AABB2 hordeTimeInfo = m_UICamera.GetCameraBounds().GetBoxWithin(0.05f, 0.94f, 0.4f, 0.98f);
			std::string hordeTimeInfoStr = "Time: ";
			std::string hordeEnemiesInfoStr = "Enemies left: ";

			float remainingWaveTime = m_map->GetRemainingWaveTime();
			float timeT = (m_map->GetTotalWaveTime() - remainingWaveTime) / m_map->GetTotalWaveTime();
			float enemiesT = (float)(m_map->m_enemiesSpawned - m_map->m_enemiesStillAlive) / (float)m_map->m_enemiesSpawned;

			Rgba8 timeInfoColor = Rgba8::InterpolateColors(Rgba8::WHITE, Rgba8::RED, timeT);
			Rgba8 enemiesInfoColor = Rgba8::InterpolateColors(Rgba8::WHITE, Rgba8::RED, enemiesT);

			if (m_map->m_hasFinishedSpawningWave) {
				hordeTimeInfoStr += Stringf("%.2f", remainingWaveTime);
			}
			else {
				hordeTimeInfoStr += "Enemies Still Spawning!";
			}

			hordeEnemiesInfoStr += Stringf("%d", m_map->m_enemiesStillAlive);
			g_squirrelFont->AddVertsForTextInBox2D(gameInfoVerts, hordeTimeInfo, m_textCellHeight, hordeTimeInfoStr, timeInfoColor, 1.0f, Vec2(0.0f, 0.5f));
			g_squirrelFont->AddVertsForTextInBox2D(gameInfoVerts, hordeEnemiesInfo, m_textCellHeight, hordeEnemiesInfoStr, enemiesInfoColor, 1.0f, Vec2(0.0f, 0.5f));
			g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
			g_theRenderer->DrawVertexArray(gameInfoVerts);
		}

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

void Game::RenderPlayerJoinedInfo(std::vector<Vertex_PCU>& verts, PlayerInfo const& playerInfo, AABB2 const& screenPane) const
{
	std::string playerNumberText = Stringf("Player %d", playerInfo.m_playerIndex + 1);
	std::string controllerInfoText = (playerInfo.m_controller == -1) ? "Mouse and Keyboard" : "Gamepad";
	std::string text = "";

	text += "Press SPACE/ENTER to start game\n";
	text += (playerInfo.m_controller == -1) ? "Press ESC " : "Press BACK ";
	text += "to leave game\n";

	g_squirrelFont->AddVertsForTextInBox2D(verts, screenPane, m_textCellHeight * 2.0f, playerNumberText, Rgba8::WHITE, 1.0f, Vec2(0.5f, 0.8f));
	g_squirrelFont->AddVertsForTextInBox2D(verts, screenPane, m_textCellHeight * 1.5f, controllerInfoText, Rgba8::WHITE, 1.0f, Vec2(0.5f, 0.6f));
	g_squirrelFont->AddVertsForTextInBox2D(verts, screenPane, m_textCellHeight, text, Rgba8::WHITE, 1.0f, Vec2(0.5f, 0.1f));
}

void Game::SetViewportsForPlayers()
{
	switch (m_joinedPlayers.size())
	{
	case 1:
		m_playerWorldCameras[0].SetViewport(AABB2::ZERO_TO_ONE);
		m_playerUICameras[0].SetViewport(AABB2::ZERO_TO_ONE);
		break;
	case 2:
		m_playerWorldCameras[0].SetViewport(AABB2(0.0f, 0.5f, 1.0f, 1.0f));
		m_playerUICameras[0].SetViewport(AABB2(0.0f, 0.5f, 1.0f, 1.0f));
		m_playerWorldCameras[1].SetViewport(AABB2(0.0f, 0.0f, 1.0f, 0.5f));
		m_playerUICameras[1].SetViewport(AABB2(0.0f, 0.0f, 1.0f, 0.5f));
		break;
	case 3:
	case 4:
	default:
		m_playerWorldCameras[0].SetViewport(AABB2::ZERO_TO_ONE);
		break;
	}
}

int Game::GetPlayerIndexForController(int controllerIndex) const
{
	for (int playerIndex = 0; playerIndex < m_joinedPlayers.size(); playerIndex++) {
		PlayerInfo const& playerInfo = m_joinedPlayers[playerIndex];
		if (playerInfo.m_controller == controllerIndex) return playerIndex;
	}

	ERROR_RECOVERABLE(Stringf("Could no find player with controller index: %d", controllerIndex));
	return -1;
}

Camera& Game::GetPlayerCamera(int playerIndex)
{
	UNUSED(playerIndex);
	return m_playerWorldCameras[playerIndex];
}

Camera& Game::GetPlayerUICamera(int playerIndex)
{
	UNUSED(playerIndex);
	return m_playerUICameras[playerIndex];
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
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

bool Game::DebugSpawnWorldWireSphere(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	if (!pointerToSelf->m_map || !pointerToSelf->m_isRaycastDebugEnabled) return false;
	DebugAddWorldWireSphere(pointerToSelf->m_map->GetPlayer(0)->m_position, 1, 5, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnWorldLine3D(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	if (!pointerToSelf->m_map || !pointerToSelf->m_isRaycastDebugEnabled) return false;

	DebugAddWorldLine(pointerToSelf->m_map->GetPlayer(0)->m_position, Vec3::ZERO, 0.125f, 5.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::XRAY);
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
	if (!pointerToSelf->m_map || !pointerToSelf->m_isRaycastDebugEnabled) return false;

	UNUSED(eventArgs);
	Mat44 invertedView = pointerToSelf->m_playerWorldCameras[0].GetViewMatrix();
	DebugAddWorldBasis(invertedView.GetOrthonormalInverse(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnWorldWireCylinder(EventArgs& eventArgs)
{
	if (!pointerToSelf->m_map || !pointerToSelf->m_isRaycastDebugEnabled) return false;

	UNUSED(eventArgs);

	float cylinderHeight = 2.0f;
	if (!pointerToSelf->m_map) return false;

	Vec3 cylbase = pointerToSelf->m_map->GetPlayer(0)->m_position;
	cylbase.z -= cylinderHeight * 0.5f;

	Vec3 cylTop = cylbase;
	cylTop.z += cylinderHeight;
	DebugAddWorldWireCylinder(cylbase, cylTop, 0.5f, 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnBillboardText(EventArgs& eventArgs)
{
	if (!pointerToSelf->m_map || !pointerToSelf->m_isRaycastDebugEnabled) return false;

	Player* const& player = pointerToSelf->m_map->GetPlayer(0);
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
	controlsStr += "1       - Equip Weapon 1\n";
	controlsStr += "2       - Equip Weapon 2\n";
	controlsStr += "2       - Add debug world line\n";
	controlsStr += "3       - Add debug world basis\n";
	controlsStr += "4       - Add debug world billboard text\n";
	controlsStr += "5       - Add debug wire cylinder\n";
	controlsStr += "6       - Add debug camera message\n";
	controlsStr += "9       - Decrease debug clock speed\n";
	controlsStr += "0       - Increase debug clock speed\n";
	controlsStr += "Left & Right Arrows       - Cycle through weapons\n";
	controlsStr += "N       - Possess other entity\n";
	controlsStr += "F       - Toggle Freefly Camera\n";

	Strings controlStringsSplit = SplitStringOnDelimiter(controlsStr, '\n');

	for (int stringIndex = 0; stringIndex < controlStringsSplit.size(); stringIndex++) {
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, controlStringsSplit[stringIndex]);
	}

	return false;
}

bool Game::RaycastDebugToggle(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	if (!pointerToSelf->m_map) {
		g_theConsole->AddLine(DevConsole::WARNING_COLOR, "No map loaded");
	}
	else {
		pointerToSelf->m_isRaycastDebugEnabled = !pointerToSelf->m_isRaycastDebugEnabled;
	}

	return false;
}
