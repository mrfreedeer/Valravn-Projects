#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/World.hpp"
#include "Game/Gameplay/Tile.hpp"

extern App* g_theApp;
extern AudioSystem* g_theAudio;

Game::Game(App* appPointer) :
	m_theApp(appPointer)
{

}

Game::~Game()
{

}

void Game::Startup()
{
	LoadAssets();
	//for (int i = 0; i < 22; i++) {
	//	g_theConsole->AddLine(Rgba8::WHITE,Stringf("Game is ON! %d", i));
	//}

	switch (m_currentState)
	{
	case GameState::AttractScreen:
		StartupAttractScreen();
		break;
	case GameState::Play:
		StartupPlay();
		break;
	case GameState::GameOver:
		StartupGameOver();
		break;
	case GameState::Victory:
		StartupVictory();
		break;
	default:
		break;
	}

	m_timeInCurrentState = 0.0f;
	g_theEventSystem->FireEvent("Test");
}

void Game::StartupAttractScreen()
{

	m_currentText = "Get Ready!";
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;

	Vec2 attractModePos(g_gameConfigBlackboard.GetValue("m_UICenterX", 0.0f), g_gameConfigBlackboard.GetValue("m_UICenterY", 0.0f));
	m_attractModePos = attractModePos;

	g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE], true);

}

void Game::StartupPlay()
{
	if (!m_playerRespawned && m_currentState != GameState::GameOver) {
		m_theWorld = new World(this);
	}
	PlaySound(GAME_SOUND::LIBRA_THEME, 1.0f, true);

	m_playerRespawned = false;
	m_isPaused = false;

	PlaySound(GAME_SOUND::CLICK);
	m_isMuted = true;
	HandleMute();
}

void Game::StartupGameOver()
{
}

void Game::StartupVictory()
{
	PlaySound(GAME_SOUND::VICTORY);
}

void Game::HandleMute()
{
	m_isMuted = !m_isMuted;
	for (int soundPlaybackIDIndex = 0; soundPlaybackIDIndex < GAME_SOUND::NUM_SOUNDS; soundPlaybackIDIndex++) {
		SoundPlaybackID& sound = g_soundPlaybackIDs[soundPlaybackIDIndex];
		if (sound == -1) continue;
		if (m_isMuted) {
			g_masterVolume = 0.0f;
			g_theAudio->SetSoundPlaybackVolume(sound, 0.0f);
		}
		else {
			g_masterVolume = 1.0f;
			g_theAudio->SetSoundPlaybackVolume(sound, 1.0f);
		}
	}
}

void Game::ShutDown()
{
	for (int soundPlaybackIDIndex = 0; soundPlaybackIDIndex < GAME_SOUND::NUM_SOUNDS; soundPlaybackIDIndex++) {
		SoundPlaybackID& sound = g_soundPlaybackIDs[soundPlaybackIDIndex];
		if (sound != -1) {
			g_theAudio->StopSound(sound);
		}
	}

	m_useTextAnimation = false;
	m_currentText = "";
	m_transitionTextColor = false;
	m_timeTextAnimation = 0.0f;
	m_isPaused = false;
	m_isMuted = false;
	g_drawDebugDistanceField = false;
	g_drawDebugHeatMapEntity = false;
}

void Game::LoadAssets()
{
	if (m_loadedAssets) return;

	g_squirrelFont = g_theRenderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");


	LoadSoundFiles();
	LoadTextures();

	XMLDoc tileXMLDoc;
	XMLError loadStatus = tileXMLDoc.LoadFile("Data/Definitions/TileDefinitions.xml");

	GUARANTEE_OR_DIE(loadStatus == XMLError::XML_SUCCESS, Stringf("TileDefinitions.xml COULD NOT BE LOADED"));
	
	g_tileSpriteSheet = new SpriteSheet(*g_textures[GAME_TEXTURE::TERRAIN], IntVec2(8, 8));
	g_explosionSpriteSheet = new SpriteSheet(*g_textures[GAME_TEXTURE::EXPLOSION], IntVec2(5, 5));

	float smallExplosionDuration = g_gameConfigBlackboard.GetValue("EXPLOSION_SMALL_DURATION", 0.3f);
	float mediumExplosionDuration = g_gameConfigBlackboard.GetValue("EXPLOSION_MEDIUM_DURATION", 1.0f);
	float largeExplosionDuration = g_gameConfigBlackboard.GetValue("EXPLOSION_LARGE_DURATION", 2.0f);

	g_explosionAnimDefinitions[(int)ExplosionScale::SMALL] = new SpriteAnimDefinition(*g_explosionSpriteSheet, 0, 24, smallExplosionDuration, SpriteAnimPlaybackType::ONCE);
	g_explosionAnimDefinitions[(int)ExplosionScale::MEDIUM] = new SpriteAnimDefinition(*g_explosionSpriteSheet, 0, 24, mediumExplosionDuration, SpriteAnimPlaybackType::ONCE);
	g_explosionAnimDefinitions[(int)ExplosionScale::LARGE] = new SpriteAnimDefinition(*g_explosionSpriteSheet, 0, 24, largeExplosionDuration, SpriteAnimPlaybackType::ONCE);

	TileDefinition::CreateTileDefinitions(*tileXMLDoc.RootElement());

	m_loadedAssets = true;
}

