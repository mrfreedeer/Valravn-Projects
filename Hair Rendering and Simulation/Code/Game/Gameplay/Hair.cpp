#include "Game/Gameplay/Hair.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/HairCollision.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/UnorderedAccessBuffer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"


float HairGuide::HairWidth = 0.001f;
float HairGuide::HairSegmentLength = 0.25f;
unsigned int HairGuide::HairSegmentCount = 12;


int HairObject::HairPerSection = 0;
int HairObject::SectionCount = 0;
int HairObject::StackCount = 0;
int HairObject::SliceCount = 0;
float HairObject::Radius = 0;


HairGuide::HairGuide(Vec3 const& position, Rgba8 const& color, Vec3 const& normal, int segmentCount) :
	m_position(position),
	m_color(color),
	m_startNormal(normal),
	m_endNormal(normal),
	m_segmentCount(segmentCount)
{
}

HairGuide::HairGuide(Vec3 const& position, Rgba8 const& color, Vec3 const& startNormal, Vec3 const& endNormal, int segmentCount) :
	m_position(position),
	m_color(color),
	m_startNormal(startNormal),
	m_endNormal(endNormal),
	m_segmentCount(segmentCount)
{
}

void HairGuide::AddVerts(std::vector<Vertex_PNCU>& hairVertexes) const
{
	Vec3 prevPosition = m_position;

	for (int segmentIndex = 0; segmentIndex < m_segmentCount; segmentIndex++) {

		float t = static_cast<float>(segmentIndex) / static_cast<float>(m_segmentCount);

		float nx = RangeMapClamped(t, m_startNormal.x, m_endNormal.x, m_startNormal.x, m_endNormal.x);
		float ny = RangeMapClamped(t, m_startNormal.y, m_endNormal.y, m_startNormal.y, m_endNormal.y);
		float nz = RangeMapClamped(t, m_startNormal.z, m_endNormal.z, m_startNormal.z, m_endNormal.z);

		Vec3 normal(nx, ny, nz);

		// Initializing at 90% of max length to the hair starts in a valid state so it doesn't freak out and explode (mostly for Mass Spring)
		Vec3 hairPosition = prevPosition + (HairGuide::HairSegmentLength * 0.1f * normal);

		hairVertexes.emplace_back(prevPosition, normal, m_color, Vec2::ZERO);
		hairVertexes.emplace_back(hairPosition, normal, m_color, Vec2::ZERO);

		prevPosition = hairPosition;
	}

}

HairObject::HairObject(Game* pointerToGame, HairObjectInit const& initParams) :
	m_initParams(initParams),
	Entity(pointerToGame, initParams.m_startingPosition)
{
	HairConstants* hairConstants = m_game->GetHairConstants();
	IntVec3 GridSize = hairConstants->GridDimensions;
	float width = hairConstants->GridCellWidth;
	float height = hairConstants->GridCellHeight;

	IntVec3 totalCells = IntVec3::ZERO;
	totalCells.x = static_cast<int>(static_cast<float>(GridSize.x) / width) + 1;
	totalCells.y = static_cast<int>(static_cast<float>(GridSize.y) / width) + 1;
	totalCells.z = static_cast<int>(static_cast<float>(GridSize.z) / height) + 1;

	int cellsToCreate = totalCells.x * totalCells.y * totalCells.z;

	m_gridUAV = new UnorderedAccessBuffer(g_theRenderer->m_device, nullptr, (size_t)cellsToCreate, sizeof(Vec4), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);
}

HairObject::~HairObject()
{
	delete m_vertexBuffer;
	delete m_hairPositionUAV;
	delete m_hairPrevPositionUAV;
	delete m_virtualPositionUAV;
	delete m_gridUAV;

	if (m_initParams.m_hairDensityMap) {
		m_initParams.m_hairDensityMap = nullptr;
	}
}

