#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

enum class GameState {
	EngineLogo,
	AttractScreen,
	Play
};

enum class ShaderEffect {
	NoEffect = -1,
	ColorBanding,
	Pixelized,
	Grayscale,
	Inverted,
	DistanceFog,
	NUM_EFFECTS
};

class App;
class Player;

class Game {

public:
	Game(App* appPointer);
	~Game();
	bool SuperTest(EventArgs&);
	bool AnotherSuperTest(EventArgs&);
	void Startup();
	void ShutDown();
	void LoadAssets();
	void LoadTextures();
	void LoadSoundFiles();

	void Update();
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render();

	void KillAllEnemies();

	void ShakeScreenCollision();
	void ShakeScreenDeath();
	void ShakeScreenPlayerDeath();
	void StopScreenShake();

	Rgba8 const GetRandomColor() const;

	static bool DebugSpawnWorldWireSphere(EventArgs& eventArgs);
	static bool DebugSpawnWorldLine3D(EventArgs& eventArgs);
	static bool DebugClearShapes(EventArgs& eventArgs);
	static bool DebugToggleRenderMode(EventArgs& eventArgs);
	static bool DebugSpawnPermanentBasis(EventArgs& eventArgs);
	static bool DebugSpawnWorldWireCylinder(EventArgs& eventArgs);
	static bool DebugSpawnBillboardText(EventArgs& eventArgs);
	static bool GetControls(EventArgs& eventArgs);

	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	std::string m_currentText = "";

private:
	App* m_theApp = nullptr;

	void StartupLogo();
	void StartupAttractScreen();
	void StartupPlay();

	void CheckIfWindowHasFocus();
	void UpdateGameState();

	void UpdateEntities(float deltaSeconds);

	void UpdateLogo(float deltaSeconds);
	void UpdateInputLogo(float deltaSeconds);

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
	
	double GetFPS() const;
	void AddDeltaToFPSCounter();
	void DisplayClocksInfo() const;

	void RenderEntities() const;
	void RenderPlay();
	void RenderAttractScreen() const;
	void RenderLogo() const;
	void RenderUI() const;
	void RenderTextAnimation() const;

private:
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

	GameState m_currentState = GameState::EngineLogo;
	GameState m_nextState = GameState::EngineLogo;

	float m_timeAlive = 0.0f;
	Vec2 m_attractModePos = Vec2(m_UICenterX, m_UICenterY);
	Camera m_AttractScreenCamera;

	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_transitionTextColor = false;

	Clock m_clock;
	EntityList m_allEntities;
	
	Player* m_player = nullptr;

	bool m_isCursorHidden = false;
	bool m_isCursorClipped = false;
	bool m_isCursorRelative = false;

	bool m_lostFocusBefore = false;
	bool m_loadedAssets = false;
	
	int m_fpsSampleSize = g_gameConfigBlackboard.GetValue("FPS_SAMPLE_SIZE", 60);
	double* m_deltaTimeSample = nullptr;
	int m_storedDeltaTimes = 0;
	int m_currentFPSAvIndex = 0;
	double m_totalDeltaTimeSample = 0.0f;

	Texture* m_logoTexture = nullptr;
	double m_timeShowingLogo = 0.0;
	double m_engineLogoLength = g_gameConfigBlackboard.GetValue("ENGINE_LOGO_LENGTH", 2.0);
	bool m_showEngineLogo = g_gameConfigBlackboard.GetValue("SHOW_ENGINE_LOGO", true);

	Shader* m_effectsShaders [(int)ShaderEffect::NUM_EFFECTS];
	bool m_applyEffects [(int) ShaderEffect::NUM_EFFECTS];
};