void Game::LoadTextures()
{
	g_textures[GAME_TEXTURE::ATTRACT_SCREEN] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/AttractScreen.png");
	g_textures[GAME_TEXTURE::VICTORY_SCREEN] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/VictoryScreen.jpg");
	g_textures[GAME_TEXTURE::GAME_OVER_SCREEN] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/YouDiedScreen.png");

	g_textures[GAME_TEXTURE::ARIES] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyAries.png");
	g_textures[GAME_TEXTURE::ENEMY_BOLT] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyBolt.png");
	g_textures[GAME_TEXTURE::ENEMY_BULLET] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyBullet.png");
	g_textures[GAME_TEXTURE::ENEMY_CANNON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyCannon.png");
	g_textures[GAME_TEXTURE::ENEMY_GATLING] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyGatling.png");
	g_textures[GAME_TEXTURE::ENEMY_SHELL] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyShell.png");
	g_textures[GAME_TEXTURE::ENEMY_TANK_0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank0.png");
	g_textures[GAME_TEXTURE::ENEMY_TANK_1] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank1.png");
	g_textures[GAME_TEXTURE::ENEMY_TANK_2] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank2.png");
	g_textures[GAME_TEXTURE::ENEMY_TANK_3] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank3.png");
	g_textures[GAME_TEXTURE::ENEMY_TANK_4] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTank4.png");
	g_textures[GAME_TEXTURE::ENEMY_TURRET_BASE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTurretBase.png");

	g_textures[GAME_TEXTURE::FRIENDLY_BOLT] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyBolt.png");
	g_textures[GAME_TEXTURE::FRIENDLY_BULLET] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyBullet.png");
	g_textures[GAME_TEXTURE::FRIENDLY_CANNON] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyCannon.png");
	g_textures[GAME_TEXTURE::FRIENDLY_GATLING] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyGatling.png");
	g_textures[GAME_TEXTURE::FRIENDLY_SHELL] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyShell.png");
	g_textures[GAME_TEXTURE::FRIENDLY_TANK_0] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyTank0.png");
	g_textures[GAME_TEXTURE::FRIENDLY_TANK_1] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyTank1.png");
	g_textures[GAME_TEXTURE::FRIENDLY_TANK_2] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyTank2.png");
	g_textures[GAME_TEXTURE::FRIENDLY_TANK_3] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyTank3.png");
	g_textures[GAME_TEXTURE::FRIENDLY_TANK_4] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyTank4.png");
	g_textures[GAME_TEXTURE::FRIENDLY_TURRET_BASE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/FriendlyTurretBase.png");
	g_textures[GAME_TEXTURE::PLAYER_TANK_BASE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankBase.png");
	g_textures[GAME_TEXTURE::PLAYER_TANK_TOP] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankTop.png");
	g_textures[GAME_TEXTURE::WALLRUBBLE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Rubble.png");
	g_textures[GAME_TEXTURE::ENEMYRUBBLE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/EnemyTankRubble.png");

	g_textures[GAME_TEXTURE::TERRAIN] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Terrain_8x8.png");

	g_textures[GAME_TEXTURE::EXPLOSION] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Explosion_5x5.png");


	for (int textureIndex = 0; textureIndex < GAME_TEXTURE::NUM_TEXTURES; textureIndex++) {
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

	// Royalty free version of Clair De Lune found here:
	// https://soundcloud.com/pianomusicgirl/claire-de-lune
	g_sounds[GAME_SOUND::BULLET_BOUNCE] = g_theAudio->CreateOrGetSound("Data/Audio/BulletBounce.wav");
	g_sounds[GAME_SOUND::BULLET_RICOCHET] = g_theAudio->CreateOrGetSound("Data/Audio/BulletRicochet.wav");
	g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->CreateOrGetSound("Data/Audio/ClaireDeLuneRoyaltyFreeFirstMinute.wav");
	g_sounds[GAME_SOUND::ENEMY_HIT] = g_theAudio->CreateOrGetSound("Data/Audio/EnemyHit.wav");
	g_sounds[GAME_SOUND::ENEMY_SHOOTING] = g_theAudio->CreateOrGetSound("Data/Audio/EnemyShoot.wav");
	g_sounds[GAME_SOUND::ENEMY_DIED] = g_theAudio->CreateOrGetSound("Data/Audio/EnemyDied.wav");
	g_sounds[GAME_SOUND::EXIT_MAP] = g_theAudio->CreateOrGetSound("Data/Audio/ExitMap.wav");
	g_sounds[GAME_SOUND::GAME_OVER] = g_theAudio->CreateOrGetSound("Data/Audio/GameOver.mp3");
	g_sounds[GAME_SOUND::LIBRA_THEME] = g_theAudio->CreateOrGetSound("Data/Audio/LibraTheme.aif");
	g_sounds[GAME_SOUND::PLAYER_HIT] = g_theAudio->CreateOrGetSound("Data/Audio/PlayerHit.wav");
	g_sounds[GAME_SOUND::PLAYER_SHOOTING] = g_theAudio->CreateOrGetSound("Data/Audio/PlayerShootNormal.ogg");
	g_sounds[GAME_SOUND::VICTORY] = g_theAudio->CreateOrGetSound("Data/Audio/Victory.mp3");
	g_sounds[GAME_SOUND::PAUSE] = g_theAudio->CreateOrGetSound("Data/Audio/Pause.mp3");
	g_sounds[GAME_SOUND::UNPAUSE] = g_theAudio->CreateOrGetSound("Data/Audio/Unpause.mp3");
	g_sounds[GAME_SOUND::CLICK] = g_theAudio->CreateOrGetSound("Data/Audio/Click.mp3");
	g_sounds[GAME_SOUND::DISCOVERED] = g_theAudio->CreateOrGetSound("Data/Audio/Discovered.mp3");

	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		if (g_sounds[soundIndex] == -1) {
			ERROR_RECOVERABLE(Stringf("FORGOT TO LOAD SOUND %d", soundIndex));
		}
	}
}

void Game::Update(float deltaSeconds)
{
	if (g_theConsole->GetMode() == DevConsoleMode::SHOWN) {
		g_theInput->ConsumeAllKeysJustPressed();
	}

	float gameDeltaSeconds = static_cast<float>(m_clock.GetDeltaTime());
	UpdateCameras(gameDeltaSeconds);
	UpdateGameState(gameDeltaSeconds);
	UpdateDeveloperCheatCodes(deltaSeconds);


	switch (m_currentState)
	{
	case GameState::AttractScreen:
		UpdateAttractScreen(gameDeltaSeconds);
		break;
	case GameState::Play:
		UpdatePlay(gameDeltaSeconds);
		break;
	case GameState::GameOver:
		UpdateGameOver(gameDeltaSeconds);
		break;
	case GameState::Victory:
		UpdateVictory(gameDeltaSeconds);
		break;
	default:
		ERROR_AND_DIE("TWILIGHT ZONE! WHAT GAME STATE IS THIS?!");
		break;
	}
}


void Game::UpdateMapTransitionAnimation(float deltaSeconds)
{
	m_currentTimeinMapTransition += deltaSeconds;
	if (m_currentTimeinMapTransition > m_transitioAnimationDuration) {
		m_currentTimeinMapTransition = 0.0f;
		m_fadingToBlack = false;

	}

	if (!m_loadedNextWorld && m_currentTimeinMapTransition > m_transitioAnimationDuration * 0.5f) {
		m_theWorld->GoToNextMap();
		m_loadedNextWorld = true;
	}

}

void Game::UpdateGameState(float deltaSeconds)
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
		case GameState::GameOver:
			ShutDown();
			break;
		}

		m_currentState = m_nextState;
		Startup();


		m_timeInCurrentState = 0.0f;

	}
	m_timeInCurrentState += deltaSeconds;
}

void Game::UpdatePlay(float deltaSeconds)
{
	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosY));
	}

	UpdateInputPlay(deltaSeconds);

	if (!m_theWorld->IsPlayerAlive()) {
		m_nextState = GameState::GameOver;
	}

	if (m_fadingToBlack) {
		UpdateMapTransitionAnimation(deltaSeconds);
	}
	m_theWorld->Update(deltaSeconds);
}

void Game::UpdateInputPlay(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_clock.TogglePause();
		if (m_isPaused) {
			UnPause();
		}
		else {
			Pause();
		}

	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		if (!m_isPaused) {
			Pause();
			m_clock.Pause();
		}
		else {
			m_nextState = GameState::AttractScreen;
			PlaySound(EXIT_MAP);
		}
	}
}