void HairObject::Render() const
{
	Mat44 modelMat = GetModelMatrix();
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->BindShader(m_initParams.m_shader);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetModelMatrix(modelMat);
	g_theRenderer->CopyAndBindModelConstants();


	if (m_initParams.m_usedConstantBuffer) {
		g_theRenderer->CopyCPUToGPU(*m_initParams.m_usedConstantBuffer, sizeof(HairConstants), m_game->m_hairConstantBuffer);
	}
	g_theRenderer->BindConstantBuffer(4, m_game->m_hairConstantBuffer);
	g_theRenderer->CopyAndBindLightConstants();

	g_theRenderer->SetShaderResource(m_hairPositionUAV, 2, ShaderBindBit::SHADER_BIND_GEOMETRY_SHADER);
	//g_theRenderer->Draw((int)m_hairVertexes.size(), 0, TopologyMode::LINELIST);

	//g_theRenderer->BindVertexBuffer(m_vertexBuffer, m_vertexBuffer->GetStride(), TopologyMode::LINELIST);

	g_theRenderer->DrawVertexBuffer(m_vertexBuffer, (int)m_hairVertexes.size(), 0, sizeof(Vertex_PNCU), TopologyMode::LINELIST);
}

// https://www.techiedelight.com/round-next-highest-power-2/
unsigned NextPowerOf2(unsigned n)
{
	// decrement `n` (to handle the case when `n` itself is a power of 2)
	n--;

	// set all bits after the last set bit
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;

	// increment `n` and return
	return ++n;
}

int HairObject::GetThreadsToDispatch(size_t amountOfHairPos) const
{
	int minThreads = (((int)amountOfHairPos / 2) / 64) + 1;
	int powerOf2Threads = (int)NextPowerOf2((unsigned int) minThreads);
	return powerOf2Threads;
}

void HairObject::SetShader(Shader* newShader)
{
	m_initParams.m_shader = newShader;
}

void HairObject::SetMutlInterpShader(Shader* newMultInterpShader)
{
	m_initParams.m_multInterpShader = newMultInterpShader;
}

void HairObject::InitializeHairUAV(HairUAVInfo& uavInitialInfo)
{
	UNUSED(uavInitialInfo);

	std::vector<Vertex_PNCU>const& hairVertexes = *uavInitialInfo.InitialHairData;

	std::vector<HairData> hairPositions;
	hairPositions.reserve((hairVertexes.size() / 2) + 1);

	std::vector<HairData> virtualHairPositions;
	virtualHairPositions.reserve(hairPositions.size());

	for (int hairVertIndex = 0, hairSegmentIndex = 0; hairVertIndex < hairVertexes.size(); hairVertIndex += 2, hairSegmentIndex++) {
		if ((hairSegmentIndex > 0) && ((hairSegmentIndex % HairGuide::HairSegmentCount) == 0)) {
			Vertex_PNCU const& prevVert = hairVertexes[(size_t)hairVertIndex - 1];
			HairData prevHairData = {
				Vec4(prevVert.m_position, 1.0f)
			};

			hairPositions.push_back(prevHairData);
		}
		Vertex_PNCU const& vertex = hairVertexes[hairVertIndex];

		HairData newHairData = {
			Vec4(vertex.m_position, 1.0f)
		};

		hairPositions.push_back(newHairData);
	}

	HairData lastHairData = { Vec4(hairVertexes[hairVertexes.size() - 1].m_position, 1.0f) };
	hairPositions.push_back(lastHairData);

	for (int virtualHairInd = 0; virtualHairInd < hairPositions.size() - 1; virtualHairInd++) {
		Vec4 currentPosition = hairPositions[virtualHairInd].Position;
		Vec4 nextPosition = hairPositions[virtualHairInd + 1].Position;

		Vec3 positionOne = Vec3(currentPosition.x, currentPosition.y, currentPosition.z);
		Vec3 positionTwo = Vec3(nextPosition.x, nextPosition.y, nextPosition.z);
		Mat44 orthoBasis = GetOrthonormalBasis(positionTwo - positionOne);

		Vec3 particlePos = (positionOne + positionTwo) * 0.5f;
		Vec3 kBasis = orthoBasis.GetKBasis3D();
		particlePos += kBasis.GetNormalized() * HairGuide::HairSegmentLength;
		//DebugAddWorldPoint(particlePos + m_position, 0.05f, -1.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

		HairData newVirtualData = {
			Vec4(particlePos, 1.0f)
		};

		virtualHairPositions.push_back(newVirtualData);
	}

	uavInitialInfo.Out_HairPositions = new UnorderedAccessBuffer(g_theRenderer->m_device, hairPositions.data(), hairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);
	uavInitialInfo.Out_PreviousHairPositions = new UnorderedAccessBuffer(g_theRenderer->m_device, hairPositions.data(), hairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);
	uavInitialInfo.Out_VirtualHairPositions = new UnorderedAccessBuffer(g_theRenderer->m_device, virtualHairPositions.data(), virtualHairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);

	g_theRenderer->BindShader(m_game->m_copyShader);
	Mat44 modelMat = GetModelMatrix();

	int numThreadsToDispatch = GetThreadsToDispatch(hairPositions.size());

	g_theRenderer->SetModelMatrix(modelMat);
	g_theRenderer->CopyAndBindModelConstants();
	g_theRenderer->SetUAV(uavInitialInfo.Out_HairPositions);
	g_theRenderer->DispatchCS(numThreadsToDispatch, 1, 1);
	g_theRenderer->SetUAV(uavInitialInfo.Out_VirtualHairPositions);
	g_theRenderer->DispatchCS(numThreadsToDispatch, 1, 1);
	g_theRenderer->ClearState();
}

