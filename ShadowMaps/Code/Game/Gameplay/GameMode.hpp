#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"
#include <filesystem>

struct IntVec2;
struct Vec2;
struct Vec3;
class Game;
class Camera;
class Player;
class MeshBuilder;
class BufferParser;
class BufferWriter;
class ConstantBuffer;
class Texture;
class ConvexPoly3DShape;

struct ShadowsConstants {
	float DepthBias = 0.0f;
	unsigned int UseBoxSampling = 0;
	unsigned int UsePCF = 0;
	unsigned int PCFSampleCount = 0;
};

struct ModelLoadingData {
	bool m_reverseWindingOrder = false;
	bool m_invertUV = false;
	float m_scale = 1.0f;
	std::string m_basis = "";
	Rgba8 m_color = Rgba8::MAGENTA; // Magenta for untextured 
	MemoryUsage m_memoryUsage = MemoryUsage::Default;
	std::string m_name = "Unnamed Mesh";
};
class GameMode {
public:
	GameMode(Game* pointerToGame, Vec2 const& UICameraSize);
	virtual ~GameMode();

	virtual void Startup();
	virtual void Shutdown();
	virtual void Update(float deltaSeconds);
	virtual void RenderMeshes() const;
	virtual void RenderEntities(bool useDiffuse = false) const;
	virtual void Render() const;
	virtual void RenderUI() const;
	virtual void RenderTextAnimation() const;
	virtual Vec3 GetPlayerPosition() const;
	virtual Camera GetWorldCamera() const;
	virtual Player* GetPlayer() const;
	virtual Rgba8 const GetRandomColor() const;
	virtual void GetRasterizerDepthSettings(int& out_depthBias, float& out_depthBiasClamp, float& out_slopeScaledBias) const;
	virtual Texture* CreateOrReplaceShadowMapTexture(IntVec2 const& dimensions) const;
	virtual void AddBasicProps();

	static void SubscribeToEvents();
	static bool ImportMesh(EventArgs& eventArgs);
	static bool ScaleMesh(EventArgs& eventArgs);
	static bool TransformMesh(EventArgs& eventArgs);
	static bool ReverseWindingOrder(EventArgs& eventArgs);
	static bool SaveToBinary(EventArgs& eventArgs);
	static bool LoadFromBinary(EventArgs& eventArgs);
	static bool InvertUV(EventArgs& eventArgs);

