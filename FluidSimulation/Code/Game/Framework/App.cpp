#include "Game/Framework/App.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Clock.hpp"

#include <thread>

RandomNumberGenerator rng;
InputSystem* g_theInput = nullptr;
Window* g_theWindow = nullptr;
Renderer* g_theRenderer = nullptr;
AudioSystem* g_theAudio = nullptr;
Game* g_theGame = nullptr;

bool App::s_isQuitting = false;

App::App() {

}

App::~App() {
}

void App::Startup()
{
	tinyxml2::XMLDocument gameConfigFile;
	XMLError loadConfigStatus = gameConfigFile.LoadFile("Data/GameConfig.xml");
	GUARANTEE_OR_DIE(loadConfigStatus == XMLError::XML_SUCCESS, "GAME CONFIG FILE DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* gameConfig = gameConfigFile.FirstChildElement("GameConfig");

	g_gameConfigBlackboard.PopulateFromXmlElementAttributes(*gameConfig);

	float UI_SIZE_X = g_gameConfigBlackboard.GetValue("UI_SIZE_X", 1600.0f);
	float UI_CENTER_X = UI_SIZE_X * 0.5f;
	float UI_SIZE_Y = g_gameConfigBlackboard.GetValue("UI_SIZE_Y", 800.0f);
	float UI_CENTER_Y = UI_SIZE_Y * 0.5f;

	float TEXT_CELL_HEIGHT = UI_SIZE_Y * 0.02f;

	float WORLD_SIZE_X = g_gameConfigBlackboard.GetValue("WORLD_SIZE_X", 200.0f);
	float WORLD_SIZE_Y = g_gameConfigBlackboard.GetValue("WORLD_SIZE_Y", 100.0f);

	float WORLD_CENTER_X = WORLD_SIZE_X * 0.5f;
	float WORLD_CENTER_Y = WORLD_SIZE_Y * 0.5f;
	float TEXT_CELL_HEIGHT_ATTRACT_SCREEN = UI_SIZE_Y * 0.1f;

	g_gameConfigBlackboard.SetValue("UI_CENTER_X", std::to_string(UI_CENTER_X));
	g_gameConfigBlackboard.SetValue("UI_CENTER_Y", std::to_string(UI_CENTER_Y));
	g_gameConfigBlackboard.SetValue("WORLD_CENTER_Y", std::to_string(WORLD_CENTER_X));
	g_gameConfigBlackboard.SetValue("WORLD_CENTER_Y", std::to_string(WORLD_CENTER_Y));
	g_gameConfigBlackboard.SetValue("TEXT_CELL_HEIGHT", std::to_string(TEXT_CELL_HEIGHT));
	g_gameConfigBlackboard.SetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", std::to_string(TEXT_CELL_HEIGHT_ATTRACT_SCREEN));

	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);

	NetworkSystemConfig networkSysConfig;
	g_theNetwork = new NetworkSystem(networkSysConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_font = "SquirrelFixedFont";
	devConsoleConfig.m_maxLinesShown = 40.5f;
	g_theConsole = new DevConsole(devConsoleConfig);

	InputSystemConfig inputSystemConfig;
	g_theInput = new InputSystem(inputSystemConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_clientAspect = 2.0f;
	windowConfig.m_windowTitle = windowConfig.m_windowTitle = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;

	g_theRenderer = new Renderer(rendererConfig);
	g_theConsole->SetRenderer(g_theRenderer);

	AudioSystemConfig audioSystemConfig;
	g_theAudio = new AudioSystem(audioSystemConfig);

	g_theGame = new Game(this);

	g_theEventSystem->Startup();
	g_theNetwork->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theConsole->Startup();
	g_theAudio->Startup();
	g_theGame->Startup();

	g_theEventSystem->SubscribeEventCallbackFunction("QuitRequested", QuitRequestedEvent);
}


void App::Update()
{
	if (g_theGame) {
		g_theGame->Update();
	}
}


//-----------------------------------------------------------------------------------------------
void App::BeginFrame()
{
	Clock::SystemBeginFrame();
	g_theEventSystem->BeginFrame();
	g_theNetwork->BeginFrame();
	g_theConsole->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theAudio->BeginFrame();
}


void App::Render() const
{
	g_theGame->Render();
}


void App::EndFrame()
{
	g_theEventSystem->EndFrame();
	g_theNetwork->EndFrame();
	g_theConsole->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theAudio->EndFrame();

	std::this_thread::yield();
}

//-----------------------------------------------------------------------------------------------
// One "frame" of the game.  Generally: Input, Update, Render.  We call this 60+ times per second.
void App::RunFrame()
{
	while (!s_isQuitting) {
		App::BeginFrame();
		App::Update();
		App::Render();
		App::EndFrame();
	}
}

void App::HandleQuitRequested()
{
	s_isQuitting = true;
}

bool App::QuitRequestedEvent(EventArgs& args)
{
	UNUSED(args);
	s_isQuitting = true;
	return true;
}

void App::Shutdown()
{
	this->s_isQuitting = true;

	g_theGame->ShutDown();
	delete g_theGame;
	g_theGame = nullptr;

	g_theAudio->Shutdown();
	delete g_theAudio;
	g_theAudio = nullptr;

	g_theConsole->Shutdown();
	delete g_theConsole;
	g_theConsole = nullptr;

	g_theRenderer->Shutdown();
	delete g_theRenderer;
	g_theRenderer = nullptr;

	g_theWindow->Shutdown();
	delete g_theWindow;
	g_theWindow = nullptr;

	g_theInput->ShutDown();
	delete g_theInput;
	g_theInput = nullptr;

	g_theNetwork->Shutdown();
	delete g_theNetwork;
	g_theNetwork = nullptr;

	g_theEventSystem->Shutdown();
	delete g_theEventSystem;
	g_theEventSystem = nullptr;

}

