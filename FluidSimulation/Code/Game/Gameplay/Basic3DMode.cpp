#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Buffer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/D3D12/Resource.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Game/Gameplay/Basic3DMode.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "ThirdParty/ImGUI/imgui.h"
#include "Game/Gameplay/Prop.hpp"
#include "Game/Gameplay/Game.hpp"

Basic3DMode* pointerToSelf = nullptr;
struct Meshlet
{
	Meshlet(unsigned int vCount, unsigned int vOffset, unsigned int primCount, unsigned int primOffset) :
		VertexCount(vCount),
		VertexOffset(vOffset),
		PrimCount(primCount),
		PrimOffset(primOffset)
	{
		Rgba8::WHITE.GetAsFloats(Color);
	}

	unsigned int VertexCount;
	unsigned int VertexOffset;
	unsigned int PrimCount;
	unsigned int PrimOffset;
	float Color[4];
};

struct GameConstants
{
	Vec3 EyePosition;
	float SpriteRadius;
	Vec3 CameraUp;
	float KernelRadius;
	Vec3 CameraLeft;
	unsigned int ParticleCount;
	Vec3 Forces; // Gravity included
	float DeltaTime; // In Seconds
	Vec3 BoundsMins;
	float RestDensity;
	Vec3 BoundsMaxs;
	unsigned int Padding;
	Vec4 ParticleColor;
};

struct BlurConstants
{
	float KernelRadius;
	float GaussianSigma;
	float ClearValue;
	unsigned int DirectionFlags; // b01 for vertical, b10 for horizontal
	float GaussianKernels[16];
};


Basic3DMode::Basic3DMode(Game* game, Vec2 const& UISize) :
	GameMode(game, UISize)
{
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);
}

Basic3DMode::~Basic3DMode()
{
	m_depthTexture = nullptr;
}

void Basic3DMode::Startup()
{
	GameMode::Startup();


	SubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	SubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	SubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	SubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	SubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	SubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	SubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	SubscribeEventCallbackFunction("Controls", GetControls);

	pointerToSelf = this;


	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Player* player = new Player(m_game, Vec3(4.0f, -4.0f, 2.0f), &m_worldCamera);
	m_player = player;
	m_player->m_orientation = EulerAngles(130.0f, -10.0f, 0.0f);

	Prop* cubeProp = new Prop(m_game, Vec3(-2.0f, 2.0f, 0.0f));
	cubeProp->m_angularVelocity.m_yawDegrees = 45.0f;

	Prop* gridProp = new Prop(m_game, Vec3::ZERO, PropRenderType::GRID);

	Prop* sphereProp = new Prop(m_game, Vec3(10.0f, -5.0f, 1.0f), 1.0f, PropRenderType::SPHERE);
	sphereProp->m_angularVelocity.m_pitchDegrees = 20.0f;
	sphereProp->m_angularVelocity.m_yawDegrees = 20.0f;

	m_allEntities.push_back(player);
	m_allEntities.push_back(cubeProp);
	m_allEntities.push_back(gridProp);
	m_allEntities.push_back(sphereProp);

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

	float axisLabelTextHeight = 0.25f;
	Mat44 xLabelTransformMatrix(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3::ZERO);
	float xLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "X - Forward");
	xLabelTransformMatrix.SetTranslation3D(Vec3(xLabelWidth * 0.7f, 0.0f, axisLabelTextHeight));

	DebugAddWorldText("X - Forward", xLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

	Mat44 yLabelTransformMatrix(Vec3(0.0f, 1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3::ZERO);
	float yLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Y - Left");
	yLabelTransformMatrix.SetTranslation3D(Vec3(-axisLabelTextHeight, yLabelWidth * 0.7f, 0.0f));

	DebugAddWorldText("Y - Left", yLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH);

	Mat44 zLabelTransformMatrix(Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3::ZERO);
	float zLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Z - Up");
	zLabelTransformMatrix.SetTranslation3D(Vec3(0.0f, axisLabelTextHeight, zLabelWidth * 0.7f));

	DebugAddWorldText("Z - Up", zLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);


	//TextureCreateInfo colorInfo;
	//colorInfo.m_dimensions = g_theWindow->GetClientDimensions();
	//colorInfo.m_format = TextureFormat::R8G8B8A8_UNORM;
	//colorInfo.m_bindFlags = TEXTURE_BIND_RENDER_TARGET_BIT | TEXTURE_BIND_SHADER_RESOURCE_BIT;
	//colorInfo.m_memoryUsage = MemoryUsage::Default;

	LoadMaterials();

	FluidSolverConfig config = {};
	config.m_particlePerSide = 10;
	config.m_pointerToParticles = &m_particles;
	config.m_simulationBounds = m_particlesBounds;
	config.m_iterations = 3;
	config.m_kernelRadius = 0.196f;
	config.m_renderingRadius = 0.075f;
	config.m_restDensity = 1000.0f;


	m_fluidSolver = FluidSolver(config);
	m_fluidSolver.InitializeParticles();

	InitializeBuffers();
	InitializeGPUPhysicsBuffers();
	InitializeTextures();

	m_worldCamera.SetColorTarget(m_backgroundRT);
	m_UICamera.SetColorTarget(m_backgroundRT);
}

void Basic3DMode::Update(float deltaSeconds)
{
	m_fps = 1.0f / deltaSeconds;
	GameMode::Update(deltaSeconds);

	UpdateInput(deltaSeconds);
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();
	UpdateImGui();

	std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);
	std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);
	DebugAddMessage(gameInfoStr, 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	DebugAddWorldWireBox(m_particlesBounds, 0.0f, Rgba8(255, 0, 0, 20), Rgba8::RED, DebugRenderMode::USEDEPTH);

	UpdateDeveloperCheatCodes(deltaSeconds);
	UpdateEntities(deltaSeconds);
	//UpdateParticles(deltaSeconds);

	DisplayClocksInfo();

	BlurConstants horizontalBlurConstants = {};
	horizontalBlurConstants.ClearValue = 0.0f;
	horizontalBlurConstants.DirectionFlags = (0b1010);

	GetGaussianKernels(m_debugConfig.m_kernelRadius, m_debugConfig.m_sigma, horizontalBlurConstants.GaussianKernels);
	horizontalBlurConstants.GaussianSigma = 1.0f;
	horizontalBlurConstants.KernelRadius = (float)m_debugConfig.m_kernelRadius;

	BlurConstants verticalBlurConstants = {};
	memcpy(&verticalBlurConstants, &horizontalBlurConstants, sizeof(BlurConstants));

	verticalBlurConstants.DirectionFlags = (0b1001);

	m_thicknessHPassCBuffer->CopyCPUToGPU(&horizontalBlurConstants, sizeof(BlurConstants));
	m_thicknessVPassCBuffer->CopyCPUToGPU(&verticalBlurConstants, sizeof(BlurConstants));

	horizontalBlurConstants.ClearValue = 1.0f;
	horizontalBlurConstants.DirectionFlags = (0b110);

	verticalBlurConstants.ClearValue = 1.0f;
	verticalBlurConstants.DirectionFlags = (0b101);

	m_depthHPassCBuffer->CopyCPUToGPU(&horizontalBlurConstants, sizeof(BlurConstants));
	m_depthVPassCBuffer->CopyCPUToGPU(&verticalBlurConstants, sizeof(BlurConstants));

	// GPU PASS
	UpdateGPUParticles(deltaSeconds);

	m_frame++;
	// CPU PASS
	// UpdateCPUParticles(deltaSeconds);
}

