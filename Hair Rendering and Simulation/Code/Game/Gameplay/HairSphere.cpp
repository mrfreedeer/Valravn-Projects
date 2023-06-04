#include "Game/Gameplay/HairSphere.hpp"
#include "ThirdParty/Squirrel/SmoothNoise.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/UnorderedAccessBuffer.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/HairCollision.hpp"


HairSphere::HairSphere(Game* pointerToGame, HairObjectInit const& initParams) :
	HairObject(pointerToGame, initParams)
{
	m_stackCount = HairObject::StackCount;
	m_sliceCount = HairObject::SliceCount;
	m_radius = HairObject::Radius;
	m_hairPerSection = HairObject::HairPerSection;

	CreateHair();

	HairUAVInfo hairUAVInfo = {};
	hairUAVInfo.InitialHairData = &m_hairVertexes;

	HairObject::InitializeHairUAV(hairUAVInfo);
	m_hairPositionUAV = hairUAVInfo.Out_HairPositions;
	m_hairPrevPositionUAV = hairUAVInfo.Out_PreviousHairPositions;
	m_virtualPositionUAV = hairUAVInfo.Out_VirtualHairPositions;

	//InitializeHairUAV();

}

HairSphere::~HairSphere()
{
	delete m_sphereBuffer;
	delete m_multInterpBuffer;
}