void HairObject::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	if (g_simulateHair) {
		int threadsSimul = ((static_cast<int>(m_hairVertexes.size()) / HairGuide::HairSegmentCount) / 64) + 1;

		if (m_initParams.m_simulationShader) {
			g_theRenderer->BindShader(m_initParams.m_simulationShader);
		}
		else {
			g_theRenderer->BindShader(m_game->GetSimulationShader());
		}
		g_theRenderer->SetModelMatrix(GetModelMatrix());
		g_theRenderer->CopyAndBindModelConstants();

		if (m_initParams.m_usedConstantBuffer) {
			g_theRenderer->CopyCPUToGPU(*m_initParams.m_usedConstantBuffer, sizeof(HairConstants), m_game->m_hairConstantBuffer);
		}

		g_theRenderer->BindConstantBuffer(4, m_game->m_hairConstantBuffer);
		g_theRenderer->BindConstantBuffer(5, GetHairCollisionBuffer());
		g_theRenderer->SetUAV(m_hairPositionUAV, 0);
		if (m_hairPrevPositionUAV) {
			g_theRenderer->SetUAV(m_hairPrevPositionUAV, 1);
		}
		if (m_virtualPositionUAV) {
			g_theRenderer->SetUAV(m_virtualPositionUAV, 2);
		}
		if (m_gridUAV) {
			g_theRenderer->SetUAV(m_gridUAV, 3);
		}
		g_theRenderer->DispatchCS(threadsSimul, 1, 1);
		g_theRenderer->SetUAV(nullptr);
		g_theRenderer->ClearState();

		//if (m_gridUAV) {
		//	g_theRenderer->BindShader(m_game->GetClearingUAVShader());
		//	g_theRenderer->SetUAV(m_gridUAV);
		//	IntVec3 gridDimensions = m_game->GetHairConstants()->GridDimensions;
		//	int threadsToDispatch = gridDimensions.x * gridDimensions.y * gridDimensions.z;
		//	//threadsToDispatch k 64;
		//	threadsToDispatch++;
		//	g_theRenderer->DispatchCS(threadsToDispatch, 1, 1);
		//	g_theRenderer->ClearState();
		//}

	}

}

HairSimulGuide::HairSimulGuide(HairSimulationInit const& hairSimulationInitial) :
	m_hairSimulationInitParams(hairSimulationInitial)
{
	m_positions.push_back(m_hairSimulationInitParams.position);
	m_segmentLength = HairGuide::HairSegmentLength;

	int const& segmentCount = m_hairSimulationInitParams.segmentCount;

	m_velocities.resize(m_hairSimulationInitParams.segmentCount);
	for (int segmentIndex = 1; segmentIndex < segmentCount; segmentIndex++) {
		Vec3 const& prevPos = m_positions[m_positions.size() - 1];
		Vec3 nextPos = prevPos + m_segmentLength * m_hairSimulationInitParams.normal;
		m_positions.push_back(nextPos);
	}
	//	m_velocities[m_velocities.size() - 1] = Vec3(0.0f, 0.0f, -0.01f);

	m_virtualParticleVelocities.resize(segmentCount - 1);
	m_virtualParticleHalfVelocities.resize(segmentCount - 1);
	m_halfVelocities.resize(segmentCount);
	//m_forces.resize(m_segmentCount);

	m_bendInitialLength = sqrtf(m_segmentLength * m_segmentLength * 2);
	m_torsionInitialLength = m_bendInitialLength * 2.0f;

	m_virtualParticlePositions.resize(segmentCount - 1); // Initializing virtual particle positions
	for (int segmentIndex = 0; segmentIndex < (segmentCount - 1); segmentIndex++) {
		m_virtualParticlePositions[segmentIndex] = GetVirtualParticlePos(segmentIndex);
	}
}