void Game::UpdateAttractScreen(float deltaSeconds)
{
	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = "Get Ready!";
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}
	m_AttractScreenCamera.SetOrthoView(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));

	UpdateInputAttractScreen(deltaSeconds);
	m_timeAlive += deltaSeconds;

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosY));
	}
}

void Game::UpdateInputAttractScreen(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed('P');
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);

	if (exitAttractMode) {
		m_nextState = GameState::Play;
	}

	if (g_theInput->WasKeyJustPressed('M')) {
		HandleMute();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}
}

void Game::UpdateGameOver(float deltaSeconds)
{
	if (m_timeInCurrentState < 3.0f) {
		m_theWorld->Update(deltaSeconds);
	}
	else {
		m_theWorld->Update(0.0f);
	}

	UpdateInputGameOver(deltaSeconds);
}

void Game::UpdateInputGameOver(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);
	if (g_theInput->WasKeyJustPressed('N') || controller.WasButtonJustPressed(XboxButtonID::Start)) {
		m_theWorld->RespawnPlayer();
		m_playerRespawned = true;
		m_nextState = GameState::Play;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
		PlaySound(EXIT_MAP);
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

	bool exitToAttract = false;
	exitToAttract = exitToAttract || g_theInput->WasKeyJustPressed('P');
	exitToAttract = exitToAttract || g_theInput->WasKeyJustPressed(KEYCODE_ESC);
	exitToAttract = exitToAttract || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitToAttract = exitToAttract || controller.WasButtonJustPressed(XboxButtonID::Back);

	if (exitToAttract) {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed('T') || controller.IsButtonDown(XboxButtonID::RightShoulder)) {
		m_clock.SetTimeDilation(0.1);
		SlowDownSound();
	}

	if (g_theInput->WasKeyJustPressed('Y')) {
		m_clock.SetTimeDilation(4);
		SpeedUpSound();
	}

	if (g_theInput->WasKeyJustPressed('O') || controller.WasButtonJustPressed(XboxButtonID::Up)) {
		m_clock.StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('M')) {
		HandleMute();
	}


	if (g_theInput->WasKeyJustPressed(KEYCODE_F1) || controller.WasButtonJustPressed(XboxButtonID::LeftThumb)) {
		g_drawDebug = !g_drawDebug;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F5)) {
		g_drawDebugDistanceField = !g_drawDebugHeatMapEntity && !g_drawDebugDistanceField;
		g_drawDebugHeatMapEntity = !g_drawDebugHeatMapEntity && !g_drawDebugDistanceField;

	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2)) {
		g_ignoreDamage = !g_ignoreDamage;
	}


	if (g_theInput->WasKeyJustPressed(KEYCODE_F3)) {
		g_ignorePhysics = !g_ignorePhysics;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F4)) {
		g_debugCamera = !g_debugCamera;
	}



	if (g_theInput->WasKeyJustPressed(KEYCODE_F8) || controller.WasButtonJustPressed(XboxButtonID::RightThumb)) {
		ShutDown();
		Startup();
	}

	if (g_theInput->WasKeyJustReleased('T') || controller.WasButtonJustReleased(XboxButtonID::RightShoulder)) {
		m_clock.SetTimeDilation(1);
		RestoreSoundSpeed();
	}

	if (g_theInput->WasKeyJustReleased('Y')) {
		m_clock.SetTimeDilation(1);
		RestoreSoundSpeed();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
	}
}

