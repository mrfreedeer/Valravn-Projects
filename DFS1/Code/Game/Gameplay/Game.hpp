#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

enum class GameState {
	AttractScreen = -1,
	Delaunay3D,
	Delaunay2D,
	NumGameStates
};

class App;
class Player;
class DelaunayShape2D;
class DelaunayShape3D;

class Game {

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

	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	std::string m_currentText = "";

private:
	App* m_theApp = nullptr;

	void StartupAttractScreen();
	void StartupDelaunay2D();
	void StartupDelaunay3D();

	void CheckIfWindowHasFocus();
	void UpdateGameState();

	void UpdateEntities(float deltaSeconds);
	void UpdateDelaunayShapes2D(float deltaSeconds);

	void UpdateDelaunay2D(float deltaSeconds);
	void UpdateInputDelaunay2D(float deltaSeconds);

	void UpdateDelaunay3D(float deltaSeconds);
	void UpdateInputDelaunay3D(float deltaSeconds);


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
	void RenderDelaunay2D() const;
	void RenderDelaunay3D() const;
	void RenderAttractScreen() const;
	void RenderUI() const;
	void RenderTextAnimation() const;

	std::string GetCurrentGameStateAsText() const;
	void RemoveEventPoint2D(Vec2 const& clickPosition);
	Vec2* GetEventPoint2D(Vec2 const& clickPosition);

	void CheckForConvexPoly2DOverlap();
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

	GameState m_currentState = GameState::AttractScreen;
	GameState m_nextState = GameState::AttractScreen;

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

	std::vector<Vec2> m_eventPoints;
	std::vector<Vec2> m_artificialPoints;
	std::string m_helperText = "";
	std::string m_gameModeName = "";

	bool m_isDraggingEventPoint = false;
	bool m_isDraggingConvexPoly = false;
	Vec2* m_selectedEventPoint = nullptr;

	bool m_drawTriangleMesh = false;
	bool m_drawVoronoiRegions = false;
	DelaunayShape2D* m_convexPoly = nullptr;
	DelaunayShape3D* m_convexPoly3D = nullptr;

	std::vector<DelaunayShape2D*> m_allPolygons;
	std::vector<DelaunayShape2D*> m_normalColorPolys;
	std::vector<DelaunayShape2D*> m_highlightedPolys;
	std::vector<Rgba8> m_allPolygonsColors;
	std::vector<Rgba8> m_normalColorPolysMap; // To quickly find the color of the non overlapping polys

	Vec2 m_nearestPointDelaunay2D = Vec2::ZERO;

	Vec3 m_rayStart = Vec3::ZERO;
	Vec3 m_rayFwd = Vec3::ZERO;
	bool m_frozenRaycast = false;

	int m_maxDelaunay3DStep = -1;
	int m_maxDelaunay2DStep = -1;
	mutable int m_maxVoronoiStep = -1;
	mutable int m_maxVoronoiStep2D = -1;
	bool m_draw3DPoly = true;
	bool m_drawSuperTetra = true;
	bool m_drawSuperTriangle = true;
	bool m_drawVoronoiEdgeProjections = true;

};