void HairSphere::CreateHair()
{
	//Rgba8 color = Rgba8::WHITE;
	Rgba8 color = Rgba8(85, 54, 21);

	float yawDegDelta = 360.0f / static_cast<float>(m_sliceCount);
	float pitchDegDelta = 180.0f / static_cast<float>(m_stackCount);

	float prevCosYaw = -1.0f;
	float prevSinYaw = -1.0f;

	float prevCosPitch = -1.0f;
	float prevSinPitch = -1.0f;

	for (float yaw = yawDegDelta; yaw <= 360.0f; yaw += yawDegDelta) {
		float cosYaw = CosDegrees(yaw);
		float sinYaw = SinDegrees(yaw);

		if (yaw == yawDegDelta) {
			prevCosYaw = CosDegrees(yaw - yawDegDelta);
			prevSinYaw = SinDegrees(yaw - yawDegDelta);
		}

		for (float pitch = -90.0f + pitchDegDelta; pitch <= 90.0f; pitch += pitchDegDelta) {
			float cosPitch = CosDegrees(pitch);
			float sinPitch = SinDegrees(pitch);

			if (pitch == -90.0f + pitchDegDelta) {
				prevCosPitch = CosDegrees(pitch - pitchDegDelta);
				prevSinPitch = SinDegrees(pitch - pitchDegDelta);
			}

			Vec3 rightBottomPos(cosYaw * cosPitch, sinYaw * cosPitch, -sinPitch);
			Vec3 rightTopPos(cosYaw * prevCosPitch, sinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftTopPos(prevCosYaw * prevCosPitch, prevSinYaw * prevCosPitch, -prevSinPitch);
			Vec3 leftBottomPos(prevCosYaw * cosPitch, prevSinYaw * cosPitch, -sinPitch);

			rightBottomPos *= m_radius;
			rightTopPos *= m_radius;
			leftTopPos *= m_radius;
			leftBottomPos *= m_radius;

			float leftU = GetFractionWithin(yaw - yawDegDelta, 0.0f, 360.0f);
			float rightU = GetFractionWithin(yaw, 0.0f, 360.0f);
			float topV = GetFractionWithin(pitch - pitchDegDelta, 90.0f, -90.0f);
			float bottomV = GetFractionWithin(pitch, 90.0f, -90.0f);

			m_sphereVertexes.emplace_back(leftBottomPos, leftBottomPos.GetNormalized(), color, Vec2(leftU, bottomV));
			m_sphereVertexes.emplace_back(rightBottomPos, rightBottomPos.GetNormalized(), color, Vec2(rightU, bottomV));
			m_sphereVertexes.emplace_back(rightTopPos, rightTopPos.GetNormalized(), color, Vec2(rightU, topV));

			m_sphereVertexes.emplace_back(leftBottomPos, leftBottomPos.GetNormalized(), color, Vec2(leftU, bottomV));
			m_sphereVertexes.emplace_back(rightTopPos, rightTopPos.GetNormalized(), color, Vec2(rightU, topV));
			m_sphereVertexes.emplace_back(leftTopPos, leftTopPos.GetNormalized(), color, Vec2(leftU, topV));

			/*HairGuide borderHair = HairGuide(leftBottomPos, Rgba8::BLUE, leftBottomPos.GetNormalized(), HairObject::SectionCount);
			m_hairs.push_back(borderHair);
			borderHair.AddVerts(m_hairVertexes);

			borderHair = HairGuide(rightTopPos, Rgba8::BLUE, rightTopPos.GetNormalized(), HairObject::SectionCount);
			m_hairs.push_back(borderHair);
			borderHair.AddVerts(m_hairVertexes);*/

			float avgDensity = 0.0f;
			if (m_initParams.m_hairDensityMap) {

				Rgba8 bottomLeftDensity = m_initParams.m_hairDensityMap->GetTexelColor(leftU, bottomV);
				Rgba8 bottomRightDensity = m_initParams.m_hairDensityMap->GetTexelColor(rightU, bottomV);
				Rgba8 topLeftDensity = m_initParams.m_hairDensityMap->GetTexelColor(leftU, topV);
				Rgba8 topRightDensity = m_initParams.m_hairDensityMap->GetTexelColor(rightU, topV);

				avgDensity += NormalizeByte(bottomLeftDensity.r);
				avgDensity += NormalizeByte(bottomRightDensity.r);
				avgDensity += NormalizeByte(topLeftDensity.r);
				avgDensity += NormalizeByte(topRightDensity.r);
				avgDensity *= 0.25f;
			}
			else {
				avgDensity = 1.0f;
			}

			Rgba8 bottomLeftColor = color;
			Rgba8 bottomRightColor = color;
			Rgba8 topLeftColor = color;
			Rgba8 topRightColor = color;

			if (m_initParams.m_hairDiffuseMap) {
				bottomLeftColor = m_initParams.m_hairDiffuseMap->GetTexelColor(leftU, bottomV);
				bottomRightColor = m_initParams.m_hairDiffuseMap->GetTexelColor(rightU, bottomV);
				topLeftColor = m_initParams.m_hairDiffuseMap->GetTexelColor(leftU, topV);
				topRightColor = m_initParams.m_hairDiffuseMap->GetTexelColor(rightU, topV);
			}


			for (int hairIndex = 0; hairIndex < (HairObject::HairPerSection * avgDensity); hairIndex++) {
				auto timeNow = std::chrono::high_resolution_clock::now();
				auto TimeInMS = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow.time_since_epoch()).count();
				float TimeInMSFloat = static_cast<float>(TimeInMS);
				float noisePos = static_cast<float>(TimeInMSFloat + (TimeInMSFloat * 0.5f * (float)hairIndex));

				float randTX = 0.5f + 0.5f * Compute1dPerlinNoise(noisePos, 250254, 7, 0.35f, 6.0f, true, 1);
				float randTY = 0.5f + 0.5f * Compute1dPerlinNoise(noisePos, 486454, 5, 0.475f, 3.0f, true, 25);

				randTX *= randTX;
				randTY *= randTY;

				Vec3 dispX = rightBottomPos - leftBottomPos;
				Vec3 dispY = rightTopPos - rightBottomPos;

				dispX *= randTX;
				dispY *= randTY;

				Vec3 randPos = leftBottomPos + dispX + dispY;

				Rgba8 colorLerpXBottom = Rgba8::InterpolateColors(bottomLeftColor, bottomRightColor, randTX);
				Rgba8 colorLerpXTop = Rgba8::InterpolateColors(topLeftColor, topRightColor, randTX);

				Rgba8 resultingColor = Rgba8::InterpolateColors(colorLerpXBottom, colorLerpXTop, randTY);

				Vec3 normal = randPos.GetNormalized();

				HairGuide newHair = HairGuide(randPos, resultingColor, normal, HairGuide::HairSegmentCount);
				m_hairs.push_back(newHair);
				newHair.AddVerts(m_hairVertexes);
			}



			prevCosPitch = cosPitch;
			prevSinPitch = sinPitch;
		}

		prevCosYaw = cosYaw;
		prevSinYaw = sinYaw;
	}

	m_hairCount = (int)m_hairs.size();

	m_vertexBuffer = new VertexBuffer(g_theRenderer->m_device, sizeof(Vertex_PNCU) * m_hairVertexes.size(), sizeof(Vertex_PNCU));
	m_sphereBuffer = new VertexBuffer(g_theRenderer->m_device, sizeof(Vertex_PNCU) * m_sphereVertexes.size(), sizeof(Vertex_PNCU));

	/* This buffer does not need information because it will be read from a buffer. HairSegments amount of vertexes must be rendered per triangle*/
	int amounOfHairToRender = int(m_sphereVertexes.size() / 3);
	amounOfHairToRender *= HairGuide::HairSegmentCount * 2; // Change this to some variable!!!!!
	m_amountOfMultInterpHair = amounOfHairToRender;

	m_multInterpBuffer = new VertexBuffer(g_theRenderer->m_device, sizeof(Vertex_PNCU) * m_amountOfMultInterpHair, sizeof(Vertex_PNCU));

	g_theRenderer->CopyCPUToGPU(m_hairVertexes.data(), sizeof(Vertex_PNCU) * m_hairVertexes.size(), m_vertexBuffer);
	g_theRenderer->CopyCPUToGPU(m_sphereVertexes.data(), sizeof(Vertex_PNCU) * m_sphereVertexes.size(), m_sphereBuffer);

}