void Game::UpdateCameras(float deltaSeconds)
{
	UpdateUICamera(deltaSeconds);
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
	g_squirrelFont->AddVertsForText2D(m_textAnimTriangles, Vec2(), m_textCellHeight, text, usedTextColor);


	float fullTextWidth = g_squirrelFont->GetTextWidth(m_textCellHeight, text);
	float textWidth = GetTextWidthForAnim(fullTextWidth);

	Vec2 iBasis = GetIBasisForTextAnim();
	Vec2 jBasis = GetJBasisForTextAnim();

	TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

	if (m_timeTextAnimation >= m_textAnimationTime) {
		m_useTextAnimation = false;
		m_timeTextAnimation = 0.0f;
	}

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
	if (timeAsFraction < m_textMovementPhasePercentage) {
		if (timeAsFraction < m_textMovementPhasePercentage * 0.5f) {
			textWidth = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhasePercentage * 0.5f, 0.0f, fullTextWidth);
		}
		else {
			return fullTextWidth;
		}
	}
	else if (timeAsFraction > m_textMovementPhasePercentage && timeAsFraction < 1 - m_textMovementPhasePercentage) {
		return fullTextWidth;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhasePercentage * 0.5f) {
			return fullTextWidth;
		}
		else {
			textWidth = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhasePercentage * 0.5f, 1.0f, fullTextWidth, 0.0f);
		}
	}
	return textWidth;

}

