#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"

enum class GameState {
	AttractScreen,
	Play
};

class App;

class Game {

public:
	Game(App* appPointer);
	~Game();
	
	void Startup();
	void ShutDown();
	void LoadSoundFiles();

	void Update(float deltaSeconds);
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render() const;

	void KillAllEnemies();

	void ShakeScreenCollision();
	void ShakeScreenDeath();
	void ShakeScreenPlayerDeath();
	void StopScreenShake();

	Rgba8 const GetRandomColor() const;

	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	std::string m_currentText = "";

private:
	App* m_theApp = nullptr;

	void StartupAttractScreen();
	void StartupPlay();

	void UpdateGameState();

	void UpdatePlay(float deltaSeconds);
	void UpdateInputPlay(float deltaSeconds);

	void UpdateAttractScreen(float deltaSeconds);
	void UpdateInputAttractScreen(float deltaSeconds);

	void UpdateCameras(float deltaSeconds);
	void UpdateUICamera(float deltaSeconds);
	void UpdateWorldCamera(float deltaSeconds);

	void UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation);
	void TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation);
	float GetTextWidthForAnim(float fullTextWidth);
	Vec2 const GetIBasisForTextAnim();
	Vec2 const GetJBasisForTextAnim();
	
	void RenderPlay() const;
	void RenderAttractScreen() const;
	void RenderUI() const;
	void RenderTextAnimation() const;

	Camera m_worldCamera;
	Camera m_UICamera;

	bool m_playingWinGameSound = false;
	bool m_playingLoseGameSound = false;
	
	bool m_screenShake = false;
	float m_screenShakeDuration = 0.0f;
	float m_screenShakeTranslation = 0.0f;
	float m_timeShakingScreen = 0.0f;

	float m_UISizeY = g_gameConfigBlackboard.GetValue("UI_SIZE_Y", 0.0f);
	float m_UISizeX = g_gameConfigBlackboard.GetValue("UI_SIZE_X", 0.0f);
	float m_UICenterX = g_gameConfigBlackboard.GetValue("UI_CENTER_X", 0.0f);
	float m_UICenterY = g_gameConfigBlackboard.GetValue("UI_CENTER_Y", 0.0f);

	float m_WORLDSizeY = g_gameConfigBlackboard.GetValue("WORLD_SIZE_Y", 0.0f);
	float m_WORLDSizeX = g_gameConfigBlackboard.GetValue("WORLD_SIZE_X", 0.0f);
	float m_WORLDCenterX = g_gameConfigBlackboard.GetValue("WORLD_CENTER_X", 0.0f);
	float m_WORLDCenterY = g_gameConfigBlackboard.GetValue("WORLD_CENTER_Y", 0.0f);

	float m_maxScreenShakeDuration = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_DURATION", 0.2f);
	float m_maxDeathScreenShakeDuration = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_DEATH_DURATION", 0.5f);
	float m_maxPlayerDeathScreenShakeDuration = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_PLAYER_DEATH_DURATION", 2.0f);

	float m_maxScreenShakeTranslation = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_TRANSLATION", 1.0f);
	float m_maxDeathScreenShakeTranslation = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_TRANSLATION_DEATH", 5.0f);
	float m_maxPlayerDeathScreenShakeTranslation = g_gameConfigBlackboard.GetValue("MAX_SCREENSHAKE_TRANSLATION_PLAYER_DEATH", 15.0f);

	float m_textAnimationPosPercentageTop = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.98f);
	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);
	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 20.0f);
	float m_textAnimationTime = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 4.0f);
	float m_textMovementPhaseTimePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 0.25f);

	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();
	float m_timeTextAnimation = 0.0f;

	GameState m_currentState = GameState::AttractScreen;
	GameState m_nextState = GameState::AttractScreen;

	float m_timeAlive = 0.0f;
	Vec2 m_attractModePos = Vec2(m_UICenterX, m_UICenterY);
	Camera m_AttractScreenCamera;

	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_transitionTextColor = false;

	Clock m_clock;

};