void HairSimulGuide::AddVerts(std::vector<Vertex_PNCU>& hairVertexes) const
{
	int const& segmentCount = m_hairSimulationInitParams.segmentCount;
	Vec3 prevSegment = m_positions[0];
	for (int segmentIndex = 1; segmentIndex < segmentCount; segmentIndex++) {
		Vec3 const& currentSegment = m_positions[segmentIndex];
		Vertex_PNCU prevVert = Vertex_PNCU(prevSegment, Vec3::ZERO, Rgba8::WHITE, Vec2::ZERO);
		Vertex_PNCU currentVert = Vertex_PNCU(currentSegment, Vec3::ZERO, Rgba8::WHITE, Vec2::ZERO);

		hairVertexes.push_back(prevVert);
		hairVertexes.push_back(currentVert);

		prevSegment = currentSegment;
	}

	if (!m_hairSimulationInitParams.isCurlyHair) {
		for (int partIndex = 0; partIndex < m_virtualParticlePositions.size(); partIndex++) {
			Vec3 const& particlePos = m_virtualParticlePositions[partIndex];
			DebugAddWorldPoint(particlePos, 0.025f, 0.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH, 8, 16);
		}
	}

}

void HairSimulGuide::Update(float deltaSeconds)
{
	switch (m_hairSimulationInitParams.usedAlgorithm)
	{
	default:
		ERROR_AND_DIE("UNSUPPORTED HAIR SIMULATION ALGORITHM");
		break;
	case SimulAlgorithm::DFTL:
		UpdateDFTL(deltaSeconds);
		break;
	case SimulAlgorithm::MASS_SPRINGS:
		UpdateMassSprings(deltaSeconds);
		break;
	}


	m_genForce = Vec3::ZERO;

}

void HairSimulGuide::AddForce(Vec3 force)
{
	m_genForce += force;
}

void HairSimulGuide::SetSpringStiffness(float const& edgeStiffness, float const& bendStiffness, float const& torsionStiffness)
{
	m_hairSimulationInitParams.edgeStiffness = edgeStiffness;
	m_hairSimulationInitParams.bendStiffness = bendStiffness;
	m_hairSimulationInitParams.torsionStiffness = torsionStiffness;
}

void HairSimulGuide::GetSpringStiffness(float& edgeStiffness, float& bendStiffness, float& torsionStiffness) const
{
	edgeStiffness = m_hairSimulationInitParams.edgeStiffness;
	bendStiffness = m_hairSimulationInitParams.bendStiffness;
	torsionStiffness = m_hairSimulationInitParams.torsionStiffness;
}

void HairSimulGuide::SetSpringLengths(float const& edgeLength, float const& bendLength, float const& torsionLength)
{
	m_segmentLength = edgeLength;
	m_bendInitialLength = bendLength;
	m_torsionInitialLength = torsionLength;
}

void HairSimulGuide::GetSpringLengths(float& edgeLength, float& bendLength, float& torsionLength) const
{
	edgeLength = m_segmentLength;
	bendLength = m_bendInitialLength;
	torsionLength = m_torsionInitialLength;
}