void HairSphere::InitializeHairUAV()
{
	std::vector<HairData> hairPositions;
	hairPositions.reserve((m_hairVertexes.size() / 2) + 1); // There are 2x hair Vertexes as they're lines that repeat some stuff.
	std::vector<HairData> virtualHairPositions;
	virtualHairPositions.reserve(hairPositions.size());
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

	m_hairPositionUAV = new UnorderedAccessBuffer(g_theRenderer->m_device, hairPositions.data(), hairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);
	m_hairPrevPositionUAV = new UnorderedAccessBuffer(g_theRenderer->m_device, hairPositions.data(), hairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);
	m_virtualPositionUAV = new UnorderedAccessBuffer(g_theRenderer->m_device, virtualHairPositions.data(), virtualHairPositions.size(), sizeof(HairData), UAVType::STRUCTURED, TextureFormat::R32G32B32A32_FLOAT);

	g_theRenderer->BindShader(m_game->m_copyShader);
	Mat44 modelMat = GetModelMatrix();

	int numThreadsToDispatch = GetThreadsToDispatch(hairPositions.size());

	g_theRenderer->SetModelMatrix(modelMat);
	g_theRenderer->CopyAndBindModelConstants();
	g_theRenderer->SetUAV(m_hairPositionUAV);
	g_theRenderer->DispatchCS(numThreadsToDispatch, 1, 1);
	g_theRenderer->SetUAV(m_virtualPositionUAV);
	g_theRenderer->DispatchCS(numThreadsToDispatch, 1, 1);
	g_theRenderer->ClearState();

}

void HairSphere::Render() const
{
	Mat44 modelMat = GetModelMatrix();
	g_theRenderer->SetModelMatrix(modelMat);
	if (m_game->m_renderHair) {
		HairObject::Render();
	}

	if (m_game->m_renderSphere) {
		Shader* defaultDiffuse = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuse");
		g_theRenderer->BindShader(defaultDiffuse);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->CopyAndBindModelConstants();

		g_theRenderer->BindVertexBuffer(m_sphereBuffer, m_sphereBuffer->GetStride());
		g_theRenderer->DrawVertexBuffer(m_sphereBuffer, (int)m_sphereVertexes.size(), 0, m_sphereBuffer->GetStride());
	}

}

