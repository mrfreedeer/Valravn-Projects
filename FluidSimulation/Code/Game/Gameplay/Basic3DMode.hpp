#pragma once
#include "Engine/Math/AABB3.hpp"
#include "Game/Gameplay/GameMode.hpp"
#include "Game/Gameplay/FluidSolver.hpp"

enum class MaterialEffect {
	NoEffect = -1,
	ColorBanding,
	Pixelized,
	Grayscale,
	Inverted,
	DistanceFog,
	NUM_EFFECTS
};

class Material;
class StructuredBuffer;

struct ImGuiConfig {
	bool m_showDebug = true;
	bool m_showOriginalDepth = false;
	bool m_showBlurredDepth = true;
	bool m_showOriginalThickness = false;
	bool m_showBlurredThickness = true;
	bool m_skipBlurPass = false;
	float m_sigma = 3.0f;
	float m_renderingRadius = 0.075f;
	int m_blurPassCount = 10;
	int m_kernelRadius = 7;
	int m_iterations = 10;
	float m_clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float m_waterColor[4] = {0.0f, 0.15f, 1.0f, 1.0f};
	Vec3 m_forces = Vec3::ZERO;
};

class Basic3DMode : public GameMode {
public:
	Basic3DMode(Game* game, Vec2 const& UISize);
	~Basic3DMode();
	virtual void Startup() override;
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Shutdown() override;


	static bool DebugSpawnWorldWireSphere(EventArgs& eventArgs);
	static bool DebugSpawnWorldLine3D(EventArgs& eventArgs);
	static bool DebugClearShapes(EventArgs& eventArgs);
	static bool DebugToggleRenderMode(EventArgs& eventArgs);
	static bool DebugSpawnPermanentBasis(EventArgs& eventArgs);
	static bool DebugSpawnWorldWireCylinder(EventArgs& eventArgs);
	static bool DebugSpawnBillboardText(EventArgs& eventArgs);
	static bool GetControls(EventArgs& eventArgs);

protected:
	virtual void UpdateDeveloperCheatCodes(float deltaSeconds);
	virtual void UpdateInput(float deltaSeconds) override;

private:
	void DisplayClocksInfo() const;

	void RenderGPUParticles() const;
	void UpdateGPUParticles(float deltaSeconds);

	void RenderCPUParticles() const;
	void UpdateCPUParticles(float deltaSeconds);

	void UpdateParticles(float deltaSeconds);
	void RenderParticles() const;
	void RenderBlurPass() const;

	float GetGaussian1D(float sigma, float dist) const;
	void GetGaussianKernels(unsigned int kernelSize, float sigma, float* kernels) const;
	void LoadMaterials();
	void InitializeBuffers();
	void InitializeGPUPhysicsBuffers();
	void InitializeTextures();
	void UpdateImGui();
	void BitonicSortTest(int* arr, size_t arraySize, unsigned int direction = 0);
	void SortGPUParticles(size_t arraySize, unsigned int direction = 0) const;
private:
	Material* m_prePassMaterial = nullptr;
	Material* m_prePassGPUMaterial = nullptr;
	Material* m_fluidColorPassMaterial = nullptr;
	Material* m_thicknessMaterial = nullptr;

	Material* m_applyForcesCS = nullptr;
	Material* m_lambdaCS = nullptr;
	Material* m_bitonicSortCS = nullptr;
	Material* m_hashParticlesCS = nullptr;
	Material* m_offsetGenerationCS = nullptr;
	Material* m_updateMovementCS = nullptr;

	Material* m_gaussianBlurMaterial = nullptr;
	Material* m_effectsMaterials[(int)MaterialEffect::NUM_EFFECTS];
	StructuredBuffer* m_meshVBuffer = nullptr ;
	StructuredBuffer* m_meshletBuffer = nullptr;
	StructuredBuffer* m_hashInfoBuffer = nullptr;
	ConstantBuffer* m_gameConstants = nullptr;
	
	StructuredBuffer* m_particlesBuffer = nullptr;
	StructuredBuffer* m_offsetsBuffer = nullptr;

	ConstantBuffer* m_thicknessHPassCBuffer = nullptr;
	ConstantBuffer* m_thicknessVPassCBuffer = nullptr;
	ConstantBuffer* m_depthHPassCBuffer = nullptr;
	ConstantBuffer* m_depthVPassCBuffer = nullptr;

	Texture* m_depthTexture = nullptr;
	Texture* m_blurredDepthTexture[2] = {};
	Texture* m_thickness = nullptr;
	Texture* m_blurredThickness[2] = {};
	Texture* m_backgroundRT = nullptr;
	bool m_applyEffects[(int)MaterialEffect::NUM_EFFECTS];
	mutable unsigned int m_currentBlurredThickness = 0;
	mutable unsigned int m_currentBlurredDepth = 0;

	AABB3 m_particlesBounds = g_gameConfigBlackboard.GetValue("BOX_BOUNDS", AABB3::ZERO_TO_ONE);
	FluidSolver m_fluidSolver = {};
	std::vector<FluidParticle> m_particles = {};
	std::vector<GPUFluidParticle> m_GPUparticles = {};
	FluidParticleMeshInfo* m_particlesMeshInfo =nullptr;
	std::vector<Vertex_PCU> m_verts = {};
	float m_fps = 0.0f;
	unsigned int m_vertsPerParticle = 0;
	ImGuiConfig m_debugConfig = {};
	unsigned int m_frame = 0;
};