//Vec3 HairSimulGuide::CalculateHalfVelocity(float deltaSeconds)
//{
//
//	Mat44 ident = Mat44();
//
//	float halfDelta = deltaSeconds * 0.5f;
//
//	float oneOverMass = 1.0f / m_hairSimulationInitParams.mass;
//
//	Mat44 mass = Mat44(Vec3(oneOverMass, 0.0f, 0.0), Vec3(0.0f, oneOverMass, 0.0), Vec3(0.0f, 0.0, oneOverMass), Vec3::ZERO);
//	Mat44 massAndStiffness = Mat44(mass);
//
//	mass.AppendScaleUniform3D(halfDelta);
//	Mat44 stiffness = Mat44(Vec3(m_hairSimulationInitParams.stiffness, 0.0f, 0.0f), Vec3(0.0f, m_stiffness, 0.0f), Vec3(0.0f, 0.0f, m_stiffness), Vec3::ZERO);
//
//
//	float  deltaSqrQrt = (deltaSeconds * deltaSeconds) * 0.25f;
//
//
//	massAndStiffness.Append(stiffness);
//
//	ident.Add(mass);
//
//	Mat44 massAndStiffnessDeltaQuart = Mat44(massAndStiffness);
//	Mat44 otherMassAndStiff = Mat44(massAndStiffness);
//	massAndStiffnessDeltaQuart.AppendScaleUniform3D(deltaSqrQrt);
//
//	ident.Add(massAndStiffnessDeltaQuart);
//
//	Mat44 inverseTerm = ident.GetInverted();
//
//	otherMassAndStiff.AppendScaleUniform3D(-halfDelta);
//	Mat44 dispMat = inverseTerm;
//	dispMat.Append(otherMassAndStiff);
//
//
//	for (int index = 1; index < m_segmentCount; index++) {
//		Vec3 currentPos = m_positions[index];
//
//		currentPos = dispMat.RightAppendVectorQuantity3D(currentPos);
//		Vec3 velocityTransf = m_velocities[index];
//		velocityTransf = inverseTerm.RightAppendVectorQuantity3D(velocityTransf);
//		m_halfVelocities[index] = velocityTransf - currentPos;
//
//	}
//
//	for (int index = 1; index < m_segmentCount; index++) {
//		Vec3 currentPos = m_positions[index];
//
//		m_positions[index] = currentPos + deltaSeconds * m_halfVelocities[index];
//
//	}
//
//
//	return Vec3::ONE;
//}

void HairSimulGuide::CalculateRestitutionForces(float deltaSeconds, bool useHalfPosition)
{
	if (m_hairSimulationInitParams.isCurlyHair) {
		CalculateRestitutionForcesCurly(deltaSeconds, useHalfPosition);
	}
	else {
		CalculateRestitutionForcesStraight(deltaSeconds, useHalfPosition);
	}

}

void HairSimulGuide::CalculateRestitutionForcesCurly(float deltaSeconds, bool useHalfPosition)
{
	float const& edgeStiffness = m_hairSimulationInitParams.edgeStiffness;
	float const& bendStiffness = m_hairSimulationInitParams.bendStiffness;
	float const& torsionStiffness = m_hairSimulationInitParams.torsionStiffness;

	int const& segmentCount = m_hairSimulationInitParams.segmentCount;


	for (int index = 0; index < segmentCount - 1; index++) {
		Vec3 posOne = m_positions[index];
		Vec3 posTwo = m_positions[index + 1];

		bool calculateBend = ((index + 2) < (segmentCount));
		bool calculateTorsion((index + 3) < (segmentCount));

		Vec3 posTwoBend = Vec3::ZERO;
		Vec3 velDiffBend = Vec3::ZERO;

		Vec3 posTwoTorsion = Vec3::ZERO;
		Vec3 velDiffTorsion = Vec3::ZERO;

		if (useHalfPosition) {
			posOne = m_positions[index] + m_prevPositions[index];
			posOne *= 0.5f;

			posTwo = m_positions[index + 1] + m_prevPositions[index + 1];
			posTwo *= 0.5f;

			if (calculateBend) {
				posTwoBend = m_positions[index + 2] + m_prevPositions[index + 2];
				posTwoBend *= 0.5f;
			}
			if (calculateTorsion) {
				posTwoTorsion = m_positions[index + 3] + m_prevPositions[index + 3];
				posTwoTorsion *= 0.5f;
			}
		}

		Vec3 disp = posTwo - posOne;
		Vec3 velDiff = m_velocities[index + 1] - m_velocities[index];


		Vec3 resultingForce = GetSpringForce(deltaSeconds, disp, velDiff, edgeStiffness, m_segmentLength);
		m_forces[index] += resultingForce * 0.5f;
		m_forces[index + 1] += -resultingForce * 0.5f;

		if (calculateBend) {
			if (!useHalfPosition) posTwoBend = m_positions[index + 2];
			Vec3 dispBend = posTwoBend - posOne;
			velDiffBend = m_velocities[index + 2] - m_velocities[index];
			Vec3 resultingForceBend = GetSpringForce(deltaSeconds, dispBend, velDiffBend, bendStiffness, m_bendInitialLength);
			m_forces[index] += resultingForceBend * 0.5f;
			m_forces[index + 2] += -resultingForceBend * 0.5f;
		}

		if (calculateTorsion) {
			if (!useHalfPosition)posTwoTorsion = m_positions[index + 3];
			Vec3 dispTorsion = posTwoTorsion - posOne;
			velDiffTorsion = m_velocities[index + 3] - m_velocities[index];
			Vec3 resultingForceTorsion = GetSpringForce(deltaSeconds, dispTorsion, velDiffTorsion, torsionStiffness, m_torsionInitialLength);
			m_forces[index] += resultingForceTorsion * 0.5f;
			m_forces[index + 3] += -resultingForceTorsion * 0.5f;
		}



	}
}