HairSphereTessellation::HairSphereTessellation(Game* pointerToGame, HairObjectInit const& initParams) :
	HairSphere(pointerToGame, initParams)
{
	InitializeHairUAVMultInterp();
}

HairSphereTessellation::~HairSphereTessellation()
{
	delete m_hairInterpUAV;
	if (m_hairInterpPrevUAV) {
		delete m_hairInterpPrevUAV;
	}

	if (m_hairInterpVirtualUAV) {
		delete m_hairInterpVirtualUAV;
	}
}

void HairSphereTessellation::InitializeHairUAVMultInterp()
{
	std::vector<Vertex_PNCU> hairVertexes;
	hairVertexes.reserve(m_sphereVertexes.size() * (HairGuide::HairSegmentCount + 1));

	for (int sphereVertInd = 0; sphereVertInd < m_sphereVertexes.size(); sphereVertInd++) {
		Vertex_PNCU const& sphereVertex = m_sphereVertexes[sphereVertInd];
		Rgba8 vertexHairDensity = m_initParams.m_hairDensityMap->GetTexelColor(sphereVertex.m_uvTexCoords);
		if (vertexHairDensity.r <= 10) { continue; }

		HairGuide newHairGuide(sphereVertex.m_position, Rgba8::WHITE, sphereVertex.m_position.GetNormalized(), HairGuide::HairSegmentCount);
		hairVertexes.reserve(13 * 2);
		newHairGuide.AddVerts(hairVertexes);
	}

	HairUAVInfo multinterpUAVInfo = {};
	multinterpUAVInfo.InitialHairData = &hairVertexes;

	HairObject::InitializeHairUAV(multinterpUAVInfo);

	m_hairInterpUAV = multinterpUAVInfo.Out_HairPositions;
	m_hairInterpPrevUAV = multinterpUAVInfo.Out_PreviousHairPositions;
	m_hairInterpVirtualUAV = multinterpUAVInfo.Out_VirtualHairPositions;
}

void HairSphereTessellation::Update(float deltaSeconds)
{
	HairSphere::Update(deltaSeconds);

	if (m_game->UseModelVertsForInterp()) {
		if (g_simulateHair) {
			int threadsSimul = ((static_cast<int>(m_hairVertexes.size()) / HairGuide::HairSegmentCount) / 64) + 1;

			g_theRenderer->BindShader(m_game->GetSimulationShader());
			g_theRenderer->SetModelMatrix(GetModelMatrix());
			g_theRenderer->CopyAndBindModelConstants();

			if (m_initParams.m_usedConstantBuffer) {
				g_theRenderer->CopyCPUToGPU(*m_initParams.m_usedConstantBuffer, sizeof(HairConstants), m_game->m_hairConstantBuffer);
			}
			g_theRenderer->BindConstantBuffer(4, m_game->m_hairConstantBuffer);
			g_theRenderer->BindConstantBuffer(5, GetHairCollisionBuffer());
			g_theRenderer->SetUAV(m_hairInterpUAV, 0);
			if (m_hairInterpPrevUAV) {
				g_theRenderer->SetUAV(m_hairInterpPrevUAV, 1);
			}
			if (m_virtualPositionUAV) {
				g_theRenderer->SetUAV(m_hairInterpVirtualUAV, 2);
			}
			if (m_gridUAV) {
				g_theRenderer->SetUAV(m_gridUAV, 3);
			}
			g_theRenderer->DispatchCS(threadsSimul, 1, 1);
			g_theRenderer->SetUAV(nullptr);
			g_theRenderer->ClearState();
		}
	}
}