void Basic3DMode::LoadMaterials()
{
	m_effectsMaterials[(int)MaterialEffect::ColorBanding] = g_theMaterialSystem->GetMaterialForName("ColorBandFX");
	m_effectsMaterials[(int)MaterialEffect::Grayscale] = g_theMaterialSystem->GetMaterialForName("GrayScaleFX");
	m_effectsMaterials[(int)MaterialEffect::Inverted] = g_theMaterialSystem->GetMaterialForName("InvertedColorFX");
	m_effectsMaterials[(int)MaterialEffect::Pixelized] = g_theMaterialSystem->GetMaterialForName("PixelizedFX");
	m_effectsMaterials[(int)MaterialEffect::DistanceFog] = g_theMaterialSystem->GetMaterialForName("DistanceFogFX");

	std::filesystem::path materialPath("Data/Materials/DepthPrePass");
	std::filesystem::path fluidColorMatPath("Data/Materials/FluidColorPass");
	std::filesystem::path thicknessMaterialPath("Data/Materials/ThicknessPrepass");
	std::filesystem::path computeMatPath("Data/Materials/ComputeShaders");
	std::filesystem::path gaussianBlurMatPath("Data/Materials/GaussianBlurPixelPass");
	m_fluidColorPassMaterial = g_theMaterialSystem->CreateOrGetMaterial(fluidColorMatPath);
	m_thicknessMaterial = g_theMaterialSystem->CreateOrGetMaterial(thicknessMaterialPath);
	m_gaussianBlurMaterial = g_theMaterialSystem->CreateOrGetMaterial(gaussianBlurMatPath);

	g_theMaterialSystem->LoadMaterialsFromXML(computeMatPath);
	g_theMaterialSystem->LoadMaterialsFromXML(materialPath);

	m_bitonicSortCS = g_theMaterialSystem->GetMaterialForName("BitonicSortCS");;
	m_hashParticlesCS = g_theMaterialSystem->GetMaterialForName("HashCS");;
	m_offsetGenerationCS = g_theMaterialSystem->GetMaterialForName("OffsetsCS");
	m_applyForcesCS = g_theMaterialSystem->GetMaterialForName("ApplyForcesCS");
	m_lambdaCS = g_theMaterialSystem->GetMaterialForName("CalculateLambdaCS");
	m_updateMovementCS = g_theMaterialSystem->GetMaterialForName("UpdateParticlesMovementCS");

	m_prePassMaterial = g_theMaterialSystem->GetMaterialForName("FluidDepthPrepass");
	m_prePassGPUMaterial = g_theMaterialSystem->GetMaterialForName("GPUFluidDepthPrepass");

}

void Basic3DMode::InitializeBuffers() {
	m_verts.clear();
	//size_t lastIndex = 0;
	constexpr unsigned int meshletSize = 64;

	std::vector<Meshlet> meshlets;
	unsigned int meshletParticleCount = 0;
	unsigned int meshletsParticleAccumulator = 0;
	for (int particleIndex = 0; particleIndex < m_particles.size(); particleIndex++, meshletParticleCount++) {
		if (meshletParticleCount + 1 >= meshletSize) {
			meshlets.emplace_back(meshletParticleCount + 1, meshletsParticleAccumulator, meshletParticleCount + 1, meshletsParticleAccumulator);
			meshletsParticleAccumulator += meshletParticleCount + 1;
			meshletParticleCount = 0;
		}

		/*	FluidParticle const& particle = m_particles[particleIndex];
			AddVertsForSphere(m_verts, config.m_renderingRadius, 4, 8, Rgba8(0, 0, 180, 100));
			TransformVertexArray3D(int(m_verts.size() - lastIndex), &m_verts[lastIndex], Mat44(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), particle.m_position));
			lastIndex = m_verts.size();*/
	}

	if (meshletParticleCount > 0) {
		meshlets.emplace_back(meshletParticleCount + 1, meshletsParticleAccumulator, meshletParticleCount + 1, meshletsParticleAccumulator);
		meshletsParticleAccumulator += meshletParticleCount + 1;
		meshletParticleCount = 0;
	}
	m_vertsPerParticle = unsigned int(m_verts.size() / m_particles.size());
	m_particlesMeshInfo = new FluidParticleMeshInfo[m_particles.size()];

	BufferDesc bufferDesc = {};
	bufferDesc.data = m_particlesMeshInfo;
	bufferDesc.memoryUsage = MemoryUsage::Default;
	bufferDesc.owner = g_theRenderer;
	bufferDesc.size = sizeof(FluidParticleMeshInfo) * m_particles.size();
	bufferDesc.stride = sizeof(FluidParticleMeshInfo);
	bufferDesc.debugName = "Mesh VBuffer";
	m_meshVBuffer = new StructuredBuffer(bufferDesc);
	bufferDesc.data = meshlets.data();
	bufferDesc.size = sizeof(Meshlet) * meshlets.size();
	bufferDesc.stride = sizeof(Meshlet);

	bufferDesc.debugName = "Meshlet Buffer";
	m_meshletBuffer = new StructuredBuffer(bufferDesc);
	//m_meshletBuffer->CopyCPUToGPU(meshlets.data(), sizeof(Meshlet) * meshlets.size());

	BufferDesc cBufferDesc = {};
	cBufferDesc.data = nullptr;
	cBufferDesc.memoryUsage = MemoryUsage::Default;
	cBufferDesc.owner = g_theRenderer;
	cBufferDesc.size = sizeof(GameConstants);
	cBufferDesc.stride = sizeof(GameConstants);
	cBufferDesc.debugName = "Game Constants";

	m_gameConstants = new ConstantBuffer(cBufferDesc);
	m_gameConstants->Initialize();


	BufferDesc blurCBufferDesc = {};
	blurCBufferDesc.data = nullptr;
	blurCBufferDesc.memoryUsage = MemoryUsage::Default;
	blurCBufferDesc.owner = g_theRenderer;
	blurCBufferDesc.size = sizeof(BlurConstants);
	blurCBufferDesc.stride = sizeof(BlurConstants);
	blurCBufferDesc.debugName = "Thickness H_Blur Constants";

	m_thicknessHPassCBuffer = new ConstantBuffer(blurCBufferDesc);
	m_thicknessHPassCBuffer->Initialize();

	blurCBufferDesc.debugName = "Depth H_Blur Constants";
	m_depthHPassCBuffer = new ConstantBuffer(blurCBufferDesc);
	m_depthHPassCBuffer->Initialize();

	blurCBufferDesc.debugName = " Thickness V_Blur Constants";
	m_thicknessVPassCBuffer = new ConstantBuffer(blurCBufferDesc);
	m_thicknessVPassCBuffer->Initialize();

	blurCBufferDesc.debugName = "Depth V_Blur Constants";
	m_depthVPassCBuffer = new ConstantBuffer(blurCBufferDesc);
	m_depthVPassCBuffer->Initialize();
}

