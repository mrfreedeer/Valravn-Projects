#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"


enum class GameState {
	AttractScreen,
	Lobby,
	GamemodeLobby,
	VictoryScreen,
	Play
};

class App;
class Player;
class Map;
class MapDefinition;

struct PlayerInfo {
	PlayerInfo(int playerIndex, int controllerIndex) :
		m_playerIndex(playerIndex),
		m_controller(controllerIndex) {

	}

	int m_playerIndex = -1;
	int m_controller = -1;
};

class Game {

public:
	Game(App* appPointer);
	~Game();
	
	void Startup();
	void ShutDown();
	void LoadAssets();
	void LoadTextures();
	void LoadSoundFiles();

	void GoToVictoryScreen();

	void Update();
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render() const;

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
	static bool RaycastDebugToggle(EventArgs& eventArgs);
	
	Camera& GetPlayerCamera(int playerIndex);
	Camera& GetPlayerUICamera(int playerIndex);


	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	std::string m_currentText = "";
	bool m_isRaycastDebugEnabled = false;

	Clock m_clock;

private:
	App* m_theApp = nullptr;

	void StartupAttractScreen();
	void StartupPlay();
	void StartupLobby();
	void StartupGamemodeLobby();
	void StartupVictory();

	void CheckIfWindowHasFocus();
	void UpdateGameState();

	void UpdatePlay(float deltaSeconds);
	void UpdateInputPlay(float deltaSeconds);

	void UpdateAttractScreen(float deltaSeconds);
	void UpdateInputAttractScreen(float deltaSeconds);

	void UpdateLobby(float deltaSeconds);
	void UpdateInputLobby(float deltaSeconds);
	
	void UpdateGamemodeLobby(float deltaSeconds);
	void UpdateInputGamemodeLobby(float deltaSeconds);

	void UpdateVictory(float deltaSeconds);
	void UpdateInputVictory(float deltaSeconds);

	void UpdateCameras(float deltaSeconds);
	void UpdateUICamera(float deltaSeconds);
	void UpdateWorldCamera(float deltaSeconds);
	void UpdateListeners();

	void UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation);
	void TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation);
	float GetTextWidthForAnim(float fullTextWidth);
	Vec2 const GetIBasisForTextAnim();
	Vec2 const GetJBasisForTextAnim();
	
	double GetFPS() const;
	void AddDeltaToFPSCounter();
	void DisplayClocksInfo() const;

	void RenderPlay() const;
	void RenderAttractScreen() const;
	void RenderLobby() const;
	void RenderGamemodeLobby() const;
	void RenderVictory() const;

	void RenderUI() const;
	void RenderTextAnimation() const;
	void RenderPlayerJoinedInfo(std::vector<Vertex_PCU>& verts, PlayerInfo const& playerInfo, AABB2 const& screenPane) const;

	void SetViewportsForPlayers();
	int GetPlayerIndexForController(int controllerIndex) const;

private:
	Camera m_playerWorldCameras[4];
	Camera m_playerUICameras[4];

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

	float m_debugRayLength = g_gameConfigBlackboard.GetValue("DEBUG_RAY_LENGTH", 10.0f);
	float m_debugShortRayLength = g_gameConfigBlackboard.GetValue("DEBUG_SHORT_RAY_LENGTH", 10.0f);
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

	MapDefinition const* m_maptoLoad = nullptr;
	Map* m_map = nullptr;
	std::vector<PlayerInfo> m_joinedPlayers;
	float m_musicVolume = g_gameConfigBlackboard.GetValue("MUSIC_VOLUME", 1.0f);

	int m_currentGamemodeOption = 0;
};