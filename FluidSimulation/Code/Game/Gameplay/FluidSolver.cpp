#include "Game/Gameplay/FluidSolver.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/MathUtils.hpp"

constexpr unsigned int X_BITSHIFT = 21;
constexpr unsigned int Y_BITSHIFT = 21;
constexpr unsigned int Z_BITSHIFT = 21;

#define EPSILON 0.00001f

void FluidSolver::InitializeParticles() const
{
	Vec3 distancePerParticle = Vec3::ZERO;
	AABB3 const& bounds = m_config.m_simulationBounds;
	unsigned int const& particlesPerside = m_config.m_particlePerSide;
	auto* particles = m_config.m_pointerToParticles;


	distancePerParticle = bounds.GetDimensions() / float(particlesPerside);

	int particleIndex = 0;

	particles->reserve(particlesPerside * 3);

	for (float x = bounds.m_mins.x; x < bounds.m_maxs.x; x += distancePerParticle.x) {
		for (float y = bounds.m_mins.y; y < bounds.m_maxs.y; y += distancePerParticle.y) {
			for (float z = bounds.m_mins.z; z < bounds.m_maxs.z; z += distancePerParticle.z, particleIndex++) {
				particles->push_back(FluidParticle(Vec3(x, y, z), Vec3::ZERO));
			}
		}
	}

}

void FluidSolver::Update(float deltaSeconds)
{
	m_forces += Vec3(0.0f, 0.0f, -9.8f);
	ApplyForces(deltaSeconds);
	UpdateNeighbors();

	for (int iteration = 0; iteration < m_config.m_iterations; iteration++) {
		CalculateLambda();
		UpdatePositionDelta(deltaSeconds);
	}

	UpdatePosition(deltaSeconds);

	m_forces = Vec3::ZERO;
}

void FluidSolver::ApplyForces(float deltaSeconds) const
{
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		particle.m_velocity += deltaSeconds * m_forces;
		particle.m_predictedPos += deltaSeconds * particle.m_velocity;
	}
}

void FluidSolver::UpdateNeighbors()
{
	m_neighbors.clear();

	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		unsigned int arrayIndex = GetIndexForPosition(particle.m_predictedPos);
		std::vector<FluidParticle*>& neighborsArray = m_neighbors[arrayIndex];
		neighborsArray.push_back(&particle);
	}

}


unsigned int FluidSolver::GetIndexForPosition(Vec3 const& position)
{
	IntVec3 coords = GetCoordsForPos(position);
	unsigned int index = GetIndexForCoords(coords);
	return index;
}

unsigned int FluidSolver::GetIndexForCoords(IntVec3 const& coords)
{
	return (coords.x << X_BITSHIFT) | (coords.y << Y_BITSHIFT) | (coords.z << Z_BITSHIFT);
}

IntVec3 FluidSolver::GetCoordsForPos(Vec3 position)
{
	IntVec3 coords = IntVec3::ZERO;

	position /= m_config.m_kernelRadius;
	coords = IntVec3((int)floorf(position.x), (int)floorf(position.y), (int)floorf(position.z));

	return coords;
}

void FluidSolver::CalculateLambda()
{
	float static oneOverRestDensity = 1.0f / m_config.m_restDensity;
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		auto [density, gradient] = SPHDensity(particle);
		float densityConstraint = (density * oneOverRestDensity) - 1.0f;
		particle.m_lambda = densityConstraint / (gradient.GetLengthSquared() + EPSILON);
		particle.m_gradient = gradient;
	}
}

float FluidSolver::Poly6Kernel(float distance)
{
	float const& kernelRadius = m_config.m_kernelRadius;
	static float const kernelRadiusSqr = kernelRadius * kernelRadius;
	static float const kernelRadiusPwr9 = (kernelRadiusSqr * kernelRadiusSqr) * (kernelRadiusSqr * kernelRadiusSqr) * kernelRadius;
	static float const kernelCoeff = 315.0f / (64.0f * static_cast<float>(M_PI) * kernelRadiusPwr9);
	if (distance > kernelRadius) return 0.0f;
	if (distance < 0.0f) return 0.0f;

	float dSqr = distance * distance;


	float distanceCoeff = (kernelRadiusSqr - dSqr);
	distanceCoeff *= distanceCoeff * distanceCoeff;

	float kernelValue = kernelCoeff * distanceCoeff;

	return kernelValue;
}

