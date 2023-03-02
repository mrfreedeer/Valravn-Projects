#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Gameplay/PlayerShip.hpp"

#include <thread>

RandomNumberGenerator rng;
InputSystem* g_theInput = nullptr;
Window* g_theWindow = nullptr;
Renderer* g_theRenderer = nullptr;
AudioSystem* g_theAudio = nullptr;
Game* g_theGame = nullptr;


bool g_drawDebug = false;

bool App::s_isQuitting = false;

App::App() {

}

App::~App() {
}

void App::Startup()
{
	EventSystemConfig eventSystemConfig;
	g_theEventSystem = new EventSystem(eventSystemConfig);

	InputSystemConfig inputSystemConfig;
	g_theInput = new InputSystem(inputSystemConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInput;
	windowConfig.m_clientAspect = 2.0f;
	windowConfig.m_windowTitle = "SD1 - A5: Starship Gold (Re-factored)";
	g_theWindow = new Window(windowConfig);

	RendererConfig rendererConfig;
	rendererConfig.m_window = g_theWindow;

	g_theRenderer = new Renderer(rendererConfig);

	DevConsoleConfig devConsoleConfig;
	devConsoleConfig.m_renderer = g_theRenderer;
	devConsoleConfig.m_font = "SquirrelFixedFont";
	devConsoleConfig.m_maxLinesShown = 20.5f;
	g_theConsole = new DevConsole(devConsoleConfig);
	AudioSystemConfig audioSystemConfig;
	g_theAudio = new AudioSystem(audioSystemConfig);

	g_theGame = new Game(this);

	g_theEventSystem->Startup();
	g_theInput->Startup();
	g_theWindow->Startup();
	g_theRenderer->Startup();
	g_theConsole->Startup();
	g_theAudio->Startup();
	g_theGame->Startup();

	g_theEventSystem->SubscribeEventCallbackFunction("QuitRequested", QuitRequestedEvent);
	g_theConsole->AddLine(DevConsole::INFO_MAJOR_COLOR, "DEV CONSOLE WORKS!!!!");
}


void App::Update()
{
	float deltaTime = static_cast<float>(Clock::GetSystemClock().GetDeltaTime());

	if (g_theGame) {
		g_theGame->Update(deltaTime);
	}
}


//-----------------------------------------------------------------------------------------------
void App::BeginFrame()
{
	Clock::SystemBeginFrame();
	g_theEventSystem->BeginFrame();
	g_theInput->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theConsole->BeginFrame();
	g_theAudio->BeginFrame();
}


//-----------------------------------------------------------------------------------------------
// Some simple OpenGL example drawing code.
// This is the graphical equivalent of printing "Hello, world."
// #SD1ToDo: Move *ALL* OpenGL code to RenderContext.cpp (only)
//
void App::Render() const
{
	g_theGame->Render();
}


void App::EndFrame()
{
	g_theEventSystem->EndFrame();
	g_theInput->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	g_theConsole->EndFrame();
	g_theAudio->EndFrame();
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
}

bool App::QuitRequestedEvent(EventArgs& args)
{
	UNUSED(args);
	s_isQuitting = true;
	return true;
}