void HairSphereTessellation::Render() const
{
	Mat44 modelMat = GetModelMatrix();
	g_theRenderer->SetModelMatrix(modelMat);
	g_theRenderer->BindConstantBuffer(4, m_game->m_hairConstantBuffer);

	if (m_game->m_renderSphere) {
		if (g_drawDebug) {
			g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::WIREFRAME, WindingOrder::COUNTERCLOCKWISE);
		}
		else {
			g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		}

		Shader* defaultDiffuse = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuse");
		g_theRenderer->BindShader(defaultDiffuse);

		/*Texture* densityMapTexture = nullptr;
		if (m_initParams.m_hairDensityMap) {
			densityMapTexture = g_theRenderer->CreateOrGetTextureFromFile(m_initParams.m_hairDensityMap->GetImageFilePath().c_str());
		}
		g_theRenderer->BindTexture(densityMapTexture);*/
		g_theRenderer->CopyAndBindModelConstants();

		g_theRenderer->BindVertexBuffer(m_sphereBuffer, m_sphereBuffer->GetStride());
		g_theRenderer->DrawVertexBuffer(m_sphereBuffer, (int)m_sphereVertexes.size(), 0, m_sphereBuffer->GetStride());
	}

	if (m_initParams.m_usedConstantBuffer) {
		g_theRenderer->CopyCPUToGPU(*m_initParams.m_usedConstantBuffer, sizeof(HairConstants), m_game->m_hairConstantBuffer);
	}
	if (m_game->m_renderHair) {
		g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->BindShader(m_initParams.m_shader);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->CopyAndBindModelConstants();
		g_theRenderer->SetShaderResource(m_hairPositionUAV, 2, ShaderBindBit::SHADER_BIND_HULL_SHADER);
		g_theRenderer->BindVertexBuffer(m_vertexBuffer, m_vertexBuffer->GetStride(), TopologyMode::CONTROL_POINT_PATCHLIST_2);
		g_theRenderer->DrawVertexBuffer(m_vertexBuffer, (int)m_hairVertexes.size(), 0, sizeof(Vertex_PNCU), TopologyMode::CONTROL_POINT_PATCHLIST_2);
	}

	if (m_game->m_renderMultInterp) {
		//g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->BindShader(m_initParams.m_multInterpShader);

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::SimplexNoise], 1, SHADER_BIND_DOMAIN_SHADER);
		g_theRenderer->CopyAndBindModelConstants();
	
		g_theRenderer->BindConstantBuffer(5, GetHairCollisionBuffer());
		g_theRenderer->CopyAndBindLightConstants();

		if (m_game->UseModelVertsForInterp()) {
			g_theRenderer->SetShaderResource(m_hairInterpUAV, 2, SHADER_BIND_DOMAIN_SHADER);
			g_theRenderer->BindVertexBuffer(m_multInterpBuffer, m_multInterpBuffer->GetStride(), TopologyMode::CONTROL_POINT_PATCHLIST_2);
			g_theRenderer->DrawVertexBuffer(m_multInterpBuffer, m_amountOfMultInterpHair, 0, sizeof(Vertex_PNCU), TopologyMode::CONTROL_POINT_PATCHLIST_2);
		}
		else {
			int amountOfInterpHair = ((int)m_hairVertexes.size() / 3) - (HairGuide::HairSegmentCount * 2) * 2; // Interpolated from three hairs, last two hairs would interpolate with invalid positions
			g_theRenderer->SetShaderResource(m_hairPositionUAV, 2, SHADER_BIND_DOMAIN_SHADER);
			g_theRenderer->BindVertexBuffer(m_vertexBuffer, m_vertexBuffer->GetStride(), TopologyMode::CONTROL_POINT_PATCHLIST_2);
			g_theRenderer->DrawVertexBuffer(m_vertexBuffer, amountOfInterpHair, 0, sizeof(Vertex_PNCU), TopologyMode::CONTROL_POINT_PATCHLIST_2);
		}
	}


}

int HairSphereTessellation::GetMultiStrandBaseCount() const
{
	if (m_game->UseModelVertsForInterp()) {
		return static_cast<int>(m_sphereVertexes.size());
	}
	else {
		return static_cast<int>(m_hairVertexes.size()) - ((HairGuide::HairSegmentCount * 2) * 6);
	}
}