Vec3 FluidSolver::SpikyKernelGradient(Vec3 const& displacement)
{
	float const& kernelRadius = m_config.m_kernelRadius;
	static float const kernelRadiusPwr6 = (kernelRadius * kernelRadius) * (kernelRadius * kernelRadius) * (kernelRadius * kernelRadius);

	float distance = displacement.GetLength();

	static float kernelCoefficient = 15.0f / (static_cast<float>(M_PI) * kernelRadiusPwr6);

	if (distance > kernelRadius) return Vec3::ZERO;
	float cubeDistanceDiff = kernelRadius - distance;
	cubeDistanceDiff *= cubeDistanceDiff * cubeDistanceDiff;
	float kernelValue = (kernelCoefficient * cubeDistanceDiff);

	return displacement.GetNormalized() * (kernelValue);
}

DensityReturnStruct FluidSolver::SPHDensity(FluidParticle const& particle)
{
	
	IntVec3 baseCoords = GetCoordsForPos(particle.m_position);
	float density = 0.0f;
	Vec3 gradient = Vec3::ZERO;
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			for (int z = -1; z <= 1; z++) {
				IntVec3 neighborCoords = baseCoords + IntVec3(x, y, z);
				unsigned int accessIndex = GetIndexForCoords(neighborCoords);
				std::vector<FluidParticle*> const& neighbors = m_neighbors[accessIndex];
				for (int neighborIndex = 0; neighborIndex < neighbors.size(); neighborIndex++) {
					FluidParticle const* const& neighbor = neighbors[neighborIndex];

					Vec3 displacement = particle.m_position - neighbor->m_position;
					float d = displacement.GetLength();
					if (d == 0.0f) continue; // ignoring self particle
					density += Poly6Kernel(d);
					gradient += SpikyKernelGradient(displacement);
				}

			}
		}
	}

	return DensityReturnStruct{ density, gradient };
}

void FluidSolver::UpdatePositionDelta(float deltaSeconds)
{
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		Vec3 deltaPos = CalculateDeltaPosition(particle);
		particle.m_predictedPos += deltaPos;
	}
}

void FluidSolver::UpdatePosition(float deltaSeconds)
{
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	AABB3 const& bounds = m_config.m_simulationBounds;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];

		if (particle.m_predictedPos.z < 0.f) {
			particle.m_predictedPos.z = 0.0f; 
		}

		particle.m_velocity = (particle.m_predictedPos - particle.m_position) / deltaSeconds;
		particle.m_prevPos = particle.m_position;
		particle.m_position = particle.m_predictedPos;
	}
}

Vec3 FluidSolver::CalculateDeltaPosition(FluidParticle const& particle)
{
	IntVec3 baseCoords = GetCoordsForPos(particle.m_position);
	Vec3 deltaPos = Vec3::ZERO;
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			for (int z = -1; z <= 1; z++) {
				IntVec3 neighborCoords = baseCoords + IntVec3(x, y, z);
				unsigned int accessIndex = GetIndexForCoords(neighborCoords);
				std::vector<FluidParticle*> const& neighbors = m_neighbors[accessIndex];
				for (int neighborIndex = 0; neighborIndex < neighbors.size(); neighborIndex++) {
					FluidParticle const* const& neighbor = neighbors[neighborIndex];
					float coefficient = neighbor->m_lambda + particle.m_lambda;
					deltaPos += coefficient * particle.m_gradient;
				}

			}
		}
	}

	return deltaPos;
}

void FluidSolver::AddForce(Vec3 force)
{
	m_forces += force;
}
