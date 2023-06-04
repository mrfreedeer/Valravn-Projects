#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

enum class GameState {
	EngineLogo = -2,
	AttractScreen,
	HairTessellation,
	HairSphereLighting,
	HairSimulation,
	HairDiscLighting, 
	NUM_GAME_STATES
};

class App;
class Player;
class HairObject;
class HairSimulGuide;	
class Shader;
class UnorderedAccessBuffer;
class MeshBuilder;

struct HairConstants;
struct SSAOConstants;
enum class SimulAlgorithm;
struct ModelLoadingData {
	bool m_reverseWindingOrder = false;
	bool m_invertUV = false;
	float m_scale = 1.0f;
	std::string m_basis = "";
	Rgba8 m_color = Rgba8::MAGENTA; // Magenta for untextured 
	MemoryUsage m_memoryUsage = MemoryUsage::Default;
	std::string m_name = "Unnamed Mesh";
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

	void Update();
	void UpdateDeveloperCheatCodes(float deltaSeconds);

	void Render() const;

	Rgba8 const GetRandomColor() const;
	Vec3 GetPlayerPosition() const;

	Shader* GetSimulationShader(SimulAlgorithm simulAlgorithm, bool isHairCurly = false) const;
	Shader* GetSimulationShader() const;
	HairConstants*& GetHairConstants();
	HairConstants* GetHairConstants() const;
	HairConstants*& GetBackHairConstants();
	HairConstants*& GetPrevHairConstants();
	HairConstants*& GetBackPrevHairConstants();

	bool UseModelVertsForInterp() const;

	static bool DebugSpawnWorldWireSphere(EventArgs& eventArgs);
	static bool DebugSpawnWorldLine3D(EventArgs& eventArgs);
	static bool DebugClearShapes(EventArgs& eventArgs);
	static bool DebugToggleRenderMode(EventArgs& eventArgs);
	static bool DebugSpawnPermanentBasis(EventArgs& eventArgs);
	static bool DebugSpawnWorldWireCylinder(EventArgs& eventArgs);
	static bool DebugSpawnBillboardText(EventArgs& eventArgs);
	static bool GetControls(EventArgs& eventArgs);

	static bool ImportMesh(EventArgs& eventArgs);
	static bool ScaleMesh(EventArgs& eventArgs);
	static bool TransformMesh(EventArgs& eventArgs);
	static bool ReverseWindingOrder(EventArgs& eventArgs);
	static bool SaveToBinary(EventArgs& eventArgs);
	static bool LoadFromBinary(EventArgs& eventArgs);
	static bool InvertUV(EventArgs& eventArgs);

	bool m_useTextAnimation = true;
	Rgba8 m_textAnimationColor = Rgba8(255, 255, 255, 255);
	std::string m_currentText = "";
	ConstantBuffer* m_hairConstantBuffer = nullptr;
	mutable bool m_randomizeHairDir = false;
	mutable bool m_renderSphere = true;
	mutable bool m_renderHair = true;
	mutable bool m_renderMultInterp = true;
	Shader* m_copyShader = nullptr;

	UnorderedAccessBuffer* GetPoissonSampleBuffer() const;
private:
	App* m_theApp = nullptr;

	void StartupLogo();
	void StartupAttractScreen();
	void StartupHairDiscLighting();
	void StartupHairSphereLighting();
	void StartupHairTessellation();
	void StartupHairSimulation();
	void InitializeHairProperties();

	void ShutdownHairDiscLighting();
	void ShutdownHairSphereLighting();
	void ShutdownHairTessellation();
	void ShutdownHairSimulation();

	void CreateSSAOKernels();
	void CheckIfWindowHasFocus();
	void UpdateGameState();

	void UpdateHairConstants();

	void UpdateEntities(float deltaSeconds);

	void UpdateLogo(float deltaSeconds);
	void UpdateInputLogo(float deltaSeconds);

	void UpdateHairSimulation(float deltaSeconds);
	void UpdateInputHairSimulation(float deltaSeconds);

	void UpdateHairTessellation(float deltaSeconds);
	void UpdateHairTessellationInput(float deltaSeconds);

	void UpdateHairDiscLighting(float deltaSeconds);
	void UpdateInputHairDiscLighting(float deltaSeconds);

	void UpdateHairSphereLighting(float deltaSeconds);
	void UpdateInputHairSphereLighting(float deltaSeconds);

	void UpdateAttractScreen(float deltaSeconds);
	void UpdateInputAttractScreen(float deltaSeconds);

	void UpdateCameras(float deltaSeconds);
	void UpdateUICamera(float deltaSeconds);
	void UpdateWorldCamera(float deltaSeconds);

	void UpdateDebugProperties();

	void UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation);
	void TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation);
	float GetTextWidthForAnim(float fullTextWidth);
	Vec2 const GetIBasisForTextAnim();
	Vec2 const GetJBasisForTextAnim();
	
	double GetFPS() const;
	void AddDeltaToFPSCounter();
	void DisplayClocksInfo() const;

	void RenderHairSimulation() const;
	void RenderHairTessellation() const;
	void RenderHairDiscLighting() const;
	void RenderHairSphereLighting() const;
	void RenderAttractScreen() const;
	void RenderLogo() const;


	void RenderImGui() const;

	void RenderImGuiRenderingSection() const;
	void RenderImGuiPresetsSection() const;
	void RenderImGuiSimulationSection() const;

	void RenderEntities() const;
	void RenderUI() const;
	void RenderTextAnimation() const;


	void SetSSAOSettings() const;
	void SetCurlinessSettings() const;
	void FetchAvailableModels();
