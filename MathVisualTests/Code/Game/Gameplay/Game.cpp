#include "Engine/Math/AABB2.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/CapsuleShape2D.hpp"
#include "Game/Gameplay/OBB2Shape2D.hpp"
#include "Game/Gameplay/AABB2Shape2D.hpp"
#include "Game/Gameplay/DiscShape2D.hpp"
#include "Game/Gameplay/GameMode.hpp"
#include "Game/Gameplay/AttractGameMode.hpp"
#include "Game/Gameplay/NearestPointGameMode.hpp"
#include "Game/Gameplay/RaycastVsDiscsGameMode.hpp"
#include "Game/Gameplay/RaycastVsBoxes2DMode.hpp"
#include "Game/Gameplay/RaycastVsConvexPoly2DMode.hpp"
#include "Game/Gameplay/RaycastVsLineSegments2DMode.hpp"
#include "Game/Gameplay/RaycastVsOBB2Mode.hpp"
#include "Game/Gameplay/3DShapesMode.hpp"
#include "Game/Gameplay/BilliardsMode.hpp"
#include "Game/Gameplay/PachinkoMachineMode.hpp"
#include "Game/Gameplay/SplinesMode.hpp"

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

	g_squirrelFont = g_theRenderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");
	Vec2 UISize = Vec2(m_UISizeX, m_UISizeY);

	switch (m_currentState)
	{
	case GameState::AttractScreen:
		m_currentGameMode = new AttractGameMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	case GameState::NearestPoint:
		m_currentGameMode = new NearestPointGameMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize, Vec2(m_WORLDSizeX, m_WORLDSizeY));
		break;
	case GameState::RaycastVsDiscs:
		m_currentGameMode = new RaycastVsDiscMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize, Vec2(m_WORLDSizeX, m_WORLDSizeY));
		break;
	case GameState::Shapes3D:
		m_currentGameMode = new Shapes3DMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	case GameState::Billiards:
		m_currentGameMode = new BilliardsMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	case GameState::RaycastVsBoxes2D:
		m_currentGameMode = new RaycastVsBoxes2DMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	case GameState::RaycastVsOBB2D:
		m_currentGameMode = new RaycastVsOBB2Mode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	case GameState::RaycastVsLineSegments2D:
		m_currentGameMode = new RaycastVsLineSegments2DMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	case GameState::PachinkoMachine:
		m_currentGameMode = new PachinkoMachineMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize, Vec2(m_WORLDSizeX, m_WORLDSizeY));
		break;
	case GameState::Splines:
		m_currentGameMode = new SplinesMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize, Vec2(m_WORLDSizeX, m_WORLDSizeY));
		break;
	case GameState::ConvexPoly2D:
		m_currentGameMode = new RaycastVsConvexPoly2DMode(this, Vec2(m_WORLDSizeX, m_WORLDSizeY), UISize);
		break;
	default:
		break;

	}

	m_currentGameMode->Startup();
}

void Game::ShutDown()
{
	m_currentGameMode->Shutdown();

	delete m_currentGameMode;
	m_currentGameMode = nullptr;
}


void Game::Update()
{
	float deltaSeconds = static_cast<float>(m_clock.GetDeltaTime());

	m_currentGameMode->Update(deltaSeconds);
	UpdateDeveloperCheatCodes(deltaSeconds);
	UpdateGameState();
}

void Game::UpdateGameState()
{
	if (m_currentState != m_nextState) {
		ShutDown();
		m_currentState = m_nextState;
		Startup();
	}
}


void Game::UpdateDeveloperCheatCodes(float deltaSeconds)
{

	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
		g_drawDebug = !g_drawDebug;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8) || controller.WasButtonJustPressed(XboxButtonID::RightThumb)) {
		m_currentGameMode->Shutdown();
		m_currentGameMode->Startup();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F6)) {
		m_nextState = GameState(((int)m_currentState - 1) % (int)GameState::NumGameStates);
		if ((int)m_nextState < 0) m_nextState = (GameState)((int)GameState::NumGameStates - 1);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7)) {
		m_nextState = GameState(((int)m_currentState + 1) % (int)GameState::NumGameStates);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) {
		g_theApp->HandleQuitRequested();
	}

	if (g_theInput->WasKeyJustPressed('P')) {
		m_clock.TogglePause();
	}

	if (g_theInput->IsKeyDown('T')) {
		m_clock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustReleased('T')) {
		m_clock.SetTimeDilation(1.0);
	}

	if (g_theInput->WasKeyJustPressed('O')) {
		m_clock.StepFrame();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
	}
}

void Game::Render() const
{
	m_currentGameMode->Render();
}

Rgba8 const Game::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}
	