Vec3 const HairSimulGuide::GetVirtualParticlePos(int startIndex) const
{
	Vec3 posOne = m_positions[startIndex];
	Vec3 posTwo = m_positions[startIndex + 1];

	Mat44 orthoBasis = GetOrthonormalBasis(posTwo - posOne);

	Vec3 particlePos = (posTwo + posOne) * 0.5f;
	Vec3 kBasis = orthoBasis.GetKBasis3D();
	particlePos += kBasis.GetNormalized() * m_segmentLength;


	return particlePos;
}

void HairSimulGuide::CalculateRestitutionForcesStraight(float deltaSeconds, bool useHalfPosition)
{
	float const& edgeStiffness = m_hairSimulationInitParams.edgeStiffness;
	float const& bendStiffness = m_hairSimulationInitParams.bendStiffness;
	float const& torsionStiffness = m_hairSimulationInitParams.torsionStiffness;
	int const& segmentCount = m_hairSimulationInitParams.segmentCount;


	for (int index = 0; index < segmentCount - 1; index++) {
		Vec3 posOne = m_positions[index];
		Vec3 posTwo = m_positions[index + 1];

		bool calculateBend = ((index + 2) < (segmentCount));
		bool calculateTorsion((index + 3) < (segmentCount));

		Vec3 posTwoBend = Vec3::ZERO;
		Vec3 velDiffBend = Vec3::ZERO;

		Vec3 posTwoTorsion = Vec3::ZERO;
		Vec3 velDiffTorsion = Vec3::ZERO;

		if (useHalfPosition) {
			posOne = m_positions[index] + m_prevPositions[index];
			posOne *= 0.5f;

			posTwo = m_positions[index + 1] + m_prevPositions[index + 1];
			posTwo *= 0.5f;

			if (calculateBend) {
				posTwoBend = m_positions[index + 2] + m_prevPositions[index + 2];
				posTwoBend *= 0.5f;
			}
			if (calculateTorsion) {
				posTwoTorsion = m_positions[index + 3] + m_prevPositions[index + 3];
				posTwoTorsion *= 0.5f;
			}
		}

		Vec3 disp = posTwo - posOne;
		Vec3 velDiff = m_velocities[index + 1] - m_velocities[index];


		Vec3 resultingForce = GetSpringForce(deltaSeconds, disp, velDiff, edgeStiffness, m_segmentLength);
		m_forces[index] += resultingForce * 0.5f;
		m_forces[index + 1] += -resultingForce * 0.5f;

		if (calculateBend) {
			if (!useHalfPosition) posTwoBend = m_positions[index + 2];
			Vec3 dispBend = posTwoBend - posOne;
			velDiffBend = m_velocities[index + 2] - m_velocities[index];
			Vec3 resultingForceBend = GetSpringForce(deltaSeconds, dispBend, velDiffBend, bendStiffness, m_bendInitialLength);
			m_forces[index] += resultingForceBend * 0.5f;
			m_forces[index + 2] += -resultingForceBend * 0.5f;
		}

	}


	// Fake particles
	for (int partIndex = 0; partIndex < (segmentCount - 1); partIndex++) { // There's one less that the total segment count

		bool calculateNextPart = (partIndex + 1) < m_virtualParticlePositions.size();
		bool calculateTorsion = (partIndex - 1) > 0;

		Vec3 particlePos = m_virtualParticlePositions[partIndex];
		Vec3 nextParticlePos = (calculateNextPart) ? m_virtualParticlePositions[partIndex + 1] : Vec3::ZERO;

		Vec3 previousHairPos = (calculateTorsion) ? m_positions[partIndex - 1] : Vec3::ZERO;
		Vec3 currHairPos = m_positions[partIndex];
		Vec3 nextHairPos = m_positions[partIndex + 1];

		if (useHalfPosition) {
			currHairPos += m_prevPositions[partIndex];
			currHairPos *= 0.5f;

			nextHairPos += m_prevPositions[partIndex];
			nextHairPos *= 0.5f;

			//particlePos += m_virtualParticlePrevPositions[partIndex];
			//particlePos *= 0.5f;

			//nextParticlePos += (calculateNextPart) ? m_virtualParticlePrevPositions[partIndex + 1] : Vec3::ZERO;
			//nextParticlePos *= 0.5f;

			previousHairPos += (calculateTorsion) ? m_prevPositions[partIndex - 1] : Vec3::ZERO;
			previousHairPos *= 0.5f;
		}

		// waRestitution for Current Hair Pos
		Vec3 dispToCurr = particlePos - currHairPos;
		Vec3 velocityDeltaCurr = m_virtualParticleVelocities[partIndex] - m_velocities[partIndex];

		Vec3 forceCurrHair = GetSpringForce(deltaSeconds, dispToCurr, velocityDeltaCurr, edgeStiffness, m_segmentLength);
		m_forces[partIndex] += forceCurrHair * 0.5f;
		m_forcesParticles[partIndex] += -forceCurrHair * 0.5f;

		Vec3 dispToNext = nextHairPos - particlePos;
		Vec3 velocityDeltaNext = m_virtualParticleVelocities[partIndex] - m_velocities[partIndex + 1];

		Vec3 forceNextHair = GetSpringForce(deltaSeconds, dispToNext, velocityDeltaNext, edgeStiffness, m_segmentLength);
		m_forcesParticles[partIndex] += forceNextHair * 0.5f;
		m_forces[partIndex + 1] += -forceNextHair * 0.5f;

		if (calculateNextPart) {
			Vec3 dispNextPart = nextParticlePos - particlePos;
			Vec3 velocityDelta = m_virtualParticleVelocities[partIndex + 1] - m_virtualParticleVelocities[partIndex];
			Vec3 forceNextPart = GetSpringForce(deltaSeconds, dispNextPart, velocityDelta, bendStiffness, m_bendInitialLength);

			m_forcesParticles[partIndex] += forceNextPart * 0.5f;
			m_forcesParticles[partIndex + 1] += -forceNextPart * 0.5f;
		}

		if (calculateTorsion) {
			Vec3 dispTorsion = particlePos - previousHairPos;
			Vec3 velocityDeltaTorsion = m_virtualParticleVelocities[partIndex] - m_velocities[partIndex - 1];
			Vec3 forceTorsion = GetSpringForce(deltaSeconds, dispTorsion, velocityDeltaTorsion, torsionStiffness, m_torsionInitialLength);

			m_forces[partIndex - 1] += forceTorsion * 0.5f;
			m_forcesParticles[partIndex] += -forceTorsion * 0.5f;
		}


	}

}