void Basic3DMode::InitializeGPUPhysicsBuffers()
{
	Vec3 aabb3Center = m_particlesBounds.GetCenter() - m_particlesBounds.GetDimensions() / 4;
	unsigned int particleTotal = 1024;
	unsigned int particlesPerside = (unsigned int)ceilf(std::cbrtf(float(particleTotal)));

	// A few will be wasted, but it's just much easier to do this, and it's only done once
	for (unsigned int x = 0; x < particlesPerside; x++) {
		for (unsigned int y = 0; y < particlesPerside; y++) {
			for (unsigned int z = 0; z < particlesPerside; z++) {
				m_GPUparticles.push_back(GPUFluidParticle(Vec3((float)x, (float)y, (float)z) * m_debugConfig.m_renderingRadius * 2.0f + aabb3Center));
			}
		}
	}
	m_GPUparticles.resize(particleTotal);

	BufferDesc particlesBufferDesc = {};
	particlesBufferDesc.data = m_GPUparticles.data();
	particlesBufferDesc.memoryUsage = MemoryUsage::Default;
	particlesBufferDesc.owner = g_theRenderer;
	particlesBufferDesc.size = sizeof(GPUFluidParticle) * particleTotal;
	particlesBufferDesc.stride = sizeof(GPUFluidParticle);
	particlesBufferDesc.debugName = "GPUFluidParticles";

	m_particlesBuffer = new StructuredBuffer(particlesBufferDesc);

	BufferDesc hashArrDesc = {};
	hashArrDesc.memoryUsage = MemoryUsage::Default;
	hashArrDesc.owner = g_theRenderer;
	hashArrDesc.size = sizeof(unsigned int) * 3 * particleTotal;
	hashArrDesc.stride = sizeof(unsigned int);
	hashArrDesc.debugName = "Hash Array";

	m_hashInfoBuffer = new StructuredBuffer(hashArrDesc);

	BufferDesc offsetsBufferDesc = {};
	offsetsBufferDesc.memoryUsage = MemoryUsage::Default;
	offsetsBufferDesc.owner = g_theRenderer;
	offsetsBufferDesc.size = sizeof(unsigned int) * particleTotal;
	offsetsBufferDesc.stride = sizeof(unsigned int);
	offsetsBufferDesc.debugName = "Offsets Array";

	m_offsetsBuffer = new StructuredBuffer(offsetsBufferDesc);
}

void Basic3DMode::InitializeTextures() {
	TextureCreateInfo thicknessTexInfo = {};
	thicknessTexInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_ALL_TEXTURE_VIEWS;
	thicknessTexInfo.m_format = TextureFormat::R32_FLOAT;
	thicknessTexInfo.m_owner = g_theRenderer;
	thicknessTexInfo.m_clearColour = Rgba8(0, 0, 0, 0);
	thicknessTexInfo.m_clearFormat = TextureFormat::R32_FLOAT;
	thicknessTexInfo.m_dimensions = g_theWindow->GetClientDimensions();
	thicknessTexInfo.m_owner = g_theRenderer;
	thicknessTexInfo.m_name = "Thickness";

	TextureCreateInfo backgroundRTInfo = {};
	backgroundRTInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT | ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT;
	backgroundRTInfo.m_format = TextureFormat::R8G8B8A8_UNORM;
	backgroundRTInfo.m_owner = g_theRenderer;
	backgroundRTInfo.m_clearColour = Rgba8(0, 0, 0, 0);
	backgroundRTInfo.m_clearFormat = TextureFormat::R8G8B8A8_UNORM;
	backgroundRTInfo.m_dimensions = g_theWindow->GetClientDimensions();
	backgroundRTInfo.m_owner = g_theRenderer;
	backgroundRTInfo.m_name = "Background RT";

	TextureCreateInfo depthTextureInfo = {};
	depthTextureInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_DEPTH_STENCIL_BIT | ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT;
	depthTextureInfo.m_format = TextureFormat::D32_FLOAT;
	depthTextureInfo.m_owner = g_theRenderer;
	depthTextureInfo.m_clearColour = Rgba8(255, 255, 255, 255);
	depthTextureInfo.m_clearFormat = TextureFormat::D32_FLOAT;
	depthTextureInfo.m_dimensions = g_theWindow->GetClientDimensions();
	depthTextureInfo.m_owner = g_theRenderer;
	depthTextureInfo.m_name = "Prepass DRT";

	m_depthTexture = g_theRenderer->CreateTexture(depthTextureInfo);
	depthTextureInfo.m_name = "Blurred Depth";
	depthTextureInfo.m_handle = nullptr;
	depthTextureInfo.m_format = TextureFormat::R32_FLOAT;
	depthTextureInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_RENDER_TARGET_BIT | ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT;
	depthTextureInfo.m_clearFormat = TextureFormat::R32_FLOAT;


	m_blurredDepthTexture[0] = g_theRenderer->CreateTexture(depthTextureInfo);
	depthTextureInfo.m_name = "Blurred Depth 1";
	depthTextureInfo.m_handle = nullptr;
	m_blurredDepthTexture[1] = g_theRenderer->CreateTexture(depthTextureInfo);

	m_thickness = g_theRenderer->CreateTexture(thicknessTexInfo);
	thicknessTexInfo.m_name = "Blurred Thickness";
	thicknessTexInfo.m_handle = nullptr;
	m_blurredThickness[0] = g_theRenderer->CreateTexture(thicknessTexInfo);
	thicknessTexInfo.m_handle = nullptr;
	thicknessTexInfo.m_name = "Blurred Thickness 1";
	m_blurredThickness[1] = g_theRenderer->CreateTexture(thicknessTexInfo);

	m_backgroundRT = g_theRenderer->CreateTexture(backgroundRTInfo);


}

