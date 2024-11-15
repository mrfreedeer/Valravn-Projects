#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB3.hpp"
#include <unordered_map>
#include <vector>

struct IntVec3;

struct FluidParticle {
	FluidParticle(Vec3 const& pos, Vec3 const& vel) : m_position(pos), m_predictedPos(pos), m_velocity(vel) {}
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_predictedPos = Vec3::ZERO;
	Vec3 m_prevPos = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_gradient = Vec3::ZERO;
	float m_lambda = 0.f;
	float m_density = 0.0f;
	std::vector<FluidParticle*> m_neighbors;
};

struct FluidParticleMeshInfo {
	Vec3 Position = Vec3::ZERO;
	float Color[4] = {};
};

struct GPUFluidParticle
{
	GPUFluidParticle() = default;
	GPUFluidParticle(Vec3 inputPos) : Position(inputPos){}
	Vec3 Position = Vec3::ZERO;
	float Density = 0.0f;
	Vec3 PredictedPosition = Vec3::ZERO;
	float Lambda = 0.0f;
	Vec3 PrevPosition = Vec3::ZERO;
	unsigned int Padding;
	Vec3 Velocity = Vec3::ZERO;
	unsigned int Padding2;
	Vec3 Gradient = Vec3::ZERO;
	unsigned int Padding3;
};

struct FluidSolverConfig {
	AABB3 m_simulationBounds = AABB3::ZERO_TO_ONE;
	std::vector<FluidParticle>* m_pointerToParticles = nullptr;
	unsigned int m_particlePerSide = 20;
	float m_renderingRadius = 0.05f;
	float m_kernelRadius = 0.0f;
	float m_restDensity = 1000.0f;
	int m_iterations = 0;
};

struct DensityReturnStruct {
	float density = 0.f;
	Vec3 gradient = Vec3::ZERO;
	float gradientLengthSum = 0.0f;
};

class FluidSolver {
public:
	FluidSolver() = default;
	FluidSolver(FluidSolverConfig const& config) : m_config(config) {}
	void InitializeParticles() const;
	void Update(float deltaSeconds);
	void AddForce(Vec3 force);
	float GetRenderingRadius() const { return m_config.m_renderingRadius; }
private:
	void ApplyForces(float deltaSeconds) const;
	void UpdateNeighbors();
	unsigned int GetIndexForPosition(Vec3 const& position);
	unsigned int GetIndexForCoords(IntVec3 const& coords);
	IntVec3 GetCoordsForPos(Vec3 position);
	void CalculateLambda();
	float Poly6Kernel(float distance);
	Vec3 SpikyKernelGradient(Vec3 const& displacement);
	DensityReturnStruct SPHDensity(FluidParticle const& particle);
	Vec3 GetViscosity(FluidParticle const& particle);
	void UpdatePositionDelta(float deltaSeconds);
	void UpdateVelocity(float deltaSeconds);
	void UpdateViscosityAndPosition(float deltaSeconds);
	Vec3 CalculateDeltaPosition(FluidParticle const& particle);
	Vec3 KeepParticleInBounds(Vec3 const& position) const;
	FluidSolverConfig m_config = {};
	std::unordered_map<unsigned int, std::vector<FluidParticle*>> m_neighborsHashmap;
	Vec3 m_forces = Vec3::ZERO;
};