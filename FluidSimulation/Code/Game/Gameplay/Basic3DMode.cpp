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
	Vec4 CameraUp;
	Vec4 CameraLeft;
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

	m_effectsMaterials[(int)MaterialEffect::ColorBanding] = g_theMaterialSystem->GetMaterialForName("ColorBandFX");
	m_effectsMaterials[(int)MaterialEffect::Grayscale] = g_theMaterialSystem->GetMaterialForName("GrayScaleFX");
	m_effectsMaterials[(int)MaterialEffect::Inverted] = g_theMaterialSystem->GetMaterialForName("InvertedColorFX");
	m_effectsMaterials[(int)MaterialEffect::Pixelized] = g_theMaterialSystem->GetMaterialForName("PixelizedFX");
	m_effectsMaterials[(int)MaterialEffect::DistanceFog] = g_theMaterialSystem->GetMaterialForName("DistanceFogFX");

	std::filesystem::path materialPath("Data/Materials/DepthPrePass");
	m_prePassMaterial = g_theMaterialSystem->CreateOrGetMaterial(materialPath);

	FluidSolverConfig config = {};
	config.m_particlePerSide = 10;
	config.m_pointerToParticles = &m_particles;
	config.m_simulationBounds = m_particlesBounds;
	config.m_iterations = 5;
	config.m_kernelRadius = 0.196f;
	config.m_renderingRadius = config.m_kernelRadius;
	config.m_restDensity = 1000.0f;


	m_fluidSolver = FluidSolver(config);
	m_fluidSolver.InitializeParticles();

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
	bufferDesc.debugName = "Mesh VBuffer 0";
	m_meshVBuffer[0] = new StructuredBuffer(bufferDesc);

	bufferDesc.debugName = "Mesh VBuffer 1";
	m_meshVBuffer[1] = new StructuredBuffer(bufferDesc);

	bufferDesc.data = meshlets.data();
	bufferDesc.size = sizeof(Meshlet) * meshlets.size();
	bufferDesc.stride = sizeof(Meshlet);

	bufferDesc.debugName = "Meshlet Buffer";
	m_meshletBuffer = new StructuredBuffer(bufferDesc);
	//m_meshletBuffer->CopyCPUToGPU(meshlets.data(), sizeof(Meshlet) * meshlets.size());

	GameConstants dummyData = {};

	BufferDesc cBufferDesc = {};
	cBufferDesc.data = nullptr;
	cBufferDesc.memoryUsage = MemoryUsage::Default;
	cBufferDesc.owner = g_theRenderer;
	cBufferDesc.size = sizeof(GameConstants);
	cBufferDesc.stride = sizeof(GameConstants);
	cBufferDesc.debugName = "Game Constants";

	m_gameConstants = new ConstantBuffer(cBufferDesc);
	m_gameConstants->Initialize();


	TextureCreateInfo depthTextureInfo = {};
	depthTextureInfo.m_bindFlags = ResourceBindFlagBit::RESOURCE_BIND_DEPTH_STENCIL_BIT | ResourceBindFlagBit::RESOURCE_BIND_SHADER_RESOURCE_BIT;
	depthTextureInfo.m_format = TextureFormat::D32_FLOAT;
	depthTextureInfo.m_owner = g_theRenderer;
	depthTextureInfo.m_clearColour = Rgba8(0, 0, 0, 0);
	depthTextureInfo.m_clearFormat = TextureFormat::D32_FLOAT;
	depthTextureInfo.m_dimensions = g_theWindow->GetClientDimensions();
	depthTextureInfo.m_owner = g_theRenderer;
	depthTextureInfo.m_name = "Prepass DRT";

	m_depthTexture = g_theRenderer->CreateTexture(depthTextureInfo);

}

