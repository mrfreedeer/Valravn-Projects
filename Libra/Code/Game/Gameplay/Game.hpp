#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"

enum class GameState {
	AttractScreen,
	Play,
	GameOver,
	Victory
};

class App;
class World;

class Game {

public:
	Game(App* appPointer);
	~Game();

	void Startup();
	void ShutDown();

	void LoadAssets();
	void LoadTextures();
	void LoadSoundFiles();

	void Update(float deltaSeconds);
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render() const;

	void Pause();
	void UnPause();
	void WinGame();
	
	void FadeToBlack();


	bool m_useTextAnimation = true;

	Rgba8 const GetRandomColor() const;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	
	std::string m_currentText = "";


private:
	void StartupAttractScreen();
	void StartupPlay();
	void StartupGameOver();
	void StartupVictory();

	void HandleMute();

	void UpdateMapTransitionAnimation(float deltaSeconds);
	void UpdateGameState(float deltaSeconds);

	void UpdatePlay(float deltaSeconds);
	void UpdateInputPlay(float deltaSeconds);

	void UpdateAttractScreen(float deltaSeconds);
	void UpdateInputAttractScreen(float deltaSeconds);

	void UpdateGameOver(float deltaSeconds);
	void UpdateInputGameOver(float deltaSeconds);

	void UpdateVictory(float deltaSeconds);
	void UpdateInputVictory(float deltaSeconds);

	void UpdateCameras(float deltaSeconds);
	void UpdateUICamera(float deltaSeconds);

	void UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation);
	void TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation);
	float GetTextWidthForAnim(float fullTextWidth);
	Vec2 const GetIBasisForTextAnim();
	Vec2 const GetJBasisForTextAnim();

	void RenderPlay() const;
	void RenderAttractScreen() const;
	void RenderGameOver() const;
	void RenderVictory() const;

	void RenderUI() const;
	void RenderUIGamePaused() const;
	void RenderGameOverUI() const;
	void RenderFadeToBlack() const;
	void RenderEntityHeatMapDebugHelpText() const;
	void RenderMutedUI() const;
	void RenderDevConsole() const;

	void RenderTextAnimation() const;

	void SpeedUpSound();
	void SlowDownSound();
	void RestoreSoundSpeed();

private:
	App* m_theApp = nullptr;
	World* m_theWorld = nullptr;

	Camera m_UICamera;

	bool m_isPaused = false;
	bool m_isMuted = false;
	bool m_fadingToBlack = false;
	bool m_loadedNextWorld = false;
	bool m_transitionTextColor = false;
	bool m_pausedTheme = false;
	bool m_loadedAssets = false;
	bool m_playerRespawned = false;

	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();

	GameState m_currentState = GameState::AttractScreen;
	GameState m_nextState = GameState::AttractScreen;

	Vec2 m_attractModePos = Vec2::ZERO;
	Camera m_AttractScreenCamera;

	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;

	float m_timeTextAnimation = 0.0f;
	float m_timeAlive = 0.0f;
	float m_timeInCurrentState = 0.0f;
	float m_currentTimeinMapTransition = 0.0f;
	float m_transitioAnimationDuration = g_gameConfigBlackboard.GetValue("FADE_TO_BLACK_TOTAL_TIME", 2.0f);

	float m_UISizeY = g_gameConfigBlackboard.GetValue("UI_SIZE_Y", 0.0f);
	float m_UISizeX = g_gameConfigBlackboard.GetValue("UI_SIZE_X", 0.0f);
	float m_UICenterX = g_gameConfigBlackboard.GetValue("UI_CENTER_X", 0.0f);
	float m_UICenterY = g_gameConfigBlackboard.GetValue("UI_CENTER_Y", 0.0f);

	float m_textAnimationPosY = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.0f);
	float m_textAnimationTime = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 1.0f);
	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 2.0f);
	float m_textMovementPhasePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 20.0f);

	Clock m_clock;
};