#include "Game/Gameplay/HairDisc.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/StreamOutputBuffer.hpp"
#include "Engine/Renderer/UnorderedAccessBuffer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Core/Time.hpp"
#include "ThirdParty/Squirrel/SmoothNoise.cpp"




HairDisc::HairDisc(Game* pointerToGame, HairObjectInit const& initParams) :
	HairObject(pointerToGame, initParams)
{
	m_hairPerSection = HairObject::HairPerSection;
	m_sectionCount = HairObject::SectionCount;
	m_radius = HairObject::Radius;
	CreateHair();
	InitializeHairUAV();
}

HairDisc::~HairDisc()
{
}

void HairDisc::CreateHair()
{
	float degreesPerSection = 360.0f / static_cast<float>(m_sectionCount);
	float angle = 0.0f;

	int expectedHairAmount = m_hairPerSection * m_sectionCount;
	int expectedVertexAmount = expectedHairAmount * 12 * 2;
	m_hairs.reserve(expectedHairAmount);
	m_hairVertexes.reserve(expectedVertexAmount);

	for (int sectionIndex = 0; sectionIndex < m_sectionCount; sectionIndex++, angle += degreesPerSection) {

		float nextAngle = angle + degreesPerSection;

		for (int hairIndex = 0; hairIndex < m_hairPerSection; hairIndex++) {
			float hairAngle = rng.GetRandomFloatInRange(angle, nextAngle);
			float hairRadius = rng.GetRandomFloatInRange(0.0f, m_radius);

			float cosDeg = CosDegrees(hairAngle);
			float sinDeg = SinDegrees(hairAngle);

			Vec3 hairPosition = Vec3(cosDeg, sinDeg, 0.0f);
			hairPosition *= hairRadius;

			float timeNow = static_cast<float>(GetCurrentTimeSeconds());

			float randX = Compute1dPerlinNoise(timeNow, 1.0f, 2, 0.5f, 0.1f, true, 0);
			float randY = Compute1dPerlinNoise(timeNow + 1.0f, 1.0f, 2, 0.5f, 0.1f, true, 0);
			float randZ = Compute1dPerlinNoise(timeNow + 2.0f, 1.0f, 2, 0.5f, 0.1f, true, 0);
			float randDeg = Compute1dPerlinNoise(timeNow + 2.0f, 1.0f, 2, 0.5f, 0.1f, true, 0);

			//float randX = rng.GetRandomFloatInRange(-1.0f, 1.0f);
			//float randY = rng.GetRandomFloatInRange(-1.0f, 1.0f);
			//float randZ = rng.GetRandomFloatInRange(-1.0f, 1.0f);

			Vec3 randDir = Vec3(randX, randY, randZ);
			randDir = CrossProduct3D(randDir.GetNormalized(), Vec3(0.0f, 0.0f, 1.0f));

			randDir.GetRotatedAroundAxis(randDeg, Vec3(0.0f, 0.0f, 1.0f));


			Vec3 normal = Vec3(0.0f, 0.0f, 1.0f) + randDir * 0.7f;
			//Vec3 normal = randDir.GetNormalized;
			normal.Normalize();

			normal = (m_game->m_randomizeHairDir) ? normal : Vec3(0.0f, 0.0f, 1.0f);

			HairGuide newHair = HairGuide(hairPosition, Rgba8::WHITE, normal, HairGuide::HairSegmentCount);
			m_hairs.emplace_back(newHair);
			newHair.AddVerts(m_hairVertexes);
			m_hairCount++;
		}
	}

	m_vertexBuffer = new VertexBuffer(g_theRenderer->m_device, sizeof(Vertex_PNCU) * m_hairVertexes.size(), sizeof(Vertex_PNCU));
	g_theRenderer->CopyCPUToGPU(m_hairVertexes.data(), sizeof(Vertex_PNCU) * m_hairVertexes.size(), m_vertexBuffer);
}

void HairDisc::InitializeHairUAV()
{
	std::vector<HairData> hairPositions;
	hairPositions.reserve((m_hairVertexes.size() / 2) + 1);

	for (int hairVertIndex = 0, hairSegmentIndex = 0; hairVertIndex < m_hairVertexes.size(); hairVertIndex += 2, hairSegmentIndex++) {
		if ((hairSegmentIndex > 0) && ((hairSegmentIndex % HairGuide::HairSegmentCount) == 0)) {
			Vertex_PNCU const& prevVert = m_hairVertexes[(size_t)hairVertIndex - 1];
			HairData prevHairData = {
				Vec4(prevVert.m_position, 1.0f)
			};

			hairPositions.push_back(prevHairData);
		}
		Vertex_PNCU const& vertex = m_hairVertexes[hairVertIndex];

		HairData newHairData = {
			Vec4(vertex.m_position, 1.0f)
		};

		hairPositions.push_back(newHairData);
	}

	HairData lastHairData = { Vec4(m_hairVertexes[m_hairVertexes.size() - 1].m_position, 1.0f) };
	hairPositions.push_back(lastHairData);

	int numThreadsToDispatch = GetThreadsToDispatch(m_hairVertexes.size());

	m_hairPositionUAV = new UnorderedAccessBuffer(g_theRenderer->m_device, hairPositions.data(), hairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);


	g_theRenderer->BindShader(m_game->m_copyShader);
	Mat44 modelMat = GetModelMatrix();

	g_theRenderer->SetModelMatrix(modelMat);
	g_theRenderer->CopyAndBindModelConstants();
	g_theRenderer->SetUAV(m_hairPositionUAV);
	g_theRenderer->DispatchCS(numThreadsToDispatch, 1, 1);
	g_theRenderer->SetUAV(nullptr);
	g_theRenderer->ClearState();

}

void HairDiscTessellation::Render() const
{
	Mat44 modelMat = GetModelMatrix();
	//g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->BindShader(m_initParams.m_shader);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetModelMatrix(modelMat);
	g_theRenderer->CopyAndBindModelConstants();
	g_theRenderer->SetShaderResource(m_hairPositionUAV, 2, ShaderBindBit::SHADER_BIND_GEOMETRY_SHADER);
	g_theRenderer->BindVertexBuffer(m_vertexBuffer, m_vertexBuffer->GetStride(), TopologyMode::CONTROL_POINT_PATCHLIST_2);
	g_theRenderer->DrawVertexBuffer(m_vertexBuffer, (int)m_hairVertexes.size(), 0, sizeof(Vertex_PNCU), TopologyMode::CONTROL_POINT_PATCHLIST_2);
}

HairDiscTessellation::HairDiscTessellation(Game* pointerToGame, HairObjectInit const& initParams) :
	HairDisc(pointerToGame, initParams)
{
}

HairDiscTessellation::~HairDiscTessellation()
{
}