	bool m_useTextAnimation = false;
	Rgba8 m_textAnimationColor = Rgba8::WHITE;
protected:
    virtual void UpdateConstantBuffers();
	virtual void UpdateInput(float deltaSeconds);
	virtual void UpdateInputFromKeyboard() = 0;
	virtual void UpdateInputFromController() = 0;
	virtual void UpdateTextAnimation(float deltaTime, std::string text, Vec2 const& textLocation, float textCellHeight);
	virtual void TransformText(Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
	virtual float GetTextWidthForAnim(float fullTextWidth);
	virtual Vec2 const GetIBasisForTextAnim();
	virtual Vec2 const GetJBasisForTextAnim();

	virtual std::string GetCurrentGameStateAsText() const;

	virtual void CheckIfWindowHasFocus();
	virtual void SetNextGameMode(int nextGameState);
	virtual Camera GetLightCamera(int lightIndex) const;
	virtual Camera GetLightCamera(Light const& light) const;
	virtual Camera GetCameraForDepthPass(Vec3 const& cameraPosition, EulerAngles const& viewOrientation) const;

	virtual bool AreThereLights() const final ;
	virtual void AddImGuiDebugControls() const;
	virtual void AddLight() const final;
	virtual Entity* GetNextMovableEntity() const;
	virtual Entity* GetPrevMovableEntity() const;
	virtual bool IsEntityMovable(Entity* entity) const = 0;
	virtual Entity* GetSelectedEntity() const;

	bool SaveSceneToFile(EventArgs& args);
    bool LoadSceneFromFile(EventArgs& args);
	virtual void SaveBaseSceneInfo(std::vector<unsigned char>& sceneBuffer) const;
	virtual void LoadBaseSceneInfo(BufferParser& bufferParser) const;
	virtual void SetLightConstants() const;
	virtual void FetchAvailableModels();
	virtual void AppendConvexPolyShape3D(BufferWriter const& bufferWriter, ConvexPoly3DShape* convexPolyShape) const;
	virtual void SaveScene(std::filesystem::path fileName) const;
	virtual void LoadScene(std::filesystem::path fileName);
	ConvexPoly3DShape* ParseConvexPolyShape3D(BufferParser& bufferParser) const;

	bool IsSecondDepthEnabled() const;
	bool IsMidpointDepthEnabled() const;
	void CreateShadowMap(Texture* shadowMapTex, CullMode cullMode) const;
	void PopulatePoissonPoints(unsigned int sampleAmount);
protected:
	Game* m_game = nullptr;
	Camera m_worldCamera;
	Camera m_UICamera;

	std::string m_currentText = "";
	Rgba8 m_startTextColor;
	Rgba8 m_endTextColor;
	bool m_transitionTextColor = false;
	Vec2 m_UICameraSize = Vec2::ZERO;
	Vec2 m_UICameraCenter = Vec2::ZERO;

	std::vector	<Vertex_PCU> m_textAnimTriangles = std::vector<Vertex_PCU>();

	float m_textAnimationPosPercentageTop = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_POS_PERCENTAGE_TOP", 0.98f);
	float m_textCellHeight = g_gameConfigBlackboard.GetValue("TEXT_CELL_HEIGHT", 20.0f);
	float m_timeAlive = 0.0f;
	float m_timeElapsedTextAnimation = 0.0f;
	float m_textAnimationDuration = g_gameConfigBlackboard.GetValue("TEXT_ANIMATION_TIME", 4.0f);
	float m_textMovementPhaseTimePercentage = g_gameConfigBlackboard.GetValue("TEXT_MOVEMENT_PHASE_TIME_PERCENTAGE", 0.25f);
	float m_nearestPointRadius = 0.45f;

	std::string m_gameModeName = "";
	std::string m_baseHelperText = "";
	std::string m_helperText = "";

	bool m_isCursorHidden = false;
	bool m_isCursorClipped = false;
	bool m_isCursorRelative = false;

	bool m_lostFocusBefore = false;
	bool m_controlLightOrientation = false;
	EulerAngles m_savedPlayerOrientation = EulerAngles::ZERO;
	Clock m_clock;

	EntityList m_allEntities;
	EntityList m_movableEntities;
	EntityList m_props;

	Player* m_player = nullptr;

	MeshBuilder* m_meshBuilder = nullptr;
	std::vector<std::filesystem::path> m_availableModels;
	ModelLoadingData* m_modelImportOptions = nullptr;

	ConstantBuffer* m_shadowMapCBuffer = nullptr;
	ShadowsConstants* m_shadowMapsConstants = nullptr;
	ShadowsConstants* m_prevShadowMapsConstants = nullptr;
	mutable Texture* m_depthTarget = nullptr;
	mutable Texture* m_secondDepthTarget = nullptr;

	Shader* m_depthOnlyShader = nullptr;
	Shader* m_defaultShadowsShader = nullptr;
	Shader* m_projectiveTextureShader = nullptr;
	Shader* m_linearDepthShader = nullptr;
	Shader* m_secondDepthShader = nullptr;
	Shader* m_midpointShader = nullptr;

	float m_lightConstantAttenuation = g_gameConfigBlackboard.GetValue("LIGHT_CONSTANT_ATTENUATION", 0.001f);
	float m_lightLinearAttenuation = g_gameConfigBlackboard.GetValue("LIGHT_LINEAR_ATTENUATION", 0.002f);
	float m_lightQuadAttenuation = g_gameConfigBlackboard.GetValue("LIGHT_QUAD_ATTENUATION", 0.005f);

	std::vector<Vec2> m_poissonPoints;
	UnorderedAccessBuffer* m_poissonSampleBuffer = nullptr;
	mutable unsigned int m_poissonSamplesAmount = g_gameConfigBlackboard.GetValue("POISSON_SAMPLE_AMOUNT_MIN", 16);
};