Vec3 HairSimulGuide::GetSpringForce(float deltaSeconds, Vec3 const& displacement, Vec3 const& velocityDelta, float stiffness, float initialLength)
{
	float const& damping = m_hairSimulationInitParams.damping;

	Vec3 dir = displacement.GetNormalized();

	float stiffOverInitial = stiffness / initialLength;


	float firstTermCoeff = DotProduct3D(displacement, dir);
	firstTermCoeff -= m_segmentLength;
	firstTermCoeff *= (stiffOverInitial);
	Vec3 firstTerm = dir * firstTermCoeff;


	float firstConst = (deltaSeconds * stiffness / (m_segmentLength + damping));

	Vec3 secondTerm = DotProduct3D(velocityDelta, dir) * dir;
	secondTerm *= firstConst;

	Vec3 resultingForce = firstTerm + secondTerm;

	return resultingForce;
}

void HairSimulGuide::UpdateDFTL(float deltaSeconds)
{
	float deltaSqr = deltaSeconds * deltaSeconds;
	m_genForce += Vec3(0.0f, 0.0f, m_gravity);
	for (int positionInd = 1; positionInd < m_hairSimulationInitParams.segmentCount; positionInd++) {
		Vec3 prevPos = m_positions[positionInd - 1];

		Vec3 newPos = m_positions[positionInd] + deltaSeconds * m_velocities[positionInd] + deltaSqr * m_genForce;

		Vec3 distToCurrentPos = newPos - prevPos;

		distToCurrentPos.ClampLength(m_segmentLength);
		Vec3 correctedPos = prevPos + distToCurrentPos;
		Vec3 correctionVec = correctedPos - newPos;
		newPos = correctedPos;

		Vec3 firstTerm = (newPos - m_positions[positionInd]) / deltaSeconds;
		Vec3 secondTerm = 0.9f * (-correctionVec) / deltaSeconds;

		m_velocities[positionInd] = firstTerm + secondTerm;
		m_positions[positionInd] = newPos;
	}

}

