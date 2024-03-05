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

class Basic3DMode : public GameMode {
public:
	Basic3DMode(Game* game, Vec2 const& UISize);
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
	double GetFPS() const;
	void AddDeltaToFPSCounter();
	void DisplayClocksInfo() const;

	void UpdateParticles(float deltaSeconds);
	void RenderParticles() const;

private:
	int m_fpsSampleSize = g_gameConfigBlackboard.GetValue("FPS_SAMPLE_SIZE", 60);
	double* m_deltaTimeSample = nullptr;
	int m_storedDeltaTimes = 0;
	int m_currentFPSAvIndex = 0;
	double m_totalDeltaTimeSample = 0.0f;

	Material* m_effectsMaterials[(int)MaterialEffect::NUM_EFFECTS];
	bool m_applyEffects[(int)MaterialEffect::NUM_EFFECTS];

	AABB3 m_particlesBounds = g_gameConfigBlackboard.GetValue("BOX_BOUNDS", AABB3::ZERO_TO_ONE);
	FluidSolver m_fluidSolver = {};
	std::vector<FluidParticle> m_particles = {};
	std::vector<Vertex_PCU> m_verts = {};
	unsigned int m_vertsPerParticle = 0;
};