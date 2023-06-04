#include "Engine/Math/AABB2.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"

extern bool g_drawDebug;
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
	LoadSoundFiles();
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

}

void Game::StartupAttractScreen()
{
	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame2D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;

	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}
}

void Game::StartupPlay()
{
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
	g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->CreateOrGetSound("Data/Audio/ClaireDeLuneRoyaltyFreeFirstMinute.wav");
	g_sounds[GAME_SOUND::DOOR_KNOCKING_SOUND] = g_theAudio->CreateOrGetSound("Data/Audio/KnockingDoor.wav");
}

void Game::Update(float deltaSeconds)
{
	if (g_theConsole->GetMode() == DevConsoleMode::SHOWN) {
		g_theInput->ConsumeAllKeysJustPressed();
	}

	UNUSED(deltaSeconds);
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
}


void Game::UpdatePlay(float deltaSeconds)
{
	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}

	UpdateInputPlay(deltaSeconds);
	UpdateDeveloperCheatCodes(deltaSeconds);

	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(m_WORLDSizeX, m_WORLDSizeY));
}

void Game::UpdateInputPlay(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::UpdateAttractScreen(float deltaSeconds)
{
	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame2D");
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

	if (g_theInput->WasKeyJustPressed('T') || controller.IsButtonDown(XboxButtonID::RightShoulder)) {
		sysClock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('O') || controller.WasButtonJustPressed(XboxButtonID::Up)) {
		Clock::GetSystemClock().StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('P') || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		sysClock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1) || controller.WasButtonJustPressed(XboxButtonID::LeftThumb)) {
		g_drawDebug = !g_drawDebug;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6) || controller.WasButtonJustPressed(XboxButtonID::Down)) {
		KillAllEnemies();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8) || controller.WasButtonJustPressed(XboxButtonID::RightThumb)) {
		ShutDown();
		Startup();
	}

	if (g_theInput->WasKeyJustReleased('T') || controller.WasButtonJustReleased(XboxButtonID::RightShoulder)) {
		sysClock.SetTimeDilation(1);
	}

	if (g_theInput->WasKeyJustPressed('K')) {
		g_soundPlaybackIDs[GAME_SOUND::DOOR_KNOCKING_SOUND] = g_theAudio->StartSound(g_sounds[DOOR_KNOCKING_SOUND]);
	}

	


	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
	}
}

void Game::UpdateCameras(float deltaSeconds)
{
	UpdateWorldCamera(deltaSeconds);

	UpdateUICamera(deltaSeconds);

}

void Game::UpdateWorldCamera(float deltaSeconds)
{
	m_worldCamera.SetOrthoView(Vec2(), Vec2(m_WORLDSizeX, m_WORLDSizeY));
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

	RenderUI();

}



void Game::RenderPlay() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::GRAY);

	Vertex_PCU* nothing = nullptr;
	g_theRenderer->DrawVertexArray(0, nothing); // If nothing is drawn, old buffer from attract screen is still up, so it flickers on game screen
	g_theRenderer->EndCamera(m_worldCamera);
}

void Game::RenderAttractScreen() const
{
	g_theRenderer->BeginCamera(m_AttractScreenCamera);
 	g_theRenderer->ClearScreen(Rgba8::BLACK);

	if (m_useTextAnimation) {
		RenderTextAnimation();
	}

	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	std::vector<Vertex_PCU> testTextVerts;
	AABB2 testTextureAABB2(740.0f,150.0f, 1040.0f, 450.f);
	AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8());
	g_theRenderer->BindTexture(testTexture);
	g_theRenderer->DrawVertexArray((int) testTextVerts.size(),testTextVerts.data());
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
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}