Vec2 const Game::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float iScale = 0.0f;
	if (timeAsFraction < m_textMovementPhasePercentage) {
		if (timeAsFraction < m_textMovementPhasePercentage * 0.5f) {
			iScale = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhasePercentage * 0.5f, 0.0f, 1.0f);
		}
		else {
			iScale = 1.0f;
		}
	}
	else if (timeAsFraction > m_textMovementPhasePercentage && timeAsFraction < 1 - m_textMovementPhasePercentage) {
		iScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhasePercentage * 0.5f) {
			iScale = 1.0f;
		}
		else {
			iScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhasePercentage * 0.5f, 1.0f, 1.0f, 0.0f);
		}
	}

	return Vec2(iScale, 0.0f);
}

Vec2 const Game::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float jScale = 0.0f;
	if (timeAsFraction < m_textMovementPhasePercentage) {
		if (timeAsFraction < m_textMovementPhasePercentage * 0.5f) {
			jScale = 0.05f;
		}
		else {
			jScale = RangeMapClamped(timeAsFraction, m_textMovementPhasePercentage * 0.5f, m_textMovementPhasePercentage, 0.05f, 1.0f);

		}
	}
	else if (timeAsFraction > m_textMovementPhasePercentage && timeAsFraction < 1 - m_textMovementPhasePercentage) {
		jScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhasePercentage * 0.5f) {
			jScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhasePercentage, 1.0f - m_textMovementPhasePercentage * 0.5f, 1.0f, 0.05f);
		}
		else {
			jScale = 0.05f;
		}
	}

	return Vec2(0.0f, jScale);
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
	case GameState::GameOver:
		RenderGameOver();
		break;
	case GameState::Victory:
		RenderVictory();
		break;
	default:
		ERROR_AND_DIE("TWILIGHT ZONE IN RENDER! WHAT GAME STATE IS THIS?!");
		break;
	}

	RenderUI();
}

void Game::Pause()
{
	m_isPaused = true;
	g_theAudio->SetSoundPlaybackSpeed(g_soundPlaybackIDs[GAME_SOUND::LIBRA_THEME], 0.0f);
	PlaySound(GAME_SOUND::PAUSE);
}

void Game::UnPause()
{
	m_isPaused = false;
	g_theAudio->SetSoundPlaybackSpeed(g_soundPlaybackIDs[GAME_SOUND::LIBRA_THEME], 1.0f);
	PlaySound(GAME_SOUND::UNPAUSE);
}

void Game::WinGame()
{
	m_nextState = GameState::Victory;
}

void Game::FadeToBlack()
{
	m_fadingToBlack = true;
	m_currentTimeinMapTransition = 0.0f;
	m_loadedNextWorld = false;
}


