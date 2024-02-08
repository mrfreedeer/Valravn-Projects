#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Framework/GameCommon.hpp"

enum class GameState {
	AttractScreen = -1,
	NearestPoint,
	RaycastVsDiscs,
	Billiards,
	RaycastVsBoxes2D,
	Shapes3D,
	RaycastVsOBB2D,
	RaycastVsLineSegments2D,
	PachinkoMachine,
	Splines,
	ConvexPoly2D,
	NumGameStates
};

class App;
class GameMode;

class Game {

public:
	Game(App* appPointer);
	~Game();
	
	void Startup();
	void ShutDown();

	void Update();
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render() const;

	void Pause() { m_isPaused = !m_isPaused; }
	bool IsPaused() { return m_isPaused; }

	Rgba8 const GetRandomColor() const;
	bool m_returnToAttactMode = false;

	float m_UISizeY = g_gameConfigBlackboard.GetValue("UI_SIZE_Y", 0.0f);
	float m_UISizeX = g_gameConfigBlackboard.GetValue("UI_SIZE_X", 0.0f);
	float m_UICenterX = g_gameConfigBlackboard.GetValue("UI_CENTER_X", 0.0f);
	float m_UICenterY = g_gameConfigBlackboard.GetValue("UI_CENTER_Y", 0.0f);

	Clock& GetClock() { return m_clock; }

private:
	void UpdateGameState();

private:
	App* m_theApp = nullptr;

	bool m_isPaused = false;
	bool m_isSlowMo = false;
	bool m_runSingleFrame = false;

	bool m_playingWinGameSound = false;
	bool m_playingLoseGameSound = false;


	GameState m_currentState = GameState::ConvexPoly2D;
	GameState m_nextState = GameState::ConvexPoly2D;
	GameMode* m_currentGameMode = nullptr;

	float m_WORLDSizeY = g_gameConfigBlackboard.GetValue("WORLD_SIZE_Y", 0.0f);
	float m_WORLDSizeX = g_gameConfigBlackboard.GetValue("WORLD_SIZE_X", 0.0f);
	float m_WORLDCenterX = g_gameConfigBlackboard.GetValue("WORLD_CENTER_X", 0.0f);
	float m_WORLDCenterY = g_gameConfigBlackboard.GetValue("WORLD_CENTER_Y", 0.0f);

	Clock m_clock;
};