void Basic3DMode::UpdateImGui()
{
	ImGui::Begin("Debug Config", &m_debugConfig.m_showDebug);

	ImGui::Checkbox("Show original depth", &m_debugConfig.m_showOriginalDepth);
	ImGui::Checkbox("Show blurred depth", &m_debugConfig.m_showBlurredDepth);

	ImGui::Checkbox("Show original thickness", &m_debugConfig.m_showOriginalThickness);
	ImGui::Checkbox("Show blurred thickness", &m_debugConfig.m_showBlurredThickness);

	ImGui::Checkbox("Skip blurring", &m_debugConfig.m_skipBlurPass);

	ImGui::SliderInt("Num blur passes", &m_debugConfig.m_blurPassCount, 1, 20);
	ImGui::SliderFloat("Sigma", &m_debugConfig.m_sigma, 0.5f, 5.0f);
	ImGui::SliderInt("Kernel Radius", &m_debugConfig.m_kernelRadius, 1, 15);
	ImGui::ColorEdit4("Clear Colour", m_debugConfig.m_clearColor);
	ImGui::ColorEdit4("Water Colour", m_debugConfig.m_waterColor);

	ImGui::End();
}

void Basic3DMode::BitonicSortTest(int* arr, size_t arraySize, unsigned int direction /*= 0*/)
{
	for (unsigned int stageInd = 2; stageInd <= arraySize; stageInd <<= 1) { // Stageind *= 2
		for (unsigned int stepInd = stageInd / 2; stepInd > 0; stepInd >>= 1) { // Stepind /= 2
			for (unsigned int numIndex = 0; numIndex < arraySize; numIndex++) {
				unsigned int otherNumInd = (numIndex ^ stepInd); // This
				if (otherNumInd > numIndex) {
					bool shouldSwap = ((numIndex & stageInd) == direction) && (arr[numIndex] > arr[otherNumInd]);
					shouldSwap = shouldSwap || ((numIndex & stageInd) != direction) && (arr[numIndex] < arr[otherNumInd]);

					if (shouldSwap) {
						int temp = arr[numIndex];
						arr[numIndex] = arr[otherNumInd];
						arr[otherNumInd] = temp;
					}
				}
			}
		}
	}

}

// 0 Ascending
// 1 Descending
void Basic3DMode::SortGPUParticles(size_t arraySize, unsigned int direction /*= 0*/) const
{

	g_theRenderer->BindComputeMaterial(m_bitonicSortCS);

	/*
	  0: Direction
	  1: Stage Index
	  2: Step Index
  */
	g_theRenderer->SetRootConstant(direction);

	for (unsigned int stageInd = 2; stageInd <= arraySize; stageInd <<= 1) { // Stageind *= 2
		g_theRenderer->SetRootConstant(stageInd, 1);
		for (unsigned int stepInd = stageInd / 2; stepInd > 0; stepInd >>= 1) { // Stepind /= 2
			g_theRenderer->SetRootConstant(stepInd, 2);

			g_theRenderer->BindRWStructuredBuffer(m_hashInfoBuffer, 0);
			g_theRenderer->Dispatch(1, 1, 1);
		}
	}

}

void Basic3DMode::UpdateCPUParticles(float deltaSeconds)
{
	UpdateParticles(deltaSeconds);
}

void Basic3DMode::Render() const
{
	RenderGPUParticles();

	for (int effectInd = 0; effectInd < (int)MaterialEffect::NUM_EFFECTS; effectInd++) {
		if (m_applyEffects[effectInd]) {
			g_theRenderer->ApplyEffect(m_effectsMaterials[effectInd], &m_worldCamera);
		}
	}


	GameMode::Render();

	std::vector<Vertex_PCU> dummyVec(3);

	if (!m_debugConfig.m_skipBlurPass) {
		RenderBlurPass();
	}

	{
		g_theRenderer->BeginCamera(m_worldCamera);
		Light firstLight = {};
		Rgba8::WHITE.GetAsFloats(firstLight.Color);
		firstLight.Enabled = true;
		firstLight.LightType = 1;
		firstLight.Position = Vec3(1.0f, 1.0f, 1.0f);
		firstLight.Direction = Vec3(0.0f, 0.0f, -1.0f);
		firstLight.QuadraticAttenuation = 0.0f;
		firstLight.LinearAttenuation = 0.05f;
		firstLight.ConstantAttenuation = 0.025f;

		g_theRenderer->SetLight(firstLight, 0);
		g_theRenderer->BindLightConstants();
		g_theRenderer->SetRenderTarget(g_theRenderer->GetDefaultRenderTarget());
		g_theRenderer->BindMaterial(m_fluidColorPassMaterial);
		g_theRenderer->SetDepthRenderTarget(nullptr);
		g_theRenderer->BindTexture(m_blurredDepthTexture[m_currentBlurredDepth]);
		g_theRenderer->BindTexture(m_blurredThickness[m_currentBlurredThickness], 1);
		g_theRenderer->BindTexture(m_backgroundRT, 2);
		g_theRenderer->BindTexture(g_theRenderer->GetCurrentDepthTarget(), 3);
		g_theRenderer->SetModelColor(Rgba8(m_debugConfig.m_waterColor));
		g_theRenderer->DrawVertexArray(dummyVec);
		g_theRenderer->EndCamera(m_worldCamera);
	}

	g_theRenderer->BeginCamera(m_UICamera);
	{
		g_theRenderer->SetRenderTarget(g_theRenderer->GetDefaultRenderTarget());

		Vec2 startQuad = Vec2::ZERO;
		Vec2 quadSize = Vec2(m_UISize.x * .75f * 0.25f, m_UISize.y * .25f);
		Vec2 endQuad = startQuad + quadSize;
		AABB2 quad = AABB2(startQuad, endQuad);

		std::vector<Vertex_PCU> verts;
		g_theRenderer->BindMaterialByName("LinearDepthVisualizer");
		if (m_debugConfig.m_showOriginalDepth) {
			AddVertsForAABB2D(verts, quad, Rgba8::WHITE, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f));
			g_theRenderer->BindTexture(m_depthTexture);
			g_theRenderer->DrawVertexArray(verts);
			startQuad.x += quadSize.x;
			endQuad.x += quadSize.x;
		}

		if (m_debugConfig.m_showBlurredDepth) {
			verts.clear();
			quad = AABB2(startQuad, endQuad);
			AddVertsForAABB2D(verts, quad, Rgba8::WHITE, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f));
			g_theRenderer->BindTexture(m_blurredDepthTexture[m_currentBlurredDepth]);
			g_theRenderer->DrawVertexArray(verts);
			startQuad.x += quadSize.x;
			endQuad.x += quadSize.x;
		}

		g_theRenderer->BindMaterial(g_theRenderer->GetDefaultMaterial(true));
		if (m_debugConfig.m_showOriginalThickness) {
			verts.clear();
			quad = AABB2(startQuad, endQuad);
			AddVertsForAABB2D(verts, quad, Rgba8::WHITE, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f));
			g_theRenderer->BindTexture(m_thickness);
			g_theRenderer->DrawVertexArray(verts);
			startQuad.x += quadSize.x;
			endQuad.x += quadSize.x;
		}

		if (m_debugConfig.m_showBlurredThickness) {
			verts.clear();
			quad = AABB2(startQuad, endQuad);
			AddVertsForAABB2D(verts, quad, Rgba8::WHITE, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f));
			g_theRenderer->BindTexture(m_blurredThickness[m_currentBlurredThickness]);
			g_theRenderer->DrawVertexArray(verts);
			startQuad.x += quadSize.x;
			endQuad.x += quadSize.x;
		}

	}
	g_theRenderer->EndCamera(m_UICamera);

	// CPU PASS
	// RenderCPUParticles();
}

