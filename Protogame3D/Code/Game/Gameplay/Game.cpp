#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Gameplay/AttractScreenMode.hpp"
#include "Game/Gameplay/Basic3DMode.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Prop.hpp"



extern bool g_drawDebug;
extern App* g_theApp;
extern AudioSystem* g_theAudio;

class DestructorUnsubTest :public EventRecipient {
public:
	bool SomeSubFunction(EventArgs&) { return true; }

};

NamedProperties npbTest;

Game::Game(App* appPointer) :
	m_theApp(appPointer)
{

	SubscribeEventCallbackFunction("SuperTest", this, &Game::SuperTest);
	SubscribeEventCallbackFunction("SuperTest", this, &Game::AnotherSuperTest);
	SubscribeEventCallbackFunction("Controls", this, &Game::SuperTest);
	UnsubscribeAllEventCallbackFunctions(this);

	DestructorUnsubTest unsubTest;
	SubscribeEventCallbackFunction("SuperTest", &unsubTest, &DestructorUnsubTest::SomeSubFunction);

	NamedProperties recursiveTest;
	recursiveTest.SetValue("Test", 42.0f);
	npbTest.SetValue("Recursion", recursiveTest);

}

Game::~Game()
{

}

bool Game::SuperTest(EventArgs& testArgs)
{
	UNUSED(testArgs);
	return true;
}

bool Game::AnotherSuperTest(EventArgs& testArgs)
{
	UNUSED(testArgs);
	g_theConsole->ExecuteXmlCommandScriptFile("Data/XMLScriptTest.xml");

	return true;
}

void Game::Startup()
{

	LoadAssets();


	switch (m_currentState)
	{
	case GameState::AttractScreen:
		StartupAttractScreen();
		break;
	case GameState::Play:
		StartupPlay();
		break;
	case GameState::EngineLogo:
		StartupLogo();
		break;
	default:
		break;
	}
	DebugAddMessage("Hello", 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void Game::StartupLogo()
{
	if (m_showEngineLogo) {
		m_logoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Engine Logo.png");
	}
	else {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::StartupAttractScreen()
{
	m_currentGameMode = new AttractScreenMode(this, m_UISize);
	m_currentGameMode->Startup();
}

void Game::StartupPlay()
{
	m_currentGameMode = new Basic3DMode(this, m_UISize);
	m_currentGameMode->Startup();
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


void Game::UpdateLogo(float deltaSeconds)
{
	UpdateInputLogo(deltaSeconds);

	m_timeShowingLogo += deltaSeconds;

	if (m_timeShowingLogo > m_engineLogoLength) {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::UpdateInputLogo(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) || controller.WasButtonJustPressed(XboxButtonID::A)) {
		m_nextState = GameState::AttractScreen;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}

}

void Game::ShutDown()
{
	m_currentGameMode->Shutdown();
	delete m_currentGameMode;
	m_currentGameMode = nullptr;

}

void Game::LoadAssets()
{
	g_squirrelFont = g_theRenderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");

	if (m_loadedAssets) return;
	LoadTextures();
	LoadSoundFiles();

	m_loadedAssets = true;
}

void Game::LoadTextures()
{
	g_textures[(int)GAME_TEXTURE::TestUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
	g_textures[(int)GAME_TEXTURE::CompanionCube] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/CompanionCube.png");
	g_textures[(int)GAME_TEXTURE::TextureTest] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

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

	float gameDeltaSeconds = static_cast<float>(m_clock.GetDeltaTime());

	UpdateGameState();
	switch (m_currentState)
	{
	case GameState::EngineLogo:
		UpdateLogo(gameDeltaSeconds);
		break;
	default:
		if(m_currentGameMode) m_currentGameMode->Update(gameDeltaSeconds);
		break;
	}

}

void Game::Render()
{
	switch (m_currentState)
	{
	case GameState::EngineLogo:
		RenderLogo();
		break;
	default:
		if(m_currentGameMode) m_currentGameMode->Render();
		break;
	}
}



void Game::ExitToAttractScreen()
{
	m_nextState = GameState::AttractScreen;
}

void Game::EnterGameMode()
{
	m_nextState = (GameState)0;
}

void Game::HandleQuitRequested()
{
	m_theApp->HandleQuitRequested();
}

void Game::RenderLogo() const
{
	/*AABB2 screen = m_UICamera.GetCameraBounds();
	AABB2 logo = screen.GetBoxWithin(Vec2(0.27f, 0.25f), Vec2(0.68f, 0.75f));
	AABB2 background = screen.GetBoxWithin(Vec2(0.34f, 0.305f), Vec2(0.605f, 0.72f));

	std::vector<Vertex_PCU> logoVerts;
	std::vector<Vertex_PCU> whiteBackgroundVerts;

	float logoAlpha = 0.0f;
	double third = m_engineLogoLength / 3.0;
	if (m_timeShowingLogo < third) {
		logoAlpha = RangeMapClamped((float)m_timeShowingLogo, 0.0f, (float)third, 0.0f, 1.0f);
	}
	else if (m_timeShowingLogo >= 2 * third) {
		logoAlpha = RangeMapClamped((float)m_timeShowingLogo, (float)third * 2.0f, (float)m_engineLogoLength, 1.0f, 0.0f);
	}
	else {
		logoAlpha = 1.0f;
	}


	Rgba8 logoColor = Rgba8::WHITE;

	logoColor.a = DenormalizeByte(logoAlpha);

	AddVertsForAABB2D(logoVerts, logo, logoColor);
	AddVertsForAABB2D(whiteBackgroundVerts, background, logoColor);

	g_theRenderer->BeginCamera(m_UICamera); {

		g_theRenderer->ClearScreen(Rgba8::BLACK);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(whiteBackgroundVerts);
		g_theRenderer->BindTexture(m_logoTexture);
		g_theRenderer->DrawVertexArray(logoVerts);
	}
	g_theRenderer->EndCamera(m_UICamera);*/


}