void Game::RenderPlay() const
{
	m_theWorld->Render();
}


void Game::RenderAttractScreen() const
{
	g_theRenderer->BeginCamera(m_AttractScreenCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));

	AABB2 attractScreenAABB2;
	attractScreenAABB2.SetCenter(Vec2(m_UICenterX, m_UICenterY));
	attractScreenAABB2.SetDimensions(Vec2(m_UISizeX, m_UISizeY));

	std::vector<Vertex_PCU> attractScreenVertexes;
	AddVertsForAABB2D(attractScreenVertexes, attractScreenAABB2, Rgba8::WHITE);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(g_textures[GAME_TEXTURE::ATTRACT_SCREEN]);
	g_theRenderer->DrawVertexArray(attractScreenVertexes);

	if (m_useTextAnimation) {
		RenderTextAnimation();
	}

	//SpriteSheet* testSpritesheet = new SpriteSheet(*m_testTexture, IntVec2(8, 8));
	//SpriteSheet* testSpritesheet = new SpriteSheet(*m_testTexture, IntVec2(8, 8));
	//const SpriteDefinition& testSpriteDef = testSpritesheet->GetSpriteDef(56);

	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	std::vector<Vertex_PCU> testTextVerts;
	AABB2 testTextureAABB2(740.0f, 150.0f, 1040.0f, 450.f);
	//AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8::WHITE, testSpriteDef.GetUVs());
	AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8::WHITE);
	g_theRenderer->BindTexture(testTexture);
	//g_theRenderer->BindTexture(&testSpriteDef.GetTexture());
	g_theRenderer->DrawVertexArray((int)testTextVerts.size(), testTextVerts.data());
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->EndCamera(m_AttractScreenCamera);
}

void Game::RenderGameOver() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	m_theWorld->Render();
}

void Game::RenderVictory() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BeginCamera(m_UICamera);
	AABB2 victoryScreenAABB2;
	victoryScreenAABB2.SetCenter(Vec2(m_UICenterX, m_UICenterY));
	victoryScreenAABB2.SetDimensions(Vec2(m_UISizeX, m_UISizeY));

	std::vector<Vertex_PCU> victoryScreenVertexes;
	AddVertsForAABB2D(victoryScreenVertexes, victoryScreenAABB2, Rgba8::WHITE);
	g_theRenderer->BindTexture(g_textures[GAME_TEXTURE::VICTORY_SCREEN]);
	g_theRenderer->DrawVertexArray(victoryScreenVertexes);

	g_theRenderer->EndCamera(m_UICamera);
}


void Game::RenderUI() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BeginCamera(m_UICamera);

	RenderUIGamePaused();
	RenderTextAnimation();
	RenderGameOverUI();
	RenderFadeToBlack();
	RenderEntityHeatMapDebugHelpText();
	RenderMutedUI();

	RenderDevConsole();

	g_theRenderer->EndCamera(m_UICamera);
}

void Game::RenderUIGamePaused() const
{
	if (m_isPaused) {
		AABB2 pauseTranslucentSquare;
		pauseTranslucentSquare.SetCenter(Vec2(m_UICenterX, m_UICenterY));
		pauseTranslucentSquare.SetDimensions(m_UISizeX, m_UISizeY);

		std::vector<Vertex_PCU> vertsForPause;

		AddVertsForAABB2D(vertsForPause, pauseTranslucentSquare, Rgba8(0, 0, 0, 100));

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)vertsForPause.size(), vertsForPause.data());
	}
}

void Game::RenderGameOverUI() const
{
	if (m_currentState == GameState::GameOver && m_timeInCurrentState >= 3.0f) {

		AABB2 gameOverAABB2;
		gameOverAABB2.SetCenter(Vec2(m_UICenterX, m_UICenterY));
		gameOverAABB2.SetDimensions(Vec2(m_UISizeX, m_UISizeY));

		std::vector<Vertex_PCU> gameOverVerts;
		AddVertsForAABB2D(gameOverVerts, gameOverAABB2, Rgba8::WHITE);
		g_theRenderer->BindTexture(g_textures[GAME_TEXTURE::GAME_OVER_SCREEN]);
		g_theRenderer->DrawVertexArray(gameOverVerts);
	}
}