float Basic3DMode::GetGaussian1D(float sigma, float dist) const
{
	float sigmaSqr = sigma * sigma;
	float distSqr = dist * dist;
	float sigmaSqrTwo = 2.0f * sigmaSqr;

	float exponent = -(distSqr / (sigmaSqrTwo));
	float eCoeff = exp(exponent);

	float scalarCoef = 1.0f / (sqrt(sigmaSqrTwo * float(M_PI)));

	return scalarCoef * eCoeff;
}


void Basic3DMode::GetGaussianKernels(unsigned int kernelSize, float sigma, float* kernels) const
{
	float total = 0.0f;
	unsigned int totalKernelSize = kernelSize + 1;
	for (unsigned int termInd = 0; termInd < totalKernelSize; termInd++)
	{
		float gaussianSample = GetGaussian1D(sigma, float(termInd));
		//total += (gaussianSample * gaussianSample);
		kernels[termInd] = gaussianSample;
		if (termInd > 0) gaussianSample *= 2.0f;
		total += (gaussianSample);
	}

	float invTotal = 1.0f / (total);
	float normalizedTotal = 0.0f;
	for (unsigned int normalInd = 0; normalInd < totalKernelSize; normalInd++)
	{
		kernels[normalInd] *= invTotal;
		normalizedTotal += kernels[normalInd];
	}


}



void Basic3DMode::UpdateGPUParticles(float deltaSeconds)
{
	static Vec3 const gravity = Vec3(0.0f, 0.0f, -9.8f);
	EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
	GameConstants gameConstants = {};
	gameConstants.BoundsMins = m_particlesBounds.m_mins;
	gameConstants.BoundsMaxs = m_particlesBounds.m_maxs;
	gameConstants.CameraUp = cameraOrientation.GetZUp(), 0.0f;
	gameConstants.CameraLeft = cameraOrientation.GetYLeft(), 0.0f;
	gameConstants.DeltaTime = deltaSeconds;
	gameConstants.EyePosition = m_worldCamera.GetViewPosition();
	gameConstants.Forces = gravity;
	gameConstants.KernelRadius = 0.196f;
	gameConstants.ParticleColor = Vec4(m_debugConfig.m_waterColor);
	gameConstants.ParticleCount = 1024;
	gameConstants.RestDensity = 1000.0f;
	gameConstants.SpriteRadius = m_debugConfig.m_renderingRadius;

	m_gameConstants->CopyCPUToGPU(&gameConstants, sizeof(GameConstants));

	// Debugging correct calculations
	m_particlesBuffer->CopyCPUToGPU(m_GPUparticles.data(), m_GPUparticles.size() * sizeof(GPUFluidParticle));

	// Debugging inf/nans
	/*if (m_frame >= 0) {
		return;
	}*/

	// Apply Forces
	g_theRenderer->BindComputeMaterial(m_applyForcesCS);
	g_theRenderer->BindConstantBuffer(m_gameConstants, 3);
	g_theRenderer->BindRWStructuredBuffer(m_particlesBuffer, 0);
	g_theRenderer->BindRWStructuredBuffer(m_hashInfoBuffer, 1);
	g_theRenderer->BindRWStructuredBuffer(m_offsetsBuffer, 2);

	g_theRenderer->Dispatch(1, 1, 1);

	// UpdateNeighbors
	g_theRenderer->BindComputeMaterial(m_hashParticlesCS);
	g_theRenderer->BindConstantBuffer(m_gameConstants, 3);
	g_theRenderer->BindRWStructuredBuffer(m_particlesBuffer, 0);
	g_theRenderer->BindRWStructuredBuffer(m_hashInfoBuffer, 1);
	g_theRenderer->BindRWStructuredBuffer(m_offsetsBuffer, 2);
	g_theRenderer->Dispatch(1, 1, 1);

	SortGPUParticles(1024, 0);
	g_theRenderer->BindComputeMaterial(m_lambdaCS);

	for (int iteration = 0; iteration < m_debugConfig.m_iterations; iteration++) {
		g_theRenderer->BindConstantBuffer(m_gameConstants, 3);
		g_theRenderer->BindRWStructuredBuffer(m_particlesBuffer, 0);
		g_theRenderer->BindRWStructuredBuffer(m_hashInfoBuffer, 1);
		g_theRenderer->BindRWStructuredBuffer(m_offsetsBuffer, 2);
		g_theRenderer->Dispatch(1, 1, 1);
	}

	g_theRenderer->BindComputeMaterial(m_updateMovementCS);
	g_theRenderer->BindConstantBuffer(m_gameConstants, 3);
	g_theRenderer->BindRWStructuredBuffer(m_particlesBuffer, 0);
	g_theRenderer->BindRWStructuredBuffer(m_hashInfoBuffer, 1);
	g_theRenderer->BindRWStructuredBuffer(m_offsetsBuffer, 2);
	g_theRenderer->Dispatch(1, 1, 1);
}

