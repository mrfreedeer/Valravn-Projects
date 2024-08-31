#include "Game/Gameplay/FluidSolver.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"


#define EPSILON 0.00001f

#define UNREASONABLE_LENGTH 100'000

void FluidSolver::InitializeParticles() const
{
	Vec3 distancePerParticle = Vec3::ZERO;
	AABB3 const& bounds = m_config.m_simulationBounds;
	unsigned int const& particlesPerside = m_config.m_particlePerSide;
	auto* particles = m_config.m_pointerToParticles;

	distancePerParticle = bounds.GetDimensions() / float(particlesPerside);


	particles->reserve(particlesPerside * 3);


	Vec3 aabb3Center = bounds.GetCenter() - bounds.GetDimensions() / 4;
	for (unsigned int x = 0; x < particlesPerside; x++) {
		for (unsigned int y = 0; y < particlesPerside; y++) {
			for (unsigned int z = 0; z < particlesPerside; z++) {
				particles->push_back(FluidParticle(Vec3((float)x, (float)y, (float)z) * m_config.m_renderingRadius * 2.0f + aabb3Center, Vec3::ZERO));
			}
		}
	}

	//int particleIndex = 0;
	/*for (float x = bounds.m_mins.x; x < bounds.m_maxs.x; x += distancePerParticle.x) {
		for (float y = bounds.m_mins.y; y < bounds.m_maxs.y; y += distancePerParticle.y) {
			for (float z = bounds.m_mins.z; z < bounds.m_maxs.z; z += distancePerParticle.z, particleIndex++) {
				float yawVariance = rng.GetRandomFloatZeroUpToOne();
				float pitchVariance = rng.GetRandomFloatZeroUpToOne();

				Vec3 varianceFwd = EulerAngles(yawVariance, pitchVariance, 0.0f).GetXForward();


				particles->push_back(FluidParticle(Vec3(x, y, z) + varianceFwd, Vec3::ZERO));
			}
		}
	}*/

}

void FluidSolver::Update(float deltaSeconds)
{
	if (deltaSeconds >= 0.05f) deltaSeconds = 0.05f;
	m_forces += Vec3(0.0f, 0.0f, -9.8f);
	ApplyForces(deltaSeconds);
	UpdateNeighbors();

	for (int iteration = 0; iteration < m_config.m_iterations; iteration++) {
		CalculateLambda();
		UpdatePositionDelta(deltaSeconds);
	}

	UpdateVelocity(deltaSeconds);
	UpdateViscosityAndPosition(deltaSeconds);
	m_forces = Vec3::ZERO;
}

void FluidSolver::ApplyForces(float deltaSeconds) const
{
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		particle.m_velocity += deltaSeconds * m_forces;
		particle.m_predictedPos = particle.m_position + deltaSeconds * particle.m_velocity;
		Vec3 newPos = KeepParticleInBounds(particle.m_predictedPos);
		particle.m_velocity += (newPos - particle.m_predictedPos) / deltaSeconds;
		particle.m_predictedPos = newPos;
	}
}

void FluidSolver::UpdateNeighbors()
{
	m_neighborsHashmap.clear();

	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		particle.m_neighbors.clear();
		unsigned int arrayIndex = GetIndexForPosition(particle.m_predictedPos);
		std::vector<FluidParticle*>& neighborsArray = m_neighborsHashmap[arrayIndex];
		neighborsArray.push_back(&particle);
	}

	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		IntVec3 baseCoords = GetCoordsForPos(particle.m_position);
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				for (int z = -1; z <= 1; z++) {
					IntVec3 neighborCoords = baseCoords + IntVec3(x, y, z);
					unsigned int accessIndex = GetIndexForCoords(neighborCoords);
					auto const it = m_neighborsHashmap.find(accessIndex);
					bool foundNeighbor = (it != m_neighborsHashmap.end());
					if (!foundNeighbor) continue;

					std::vector<FluidParticle*>& neighborParticles = m_neighborsHashmap[accessIndex];
					particle.m_neighbors.insert(particle.m_neighbors.end(), neighborParticles.begin(), neighborParticles.end());
				}
			}
		}
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
	unsigned int const p1 = 73856093 * coords.x;
	unsigned int const p2 = 19349663 * coords.y;
	unsigned int const p3 = 83492791 * coords.z;

	return p1 + p2 + p3;
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
		auto [density, gradient, lengthSqrSum] = SPHDensity(particle);
		particle.m_density = density;
		float densityConstraint = (density * oneOverRestDensity) - 1.0f;

		if (densityConstraint <= 0.0f) {
			densityConstraint = 0.0f;
		}

		lengthSqrSum += gradient.GetLengthSquared();
		particle.m_lambda = -densityConstraint / (lengthSqrSum + EPSILON);
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

	static float kernelCoefficient = -45.0f / (static_cast<float>(M_PI) * kernelRadiusPwr6);

	if (distance > kernelRadius) return Vec3::ZERO;

	float distanceSqr = kernelRadius - distance;
	distanceSqr *= distanceSqr;
	float kernelValue = (kernelCoefficient * distanceSqr);

	Vec3 gradientValue = displacement.GetNormalized() * (kernelValue);

	return gradientValue;
}

