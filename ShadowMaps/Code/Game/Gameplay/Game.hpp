#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

enum class GameState {
	EngineLogo = -3,
	BasicShapes,
	AttractScreen,
	Models,
	NUM_GAME_STATES
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
class GameMode;

class Game {
	friend class GameMode;
public:
	Game(App* appPointer);
	~Game();
	
	void Startup();
	void ShutDown();
	void LoadAssets();
	void LoadTextures();
	void LoadSoundFiles();

	void Update();
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render();
	void Quit();

	static bool DebugSpawnWorldWireSphere(EventArgs& eventArgs);
	static bool DebugSpawnWorldLine3D(EventArgs& eventArgs);
	static bool DebugClearShapes(EventArgs& eventArgs);
	static bool DebugToggleRenderMode(EventArgs& eventArgs);
	static bool DebugSpawnPermanentBasis(EventArgs& eventArgs);
	static bool DebugSpawnWorldWireCylinder(EventArgs& eventArgs);
	static bool DebugSpawnBillboardText(EventArgs& eventArgs);
	static bool GetControls(EventArgs& eventArgs);

private:
	App* m_theApp = nullptr;

	void UpdateGameState();
	double GetFPS() const;
	void AddDeltaToFPSCounter();

private:
	float m_UISizeY = g_gameConfigBlackboard.GetValue("UI_SIZE_Y", 0.0f);
	float m_UISizeX = g_gameConfigBlackboard.GetValue("UI_SIZE_X", 0.0f);
	float m_UICenterX = g_gameConfigBlackboard.GetValue("UI_CENTER_X", 0.0f);
	float m_UICenterY = g_gameConfigBlackboard.GetValue("UI_CENTER_Y", 0.0f);

	float m_WORLDSizeY = g_gameConfigBlackboard.GetValue("WORLD_SIZE_Y", 0.0f);
	float m_WORLDSizeX = g_gameConfigBlackboard.GetValue("WORLD_SIZE_X", 0.0f);
	float m_WORLDCenterX = g_gameConfigBlackboard.GetValue("WORLD_CENTER_X", 0.0f);
	float m_WORLDCenterY = g_gameConfigBlackboard.GetValue("WORLD_CENTER_Y", 0.0f);


	GameState m_currentState = GameState::EngineLogo;
	GameState m_nextState = GameState::EngineLogo;
	GameMode* m_currentGameMode = nullptr;

	float m_timeAlive = 0.0f;

	Clock m_clock;

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