void Basic3DMode::RenderCPUParticles() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	{
		g_theRenderer->SetRenderTarget(m_backgroundRT);
		g_theRenderer->ClearRenderTarget(0, Rgba8(m_debugConfig.m_clearColor));
		g_theRenderer->ClearDepth();
		g_theRenderer->BindMaterial(nullptr);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);
		g_theRenderer->BindTexture(nullptr);
		RenderEntities();

		Light firstLight = {};
		Rgba8::WHITE.GetAsFloats(firstLight.Color);
		firstLight.Enabled = true;
		firstLight.LightType = 1;
		firstLight.Position = Vec3(1.0f, 1.0f, 1.0f);
		firstLight.Direction = Vec3(0.0f, 0.0f, -1.0f);
		firstLight.QuadraticAttenuation = 0.0f;
		firstLight.LinearAttenuation = 0.05f;
		firstLight.ConstantAttenuation = 0.025f;

		DebugAddWorldPoint(firstLight.Position, 0.1f, 0.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);

		g_theRenderer->SetLight(firstLight, 0);
		g_theRenderer->SetDepthRenderTarget(m_depthTexture);
		g_theRenderer->ClearDepth(1.0f);
		g_theRenderer->BindLightConstants();
		g_theRenderer->BindMaterial(m_prePassMaterial);
		RenderParticles();

		g_theRenderer->BindMaterial(m_thicknessMaterial);
		g_theRenderer->SetRenderTarget(m_thickness);
		g_theRenderer->ClearRenderTarget(0, Rgba8::BLACK);
		RenderParticles();

	}
	g_theRenderer->EndCamera(m_worldCamera);
}

void Basic3DMode::Shutdown()
{
	pointerToSelf = nullptr;


	GameMode::Shutdown();

	UnsubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	UnsubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	UnsubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	UnsubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	UnsubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	UnsubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	UnsubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	UnsubscribeEventCallbackFunction("Controls", GetControls);

	m_particles.clear();
	m_verts.clear();

	delete m_gameConstants;
	m_gameConstants = nullptr;

	delete m_particlesMeshInfo;
	m_particlesMeshInfo = nullptr;

	delete m_particlesBuffer;
	m_particlesBuffer = nullptr;

	delete m_offsetsBuffer;
	m_offsetsBuffer = nullptr;

	delete m_hashInfoBuffer;
	m_hashInfoBuffer = nullptr;

	delete m_meshVBuffer;
	m_meshVBuffer = nullptr;

	delete m_meshletBuffer;
	m_meshletBuffer = nullptr;

	delete m_thicknessHPassCBuffer;
	m_thicknessHPassCBuffer = nullptr;

	delete m_thicknessVPassCBuffer;
	m_thicknessVPassCBuffer = nullptr;

	delete m_depthHPassCBuffer;
	m_depthHPassCBuffer = nullptr;

	delete m_depthVPassCBuffer;
	m_depthVPassCBuffer = nullptr;

	if (m_particlesBuffer) {
		delete m_particlesBuffer;
		m_particlesBuffer = nullptr;
	}

	if (m_offsetsBuffer) {
		delete m_offsetsBuffer;
		m_offsetsBuffer = nullptr;
	}

	if (m_hashInfoBuffer) {
		delete m_hashInfoBuffer;
		m_hashInfoBuffer = nullptr;
	}
}

void Basic3DMode::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	GameMode::UpdateDeveloperCheatCodes(deltaSeconds);


	if (g_theInput->WasKeyJustPressed('1')) {
		FireEvent("DebugAddWorldWireSphere");
	}

	if (g_theInput->WasKeyJustPressed('2')) {
		FireEvent("DebugAddWorldLine");
	}

	if (g_theInput->WasKeyJustPressed('3')) {
		FireEvent("DebugAddBasis");
	}

	if (g_theInput->WasKeyJustPressed('4')) {
		FireEvent("DebugAddBillboardText");
	}

	if (g_theInput->WasKeyJustPressed('5')) {
		FireEvent("DebugAddWorldWireCylinder");
	}

	if (g_theInput->WasKeyJustPressed('6')) {
		EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
		std::string cameraOrientationInfo = Stringf("Camera Orientation: (%.2f, %.2f, %.2f)", cameraOrientation.m_yawDegrees, cameraOrientation.m_pitchDegrees, cameraOrientation.m_rollDegrees);
		DebugAddMessage(cameraOrientationInfo, 5.0f, Rgba8::WHITE, Rgba8::RED);
	}

	if (g_theInput->WasKeyJustPressed('9')) {
		Clock const& debugClock = DebugRenderGetClock();
		double newTimeDilation = debugClock.GetTimeDilation();
		newTimeDilation -= 0.1;
		if (newTimeDilation < 0.1) newTimeDilation = 0.1;
		DebugRenderSetTimeDilation(newTimeDilation);
	}

	if (g_theInput->WasKeyJustPressed('0')) {
		Clock const& debugClock = DebugRenderGetClock();
		double newTimeDilation = debugClock.GetTimeDilation();
		newTimeDilation += 0.1;
		if (newTimeDilation > 10.0) newTimeDilation = 10.0;
		DebugRenderSetTimeDilation(newTimeDilation);
	}
}

void Basic3DMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_game->ExitToAttractScreen();
	}


	for (int effectIndex = 0; effectIndex < (int)MaterialEffect::NUM_EFFECTS; effectIndex++) {
		int keyCode = 97 + effectIndex; // 97 is 1 numpad
		if (g_theInput->WasKeyJustPressed((unsigned short)keyCode)) {
			m_applyEffects[effectIndex] = !m_applyEffects[effectIndex];
		}
	}
	if (g_theInput->WasKeyJustPressed('F')) {
		m_applyEffects[0] = !m_applyEffects[0];
	}
	if (g_theInput->WasKeyJustPressed('G')) {
		m_applyEffects[1] = !m_applyEffects[1];
	}
	if (g_theInput->WasKeyJustPressed('J')) {
		m_applyEffects[2] = !m_applyEffects[2];
	}
	if (g_theInput->WasKeyJustPressed('K')) {
		m_applyEffects[3] = !m_applyEffects[3];
	}
	if (g_theInput->WasKeyJustPressed('L')) {
		m_applyEffects[4] = !m_applyEffects[4];
	}
}