DensityReturnStruct FluidSolver::SPHDensity(FluidParticle const& particle)
{

	float density = 0.0f;
	float gradientLengthSum = 0.0f;
	Vec3 gradientSum = Vec3::ZERO;
	float oneOverRestDensity = 1.0f / m_config.m_restDensity;

	auto const& neighbors = particle.m_neighbors;

	for (int neighborIndex = 0; neighborIndex < neighbors.size(); neighborIndex++) {
		FluidParticle const* const& neighbor = neighbors[neighborIndex];

		Vec3 displacement = particle.m_predictedPos - neighbor->m_predictedPos;
		float d = displacement.GetLength();
		//if (d == 0.0f) { continue; }; // ignoring self particle
		density += Poly6Kernel(d);
		Vec3 gradient = -SpikyKernelGradient(displacement) * oneOverRestDensity;
		gradientSum += gradient;
		gradientLengthSum += gradient.GetLengthSquared();
	}


	return DensityReturnStruct{ density, gradientSum, gradientLengthSum }; 
}

Vec3 FluidSolver::GetViscosity(FluidParticle const& particle)
{
	std::vector<FluidParticle*> const& neighbors = particle.m_neighbors;


	Vec3 viscosity = Vec3::ZERO;


	for (int neighborIndex = 0; neighborIndex < neighbors.size(); neighborIndex++) {
		FluidParticle const* const& neighbor = neighbors[neighborIndex];

		Vec3 displacement = particle.m_predictedPos - neighbor->m_predictedPos;
		Vec3 diffVelocities = neighbor->m_velocity - particle.m_velocity;
		float d = displacement.GetLength();
		//if (d == 0.0f) { continue; }; // ignoring self particle
		viscosity += diffVelocities * Poly6Kernel(d);

	}

	return viscosity * 0.01f;
}

void FluidSolver::UpdatePositionDelta(float deltaSeconds)
{
	UNUSED(deltaSeconds)
		std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {
		FluidParticle& particle = particles[particleIndex];
		Vec3 deltaPos = CalculateDeltaPosition(particle);
		/*if (deltaPos.GetLengthSquared() > UNREASONABLE_LENGTH) {
			deltaPos = deltaPos;
		}*/
		particle.m_predictedPos += deltaPos;
	}
}

void FluidSolver::UpdateVelocity(float deltaSeconds)
{
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	//AABB3 const& bounds = m_config.m_simulationBounds;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {

		FluidParticle& particle = particles[particleIndex];

		/*for (int otherParticle = particleIndex + 1; otherParticle < particles.size(); otherParticle++) {
			PushSphereOutOfPoint(particle.m_predictedPos, 0.05f, particles[otherParticle].m_position);
		}*/
		particle.m_velocity = (particle.m_predictedPos - particle.m_position) / deltaSeconds;

		if (particle.m_velocity.z > 9.8f) {
			particle.m_velocity = particle.m_velocity;
		}
		/*if (particle.m_velocity.GetLengthSquared() > UNREASONABLE_LENGTH) {
			particle.m_velocity = particle.m_velocity;
			prevParticlePos = prevParticlePos;
		}*/



	}
}

void FluidSolver::UpdateViscosityAndPosition(float deltaSeconds)
{
	std::vector<FluidParticle>& particles = *m_config.m_pointerToParticles;
	//AABB3 const& bounds = m_config.m_simulationBounds;
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {

		FluidParticle& particle = particles[particleIndex];
		particle.m_velocity += GetViscosity(particle) / m_config.m_restDensity;
	}
	for (int particleIndex = 0; particleIndex < particles.size(); particleIndex++) {

		FluidParticle& particle = particles[particleIndex];
		Vec3 prevParticlePos = particle.m_predictedPos;
		particle.m_prevPos = particle.m_position;
		Vec3 positionBeforeCorrection = particle.m_predictedPos;
		particle.m_predictedPos = KeepParticleInBounds(particle.m_predictedPos);
		particle.m_velocity += (particle.m_predictedPos - positionBeforeCorrection) / deltaSeconds;


		particle.m_position = particle.m_predictedPos;
	}
}

Vec3 FluidSolver::CalculateDeltaPosition(FluidParticle const& particle)
{
	if (particle.m_density < m_config.m_restDensity) return Vec3::ZERO;
	std::vector<FluidParticle*> const& neighbors = particle.m_neighbors;


	//std::vector<FluidParticle> debugNeighbors;
	//std::vector<Vec3> deltaPosDebug;
	Vec3 deltaPos = Vec3::ZERO;


	for (int neighborIndex = 0; neighborIndex < neighbors.size(); neighborIndex++) {
		FluidParticle const* const& neighbor = neighbors[neighborIndex];
		Vec3 displacement = particle.m_predictedPos - neighbor->m_predictedPos;
		float coefficient = neighbor->m_lambda + particle.m_lambda;
		Vec3 gradient = SpikyKernelGradient(displacement);
		deltaPos += coefficient * gradient;
		//debugNeighbors.push_back(*neighbor);
		//deltaPosDebug.push_back(coefficient * particle.m_gradient);
	}
	return deltaPos / m_config.m_restDensity;
}

Vec3 FluidSolver::KeepParticleInBounds(Vec3 const& position) const
{
	AABB3 const& bounds = m_config.m_simulationBounds;

	//Vec3 correctedPosition = bounds.GetNearestPoint(position);
	Vec3 correctedPosition = position;

	if (position.z < bounds.m_mins.z) correctedPosition.z = bounds.m_mins.z;
	//if (position.z > bounds.m_maxs.z) {
	//	correctedPosition.z = bounds.m_maxs.z;
	//}

	if (position.x < bounds.m_mins.x) correctedPosition.x = bounds.m_mins.x;
	if (position.x > bounds.m_maxs.x) correctedPosition.x = bounds.m_maxs.x;

	if (position.y < bounds.m_mins.y) correctedPosition.y = bounds.m_mins.y;
	if (position.y > bounds.m_maxs.y) correctedPosition.y = bounds.m_maxs.y;


	return correctedPosition;
}

void FluidSolver::AddForce(Vec3 force)
{
	m_forces += force;
}