void Game::RenderFadeToBlack() const
{
	if (m_fadingToBlack && m_currentState == GameState::Play) {

		AABB2 fadeVerts;
		fadeVerts.SetCenter(Vec2(m_UICenterX, m_UICenterY));
		fadeVerts.SetDimensions(Vec2(m_UISizeX, m_UISizeY));

		std::vector<Vertex_PCU> fadeToBlackVerts;
		Rgba8 color = Rgba8::BLACK;

		float alphaValue = 0.0f;
		if (m_currentTimeinMapTransition <= m_transitioAnimationDuration * 0.5f) {
			alphaValue = RangeMapClamped(m_currentTimeinMapTransition, 0.0f, m_transitioAnimationDuration * 0.5f, 0.0f, 255.0f);
		}
		else {
			alphaValue = RangeMapClamped(m_currentTimeinMapTransition, m_transitioAnimationDuration * 0.5f, m_transitioAnimationDuration, 255.0f, 0.0f);
		}

		color.a = static_cast<unsigned char>(alphaValue);

		AddVertsForAABB2D(fadeToBlackVerts, fadeVerts, color);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(fadeToBlackVerts);
	}
}

void Game::RenderEntityHeatMapDebugHelpText() const
{
	if (g_drawDebugHeatMapEntity) {

		std::vector<Vertex_PCU> textVerts;
		float textYPos = m_UISizeY * 0.98f;
		float textXPos = m_UISizeX * 0.1f;

		g_squirrelFont->AddVertsForText2D(textVerts, Vec2(textXPos, textYPos), 15, "Press F6 to cycle through current list Entities' Heat maps", Rgba8::BLUE);
		g_squirrelFont->AddVertsForText2D(textVerts, Vec2(textXPos, textYPos * 0.95f), 15, "Press F7 to cycle through the list of Entities", Rgba8::BLUE);
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(textVerts);
	}
}

void Game::RenderMutedUI() const
{
	if (g_masterVolume == 0.0f) {
		std::vector<Vertex_PCU> textVerts;
		float textYPos = m_UISizeY * 0.98f;
		float textXPos = m_UISizeX * 0.8f;

		g_squirrelFont->AddVertsForText2D(textVerts, Vec2(textXPos, textYPos), 15, "Muted", Rgba8::BLUE);
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(textVerts);
	}
}

void Game::RenderDevConsole() const
{
	if (g_theConsole->GetMode() != DevConsoleMode::SHOWN) return;

	AABB2 UIBounds(Vec2::ZERO, Vec2(m_UISizeX, m_UISizeY));

	g_theConsole->Render(UIBounds);
}

void Game::RenderTextAnimation() const
{
	if (m_useTextAnimation) {
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		if (m_textAnimTriangles.size() > 0) {
			g_theRenderer->DrawVertexArray(int(m_textAnimTriangles.size()), m_textAnimTriangles.data());
		}
	}
}

void Game::SpeedUpSound()
{
	for (int soundPlaybackIDIndex = 0; soundPlaybackIDIndex <= GAME_SOUND::NUM_SOUNDS; soundPlaybackIDIndex++) {
		if (g_soundPlaybackIDs[soundPlaybackIDIndex] != -1) {
			g_theAudio->SetSoundPlaybackSpeed(g_soundPlaybackIDs[soundPlaybackIDIndex], 4.0f);
		}
	}
}

void Game::SlowDownSound()
{
	for (int soundPlaybackIDIndex = 0; soundPlaybackIDIndex <= GAME_SOUND::NUM_SOUNDS; soundPlaybackIDIndex++) {
		if (g_soundPlaybackIDs[soundPlaybackIDIndex] != -1) {
			g_theAudio->SetSoundPlaybackSpeed(g_soundPlaybackIDs[soundPlaybackIDIndex], 0.1f);
		}
	}
}

void Game::RestoreSoundSpeed()
{
	for (int soundPlaybackIDIndex = 0; soundPlaybackIDIndex <= GAME_SOUND::NUM_SOUNDS; soundPlaybackIDIndex++) {
		if (g_soundPlaybackIDs[soundPlaybackIDIndex] != -1) {
			g_theAudio->SetSoundPlaybackSpeed(g_soundPlaybackIDs[soundPlaybackIDIndex], 1.0f);
		}
	}
}



Rgba8 const Game::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}