bool Basic3DMode::DebugSpawnWorldWireSphere(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldWireSphere(pointerToSelf->m_player->m_position, 1, 5, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Basic3DMode::DebugSpawnWorldLine3D(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldLine(pointerToSelf->m_player->m_position, Vec3::ZERO, 0.125f, 5.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::XRAY);
	return false;
}

bool Basic3DMode::DebugClearShapes(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugRenderClear();
	return false;
}

bool Basic3DMode::DebugToggleRenderMode(EventArgs& eventArgs)
{
	static bool isDebuggerRenderSystemVisible = true;
	UNUSED(eventArgs);

	isDebuggerRenderSystemVisible = !isDebuggerRenderSystemVisible;
	if (isDebuggerRenderSystemVisible) {
		DebugRenderSetVisible();
	}
	else {
		DebugRenderSetHidden();
	}

	return false;
}

bool Basic3DMode::DebugSpawnPermanentBasis(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	Mat44 invertedView = pointerToSelf->m_worldCamera.GetViewMatrix();
	DebugAddWorldBasis(invertedView.GetOrthonormalInverse(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	return false;
}

bool Basic3DMode::DebugSpawnWorldWireCylinder(EventArgs& eventArgs)
{
	UNUSED(eventArgs);

	float cylinderHeight = 2.0f;
	Vec3 cylbase = pointerToSelf->m_player->m_position;
	cylbase.z -= cylinderHeight * 0.5f;

	Vec3 cylTop = cylbase;
	cylTop.z += cylinderHeight;
	DebugAddWorldWireCylinder(cylbase, cylTop, 0.5f, 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Basic3DMode::DebugSpawnBillboardText(EventArgs& eventArgs)
{
	Player* const& player = pointerToSelf->m_player;
	std::string playerInfo = Stringf("Position: (%f, %f, %f)\nOrientation: (%f, %f, %f)", player->m_position.x, player->m_position.y, player->m_position.z, player->m_orientation.m_yawDegrees, player->m_orientation.m_pitchDegrees, player->m_orientation.m_rollDegrees);
	DebugAddWorldBillboardText(playerInfo, player->m_position, 0.25f, Vec2(0.5f, 0.5f), 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	UNUSED(eventArgs);
	return false;
}

bool Basic3DMode::GetControls(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	std::string controlsStr = "";
	controlsStr += "~       - Toggle dev console\n";
	controlsStr += "ESC     - Exit to attract screen\n";
	controlsStr += "F8      - Reset game\n";
	controlsStr += "W/S/A/D - Move forward/backward/left/right\n";
	controlsStr += "Z/C     - Move upward/downward\n";
	controlsStr += "Q/E     - Roll to left/right\n";
	controlsStr += "Mouse   - Aim camera\n";
	controlsStr += "Shift   - Sprint\n";
	controlsStr += "T       - Slow mode\n";
	controlsStr += "Y       - Fast mode\n";
	controlsStr += "O       - Step frame\n";
	controlsStr += "P       - Toggle pause\n";
	controlsStr += "1       - Add debug wire sphere\n";
	controlsStr += "2       - Add debug world line\n";
	controlsStr += "3       - Add debug world basis\n";
	controlsStr += "4       - Add debug world billboard text\n";
	controlsStr += "5       - Add debug wire cylinder\n";
	controlsStr += "6       - Add debug camera message\n";
	controlsStr += "9       - Decrease debug clock speed\n";
	controlsStr += "0       - Increase debug clock speed";

	Strings controlStringsSplit = SplitStringOnDelimiter(controlsStr, '\n');

	for (int stringIndex = 0; stringIndex < controlStringsSplit.size(); stringIndex++) {
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, controlStringsSplit[stringIndex]);
	}

	return false;
}

void Basic3DMode::DisplayClocksInfo() const
{
	Clock& devClock = g_theConsole->m_clock;
	Clock const& debugClock = DebugRenderGetClock();

	double devClockFPS = 1.0 / devClock.GetDeltaTime();
	double gameFPS = m_fps;
	double debugClockFPS = 1.0 / debugClock.GetDeltaTime();

	double devClockTotalTime = devClock.GetTotalTime();
	double gameTotalTime = m_clock.GetTotalTime();
	double debugTotalTime = debugClock.GetTotalTime();

	double devClockScale = devClock.GetTimeDilation();
	double gameScale = m_clock.GetTimeDilation();
	double debugScale = debugClock.GetTimeDilation();


	std::string devClockInfo = Stringf("Dev Console:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", devClockTotalTime, devClockFPS, devClockScale);
	std::string debugClockInfo = Stringf("Debug Render:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", debugTotalTime, debugClockFPS, debugScale);
	std::string gameClockInfo = Stringf("Game:\t | Time: %.2f  FPS: %.2f  Scale: %.2f", gameTotalTime, gameFPS, gameScale);

	Vec2 devClockInfoPos(m_UISize.x - g_squirrelFont->GetTextWidth(m_textCellHeight, devClockInfo), m_UISize.y - m_textCellHeight);
	Vec2 gameClockInfoPos(m_UISize.x - g_squirrelFont->GetTextWidth(m_textCellHeight, gameClockInfo), m_UISize.y - (m_textCellHeight * 2.0f));
	Vec2 debugClockInfoPos(m_UISize.x - g_squirrelFont->GetTextWidth(m_textCellHeight, debugClockInfo), m_UISize.y - (m_textCellHeight * 3.0f));

	DebugAddScreenText(devClockInfo, devClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(debugClockInfo, debugClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(gameClockInfo, gameClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
}

void Basic3DMode::RenderGPUParticles() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	{
		g_theRenderer->SetRenderTarget(m_backgroundRT);
		g_theRenderer->ClearRenderTarget(0, Rgba8(m_debugConfig.m_clearColor));
		g_theRenderer->ClearDepth();
		g_theRenderer->BindMaterial(nullptr);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);
		g_theRenderer->BindTexture(nullptr);
		RenderEntities();

		Light firstLight = {};
		Rgba8::WHITE.GetAsFloats(firstLight.Color);
		firstLight.Enabled = true;
		firstLight.LightType = 1;
		firstLight.Position = Vec3(1.0f, 1.0f, 1.0f);
		firstLight.Direction = Vec3(0.0f, 0.0f, -1.0f);
		firstLight.QuadraticAttenuation = 0.0f;
		firstLight.LinearAttenuation = 0.05f;
		firstLight.ConstantAttenuation = 0.025f;

		DebugAddWorldPoint(firstLight.Position, 0.1f, 0.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);

		g_theRenderer->SetLight(firstLight, 0);
		g_theRenderer->SetDepthRenderTarget(m_depthTexture);
		g_theRenderer->ClearDepth(1.0f);
		g_theRenderer->BindLightConstants();
		g_theRenderer->BindMaterial(m_prePassGPUMaterial);


		unsigned int dispatchThreadAmount = static_cast<unsigned int>(ceilf(1024.f / 64.0f));
		// DepthPrepass
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindStructuredBuffer(m_particlesBuffer, 1);
		g_theRenderer->BindConstantBuffer(m_gameConstants, 3);

		g_theRenderer->SetModelMatrix(Mat44());
		g_theRenderer->SetModelColor(Rgba8::WHITE);

		g_theRenderer->DispatchMesh(dispatchThreadAmount, 1, 1);

		// ThicknessPass
		g_theRenderer->BindMaterial(m_thicknessMaterial);
		g_theRenderer->SetRenderTarget(m_thickness);
		g_theRenderer->ClearRenderTarget(0, Rgba8::BLACK);

		g_theRenderer->DispatchMesh(dispatchThreadAmount, 1, 1);

	}
	g_theRenderer->EndCamera(m_worldCamera);
}

void Basic3DMode::UpdateParticles(float deltaSeconds)
{
	m_fluidSolver.Update(deltaSeconds);
	//for (int vertIndex = 0, particleIndex = 0; vertIndex < m_verts.size(); vertIndex += m_vertsPerParticle, particleIndex++) {
	//	FluidParticle const& particle = m_particles[particleIndex];
	//	//Vertex_PCU* vertices = &m_verts[vertIndex];
	//	//TransformVertexArray3D(m_vertsPerParticle, vertices, Mat44(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), particle.m_position - particle.m_prevPos));
	//}

	for (int particleIndex = 0; particleIndex < m_particles.size(); particleIndex++) {
		FluidParticle const& particle = m_particles[particleIndex];
		FluidParticleMeshInfo& particleMesh = m_particlesMeshInfo[particleIndex];
		particleMesh.Position = particle.m_position;
		Rgba8(0, 0, 160, 120).GetAsFloats(particleMesh.Color);
	}

	StructuredBuffer* currentVBuffer = m_meshVBuffer;
	currentVBuffer->CopyCPUToGPU(m_particlesMeshInfo, sizeof(FluidParticleMeshInfo) * m_particles.size());

}

void Basic3DMode::RenderParticles() const
{
	//for (int particleIndex = 0; particleIndex < m_particles.size(); particleIndex++) {
	//	FluidParticle const& particle = m_particles[particleIndex];
	//	AddVertsForSphere(verts, 0.15f, 2, 4);
	//}
	unsigned int dispatchThreadAmount = static_cast<unsigned int>(ceilf(static_cast<float>(m_particles.size()) / 64.0f));
	StructuredBuffer* currentVBuffer = m_meshVBuffer;

	GameConstants gameConstants = {};
	EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
	gameConstants.CameraUp = cameraOrientation.GetZUp(), 0.0f;
	gameConstants.CameraLeft = cameraOrientation.GetYLeft(), 0.0f;
	gameConstants.EyePosition = m_worldCamera.GetViewPosition();
	gameConstants.SpriteRadius = m_fluidSolver.GetRenderingRadius();

	m_gameConstants->CopyCPUToGPU(&gameConstants, sizeof(GameConstants));
	//g_theRenderer->BindMaterial(m_prePassMaterial);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindStructuredBuffer(currentVBuffer, 1);
	g_theRenderer->BindStructuredBuffer(m_meshletBuffer, 2);
	g_theRenderer->BindConstantBuffer(m_gameConstants, 3);

	g_theRenderer->SetModelMatrix(Mat44());
	g_theRenderer->SetModelColor(Rgba8(m_debugConfig.m_waterColor));

	g_theRenderer->DispatchMesh(dispatchThreadAmount, 1, 1);
	//g_theRenderer->DrawVertexArray(m_verts);

}

void Basic3DMode::RenderBlurPass() const
{
	std::vector<Vertex_PCU> dummyVec(3);
	{
		g_theRenderer->BeginCamera(m_worldCamera);

		g_theRenderer->BindMaterial(m_gaussianBlurMaterial);
		g_theRenderer->SetRenderTarget(m_blurredThickness[m_currentBlurredThickness]);
		g_theRenderer->ClearRenderTarget(0, Rgba8::BLACK);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
		for (int passIndex = 0; passIndex < m_debugConfig.m_blurPassCount; passIndex++) {
			if (passIndex == 0) {
				g_theRenderer->BindTexture(m_thickness);
			}
			else {
				m_currentBlurredThickness = (m_currentBlurredThickness + 1) % 2;
				unsigned int prevBlurTex = (m_currentBlurredThickness + 1) % 2;

				g_theRenderer->BindTexture(m_blurredThickness[prevBlurTex]);
			}

			g_theRenderer->SetRenderTarget(m_blurredThickness[m_currentBlurredThickness]);
			g_theRenderer->BindConstantBuffer(m_thicknessHPassCBuffer, 4);
			g_theRenderer->DrawVertexArray(dummyVec);

			m_currentBlurredThickness = (m_currentBlurredThickness + 1) % 2;
			unsigned int prevBlurTex = (m_currentBlurredThickness + 1) % 2;
			g_theRenderer->BindTexture(m_blurredThickness[prevBlurTex]);
			g_theRenderer->SetRenderTarget(m_blurredThickness[m_currentBlurredThickness]);

			g_theRenderer->BindConstantBuffer(m_thicknessVPassCBuffer, 4);
			g_theRenderer->DrawVertexArray(dummyVec);
		}


		g_theRenderer->SetRenderTarget(m_blurredDepthTexture[m_currentBlurredDepth]);
		g_theRenderer->ClearRenderTarget(0, Rgba8::BLACK);

		for (int passIndex = 0; passIndex < m_debugConfig.m_blurPassCount; passIndex++) {
			if (passIndex == 0) {
				g_theRenderer->BindTexture(m_depthTexture);
			}
			else {
				m_currentBlurredDepth = (m_currentBlurredDepth + 1) % 2;
				unsigned int prevBlurTex = (m_currentBlurredDepth + 1) % 2;

				g_theRenderer->BindTexture(m_blurredDepthTexture[prevBlurTex]);
			}

			g_theRenderer->SetRenderTarget(m_blurredDepthTexture[m_currentBlurredDepth]);
			g_theRenderer->BindConstantBuffer(m_depthHPassCBuffer, 4);
			g_theRenderer->DrawVertexArray(dummyVec);

			m_currentBlurredDepth = (m_currentBlurredDepth + 1) % 2;
			unsigned int prevBlurTex = (m_currentBlurredDepth + 1) % 2;
			g_theRenderer->BindTexture(m_blurredDepthTexture[prevBlurTex]);
			g_theRenderer->SetRenderTarget(m_blurredDepthTexture[m_currentBlurredDepth]);

			g_theRenderer->BindConstantBuffer(m_depthVPassCBuffer, 4);
			g_theRenderer->DrawVertexArray(dummyVec);
		}

		g_theRenderer->EndCamera(m_worldCamera);

	}
}