void HairSimulGuide::UpdateMassSprings(float deltaSeconds)
{
	int const& segmentCount = m_hairSimulationInitParams.segmentCount;
	bool const& isCurly = m_hairSimulationInitParams.isCurlyHair;

	//for (int segmentIndex = 0; segmentIndex < (segmentCount - 1); segmentIndex++) {
	//	m_virtualParticlePositions[segmentIndex] = GetVirtualParticlePos(segmentIndex);
	//}
	m_forces.clear();
	m_forces.resize(segmentCount);
	m_forcesParticles.clear();
	m_forcesParticles.resize(segmentCount - 1);

	CalculateRestitutionForces(deltaSeconds);
	//CalculateHalfVelocity(deltaSeconds);
	m_genForce += Vec3(0.0f, 0.0f, m_gravity);

	for (int index = 1; index < segmentCount; index++) {
		Vec3 const& velocity = m_velocities[index];
		m_forces[index] += m_genForce;

		m_halfVelocities[index] = velocity + deltaSeconds * 0.5f * m_forces[index] / m_hairSimulationInitParams.mass;

	}

	if (!isCurly) {
		for (int index = 0; index < segmentCount - 1; index++) {
			Vec3 const& velocity = m_virtualParticleVelocities[index];
			//m_forcesParticles[index] += m_genForce;

			m_virtualParticleHalfVelocities[index] = velocity + deltaSeconds * 0.5f * m_forcesParticles[index] / m_hairSimulationInitParams.mass;

		}
	}

	m_prevPositions.resize(m_positions.size());
	m_prevPositions[0] = m_positions[0];

	for (int posIndex = 1; posIndex < segmentCount; posIndex++) {
		m_prevPositions[posIndex] = m_positions[posIndex];
		m_positions[posIndex] = m_positions[posIndex] + deltaSeconds * m_halfVelocities[posIndex];
	}

	if (!isCurly) {
		m_virtualParticlePrevPositions.resize(m_virtualParticlePositions.size());
		for (int posIndex = 0; posIndex < m_virtualParticlePositions.size(); posIndex++) {
			m_virtualParticlePrevPositions[posIndex] = m_virtualParticlePositions[posIndex];
			m_virtualParticlePositions[posIndex] = m_virtualParticlePositions[posIndex] + deltaSeconds * m_virtualParticleHalfVelocities[posIndex];
		}
	}

	m_forces.clear();
	m_forces.resize(segmentCount);
	m_forcesParticles.clear();
	m_forcesParticles.resize(segmentCount - 1);

	//for (int segmentIndex = 0; segmentIndex < (segmentCount - 1); segmentIndex++) {
	//	m_virtualParticlePositions[segmentIndex] = GetVirtualParticlePos(segmentIndex);
	//}

	CalculateRestitutionForces(deltaSeconds, true);
	for (int index = 1; index < segmentCount; index++) {
		Vec3 velocity = m_velocities[index];
		m_forces[index] += m_genForce;
		m_halfVelocities[index] = velocity + deltaSeconds * 0.5f * m_forces[index] / m_hairSimulationInitParams.mass;

	}

	for (int index = 1; index < segmentCount; index++) {

		m_velocities[index] = 2.0f * m_halfVelocities[index] - m_velocities[index];

	}

	if (!isCurly) {
		for (int index = 0; index < segmentCount - 1; index++) {
			Vec3 const& velocity = m_virtualParticleVelocities[index];
			//m_forcesParticles[index] += m_genForce;

			m_virtualParticleHalfVelocities[index] = velocity + deltaSeconds * 0.5f * m_forcesParticles[index] / m_hairSimulationInitParams.mass;

		}


		for (int index = 0; index < m_virtualParticleVelocities.size(); index++) {

			m_virtualParticleVelocities[index] = 2.0f * m_virtualParticleHalfVelocities[index] - m_virtualParticleVelocities[index];

		}
	}

}