private:
	Camera m_worldCamera;
	Camera m_UICamera;
	Camera m_AttractScreenCamera;
	
	GameState m_currentState = GameState::EngineLogo;
	GameState m_nextState = GameState::EngineLogo;

	float m_timeAlive = 0.0f;

	float m_simulationInitialSlowmo = g_gameConfigBlackboard.GetValue("HAIR_INITIAL_SLOWMO_DURATION", 1.0f);

	float m_UISizeY = g_gameConfigBlackboard.GetValue("UI_SIZE_Y", 0.0f);
	float m_UISizeX = g_gameConfigBlackboard.GetValue("UI_SIZE_X", 0.0f);
	float m_UICenterX = g_gameConfigBlackboard.GetValue("UI_CENTER_X", 0.0f);
	float m_UICenterY = g_gameConfigBlackboard.GetValue("UI_CENTER_Y", 0.0f);

	float m_WORLDSizeY = g_gameConfigBlackboard.GetValue("WORLD_SIZE_Y", 0.0f);
	float m_WORLDSizeX = g_gameConfigBlackboard.GetValue("WORLD_SIZE_X", 0.0f);
	float m_WORLDCenterX = g_gameConfigBlackboard.GetValue("WORLD_CENTER_X", 0.0f);
	float m_WORLDCenterY = g_gameConfigBlackboard.GetValue("WORLD_CENTER_Y", 0.0f);

	float m_textAnimationPosPercentageTop = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.98f);
	float m_textCellHeightAttractScreen = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT_ATTRACT_SCREEN", 40.0f);
	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 20.0f);
	float m_textAnimationTime = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 4.0f);
	float m_textMovementPhaseTimePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 0.25f);
	float m_timeTextAnimation = 0.0f;

	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();


	Vec2 m_attractModePos = Vec2(m_UICenterX, m_UICenterY);

	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_transitionTextColor = false;

	Clock m_clock;
	EntityList m_allEntities;
	std::vector<HairObject*> m_allHairObjects;

	Player* m_player = nullptr;

	bool m_isCursorHidden = false;
	bool m_isCursorClipped = false;
	bool m_isCursorRelative = false;

	bool m_lostFocusBefore = false;
	bool m_loadedAssets = false;
	bool m_initializedHairProperties = false;
	
	bool m_showEngineLogo = g_gameConfigBlackboard.GetValue("SHOW_ENGINE_LOGO", true);
	
	Texture* m_logoTexture = nullptr;

	int m_fpsSampleSize = g_gameConfigBlackboard.GetValue("FPS_SAMPLE_SIZE", 60);
	int m_storedDeltaTimes = 0;
	int m_currentFPSAvIndex = 0;

	double* m_deltaTimeSample = nullptr;
	double m_totalDeltaTimeSample = 0.0f;
	double m_timeShowingLogo = 0.0;
	double m_engineLogoLength = g_gameConfigBlackboard.GetValue("ENGINE_LOGO_LENGTH", 2.0);




	Vec3 m_lastPlayerPos = Vec3::ZERO;
	EulerAngles m_lastPlayerOrientation = EulerAngles::ZERO;

	HairObject* m_hairObjectKajiya = nullptr;
	HairObject* m_hairObjectMarschner = nullptr;
	Shader* m_diffuseKajiya = nullptr;
	Shader* m_diffuseMarschner = nullptr;
	Shader* m_diffuseMarschnerCPUSim = nullptr;
	Shader* m_diffuseTessKajiya = nullptr;
	Shader* m_diffuseTessMarschner = nullptr;
	Shader* m_diffuseMultTessMarschner = nullptr;
	Shader* m_diffuseMultTessKajiya = nullptr;
	Shader* m_DFTLCShader = nullptr;
	Shader* m_MassSpringsCurlyCShader = nullptr;
	Shader* m_MassSpringsStraightShader = nullptr;
	Shader* m_SSAO = nullptr;
	Shader* m_SampleDownsizer = nullptr;


	float m_hairWidthMin = g_gameConfigBlackboard.GetValue("HAIR_WIDTH_MIN", 0.00001f);
	float m_hairWidthMax = g_gameConfigBlackboard.GetValue("HAIR_WIDTH_MAX", 0.01f);

	int m_hairSpecularExpMin = g_gameConfigBlackboard.GetValue("HAIR_SPECULAR_EXP_MIN", 0);
	int m_hairSpecularExpMax = g_gameConfigBlackboard.GetValue("HAIR_SPECULAR_EXP_MAX", 16);


	float m_prevSegmentLength = 0.0f;


	double m_startTime = 0.0;


	mutable bool m_isUsingKajiya = false;
	mutable bool m_isUsingMarschner = false;

	Rgba8 m_brown = Rgba8(99,69,36);
	Rgba8 m_black = Rgba8(5,5,5);

	HairConstants* m_currentConstants[2] = {};
	HairConstants* m_prevConstants[2] = {};
	SSAOConstants* m_currentSSAO = nullptr;
	SSAOConstants* m_prevSSAO = nullptr;

	HairSimulGuide* m_CPUSimulObject = nullptr;

	bool m_areSSAOKernelsDirty = true;
	UnorderedAccessBuffer* m_SSAOKernels = nullptr;
	MeshBuilder* m_meshBuilder = nullptr;
	std::vector<std::filesystem::path> m_availableModels;
	ModelLoadingData* m_modelImportOptions = nullptr;

	Image* m_densityMap = nullptr;
	Image* m_diffuseMap = nullptr;
	bool m_renderSkyBox = true;
};