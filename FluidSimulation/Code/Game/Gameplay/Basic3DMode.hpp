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

class Basic3DMode : public GameMode {
public:
	Basic3DMode(Game* game, Vec2 const& UISize);
	~Basic3DMode();
	virtual void Startup() override;
	virtual void Update(float deltaSeconds);
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

	void UpdateParticles(float deltaSeconds);
	void RenderParticles() const;

private:
	Material* m_prePassMaterial = nullptr;
	Material* m_fluidColorPassMaterial = nullptr;
	Material* m_thicknessMaterial = nullptr;
	Material* m_computeMaterial = nullptr;
	Material* m_effectsMaterials[(int)MaterialEffect::NUM_EFFECTS];
	StructuredBuffer* m_meshVBuffer[2] = {} ;
	StructuredBuffer* m_meshletBuffer = nullptr;
	StructuredBuffer* m_computeBuffer = nullptr;
	ConstantBuffer* m_gameConstants = nullptr;
	Texture* m_depthTexture = nullptr;
	Texture* m_thickness = nullptr;
	Texture* m_backgroundRT = nullptr;
	bool m_applyEffects[(int)MaterialEffect::NUM_EFFECTS];
	unsigned int m_currentVBuffer = 0;

	AABB3 m_particlesBounds = g_gameConfigBlackboard.GetValue("BOX_BOUNDS", AABB3::ZERO_TO_ONE);
	FluidSolver m_fluidSolver = {};
	std::vector<FluidParticle> m_particles = {};
	FluidParticleMeshInfo* m_particlesMeshInfo =nullptr;
	std::vector<Vertex_PCU> m_verts = {};
	float m_fps = 0.0f;
	unsigned int m_vertsPerParticle = 0;
};