void Basic3DMode::Update(float deltaSeconds)
{
	m_currentVBuffer = (m_currentVBuffer + 1) % 2;

	m_fps = 1.0f / deltaSeconds;
	GameMode::Update(deltaSeconds);

	UpdateInput(deltaSeconds);
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();


	std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);
	std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);
	DebugAddMessage(gameInfoStr, 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	DebugAddWorldWireBox(m_particlesBounds, 0.0f, Rgba8(255, 0, 0, 20), Rgba8::RED, DebugRenderMode::USEDEPTH);

	UpdateDeveloperCheatCodes(deltaSeconds);
	UpdateEntities(deltaSeconds);
	UpdateParticles(deltaSeconds);

	DisplayClocksInfo();


}

void Basic3DMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	{
		g_theRenderer->ClearScreen(Rgba8::BLACK);
		g_theRenderer->BindMaterial(nullptr);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);
		g_theRenderer->BindTexture(nullptr);
		RenderEntities();

		g_theRenderer->SetDepthRenderTarget(m_depthTexture);
		g_theRenderer->ClearDepth(0.0f);
		RenderParticles();

	}
	g_theRenderer->EndCamera(m_worldCamera);


	g_theRenderer->BeginCamera(m_UICamera);
	{
		AABB2 quad(Vec2::ZERO, m_UISize * 0.25f);
		std::vector<Vertex_PCU> verts;
		AddVertsForAABB2D(verts, quad, Rgba8::WHITE, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f));
		g_theRenderer->BindMaterialByName("LinearDepthVisualizer");
		//g_theRenderer->BindDepthAsTexture();
		g_theRenderer->BindTexture(m_depthTexture);
		g_theRenderer->DrawVertexArray(verts);

	}
	g_theRenderer->EndCamera(m_UICamera);



	for (int effectInd = 0; effectInd < (int)MaterialEffect::NUM_EFFECTS; effectInd++) {
		if (m_applyEffects[effectInd]) {
			g_theRenderer->ApplyEffect(m_effectsMaterials[effectInd], &m_worldCamera);
		}
	}

	GameMode::Render();

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

	delete m_meshVBuffer[0];
	delete m_meshVBuffer[1];
	m_meshVBuffer[0] = nullptr;
	m_meshVBuffer[1] = nullptr;

	delete m_meshletBuffer;
	m_meshletBuffer = nullptr;
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
		Rgba8(0,0,160,120).GetAsFloats(particleMesh.Color);
	}

	StructuredBuffer* currentVBuffer = m_meshVBuffer[m_currentVBuffer];
	currentVBuffer->CopyCPUToGPU(m_particlesMeshInfo, sizeof(FluidParticleMeshInfo) * m_particles.size());

}

void Basic3DMode::RenderParticles() const
{
	//for (int particleIndex = 0; particleIndex < m_particles.size(); particleIndex++) {
	//	FluidParticle const& particle = m_particles[particleIndex];
	//	AddVertsForSphere(verts, 0.15f, 2, 4);
	//}
	unsigned int dispatchThreadAmount = static_cast<unsigned int>(ceilf(static_cast<float>(m_particles.size()) / 64.0f));
	StructuredBuffer* currentVBuffer = m_meshVBuffer[m_currentVBuffer];

	GameConstants gameConstants = {};
	EulerAngles cameraOrientation = m_worldCamera.GetViewOrientation();
	gameConstants.CameraUp = Vec4(cameraOrientation.GetZUp(), 0.0f);
	gameConstants.CameraLeft = Vec4(cameraOrientation.GetYLeft(), 0.0f);
	gameConstants.EyePosition = m_worldCamera.GetViewPosition();
	gameConstants.SpriteRadius = m_fluidSolver.GetRenderingRadius();

	m_gameConstants->CopyCPUToGPU(&gameConstants, sizeof(GameConstants));
	g_theRenderer->BindMaterial(m_prePassMaterial);
	//g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindStructuredBuffer(currentVBuffer, 1);
	g_theRenderer->BindStructuredBuffer(m_meshletBuffer, 2);
	g_theRenderer->BindConstantBuffer(m_gameConstants, 3);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetModelMatrix(Mat44());
	g_theRenderer->SetModelColor(Rgba8::WHITE);

	g_theRenderer->DispatchMesh(dispatchThreadAmount, 1, 1);
	//g_theRenderer->DrawVertexArray(m_verts);

}
