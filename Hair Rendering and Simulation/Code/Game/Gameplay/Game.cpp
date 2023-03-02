#include "Engine/Core/Time.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Mesh.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Prop.hpp"
#include "Game/Gameplay/HairDisc.hpp"
#include "Game/Gameplay/HairSphere.hpp"
#include "Game/Gameplay/HairCollision.hpp"
#include "ThirdParty/ImGUI/imgui.h"
#include "ThirdParty/ImGUI/imgui_impl_dx11.h"
#include "ThirdParty/Squirrel/SmoothNoise.hpp"

struct ImGuiSettingsInfo {
	Light m_sceneLights[8] = {};
	int m_selectedLight = 0;
	int m_amountOfLights = 0;
	float m_distFromObject = 5.0f;
	bool m_requiresShutdown = false;
	bool m_restartRequested = false;
	float m_hairColor[4];
	float m_ambientLight = 0.25f;
	int m_SSAOLevels = 1;
	int m_hairCurliness = 10;
	bool m_refreshModels = false;
	int m_interpPositionsFrom = 1; // 0 Model 1 Other Hairs
	int m_interpPositionsInADir = 0; // 0 Radius 1 Plane
	int m_usedShader = 0; // 0 Marschner 1 Kajiya
	float m_windForce = 0.5f;
#ifdef ENGINE_ANTIALIASING
	bool m_isAntialiasingOn = true;
	unsigned int m_aaLevel = 1;
#else
	bool m_isAntialiasingOn = false;
	unsigned int m_aaLevel = 1;
#endif
};

ImGuiSettingsInfo imGuiSettings = {};

extern bool g_drawDebug;
extern App* g_theApp;
extern AudioSystem* g_theAudio;
Game* pointerToSelf = nullptr;

struct CollisionConstants {
	HairCollisionObject CollisionObjects[30] = {};
};


struct SSAOConstants {
	SSAOConstants() = default;
	SSAOConstants(SSAOConstants const& copyFrom) = default;
	float MaxOcclusionPerSample;
	float SSAOFalloff;
	float SampleRadius;
	int SampleSize;
	bool UseSSAO = true;
};


CollisionConstants g_CollisionConstants;

Game::Game(App* appPointer) :
	m_theApp(appPointer)
{
	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);

	m_worldCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system
	pointerToSelf = this;

	SubscribeEventCallbackFunction("DebugAddWorldWireSphere", DebugSpawnWorldWireSphere);
	SubscribeEventCallbackFunction("DebugAddWorldLine", DebugSpawnWorldLine3D);
	SubscribeEventCallbackFunction("DebugRenderClear", DebugClearShapes);
	SubscribeEventCallbackFunction("DebugRenderToggle", DebugToggleRenderMode);
	SubscribeEventCallbackFunction("DebugAddBasis", DebugSpawnPermanentBasis);
	SubscribeEventCallbackFunction("DebugAddWorldWireCylinder", DebugSpawnWorldWireCylinder);
	SubscribeEventCallbackFunction("DebugAddBillboardText", DebugSpawnBillboardText);
	SubscribeEventCallbackFunction("Controls", GetControls);
	SubscribeEventCallbackFunction("ImportMesh", ImportMesh);
	SubscribeEventCallbackFunction("ScaleMesh", ScaleMesh);
	SubscribeEventCallbackFunction("TransformMesh", TransformMesh);
	SubscribeEventCallbackFunction("ReverseWinding", ReverseWindingOrder);
	SubscribeEventCallbackFunction("Save", SaveToBinary);
	SubscribeEventCallbackFunction("Load", LoadFromBinary);
	SubscribeEventCallbackFunction("InvertUV", InvertUV);

	m_startTime = GetCurrentTimeSeconds();

}

Game::~Game()
{
	ShutdownHairCollision();
	delete m_hairConstantBuffer;
	delete m_currentConstants;
	m_currentConstants = nullptr;

	delete m_prevConstants;
	m_prevConstants = nullptr;

	delete m_currentSSAO;
	m_currentSSAO = nullptr;

	delete m_prevSSAO;
	m_prevSSAO = nullptr;


}

HairConstants* Game::GetHairConstants() const
{
	return m_currentConstants;
}

bool Game::UseModelVertsForInterp() const
{
	return imGuiSettings.m_interpPositionsFrom == 0;
}

void Game::Startup()
{
	LoadAssets();
	InitializeHairProperties();
	if (m_deltaTimeSample) {
		delete m_deltaTimeSample;
		m_deltaTimeSample = nullptr;
	}

	m_deltaTimeSample = new double[m_fpsSampleSize];
	m_storedDeltaTimes = 0;
	m_totalDeltaTimeSample = 0.0f;

	g_squirrelFont = g_theRenderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");

	if (!m_hairConstantBuffer) {
		m_hairConstantBuffer = new ConstantBuffer(g_theRenderer->m_device, sizeof(HairConstants));
	}
	m_modelImportOptions = new ModelLoadingData();
	m_modelImportOptions->m_name = "Model to load";
	m_modelImportOptions->m_basis = "i,j,k";

	switch (m_currentState)
	{
	case GameState::AttractScreen:
		StartupAttractScreen();
		break;
	case GameState::HairDiscLighting:
		StartupHairDiscLighting();
		break;
	case GameState::HairSphereLighting:
		StartupHairSphereLighting();
		break;
	case GameState::HairTessellation:
		StartupHairTessellation();
		break;
	case GameState::HairSimulation:
		StartupHairSimulation();
		break;
	case GameState::EngineLogo:
		StartupLogo();
		break;
	default:
		break;
	}

	DebugAddMessage("Hello", 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void Game::StartupLogo()
{
	if (m_showEngineLogo) {
		m_logoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Engine Logo.png");
	}
	else {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::StartupAttractScreen()
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;


	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}

	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	DebugRenderClear();
}

void Game::StartupHairDiscLighting()
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Player* player = new Player(this, Vec3::ZERO, &m_worldCamera);
	m_player = player;
	m_player->m_orientation = m_lastPlayerOrientation;
	m_player->m_position = m_lastPlayerPos;
	//Prop* gridProp = new Prop(this, Vec3::ZERO, PropRenderType::GRID);


	HairObjectInit startingParams = {};
	startingParams.m_startingPosition = Vec3(1.0f, 1.0f, 0.0f);
	startingParams.m_shader = m_diffuseKajiya;


	m_hairObjectKajiya = new HairDisc(this, startingParams);

	startingParams.m_startingPosition = Vec3(1.0f, 15.0f, 0.0f);
	startingParams.m_shader = m_diffuseMarschner;
	m_hairObjectMarschner = new HairDisc(this, startingParams);

	DebugAddWorldBillboardText("Kajiya", m_hairObjectKajiya->m_position + Vec3(0.0f, 0.0f, 1.0f), 1.0f, Vec2(0.5f, 0.5f), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);
	DebugAddWorldBillboardText("Marschner", m_hairObjectMarschner->m_position + Vec3(0.0f, 0.0f, 1.0f), 1.0f, Vec2(0.5f, 0.5f), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);

	m_allEntities.push_back(player);
	//m_allEntities.push_back(gridProp);
	m_allEntities.push_back(m_hairObjectMarschner);
	m_allEntities.push_back(m_hairObjectKajiya);

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

	Light& firstLight = imGuiSettings.m_sceneLights[0];
	firstLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(firstLight.Color);
	firstLight.Position = m_hairObjectKajiya->m_position + Vec3(0.0f, 2.0f, 3.0f);

	Light& secLight = imGuiSettings.m_sceneLights[1];
	secLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(secLight.Color);
	secLight.Position = m_hairObjectMarschner->m_position + Vec3(0.0f, 2.0f, 3.0f);

	imGuiSettings.m_amountOfLights += 2;

	m_prevConstants->SpecularCoefficient = m_hairObjectKajiya->m_specularCoefficient;
	m_prevConstants->DiffuseCoefficient = m_hairObjectKajiya->m_diffuseCoefficient;

	m_currentConstants->DiffuseCoefficient = m_prevConstants->DiffuseCoefficient;
	m_currentConstants->SpecularCoefficient = m_prevConstants->SpecularCoefficient;
	m_currentConstants->SpecularExponent = m_prevConstants->SpecularExponent;

	m_isUsingKajiya = true;
	m_isUsingMarschner = true;

	//float axisLabelTextHeight = 0.25f;
	//Mat44 xLabelTransformMatrix(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3::ZERO);
	//float xLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "X - Forward");
	//xLabelTransformMatrix.SetTranslation3D(Vec3(xLabelWidth * 0.7f, 0.0f, axisLabelTextHeight));

	//DebugAddWorldText("X - Forward", xLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);

	//Mat44 yLabelTransformMatrix(Vec3(0.0f, 1.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3::ZERO);
	//float yLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Y - Left");
	//yLabelTransformMatrix.SetTranslation3D(Vec3(-axisLabelTextHeight, yLabelWidth * 0.7f, 0.0f));

	//DebugAddWorldText("Y - Left", yLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::GREEN, Rgba8::GREEN, DebugRenderMode::USEDEPTH);

	//Mat44 zLabelTransformMatrix(Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3::ZERO);
	//float zLabelWidth = g_squirrelFont->GetTextWidth(axisLabelTextHeight, "Z - Up");
	//zLabelTransformMatrix.SetTranslation3D(Vec3(0.0f, axisLabelTextHeight, zLabelWidth * 0.7f));

	//DebugAddWorldText("Z - Up", zLabelTransformMatrix, axisLabelTextHeight, Vec2(0.5f, 0.5f), -1.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);

}

void Game::StartupHairSphereLighting()
{

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Player* player = new Player(this, Vec3::ZERO, &m_worldCamera);
	m_player = player;

	m_player->m_orientation = m_lastPlayerOrientation;
	m_player->m_position = m_lastPlayerPos;
	//Prop* gridProp = new Prop(this, Vec3::ZERO, PropRenderType::GRID);

	HairObjectInit startingParams = {};
	startingParams.m_startingPosition = Vec3(1.0f, 1.0f, 0.0f);
	startingParams.m_shader = m_diffuseKajiya;

	m_hairObjectKajiya = new HairSphere(this, startingParams);


	startingParams.m_startingPosition = Vec3(1.0f, 15.0f, 0.0f);
	startingParams.m_shader = m_diffuseMarschner;
	m_hairObjectMarschner = new HairSphere(this, startingParams);

	AddHairCollisionSphere(m_hairObjectKajiya->m_position, HairObject::Radius);
	AddHairCollisionSphere(m_hairObjectMarschner->m_position, HairObject::Radius);

	DebugAddWorldBillboardText("Kajiya", m_hairObjectKajiya->m_position + Vec3(0.0f, 0.0f, 1.0f), 1.0f, Vec2(0.5f, 0.5f), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);
	DebugAddWorldBillboardText("Marschner", m_hairObjectMarschner->m_position + Vec3(0.0f, 0.0f, 1.0f), 1.0f, Vec2(0.5f, 0.5f), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);

	m_allEntities.push_back(player);
	//m_allEntities.push_back(gridProp);
	m_allEntities.push_back(m_hairObjectMarschner);
	m_allEntities.push_back(m_hairObjectKajiya);

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);


	Light& firstLight = imGuiSettings.m_sceneLights[0];
	firstLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(firstLight.Color);
	firstLight.Position = m_hairObjectKajiya->m_position + Vec3(0.0f, 2.0f, 3.0f);

	Light& secLight = imGuiSettings.m_sceneLights[1];
	secLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(secLight.Color);
	secLight.Position = m_hairObjectMarschner->m_position + Vec3(0.0f, 2.0f, 3.0f);

	imGuiSettings.m_amountOfLights += 2;

	m_prevConstants->SpecularCoefficient = m_hairObjectKajiya->m_specularCoefficient;
	m_prevConstants->DiffuseCoefficient = m_hairObjectKajiya->m_diffuseCoefficient;

	m_currentConstants->DiffuseCoefficient = m_prevConstants->DiffuseCoefficient;
	m_currentConstants->SpecularCoefficient = m_prevConstants->SpecularCoefficient;
	m_currentConstants->SpecularExponent = m_hairObjectKajiya->m_specularExponent;


	m_isUsingKajiya = true;
	m_isUsingMarschner = true;
}

void Game::StartupHairTessellation()
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Player* player = new Player(this, Vec3::ZERO, &m_worldCamera);
	m_player = player;

	m_player->m_orientation = m_lastPlayerOrientation;
	m_player->m_position = m_lastPlayerPos;

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	g_theInput->ResetMouseClientDelta();

	m_allEntities.push_back(player);


	m_prevConstants->SpecularCoefficient = 0.5f;
	m_prevConstants->DiffuseCoefficient = 0.5f;

	m_currentConstants->DiffuseCoefficient = m_prevConstants->DiffuseCoefficient;
	m_currentConstants->SpecularCoefficient = m_prevConstants->SpecularCoefficient;
	m_currentConstants->SpecularExponent = 4;


	//m_hairObjectKajiya = new HairDiscTessellation(this, Vec3(-5.0, -5.0f, 0.0f), m_diffuseTessKajiya);

	Image* densityTest = new Image("Data/Images/DensityMapTest.png");

	HairObjectInit startingParams = {};
	startingParams.m_startingPosition = Vec3(6.0, 1.0f, 0.0f);
	startingParams.m_shader = m_diffuseTessMarschner;
	startingParams.m_multInterpShader = m_diffuseMultTessMarschner;

	startingParams.m_hairDensityMap = densityTest;

	Plane3D limitingPlane = {};
	limitingPlane.m_planeNormal = Vec3(1.0f, 0.0f, 0.0f);
	limitingPlane.m_distToPlane = startingParams.m_startingPosition.GetLength() - 0.4f;
	m_currentConstants->LimitingPlane = limitingPlane;

	m_hairObjectMarschner = new HairSphereTessellation(this, startingParams);
	AddHairCollisionSphere(m_hairObjectMarschner->m_position, HairObject::Radius);
	//m_allEntities.push_back(m_hairObjectKajiya);
	m_allEntities.push_back(m_hairObjectMarschner);
	m_allHairObjects.push_back(m_hairObjectMarschner);

	m_prevSegmentLength = HairGuide::HairSegmentLength;

	Light& firstLight = imGuiSettings.m_sceneLights[0];
	firstLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(firstLight.Color);
	firstLight.Position = m_hairObjectMarschner->m_position + Vec3(0.0f, 2.0f, 3.0f);

	Light& secLight = imGuiSettings.m_sceneLights[1];
	secLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(secLight.Color);
	secLight.Position = m_hairObjectMarschner->m_position + Vec3(0.0f, 2.0f, -3.0f);

	imGuiSettings.m_amountOfLights += 2;

	//m_isUsingKajiya = true;
	m_isUsingMarschner = true;
}

void Game::StartupHairSimulation()
{

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	Player* player = new Player(this, Vec3::ZERO, &m_worldCamera);
	m_player = player;

	m_player->m_orientation = m_lastPlayerOrientation;
	m_player->m_position = m_lastPlayerPos;

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	DebugAddWorldBasis(Mat44(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	g_theInput->ResetMouseClientDelta();

	m_allEntities.push_back(player);



	m_prevConstants->SpecularCoefficient = 0.5f;
	m_prevConstants->DiffuseCoefficient = 0.5f;

	m_currentConstants->DiffuseCoefficient = m_prevConstants->DiffuseCoefficient;
	m_currentConstants->SpecularCoefficient = m_prevConstants->SpecularCoefficient;
	m_currentConstants->SpecularExponent = 4;

	m_prevSegmentLength = HairGuide::HairSegmentLength;

	m_isUsingKajiya = false;
	m_isUsingMarschner = false;

	Vec3 hairPos = m_player->m_position + Vec3(4.0f, 2.0f, 0.0f);

	HairSimulationInit initialParams = {};
	initialParams.mass = m_currentConstants->Mass;
	initialParams.normal = Vec3(1.0f, 0.0f, 1.0f);
	initialParams.position = hairPos;
	initialParams.edgeStiffness = m_currentConstants->EdgeStiffness;
	initialParams.bendStiffness = m_currentConstants->BendStiffness;
	initialParams.torsionStiffness = m_currentConstants->TorsionStiffness;
	initialParams.damping = m_currentConstants->DampingCoefficient;
	initialParams.segmentCount = HairGuide::HairSegmentCount;
	initialParams.usedAlgorithm = (SimulAlgorithm)m_currentConstants->SimulationAlgorithm;
	initialParams.isCurlyHair = m_currentConstants->IsHairCurly;

	m_CPUSimulObject = new HairSimulGuide(initialParams); // If stiffness is too high, simulation kind of explodes. This is a good empirical value
	DebugAddWorldBillboardText("Simulated CPU", m_CPUSimulObject->m_positions[0] + Vec3(0.0, 0.0f, 1.0f), 0.20f, Vec2(0.5f, 0.5f), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);

	Light& firstLight = imGuiSettings.m_sceneLights[0];
	firstLight.Enabled = true;
	Rgba8::WHITE.GetAsFloats(firstLight.Color);
	firstLight.Position = hairPos + Vec3(1.0f, 1.0f, 3.0f);

	imGuiSettings.m_amountOfLights++;

	float _throwawayEdgeLegth = 0.0f;
	m_CPUSimulObject->GetSpringLengths(_throwawayEdgeLegth, m_currentConstants->BendInitialLength, m_currentConstants->TorsionInitialLength);


}

void Game::InitializeHairProperties()
{
	if (m_initializedHairProperties) return;

	HairGuide::HairWidth = g_gameConfigBlackboard.GetValue("HAIR_SEGMENT_WIDTH", 0.006f);
	HairGuide::HairSegmentLength = g_gameConfigBlackboard.GetValue("HAIR_SEGMENT_LENGTH", 0.25f);
	HairGuide::HairSegmentCount = static_cast<unsigned int>(g_gameConfigBlackboard.GetValue("HAIR_SEGMENT_COUNT", 12));

	m_prevSegmentLength = HairGuide::HairSegmentLength;
	imGuiSettings.m_requiresShutdown = false;
	imGuiSettings.m_restartRequested = false;
	m_initializedHairProperties = true;

	HairObject::HairPerSection = g_gameConfigBlackboard.GetValue("DISC_HAIR_PER_SECTION", 1600);
	HairObject::SectionCount = g_gameConfigBlackboard.GetValue("DISC_SECTION_COUNT", 12);
	HairObject::Radius = g_gameConfigBlackboard.GetValue("DISC_RADIUS", 5.0f);
	HairObject::SliceCount = g_gameConfigBlackboard.GetValue("SPHERE_SLICE_COUNT", 12);
	HairObject::StackCount = g_gameConfigBlackboard.GetValue("SPHERE_STACK_COUNT", 12);

	m_diffuseKajiya = g_theRenderer->CreateOrGetShader("Data/Shaders/HairKajiya.hlsl");
	m_diffuseTessKajiya = g_theRenderer->CreateOrGetShader("Data/Shaders/HairKajiyaTess.hlsl");

	m_diffuseMarschner = g_theRenderer->CreateOrGetShader("Data/Shaders/HairMarschner.hlsl");
	m_diffuseMarschnerCPUSim = g_theRenderer->CreateOrGetShader("Data/Shaders/HairMarschnerCPUSim.hlsl");
	m_diffuseTessMarschner = g_theRenderer->CreateShader("Data/Shaders/HairMarschnerTess.hlsl");

	m_copyShader = g_theRenderer->CreateCSOnlyShader("Data/Shaders/HairToUAV.hlsl");

	m_diffuseMultTessMarschner = g_theRenderer->CreateOrGetShader("Data/Shaders/HairMarschnerTessMultInterp.hlsl");
	m_diffuseMultTessKajiya = g_theRenderer->CreateOrGetShader("Data/Shaders/HairKajiyaTessMultInterp.hlsl");

	m_DFTLCShader = g_theRenderer->CreateCSOnlyShader("Data/Shaders/CSDFTLHairSim");
	m_MassSpringsCurlyCShader = g_theRenderer->CreateCSOnlyShader("Data/Shaders/CSMassSpringCurlySim");
	m_MassSpringsStraightShader = g_theRenderer->CreateCSOnlyShader("Data/Shaders/CSMassSpringStraightSim");

	m_SSAO = g_theRenderer->CreateOrGetShader("Data/Shaders/SSAO.hlsl");
	m_SampleDownsizer = g_theRenderer->CreateOrGetShader("Data/Shaders/DownsampleDepthBuffer.hlsl");
	m_brown.GetAsFloats(imGuiSettings.m_hairColor);

	g_theRenderer->SetAmbientIntensity(Rgba8(0.4f));

	m_lastPlayerPos = Vec3(-15.0f, 7.0f, 0.0f);

	m_currentConstants = new HairConstants();
	m_currentConstants->ScaleShift = -10.0f;
	m_currentConstants->LongitudinalWidth = 10.0f;
	m_currentConstants->UseUnrealParameters = true;
	m_currentConstants->SpecularMarchner = 0.5f;
	m_currentConstants->InterpolationFactor = 1;
	m_currentConstants->TessellationFactor = 1.0f;
	m_currentConstants->InterpolationRadius = 1.0f;
	m_currentConstants->EdgeStiffness = 1200.0f;
	m_currentConstants->BendStiffness = 1200.0f;
	m_currentConstants->TorsionStiffness = 1200.0f;
	m_currentConstants->DampingCoefficient = g_gameConfigBlackboard.GetValue("HAIR_DAMPING_COEFFICIENT", 0.925f);
	m_currentConstants->IsHairCurly = g_gameConfigBlackboard.GetValue("IS_HAIR_CURLY", true);
	m_currentConstants->SegmentLength = HairGuide::HairSegmentLength;
	m_currentConstants->Mass = 3.0f;
	m_currentConstants->SimulationAlgorithm = static_cast<unsigned int>(SimulAlgorithm::DFTL);
	m_currentConstants->FrictionCoefficient = g_gameConfigBlackboard.GetValue("HAIR_FRICTION", 0.1f);
	m_currentConstants->GridCellWidth = g_gameConfigBlackboard.GetValue("GRID_CELL_WIDTH", 1.0f);
	m_currentConstants->GridCellHeight = g_gameConfigBlackboard.GetValue("GRID_CELL_HEIGHT", 1.0f);
	m_currentConstants->GridDimensions = g_gameConfigBlackboard.GetValue("GRID_DIMENSIONS", IntVec3(100, 100, 100));
	m_currentConstants->CollisionTolerance = g_gameConfigBlackboard.GetValue("COLLISION_TOLERANCE", 0.1f);
	m_currentConstants->InterpolationFactor = g_gameConfigBlackboard.GetValue("HAIR_INTERPOLATION", 1);
	m_currentConstants->InterpolationFactorMultiStrand = g_gameConfigBlackboard.GetValue("HAIR_INTERPOLATION_MULTISTRAND", 2);
	m_currentConstants->InterpolationRadius = g_gameConfigBlackboard.GetValue("HAIR_INTERPOLATION_RADIUS", 0.1f);
	m_currentConstants->TessellationFactor = g_gameConfigBlackboard.GetValue("HAIR_TESELLATION", 1.0f);
	m_currentConstants->StrainLimitingCoefficient = g_gameConfigBlackboard.GetValue("HAIR_STRAIN_LIMITING_COEFFICIENT", 0.5f);

	m_prevConstants = new HairConstants(*m_currentConstants);

	m_currentSSAO = new SSAOConstants();
	imGuiSettings.m_SSAOLevels = g_gameConfigBlackboard.GetValue("SSAO_LEVEL", 5);
	SetSSAOSettings();

	m_prevSSAO = new SSAOConstants(*m_currentSSAO);

	StartupHairCollision();
	FetchAvailableModels();
	/*TextureCreateInfo shadowMapCreateInfo;
	Window* window = Window::GetWindowContext();

	shadowMapCreateInfo.m_name = "Hair Shadow Map";
	shadowMapCreateInfo.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT | TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
	shadowMapCreateInfo.m_format = TextureFormat::R24G8_TYPELESS;
	shadowMapCreateInfo.m_dimensions = IntVec2(window->GetClientDimensions());
	shadowMapCreateInfo.m_memoryUsage = MemoryUsage::GPU;

	m_shadowMap = g_theRenderer->CreateTexture(shadowMapCreateInfo);*/
}

void Game::ShutdownHairDiscLighting()
{
	m_lastPlayerPos = m_player->m_position;
	m_lastPlayerOrientation = m_player->m_orientation;

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_hairObjectKajiya = nullptr;
	m_player = nullptr;

	m_hairObjectMarschner = nullptr;

}

void Game::ShutdownHairSphereLighting()
{
	m_lastPlayerPos = m_player->m_position;
	m_lastPlayerOrientation = m_player->m_orientation;

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_player = nullptr;
	m_hairObjectKajiya = nullptr;
	m_hairObjectMarschner = nullptr;
}

void Game::ShutdownHairTessellation()
{
	m_lastPlayerPos = m_player->m_position;
	m_lastPlayerOrientation = m_player->m_orientation;

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_player = nullptr;
	m_hairObjectKajiya = nullptr;
	m_hairObjectMarschner = nullptr;

}

void Game::ShutdownHairSimulation()
{
	m_lastPlayerPos = m_player->m_position;
	m_lastPlayerOrientation = m_player->m_orientation;

	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_player = nullptr;
	m_hairObjectKajiya = nullptr;
	m_hairObjectMarschner = nullptr;

	delete m_CPUSimulObject;
	m_CPUSimulObject = nullptr;

}

void Game::UpdateGameState()
{
	if (m_currentState != m_nextState || imGuiSettings.m_restartRequested) {
		ShutDown();
		m_currentState = m_nextState;
		Startup();
	}
}

void Game::UpdateHairConstants()
{
	delete m_prevConstants;
	m_prevConstants = m_currentConstants;

	m_currentConstants = new HairConstants(*m_prevConstants);
	m_currentConstants->EyePosition = GetPlayerPosition();
	m_currentConstants->HairWidth = HairGuide::HairWidth;
	m_currentConstants->SegmentLength = HairGuide::HairSegmentLength;
	m_currentConstants->StartTime = static_cast<float>(m_startTime);
	m_currentConstants->HairSegmentCount = HairGuide::HairSegmentCount;
	m_currentConstants->InterpolateUsingRadius = !((bool)imGuiSettings.m_interpPositionsInADir); // Radius is 0, Plane is 1, so !

	g_theRenderer->SetAmbientIntensity(imGuiSettings.m_ambientLight);
	g_theRenderer->CopyCPUToGPU(m_currentConstants, sizeof(HairConstants), m_hairConstantBuffer);

	m_currentConstants->ExternalForces = Vec3::ZERO;


	delete m_prevSSAO;
	m_prevSSAO = m_currentSSAO;

	m_currentSSAO = new SSAOConstants(*m_prevSSAO);

	g_theRenderer->SetSSAOSampleRadius(m_currentSSAO->SampleRadius);
	g_theRenderer->SetSSAOFalloff(m_currentSSAO->SSAOFalloff);
	g_theRenderer->SetSampleSize(m_currentSSAO->SampleSize);
	g_theRenderer->SetOcclusionPerSample(m_currentSSAO->MaxOcclusionPerSample);
}

void Game::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		if (entity) {
			entity->Update(deltaSeconds);
		}
	}
}

void Game::UpdateLogo(float deltaSeconds)
{
	UpdateInputLogo(deltaSeconds);

	m_timeShowingLogo += deltaSeconds;

	if (m_timeShowingLogo > m_engineLogoLength) {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::UpdateInputLogo(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) || controller.WasButtonJustPressed(XboxButtonID::A)) {
		m_nextState = GameState::AttractScreen;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}

}

void Game::UpdateHairSimulation(float deltaSeconds)
{
	UpdateEntities(deltaSeconds);
	UpdateInputHairSimulation(deltaSeconds);

	m_CPUSimulObject->AddForce(Vec3(0.0f, 0.0f, -9.8f));
	m_CPUSimulObject->Update(deltaSeconds);
	m_CPUSimulObject->m_hairSimulationInitParams.usedAlgorithm = (SimulAlgorithm)m_currentConstants->SimulationAlgorithm;
	m_CPUSimulObject->SetSpringLengths(HairGuide::HairSegmentLength, m_currentConstants->BendInitialLength, m_currentConstants->TorsionInitialLength);
	m_CPUSimulObject->SetSpringStiffness(m_currentConstants->EdgeStiffness, m_currentConstants->BendStiffness, m_currentConstants->TorsionStiffness);
	m_CPUSimulObject->m_hairSimulationInitParams.isCurlyHair = m_currentConstants->IsHairCurly;
	m_CPUSimulObject->m_gravity = m_currentConstants->Gravity;




	DebugAddMessage(Stringf("Mode: Hair Simulation"), 0.0f, Rgba8::BLUE, Rgba8::BLUE);
	DebugAddMessage("Press B to add wind (dependent on distance to object)", 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	DisplayClocksInfo();
}

void Game::UpdateInputHairSimulation(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}

	if (g_theInput->IsKeyDown('B')) {
		Vec3 dispToObject = m_CPUSimulObject->m_positions[0] - m_player->m_position;
		float distToObject = dispToObject.GetLength();

		float forceMultiplier = RangeMapClamped(distToObject, 0.0f, 3.0f, 80.0f, 5.0f);

		Vec3 force = dispToObject.GetNormalized() * forceMultiplier;
		m_CPUSimulObject->AddForce(force);

		m_currentConstants->ExternalForces += force;
	}
	if (g_theInput->IsKeyDown('G')) {
		Vec3 dispToObject = m_CPUSimulObject->m_positions[0] - m_player->m_position;
		float distToObject = dispToObject.GetLength();

		float forceMultiplier = RangeMapClamped(distToObject, 0.0f, 3.0f, 80.0f, 5.0f);

		Vec3 force = Vec3(0.0f, 0.0f, 1.0f) * forceMultiplier;
		m_CPUSimulObject->AddForce(force);

		m_currentConstants->ExternalForces += force;
	}

}

void Game::UpdateHairTessellation(float deltaSeconds)
{
	UpdateEntities(deltaSeconds);
	UpdateHairTessellationInput(deltaSeconds);

	//if (imGuiSettings.m_clearFaceTechnique == 1) { // 1 for wind
	m_currentConstants->ExternalForces += Vec3(sinf(m_timeAlive * 1.34f), cosf(m_timeAlive * 1.98f), 0.0f) * imGuiSettings.m_windForce; // Random picked values
	//}

	DebugAddMessage(Stringf("Mode: Hair Tessellation"), 0.0f, Rgba8::BLUE, Rgba8::BLUE);
	DebugAddMessage("Press B to add wind (dependent on distance to object)", 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	DisplayClocksInfo();
}

void Game::UpdateHairTessellationInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE) || controller.WasButtonJustPressed(XboxButtonID::A)) {
		m_nextState = GameState::AttractScreen;
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}




}

void Game::ShutDown()
{
	delete m_modelImportOptions;
	for (int soundIndexPlaybackID = 0; soundIndexPlaybackID < GAME_SOUND::NUM_SOUNDS; soundIndexPlaybackID++) {
		SoundPlaybackID soundPlayback = g_soundPlaybackIDs[soundIndexPlaybackID];
		if (soundPlayback != -1) {
			g_theAudio->StopSound(soundPlayback);
		}
	}
	m_useTextAnimation = false;
	m_currentText = "";
	m_transitionTextColor = false;
	m_timeTextAnimation = 0.0f;
	m_deltaTimeSample = nullptr;
	m_timeAlive = 0.0f;


	if (m_currentConstants) {
		Plane3D limitingPlane = {};
		m_currentConstants->LimitingPlane = limitingPlane;
	}

	switch (m_currentState)
	{
	case GameState::HairDiscLighting:
		ShutdownHairDiscLighting();
		break;
	case GameState::HairSphereLighting:
		ShutdownHairSphereLighting();
		break;
	case GameState::HairTessellation:
		ShutdownHairTessellation();
		break;
	case GameState::HairSimulation:
		ShutdownHairSimulation();
		break;
	default:
		break;
	}

	g_theRenderer->ClearLights();
	for (int lightIndex = 0; lightIndex < 8; lightIndex++) {
		imGuiSettings.m_sceneLights[lightIndex].Enabled = false;
	}

	imGuiSettings.m_amountOfLights = 0;

	ClearCollisionObjects();
	DebugRenderClear();
	imGuiSettings.m_requiresShutdown = false;
	imGuiSettings.m_restartRequested = false;
	m_isUsingKajiya = false;
	m_isUsingMarschner = false;
}

void Game::LoadAssets()
{
	if (m_loadedAssets) return;
	LoadTextures();
	LoadSoundFiles();

	m_loadedAssets = true;
}

void Game::LoadTextures()
{
	g_textures[(int)GAME_TEXTURE::TestUV] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
	g_textures[(int)GAME_TEXTURE::CompanionCube] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/CompanionCube.png");
	g_textures[(int)GAME_TEXTURE::SimplexNoise] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/SimplexNoise.png");
	g_textures[(int)GAME_TEXTURE::PerlinNoise] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PerlinNoise.png");

	for (int textureIndex = 0; textureIndex < (int)GAME_TEXTURE::NUM_TEXTURES; textureIndex++) {
		if (!g_textures[textureIndex]) {
			ERROR_RECOVERABLE(Stringf("FORGOT TO LOAD TEXTURE %d", textureIndex));
		}
	}
}

void Game::LoadSoundFiles()
{
	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		g_sounds[soundIndex] = (SoundID)-1;
	}

	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		g_soundPlaybackIDs[soundIndex] = (SoundPlaybackID)-1;
	}
	//winGameSound = g_theAudio->CreateOrGetSound("Data/Audio/WinGameSound.wav");
	//loseGameSound = g_theAudio->CreateOrGetSound("Data/Audio/LoseGameSound.wav");

	// Royalty free version of Clair De Lune found here:
	// https://soundcloud.com/pianomusicgirl/claire-de-lune
	g_soundSources[GAME_SOUND::CLAIRE_DE_LUNE] = "Data/Audio/ClaireDeLuneRoyaltyFreeFirstMinute.wav";
	g_soundSources[GAME_SOUND::DOOR_KNOCKING_SOUND] = "Data/Audio/KnockingDoor.wav";

	for (int soundIndex = 0; soundIndex < GAME_SOUND::NUM_SOUNDS; soundIndex++) {
		SoundID soundID = g_theAudio->CreateOrGetSound(g_soundSources[soundIndex]);
		g_sounds[soundIndex] = soundID;
		g_soundEnumIDBySource[g_soundSources[soundIndex]] = (GAME_SOUND)soundIndex;
	}
}

void Game::Update()
{
	AddDeltaToFPSCounter();
	CheckIfWindowHasFocus();
	if (imGuiSettings.m_refreshModels) {
		FetchAvailableModels();
		imGuiSettings.m_refreshModels = false;
	}

	float gameDeltaSeconds = static_cast<float>(m_clock.GetDeltaTime());
	m_timeAlive += gameDeltaSeconds;
	m_currentConstants->DeltaTime = gameDeltaSeconds;
	if ((SimulAlgorithm)m_currentConstants->SimulationAlgorithm == SimulAlgorithm::MASS_SPRINGS) {
		if (m_timeAlive <= m_simulationInitialSlowmo) {
			m_currentConstants->DeltaTime *= 0.05f;
		}
	}

	UpdateCameras(gameDeltaSeconds);
	UpdateGameState();

	if (m_areSSAOKernelsDirty) {
		CreateSSAOKernels();
		m_areSSAOKernelsDirty = false;
	}

	bool addHelperText = false;

	switch (m_currentState)
	{
	case GameState::AttractScreen:
		UpdateAttractScreen(gameDeltaSeconds);
		break;
	case GameState::HairDiscLighting:
		UpdateHairDiscLighting(gameDeltaSeconds);
		addHelperText = true;
		break;
	case GameState::HairSphereLighting:
		UpdateHairSphereLighting(gameDeltaSeconds);
		addHelperText = true;
		break;
	case GameState::HairTessellation:
		UpdateHairTessellation(gameDeltaSeconds);
		addHelperText = true;
		break;
	case GameState::HairSimulation:
		UpdateHairSimulation(gameDeltaSeconds);
		addHelperText = true;
		break;
	case GameState::EngineLogo:
		UpdateLogo(gameDeltaSeconds);
		break;
	default:
		break;
	}

	if (addHelperText) {
		std::string helperText = "Shift + F1 to enable/disable mouse";
		helperText += ", F6/F7 to switch scenes";

		DebugAddMessage(helperText, 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	}

	UpdateDeveloperCheatCodes(gameDeltaSeconds);
	UpdateDebugProperties();

	if (m_player) {
		UpdateHairConstants();
	}

}

void Game::CreateSSAOKernels()
{
	std::vector<Vec4> kernels;
	for (int sampleIndex = 0; sampleIndex < m_currentSSAO->SampleSize; sampleIndex++) {
		float position = rng.GetRandomFloatZeroUpToOne();
		float x = Compute1dPerlinNoise(position, 0.001f, 8, 0.5f, 160.0f, true, 0);
		float y = Compute1dPerlinNoise(position, 0.001f, 4, 0.25f, 200.0f, true, 1);
		float z = 0.5f + 0.5f * Compute1dPerlinNoise(position, 0.001f, 6, 0.8f, 250.0f, true, 2);

		kernels.emplace_back(x, y, z, 1.0f);
	}

	g_theRenderer->CreateSSAOKernelsUAV(kernels);
}

void Game::CheckIfWindowHasFocus()
{
	bool isDevConsoleShown = g_theConsole->GetMode() == DevConsoleMode::SHOWN;
	if (isDevConsoleShown) {
		g_theInput->ConsumeAllKeysJustPressed();
	}

	if (g_theWindow->HasFocus()) {
		if (m_lostFocusBefore) {
			g_theInput->ResetMouseClientDelta();
			m_lostFocusBefore = false;
		}
		if (isDevConsoleShown) {
			g_theInput->SetMouseMode(false, false, false);
		}
		else {
			g_theInput->SetMouseMode(m_isCursorHidden, m_isCursorClipped, m_isCursorRelative);
		}
	}
	else {
		m_lostFocusBefore = true;
	}
}

void Game::UpdateHairDiscLighting(float deltaSeconds)
{
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}

	UpdateInputHairDiscLighting(deltaSeconds);

	/*std::string playerPosInfo = Stringf("Player position: (%.2f, %.2f, %.2f)", m_player->m_position.x, m_player->m_position.y, m_player->m_position.z);
	DebugAddMessage(playerPosInfo, 0, Rgba8::WHITE, Rgba8::WHITE);
	std::string gameInfoStr = Stringf("Delta: (%.2f, %.2f)", mouseClientDelta.x, mouseClientDelta.y);*/
	//DebugAddMessage(gameInfoStr, 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	UpdateEntities(deltaSeconds);

	DebugAddMessage(Stringf("Mode: Hair Disc Lighting"), 0.0f, Rgba8::BLUE, Rgba8::BLUE);

	std::string lightControls = "I,J,K,L and Arrows to control light movement. Y/U & N/M to control light height";
	DebugAddMessage(lightControls, 0.0f, Rgba8::WHITE, Rgba8::WHITE);


	DisplayClocksInfo();
}

void Game::UpdateInputHairDiscLighting(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::UpdateHairSphereLighting(float deltaSeconds)
{
	UpdateEntities(deltaSeconds);
	UpdateInputHairSphereLighting(deltaSeconds);

	DebugAddMessage(Stringf("Mode: Hair Sphere Lighting"), 0.0f, Rgba8::BLUE, Rgba8::BLUE);
	DebugAddMessage("Press B to add wind (dependent on distance to object)", 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	DisplayClocksInfo();
}

void Game::UpdateInputHairSphereLighting(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_nextState = GameState::AttractScreen;
	}
}

void Game::UpdateAttractScreen(float deltaSeconds)
{
	//DebugRenderClear();
	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}
	m_AttractScreenCamera.SetOrthoView(Vec2(0.0f, 0.0f), Vec2(m_UISizeX, m_UISizeY));

	UpdateInputAttractScreen(deltaSeconds);
	m_timeAlive += deltaSeconds;

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICenterX, m_UISizeY * m_textAnimationPosPercentageTop));
	}
}

void Game::UpdateInputAttractScreen(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed(KEYCODE_SPACE);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::A);

	if (exitAttractMode) {
		m_nextState = (GameState)0;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_theApp->HandleQuitRequested();
	}
	UpdateDeveloperCheatCodes(deltaSeconds);
}

void Game::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	XboxController controller = g_theInput->GetController(0);
	Clock& sysClock = Clock::GetSystemClock();

	if (g_theInput->IsKeyDown('B')) {

		HairObject* usedObject = (m_hairObjectKajiya) ? m_hairObjectKajiya : m_hairObjectMarschner;
		Vec3 dispToObject = usedObject->m_position - m_player->m_position;
		float distToObject = dispToObject.GetLength();

		float forceMultiplier = RangeMapClamped(distToObject, 0.0f, 3.0f, -10.0f * m_currentConstants->Gravity, -2.0f * m_currentConstants->Gravity);

		Vec3 force = dispToObject.GetNormalized() * forceMultiplier;

		m_currentConstants->ExternalForces += force;
	}

	if (g_theInput->IsKeyDown('G')) {
		Vec3 force = Vec3(0.0f, 0.0f, -2.0f * m_currentConstants->Gravity);

		m_currentConstants->ExternalForces += force;
	}

	if (g_theInput->WasKeyJustPressed('T')) {
		sysClock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('O')) {
		Clock::GetSystemClock().StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('P')) {
		sysClock.TogglePause();
	}



	if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) {
		ShutDown();
		Startup();
	}

	if (g_theInput->WasKeyJustReleased('T')) {
		sysClock.SetTimeDilation(1);
	}

	if (g_theInput->WasKeyJustPressed('K')) {
		g_soundPlaybackIDs[GAME_SOUND::DOOR_KNOCKING_SOUND] = g_theAudio->StartSound(g_sounds[DOOR_KNOCKING_SOUND]);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE)) {
		g_theConsole->ToggleMode(DevConsoleMode::SHOWN);
		g_theInput->ResetMouseClientDelta();
	}

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

	if (g_theInput->WasKeyJustPressed(KEYCODE_F7)) {
		m_nextState = (GameState)(((int)m_currentState + 1) % ((int)GameState::NUM_GAME_STATES));
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F6)) {
		int nextState = (int)m_currentState - 1;
		if (nextState < 0) nextState = ((int)GameState::NUM_GAME_STATES - 1);

		m_nextState = (GameState)nextState;
	}


	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	if (m_player) {
		m_player->m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

		float speedUsed = m_player->m_playerNormalSpeed * 0.25f;
		float zSpeedUsed = m_player->m_playerZSpeed * 0.25f;

		iBasis.z = 0;
		jBasis.z = 0;

		Vec3 velocity = Vec3::ZERO;

		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			speedUsed *= 3.0f;
			zSpeedUsed *= 1.5f;
		}

		if (g_theInput->IsKeyDown(KEYCODE_UPARROW)) {
			velocity += iBasis * speedUsed;
		}

		if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW)) {
			velocity -= iBasis * speedUsed;
		}

		if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW)) {
			velocity += jBasis * speedUsed;
		}

		if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW)) {
			velocity -= jBasis * speedUsed;
		}


		if (g_theInput->IsKeyDown('N')) {
			velocity.z += zSpeedUsed;
		}

		if (g_theInput->IsKeyDown('M')) {
			velocity.z += -zSpeedUsed;
		}

		if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
			if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
				MouseState mouseState = g_theInput->GetMouseState();
				m_isCursorClipped = !m_isCursorClipped;
				m_isCursorHidden = !m_isCursorHidden;
				m_isCursorRelative = !m_isCursorRelative;

				g_theInput->SetMouseMode(mouseState);
			}
		}
		else {
			if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
				g_drawDebug = !g_drawDebug;
			}
		}

		Light& currentLight = imGuiSettings.m_sceneLights[imGuiSettings.m_selectedLight];
		currentLight.Position += velocity * deltaSeconds;
	}
}

void Game::UpdateCameras(float deltaSeconds)
{
	UpdateWorldCamera(deltaSeconds);

	UpdateUICamera(deltaSeconds);

}

void Game::UpdateWorldCamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	// #FixBeforeSubmitting
	//m_worldCamera.SetOrthoView(Vec2(-1, -1 ), Vec2(1, 1));
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.01f, 100.0f);
}

void Game::UpdateDebugProperties()
{

	if (m_prevConstants->DiffuseCoefficient != m_currentConstants->DiffuseCoefficient) {
		m_currentConstants->SpecularCoefficient = 1.0f - m_currentConstants->DiffuseCoefficient;
		m_prevConstants->SpecularCoefficient = m_currentConstants->SpecularCoefficient;
	}

	if (m_prevConstants->SpecularCoefficient != m_currentConstants->SpecularCoefficient) {
		m_currentConstants->DiffuseCoefficient = 1.0f - m_currentConstants->SpecularCoefficient;
		m_prevConstants->DiffuseCoefficient = m_currentConstants->DiffuseCoefficient;
	}

	bool isDisc = dynamic_cast<HairDisc*>(m_hairObjectKajiya) || dynamic_cast<HairDisc*>(m_hairObjectMarschner);
	bool isSphere = dynamic_cast<HairSphere*>(m_hairObjectKajiya) || dynamic_cast<HairSphere*>(m_hairObjectMarschner);

	if (isDisc) {
		int sectionCount = (m_hairObjectKajiya) ? m_hairObjectKajiya->m_sectionCount : m_hairObjectMarschner->m_sectionCount;
		if (!imGuiSettings.m_requiresShutdown && sectionCount != HairDisc::SectionCount) {
			imGuiSettings.m_requiresShutdown = true;
		}

	}
	else if (isSphere) {
		int sliceCount = (m_hairObjectKajiya) ? m_hairObjectKajiya->m_sliceCount : m_hairObjectMarschner->m_sliceCount;
		int stackCount = (m_hairObjectKajiya) ? m_hairObjectKajiya->m_stackCount : m_hairObjectMarschner->m_stackCount;

		if (!imGuiSettings.m_requiresShutdown) {
			if ((sliceCount != HairObject::SliceCount) || (stackCount != HairObject::StackCount)) {
				imGuiSettings.m_requiresShutdown = true;
			}
		}
	}

	if (m_hairObjectKajiya || m_hairObjectMarschner) {
		int hairPerSection = (m_hairObjectKajiya) ? m_hairObjectKajiya->m_hairPerSection : m_hairObjectMarschner->m_hairPerSection;
		if (!imGuiSettings.m_requiresShutdown && hairPerSection != HairDisc::HairPerSection) {
			imGuiSettings.m_requiresShutdown = true;
		}

		float radius = (m_hairObjectKajiya) ? m_hairObjectKajiya->m_radius : m_hairObjectMarschner->m_radius;
		if (!imGuiSettings.m_requiresShutdown && radius != HairDisc::Radius) {
			imGuiSettings.m_requiresShutdown = true;
		}


		if (!imGuiSettings.m_requiresShutdown && m_prevSegmentLength != HairGuide::HairSegmentLength) {
			imGuiSettings.m_requiresShutdown = true;
			m_prevSegmentLength = HairGuide::HairSegmentLength;
		}
	}




	switch (m_currentState)
	{
	case GameState::HairSphereLighting:
	case GameState::HairTessellation:
	case GameState::HairDiscLighting:
	{
		if (!m_hairObjectKajiya) return;
		m_hairObjectKajiya->m_specularExponent = m_currentConstants->SpecularExponent;
		m_hairObjectKajiya->m_specularCoefficient = m_currentConstants->SpecularCoefficient;
		m_hairObjectKajiya->m_diffuseCoefficient = m_currentConstants->DiffuseCoefficient;

		if (m_hairObjectMarschner) {
			m_hairObjectMarschner->m_specularExponent = m_currentConstants->SpecularExponent;
			m_hairObjectMarschner->m_specularCoefficient = m_currentConstants->SpecularCoefficient;
			m_hairObjectMarschner->m_diffuseCoefficient = m_currentConstants->DiffuseCoefficient;
		}

		break;
	}

	}


	m_prevConstants->DiffuseCoefficient = m_currentConstants->DiffuseCoefficient;
	m_prevConstants->SpecularCoefficient = m_currentConstants->SpecularCoefficient;
}

void Game::UpdateUICamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	m_UICamera.SetOrthoView(Vec2(), Vec2(m_UISizeX, m_UISizeY));
}


void Game::UpdateTextAnimation(float deltaTime, std::string text, Vec2 textLocation)
{
	m_timeTextAnimation += deltaTime;
	m_textAnimTriangles.clear();

	Rgba8 usedTextColor = m_textAnimationColor;
	if (m_transitionTextColor) {

		float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0, m_textAnimationTime);

		usedTextColor.r = static_cast<unsigned char>(Interpolate(m_startTextColor.r, m_endTextColor.r, timeAsFraction));
		usedTextColor.g = static_cast<unsigned char>(Interpolate(m_startTextColor.g, m_endTextColor.g, timeAsFraction));
		usedTextColor.b = static_cast<unsigned char>(Interpolate(m_startTextColor.b, m_endTextColor.b, timeAsFraction));
		usedTextColor.a = static_cast<unsigned char>(Interpolate(m_startTextColor.a, m_endTextColor.a, timeAsFraction));

	}

	g_squirrelFont->AddVertsForText2D(m_textAnimTriangles, Vec2::ZERO, m_textCellHeightAttractScreen, text, usedTextColor, 0.6f);

	float fullTextWidth = g_squirrelFont->GetTextWidth(m_textCellHeightAttractScreen, text, 0.6f);
	float textWidth = GetTextWidthForAnim(fullTextWidth);

	Vec2 iBasis = GetIBasisForTextAnim();
	Vec2 jBasis = GetJBasisForTextAnim();

	TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

	if (m_timeTextAnimation >= m_textAnimationTime) {
		m_useTextAnimation = false;
		m_timeTextAnimation = 0.0f;
	}

}


void Game::TransformText(Vec2 iBasis, Vec2 jBasis, Vec2 translation)
{
	for (int vertIndex = 0; vertIndex < int(m_textAnimTriangles.size()); vertIndex++) {
		Vec3& vertPosition = m_textAnimTriangles[vertIndex].m_position;
		TransformPositionXY3D(vertPosition, iBasis, jBasis, translation);
	}
}

float Game::GetTextWidthForAnim(float fullTextWidth) {

	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float textWidth = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			textWidth = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, fullTextWidth);
		}
		else {
			return fullTextWidth;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		return fullTextWidth;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			return fullTextWidth;
		}
		else {
			textWidth = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, fullTextWidth, 0.0f);
		}
	}
	return textWidth;

}

Vec2 const Game::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float iScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = RangeMapClamped(timeAsFraction, 0.0f, m_textMovementPhaseTimePercentage * 0.5f, 0.0f, 1.0f);
		}
		else {
			iScale = 1.0f;
		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		iScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			iScale = 1.0f;
		}
		else {
			iScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 1.0f, 0.0f);
		}
	}

	return Vec2(iScale, 0.0f);
}

Vec2 const Game::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeTextAnimation, 0.0f, m_textAnimationTime);
	float jScale = 0.0f;
	if (timeAsFraction < m_textMovementPhaseTimePercentage) {
		if (timeAsFraction < m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = 0.05f;
		}
		else {
			jScale = RangeMapClamped(timeAsFraction, m_textMovementPhaseTimePercentage * 0.5f, m_textMovementPhaseTimePercentage, 0.05f, 1.0f);

		}
	}
	else if (timeAsFraction > m_textMovementPhaseTimePercentage && timeAsFraction < 1 - m_textMovementPhaseTimePercentage) {
		jScale = 1.0f;
	}
	else {
		if (timeAsFraction < 1.0f - m_textMovementPhaseTimePercentage * 0.5f) {
			jScale = RangeMapClamped(timeAsFraction, 1.0f - m_textMovementPhaseTimePercentage, 1.0f - m_textMovementPhaseTimePercentage * 0.5f, 1.0f, 0.05f);
		}
		else {
			jScale = 0.05f;
		}
	}

	return Vec2(0.0f, jScale);
}

double Game::GetFPS() const
{
	if (m_storedDeltaTimes < m_fpsSampleSize) return 1 / Clock::GetSystemClock().GetDeltaTime();

	double fps = m_fpsSampleSize / m_totalDeltaTimeSample;

	return fps;
}

void Game::AddDeltaToFPSCounter()
{
	int prevIndex = m_currentFPSAvIndex;
	m_currentFPSAvIndex++;
	if (m_currentFPSAvIndex >= m_fpsSampleSize) m_currentFPSAvIndex = 0;
	m_deltaTimeSample[m_currentFPSAvIndex] = Clock::GetSystemClock().GetDeltaTime();

	m_totalDeltaTimeSample += Clock::GetSystemClock().GetDeltaTime();
	m_storedDeltaTimes++;

	if (m_storedDeltaTimes > m_fpsSampleSize) {
		m_totalDeltaTimeSample -= m_deltaTimeSample[prevIndex];
	}

}

void Game::DisplayClocksInfo() const
{
	Clock& devClock = g_theConsole->m_clock;
	Clock const& debugClock = DebugRenderGetClock();

	double devClockFPS = 1.0 / devClock.GetDeltaTime();
	double gameFPS = 1.0 / Clock::GetSystemClock().GetDeltaTime();
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

	Vec2 devClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, devClockInfo), 0);
	Vec2 gameClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, gameClockInfo), m_textCellHeight);
	Vec2 debugClockInfoPos(m_UISizeX - g_squirrelFont->GetTextWidth(m_textCellHeight, debugClockInfo), m_textCellHeight * 2);

	DebugAddScreenText(devClockInfo, devClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(debugClockInfo, debugClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddScreenText(gameClockInfo, gameClockInfoPos, 0.0f, Vec2(1.0f, 0.0f), m_textCellHeight, Rgba8::WHITE, Rgba8::WHITE);
}


void Game::Render() const
{
	g_theRenderer->ClearLights();
	for (int lightIndex = 0; lightIndex < 8; lightIndex++) {
		Light const& currentLight = imGuiSettings.m_sceneLights[lightIndex];
		g_theRenderer->SetLight(currentLight, lightIndex);

		Rgba8 color = Rgba8::BLUE;
		if (imGuiSettings.m_selectedLight == lightIndex) color = Rgba8::MAGENTA;
		if (currentLight.Enabled) {
			DebugAddWorldPoint(currentLight.Position, 0.1f, 0.0f, color, color, DebugRenderMode::ALWAYS);
		}
	}

	switch (m_currentState)
	{
	case GameState::AttractScreen:
		RenderAttractScreen();
		break;
	case GameState::HairDiscLighting:
		RenderHairDiscLighting();
		RenderImGui();
		break;
	case GameState::HairSphereLighting:
		RenderHairSphereLighting();
		RenderImGui();
		break;
	case GameState::HairTessellation:
		RenderHairTessellation();
		RenderImGui();
		break;
	case GameState::HairSimulation:
		RenderHairSimulation();
		RenderImGui();
		break;
	case GameState::EngineLogo:
		RenderLogo();
		break;
	default:
		break;
	}


	if (m_currentSSAO->UseSSAO) {

		Texture* singleSampleDepth = nullptr;
		if (g_theRenderer->IsAntialiasingOn()) {
			TextureCreateInfo infoDepth;
			infoDepth.m_name = "DefaultDepth";
			infoDepth.m_dimensions = g_theRenderer->GetActiveColorTarget()->GetDimensions();
			infoDepth.m_format = TextureFormat::R24G8_TYPELESS;
			infoDepth.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT | TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
			infoDepth.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT | TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
			infoDepth.m_memoryUsage = MemoryUsage::GPU;


			singleSampleDepth = g_theRenderer->CreateTexture(infoDepth);
			g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
			g_theRenderer->DownsampleTextureWithShader(singleSampleDepth, g_theRenderer->GetCurrentDepthTarget(), m_SampleDownsizer);
		}

		g_theRenderer->BindTexture(g_theRenderer->GetActiveColorTarget());
		g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::PerlinNoise], 2);
		g_theRenderer->CopyAndBindLightConstants();
		g_theRenderer->BindSSAOKernels(3);
		g_theRenderer->ApplyEffect(m_SSAO, &m_worldCamera, singleSampleDepth);

		g_theRenderer->DestroyTexture(singleSampleDepth);
	}


	DebugRenderWorld(m_worldCamera);
	RenderUI();
	DebugRenderScreen(m_UICamera);

}



void Game::RenderEntities() const
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity const* entity = m_allEntities[entityIndex];
		entity->Render();
	}
}

void Game::RenderHairSimulation() const
{

	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindConstantBuffer(4, m_hairConstantBuffer);

	g_theRenderer->SetModelColor(imGuiSettings.m_hairColor);
	g_theRenderer->CopyAndBindModelConstants();

	g_theRenderer->CopyAndBindLightConstants();

	if (m_hairObjectKajiya) {
		int hairCount = m_hairObjectKajiya->GetHairCount();
		int hairInterpolated = ((int)m_currentConstants->InterpolationFactor * hairCount) - hairCount;

		DebugAddMessage(Stringf("Hair Count: %d", hairCount), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
		DebugAddMessage(Stringf("Interp. Approx: %d", hairInterpolated), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
		DebugAddMessage(Stringf("Total. Approx: %d", hairCount + hairInterpolated), 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	}

	//RenderEntities();

	std::vector<Vertex_PNCU> simulTest;
	m_CPUSimulObject->AddVerts(simulTest);

	g_theRenderer->BindShader(m_diffuseMarschnerCPUSim);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindConstantBuffer(4, m_hairConstantBuffer);

	g_theRenderer->SetModelColor(Rgba8::WHITE);
	g_theRenderer->CopyAndBindModelConstants();

	g_theRenderer->DrawVertexArray(simulTest, TopologyMode::LINELIST);
	g_theRenderer->EndCamera(m_worldCamera);
}

void Game::RenderHairTessellation() const
{


	/*{
		Camera lightCamera;
		Vec3 iBasis(0.0f, -1.0f, 0.0f);
		Vec3 jBasis(0.0f, 0.0f, 1.0f);
		Vec3 kBasis(1.0f, 0.0f, 0.0f);

		lightCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis);
		Vec3 fwdToObject = m_hairObjectMarschner->m_position - pointLight.Position;
		fwdToObject.Normalize();

		lightCamera.SetPerspectiveView(m_worldCamera.GetAspect(), m_worldCamera.GetFovDegrees(), m_worldCamera.GetNear(), m_worldCamera.GetFar());
		lightCamera.SetPosition(pointLight.Position);
		lightCamera.SetDepthTarget(m_shadowMap);
		lightCamera.SetOrientation(EulerAngles::CreateEulerAngleFromForward(fwdToObject));

		g_theRenderer->BeginDepthOnlyCamera(lightCamera);
		g_theRenderer->ClearScreen(Rgba8::GRAY);
		g_theRenderer->BindShader(m_diffuseTessMarschner);

		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindConstantBuffer(4, m_hairConstantBuffer);

		g_theRenderer->SetModelColor(m_hairColor);
		g_theRenderer->CopyAndBindModelConstants();

		RenderEntities();
		g_theRenderer->EndCamera(lightCamera);
	}*/

	//g_theRenderer->SetLight(pointLight, 0);
	//g_theRenderer->SetLight(secPointLight, 1);
	//g_theRenderer->CopyAndBindLightConstants();
	{
		g_theRenderer->BeginCamera(m_worldCamera);
		g_theRenderer->ClearScreen(Rgba8::GRAY);

		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindConstantBuffer(4, m_hairConstantBuffer);

		g_theRenderer->SetModelColor(imGuiSettings.m_hairColor);
		g_theRenderer->CopyAndBindModelConstants();
		g_theRenderer->CopyAndBindLightConstants();



		RenderEntities();

		if (m_meshBuilder) {
			Shader* normalShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuse");
			g_theRenderer->BindShader(normalShader);
			g_theRenderer->CopyAndBindLightConstants();
			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray(m_meshBuilder->m_vertexes);
		}
		g_theRenderer->EndCamera(m_worldCamera);

	}


	/*HairGuide testHair = HairGuide(Vec3(10.0f, 10.0f, 0.0f), Rgba8::WHITE, Vec3(0.0f, 0.0f, 1.0f), 12);
	std::vector<Vertex_PNCU> hairVerts;
	testHair.AddVerts(hairVerts);
	g_theRenderer->DrawVertexArray(hairVerts, TopologyMode::CONTROL_POINT_PATCHLIST_2);*/

	int hairCountKajiya = 0;
	int hairInterpolated = 0;
	int hairCountMarschner = 0;

	if (m_hairObjectKajiya) {
		int kajiyaHairCount = m_hairObjectKajiya->GetHairCount();
		hairInterpolated += ((int)m_currentConstants->InterpolationFactor * kajiyaHairCount) - kajiyaHairCount;
		if (m_renderMultInterp) {
			float multStrandEstimate = 0.3f * m_hairObjectKajiya->GetMultiStrandBaseCount() * ((int)m_currentConstants->InterpolationFactorMultiStrand - 1);
			hairInterpolated += static_cast<int>(multStrandEstimate);
		}
	}

	if (m_hairObjectMarschner) {
		hairCountMarschner = m_hairObjectMarschner->GetHairCount();
		hairInterpolated += ((int)m_currentConstants->InterpolationFactor * hairCountMarschner) - hairCountMarschner;
		if (m_renderMultInterp) {
			float multStrandEstimate = 0.3f * m_hairObjectMarschner->GetMultiStrandBaseCount() * ((int)m_currentConstants->InterpolationFactorMultiStrand - 1);
			hairInterpolated += static_cast<int>(multStrandEstimate);
		}
	}

	int kajiyaAndMarschnerHairCount = hairCountMarschner + hairCountKajiya;
	DebugAddMessage(Stringf("Hair Count: %d", kajiyaAndMarschnerHairCount), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddMessage(Stringf("Interp. Approx: %d", hairInterpolated), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddMessage(Stringf("Total. Approx: %d", kajiyaAndMarschnerHairCount + hairInterpolated), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void Game::RenderHairDiscLighting() const
{

	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::GRAY);

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	g_theRenderer->SetModelColor(Rgba8(imGuiSettings.m_hairColor));

	if (m_hairObjectKajiya) {
		DebugAddMessage(Stringf("Hair Count: %d", m_hairObjectKajiya->GetHairCount()), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
	}

	RenderEntities();

	g_theRenderer->EndCamera(m_worldCamera);
}

void Game::RenderHairSphereLighting() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8::GRAY);

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	g_theRenderer->BindConstantBuffer(4, m_hairConstantBuffer);
	g_theRenderer->SetModelColor(imGuiSettings.m_hairColor);
	g_theRenderer->CopyAndBindModelConstants();

	if (m_hairObjectKajiya) {
		int hairCount = m_hairObjectKajiya->GetHairCount();
		int hairInterpolated = ((int)m_currentConstants->InterpolationFactor * hairCount) - hairCount;

		DebugAddMessage(Stringf("Hair Count: %d", hairCount), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
		DebugAddMessage(Stringf("Interp. Approx: %d", hairInterpolated), 0.0f, Rgba8::WHITE, Rgba8::WHITE);
		DebugAddMessage(Stringf("Total. Approx: %d", hairCount + hairInterpolated), 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	}


	RenderEntities();

	g_theRenderer->EndCamera(m_worldCamera);

}

void Game::RenderAttractScreen() const
{
	g_theRenderer->BeginCamera(m_AttractScreenCamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetDepthStencilState(DepthTest::ALWAYS, false);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);
	if (m_useTextAnimation) {
		RenderTextAnimation();
	}


	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	std::vector<Vertex_PCU> testTextVerts;
	AABB2 testTextureAABB2(740.0f, 150.0f, 1040.0f, 450.f);
	AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8());
	g_theRenderer->BindTexture(testTexture);
	g_theRenderer->DrawVertexArray((int)testTextVerts.size(), testTextVerts.data());
	g_theRenderer->BindTexture(nullptr);

	/*g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	Vertex_PCU vertices[] =
	{
	Vertex_PCU(Vec3(600.0f, 400.0f, 0.0f), Rgba8::CYAN, Vec2(0.0f, 0.0f)),
	Vertex_PCU(Vec3(1000.0f,  400.0f, 0.0f), Rgba8::BLUE, Vec2(0.0f, 0.0f)),
	Vertex_PCU(Vec3(800.0f, 600.0f, 0.0f), Rgba8::ORANGE, Vec2(0.0f, 0.0f)),
	};*/

	//g_theRenderer->DrawVertexArray(3, vertices);

	g_theRenderer->EndCamera(m_AttractScreenCamera);

}

void Game::RenderLogo() const
{

	AABB2 screen = m_UICamera.GetCameraBounds();
	AABB2 logo = screen.GetBoxWithin(Vec2(0.27f, 0.25f), Vec2(0.68f, 0.75f));
	AABB2 background = screen.GetBoxWithin(Vec2(0.34f, 0.305f), Vec2(0.605f, 0.72f));

	std::vector<Vertex_PCU> logoVerts;
	std::vector<Vertex_PCU> whiteBackgroundVerts;

	float logoAlpha = 0.0f;
	double third = m_engineLogoLength / 3.0;
	if (m_timeShowingLogo < third) {
		logoAlpha = RangeMapClamped((float)m_timeShowingLogo, 0.0f, (float)third, 0.0f, 1.0f);
	}
	else if (m_timeShowingLogo >= 2 * third) {
		logoAlpha = RangeMapClamped((float)m_timeShowingLogo, (float)third * 2.0f, (float)m_engineLogoLength, 1.0f, 0.0f);
	}
	else {
		logoAlpha = 1.0f;
	}


	Rgba8 logoColor = Rgba8::WHITE;

	logoColor.a = DenormalizeByte(logoAlpha);

	AddVertsForAABB2D(logoVerts, logo, logoColor);
	AddVertsForAABB2D(whiteBackgroundVerts, background, logoColor);

	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(whiteBackgroundVerts);
	g_theRenderer->BindTexture(m_logoTexture);
	g_theRenderer->DrawVertexArray(logoVerts);
}


void Game::RenderUI() const
{

	g_theRenderer->BeginCamera(m_UICamera);

	if (m_useTextAnimation && m_currentState != GameState::AttractScreen) {

		RenderTextAnimation();
	}

	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	AABB2 screenBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());


	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);

	std::vector<Vertex_PCU> gameInfoVerts;

	g_theConsole->Render(devConsoleBounds);
	g_theRenderer->EndCamera(m_UICamera);
}

void Game::RenderTextAnimation() const
{
	if (m_textAnimTriangles.size() > 0) {
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(int(m_textAnimTriangles.size()), m_textAnimTriangles.data());
	}
}

void Game::RenderImGui() const
{
	RenderImGuiRenderingSection();

	RenderImGuiPresetsSection();

	RenderImGuiSimulationSection();

	//ImGui::ShowDemoWindow();
}

void Game::RenderImGuiRenderingSection() const
{
	ImGuiWindowFlags winmdowFlags = ImGuiWindowFlags_AlwaysAutoResize;
	winmdowFlags |= ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Rendering Properties", NULL, winmdowFlags);

	ImGuiTreeNodeFlags_ renderingHeaderFlags = ImGuiTreeNodeFlags_DefaultOpen;

	if (m_currentState == GameState::HairTessellation) {
		const char* usedShaderOptions[] = { "Marschner", "Kajiya" };
		int currentShader = (imGuiSettings.m_usedShader);
		ImGui::Combo("Used Shader", &imGuiSettings.m_usedShader, usedShaderOptions, IM_ARRAYSIZE(usedShaderOptions));

		if (currentShader != imGuiSettings.m_usedShader) {
			if (imGuiSettings.m_usedShader == 0) { // Marschner;
				/*
				It should be m_hairObjectAny, but changing it now is a lot of work
				Just bear in mind that this object can be using either Marschner or Kajiya Shaders
				*/
				m_isUsingMarschner = true;
				m_isUsingKajiya = false;
				m_hairObjectMarschner->SetShader(m_diffuseTessMarschner);
				m_hairObjectMarschner->SetMutlInterpShader(m_diffuseMultTessMarschner);
			}
			else { // Kajiya
				m_isUsingMarschner = false;
				m_isUsingKajiya = true;
				m_hairObjectMarschner->SetShader(m_diffuseTessKajiya);
				m_hairObjectMarschner->SetMutlInterpShader(m_diffuseMultTessKajiya);
			}
		}

	}

	if (ImGui::CollapsingHeader("SSAO", renderingHeaderFlags)) {
		ImGui::Checkbox("Use SSAO", &m_currentSSAO->UseSSAO);
		if (m_prevSSAO->UseSSAO) {
			ImGui::SliderInt("Level", &imGuiSettings.m_SSAOLevels, 1, 10);
			SetSSAOSettings();
		}

	}

	if (ImGui::CollapsingHeader("Shader Constants", renderingHeaderFlags)) {

		ImGui::ColorEdit3("Diffuse", imGuiSettings.m_hairColor);
		float max = 100.0f;
		ImGui::SliderScalar("Hair Width", ImGuiDataType_Float, &HairGuide::HairWidth, &m_hairWidthMin, &max, "%.10f", ImGuiSliderFlags_Logarithmic);
		ImGui::DragFloat("Hair Segment Length", &HairGuide::HairSegmentLength, 0.005f, 0.05f, 5.0f, "%.3f", ImGuiSliderFlags_Logarithmic);

		//int prevInterpolation = m_currentConstants->InterpolationFactor;
		//int prevInterpolationMultStrand = m_currentConstants->InterpolationFactorMultiStrand;

		if (m_currentState == GameState::HairTessellation) {
			ImGui::SliderInt("Interpolation factor", &m_currentConstants->InterpolationFactor, 1, 5);
			const char* singleStrandInterpOptions[] = { "Radius", "Plane" };
			ImGui::Combo("Create Single-Strand Hair in a:", &imGuiSettings.m_interpPositionsInADir, singleStrandInterpOptions, IM_ARRAYSIZE(singleStrandInterpOptions));

			const char* interpOptions[] = { "Model", "Other hairs" };
			ImGui::Combo("Interpolate using positions from", &imGuiSettings.m_interpPositionsFrom, interpOptions, IM_ARRAYSIZE(interpOptions));
			ImGui::SliderInt("Multi-Strand Interpolation factor", &m_currentConstants->InterpolationFactorMultiStrand, 1, 15);
			ImGui::SliderFloat("Interpolation radius", &m_currentConstants->InterpolationRadius, 0.01f, 0.2f, "%.3f", ImGuiSliderFlags_Logarithmic);
			ImGui::SliderFloat("Tessellation factor", &m_currentConstants->TessellationFactor, 1.0f, 18.0f);
		}


		bool wasAliasingOn = imGuiSettings.m_isAntialiasingOn;
		bool didAALevelChange = false;
		ImGui::Checkbox("Anti-Aliasing", &imGuiSettings.m_isAntialiasingOn);
		if (imGuiSettings.m_isAntialiasingOn) {
			ImGui::SameLine();
			ImGui::Text("Level: ");
			ImGui::SameLine();

			bool divAALevel = ImGui::Button("-");
			ImGui::SameLine();
			ImGui::Text(Stringf("%d", imGuiSettings.m_aaLevel).c_str());
			ImGui::SameLine();
			bool multAALevel = ImGui::Button("+");

			if (divAALevel) imGuiSettings.m_aaLevel /= 2;
			if (multAALevel) imGuiSettings.m_aaLevel *= 2;

			if (imGuiSettings.m_aaLevel == 0) imGuiSettings.m_aaLevel++;
			if (imGuiSettings.m_aaLevel > 8) imGuiSettings.m_aaLevel = 8;

			didAALevelChange = divAALevel || multAALevel;
		}

		if (didAALevelChange || (wasAliasingOn != imGuiSettings.m_isAntialiasingOn)) {
			g_theRenderer->SetAntiAliasing(imGuiSettings.m_isAntialiasingOn, imGuiSettings.m_aaLevel);
		}


		if (m_isUsingKajiya) {
			if (ImGui::CollapsingHeader("Kajiya Shader Settings")) {
				ImGui::SliderInt("Specular Exponent", &m_currentConstants->SpecularExponent, m_hairSpecularExpMin, m_hairSpecularExpMax);

				ImGui::SliderFloat("Specular Constant (Ks)", &m_currentConstants->SpecularCoefficient, 0.0f, 1.0f);
				ImGui::SliderFloat("Diffuse  Constant (Kd)", &m_currentConstants->DiffuseCoefficient, 0.0f, 1.0f);
			}
		}

		if (m_isUsingMarschner) {
			if (ImGui::CollapsingHeader("Marschner Shader Settings")) {
				ImGui::DragFloat("Scale Shift (alpha)", &m_currentConstants->ScaleShift, 0.01f, -10.0f, -5.0f, "%.5f");
				ImGui::DragFloat("Longitudinal Width (Beta)", &m_currentConstants->LongitudinalWidth, 0.01f, 5.0f, 10.0f, "%.5f");
				ImGui::DragFloat("Specular Coefficient", &m_currentConstants->SpecularMarchner, 0.01f, 0.0f, 2.0f, "%.5f");
				ImGui::DragFloat("Ambient Lighting", &imGuiSettings.m_ambientLight, 0.0001f, 0.0f, 1.0f, "%.10f", ImGuiSliderFlags_Logarithmic);
				ImGui::Checkbox("Use Unreal Parameters", &m_currentConstants->UseUnrealParameters);
			}
		}
	}

	bool isSphere = dynamic_cast<HairSphere*>(m_hairObjectKajiya) || dynamic_cast<HairSphere*>(m_hairObjectMarschner);
	bool isDisc = dynamic_cast<HairDisc*>(m_hairObjectKajiya) || dynamic_cast<HairDisc*>(m_hairObjectMarschner);

	std::string headerText = "Object Properties: ";
	if (isDisc) {
		ImGuiTreeNodeFlags_ objectHeaderFlags = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_None;

		headerText += "Hair Disc";

		if (ImGui::CollapsingHeader(headerText.c_str(), objectHeaderFlags)) {
			ImGui::DragInt("Section Count", &HairObject::SectionCount, 1, 1, 36);
			ImGui::DragInt("Hair Per Section", &HairObject::HairPerSection, 5, 1, INT_MAX);
			ImGui::DragFloat("Disc Radius", &HairObject::Radius, 0.5f, 1.0f, 20.0f);
		}
	}

	if (isSphere) {
		ImGuiTreeNodeFlags_ objectHeaderFlags = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_None;

		headerText += "Hair Sphere";

		if (ImGui::CollapsingHeader(headerText.c_str(), objectHeaderFlags)) {
			ImGui::DragInt("Stack Count", &HairObject::StackCount, 1, 1, 36);
			ImGui::DragInt("Slice Count", &HairObject::SliceCount, 1, 1, 36);
			ImGui::DragInt("Hair Per Section", &HairObject::HairPerSection, 5, 1, INT_MAX);
			ImGui::DragFloat("Sphere Radius", &HairObject::Radius, 0.5f, 1.0f, 20.0f);
		}
	}
	if (imGuiSettings.m_requiresShutdown) {
		ImGui::Text("Restarting is required for updating changed properties");
		imGuiSettings.m_restartRequested = ImGui::Button("Restart");
	}

	ImGui::End();

}

void Game::SetSSAOSettings() const
{
	float SSAOLevelsAsFloat = static_cast<float>(imGuiSettings.m_SSAOLevels);
	float fractionWithin = GetFractionWithin(SSAOLevelsAsFloat, 1.0f, 10.0f);
	//float interpValue = 1.0f - ((1.0f - fractionWithin) * (1.0f - fractionWithin));
	float interpValue = fractionWithin * fractionWithin;

	m_currentSSAO->SampleSize = 32;
	m_currentSSAO->MaxOcclusionPerSample = 0.0000005068f;
	m_currentSSAO->SampleRadius = Interpolate(0.0000015136f, 0.0000354785f, interpValue);
	m_currentSSAO->SSAOFalloff = 0.00000001f;
}

void Game::FetchAvailableModels()
{
	m_availableModels.clear();
	std::string modelsPath = "Data/Models/";
	for (auto const& entry : std::filesystem::directory_iterator(modelsPath)) {
		std::string fileName = entry.path().filename().string();
		std::string extension = entry.path().filename().extension().string();

		if (!AreStringsEqualCaseInsensitive(extension, ".obj")) continue;

		m_availableModels.push_back(entry.path());
	}

}

void Game::RenderImGuiSimulationSection() const
{

	bool useMassSpring = (static_cast<SimulAlgorithm>(m_currentConstants->SimulationAlgorithm) == SimulAlgorithm::MASS_SPRINGS);
	bool useDFTL = (static_cast<SimulAlgorithm>(m_currentConstants->SimulationAlgorithm) == SimulAlgorithm::DFTL);
	ImGuiWindowFlags simulationWindowFlags = ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Simulation Properties", NULL, simulationWindowFlags);

	ImGuiTreeNodeFlags_ simulationFlags = ImGuiTreeNodeFlags_DefaultOpen;
	if (ImGui::CollapsingHeader("General", simulationFlags)) {
		if (m_currentState == GameState::HairTessellation) {
			ImGui::DragFloat("Wind Force", &imGuiSettings.m_windForce, 0.25f, 0.0f, 100.0f);
		}

		ImGui::Checkbox("Simulate Hair", &g_simulateHair);
		ImGui::DragFloat("Gravity", &m_currentConstants->Gravity, 0.25f, -20.0f, 0.0f);

		ImGui::DragFloat("Friction", &m_currentConstants->FrictionCoefficient, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Collision Tolerance", &m_currentConstants->CollisionTolerance, 0.01f, -1.0f, 1.0f);

		float dampingMin = 0.0f;
		float dampingMax = 200.0f;
		float dampingSpeed = 0.25f;

		if (useDFTL) {
			dampingMax = 1.0f;
			dampingSpeed = 0.005f;
			m_currentConstants->DampingCoefficient = Clamp(m_currentConstants->DampingCoefficient, dampingMin, dampingMax);
		}

		if (useMassSpring) {
			dampingMax = 2.0f;
			if (!m_currentConstants->IsHairCurly) {
				dampingMin = 0.0f;
			}
			else {
				dampingMin = 0.1f;
				if (m_currentConstants->DampingCoefficient < 0.1f) m_currentConstants->DampingCoefficient = 0.1f;
			}
		}
		ImGui::DragFloat("Damping Coefficient", &m_currentConstants->DampingCoefficient, dampingSpeed, dampingMin, dampingMax);


		int prevCurliness = imGuiSettings.m_hairCurliness;
		if (useMassSpring) {
			ImGui::BeginDisabled();
			ImGui::Checkbox("Is Hair Curly", &m_currentConstants->IsHairCurly);
			ImGui::EndDisabled();
			if (m_currentConstants->IsHairCurly) {
				ImGui::SliderInt("Curliness", &imGuiSettings.m_hairCurliness, 1, 10);
			}

			if (prevCurliness != imGuiSettings.m_hairCurliness) {
				SetCurlinessSettings();
			}

		}

	}


	if (useMassSpring) {
		if (ImGui::CollapsingHeader("Advanced Settings")) {
			float strainLimitMax = 1.0f;
			float strainLimitMin = 0.0f;
			if (!m_currentConstants->IsHairCurly) {
				strainLimitMax = 0.7f;
				strainLimitMin = 0.4f;
				if (m_currentConstants->StrainLimitingCoefficient > 0.75f) m_currentConstants->StrainLimitingCoefficient = 0.75f;
			}
			ImGui::DragFloat("Strain Limiting Coefficient", &m_currentConstants->StrainLimitingCoefficient, 0.001f, strainLimitMin, strainLimitMax);

			if (ImGui::CollapsingHeader("Spring Lengths", simulationFlags)) {
				ImGui::Text(Stringf("Edge Spring length is segment length(%.3f)", HairGuide::HairSegmentLength).c_str());

				float maxBendLength = 0.0f;
				float maxTorsionLength = 0.0f;
				if (m_currentConstants->IsHairCurly) {
					maxBendLength = HairGuide::HairSegmentLength * 4.0f;
					maxTorsionLength = HairGuide::HairSegmentLength * 5.0f;
				}
				else {
					maxBendLength = HairGuide::HairSegmentLength * 5.0f;
					maxTorsionLength = HairGuide::HairSegmentLength * 6.0f;
				}


				if (m_currentConstants->TorsionInitialLength < m_currentConstants->BendInitialLength * 1.2f) {
					m_currentConstants->TorsionInitialLength = m_currentConstants->BendInitialLength * 1.2f;
				}
				/*
				if (!m_currentConstants->IsHairCurly) {
						maxBendLength = HairGuide::HairSegmentLength * 5.0f;
						maxTorsionLength = HairGuide::HairSegmentLength * 6.0f;
					}
				*/

				ImGui::DragFloat("Bend Spring Length", &m_currentConstants->BendInitialLength, 0.05f, HairGuide::HairSegmentLength * 1.2f, maxBendLength);
				ImGui::DragFloat("Torsion Spring Length", &m_currentConstants->TorsionInitialLength, 0.05f, m_currentConstants->BendInitialLength, maxTorsionLength);
			}
			if (ImGui::CollapsingHeader("Spring Stiffnesses", simulationFlags)) {
				float stiffnessMin = 100.0f;
				float stiffnessMax = 2000.0f;

				if (!m_currentConstants->IsHairCurly) {
					stiffnessMax = 1800.0f;
				}

				ImGui::DragFloat("Edge Spring Stiffness", &m_currentConstants->EdgeStiffness, 20.0f, stiffnessMin, stiffnessMax);
				ImGui::DragFloat("Bend Spring Stiffness", &m_currentConstants->BendStiffness, 20.0f, stiffnessMin, stiffnessMax * 0.8f);
				ImGui::DragFloat("Torsion Spring Stiffness", &m_currentConstants->TorsionStiffness, 20.0f, stiffnessMin, stiffnessMax * 0.7f);
			}
		}
	}

	ImGui::End();
}

void Game::SetCurlinessSettings() const
{
	float curlinessAsFloat = static_cast<float>(imGuiSettings.m_hairCurliness);
	m_currentConstants->Gravity = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, -20.0f, -9.8f);
	m_currentConstants->FrictionCoefficient = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 0.0f, 0.08f);
	m_currentConstants->StrainLimitingCoefficient = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 0.55f, 1.0f);
	m_currentConstants->BendInitialLength = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 1.0f, 0.3f);
	m_currentConstants->TorsionInitialLength = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 1.2f, 0.36f);
	m_currentConstants->TorsionInitialLength = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 1.2f, 0.36f);
	m_currentConstants->DampingCoefficient = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 0.1f, 0.5f);
	m_currentConstants->TessellationFactor = RangeMapClamped(curlinessAsFloat, 1.0f, 10.0f, 18.0f, 13.0f);
}

void Game::RenderImGuiPresetsSection() const
{
	ImGuiWindowFlags presetsWindowFlags = ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Presets", NULL, presetsWindowFlags);

	if (ImGui::CollapsingHeader("Models")) {

		imGuiSettings.m_refreshModels = ImGui::Button("Refresh");

		char basisBuffer[10];
		strncpy_s(basisBuffer, m_modelImportOptions->m_basis.c_str(), sizeof(basisBuffer) - 1);
		if (ImGui::InputText("Basis", basisBuffer, sizeof(basisBuffer))) {
			m_modelImportOptions->m_basis = basisBuffer;
		}
		ImGui::Checkbox("Reverse Winding Order", &m_modelImportOptions->m_reverseWindingOrder);

		if (ImGui::BeginCombo("Available Files", m_modelImportOptions->m_name.c_str())) {
			for (std::filesystem::path modelPath : m_availableModels) {
				std::string fileName = modelPath.filename().string();
				std::string extension = modelPath.filename().extension().string();

				bool selectedEntry = false;
				if (ImGui::Selectable(fileName.c_str(), &selectedEntry)) {
					EventArgs loadArgs;
					m_modelImportOptions->m_name = fileName;
					loadArgs.SetValue("path", modelPath.string());
					loadArgs.SetValue("basis", m_modelImportOptions->m_basis);
					std::string reverseWind = (m_modelImportOptions->m_reverseWindingOrder) ? "true" : "false";
					loadArgs.SetValue("reverseWinding", reverseWind);
					Game::ImportMesh(loadArgs);
				}
			}
			ImGui::EndCombo();
		}
	}

	bool applyColorPreset[3] = {};
	ImGuiTreeNodeFlags_ presetsFlags = ImGuiTreeNodeFlags_DefaultOpen;
	if (ImGui::CollapsingHeader("Color", presetsFlags)) {
		applyColorPreset[0] = ImGui::Button("Brown");
		ImGui::SameLine();
		applyColorPreset[1] = ImGui::Button("Black");
		ImGui::SameLine();
		applyColorPreset[2] = ImGui::Button("Red");
	}

	if (applyColorPreset[0]) {
		m_brown.GetAsFloats(imGuiSettings.m_hairColor);
	}

	if (applyColorPreset[1]) {
		m_black.GetAsFloats(imGuiSettings.m_hairColor);
	}

	if (applyColorPreset[2]) {
		Rgba8::LIGHTRED.GetAsFloats(imGuiSettings.m_hairColor);
	}

	bool moveLightsTo[6] = {};
	HairObject* currentEntity = (m_allHairObjects.size() > 0) ? m_allHairObjects[m_selectedEntity] : nullptr;

	if (ImGui::CollapsingHeader("Lights", presetsFlags)) {
		ImGui::Text(Stringf("Num Lights: %d", imGuiSettings.m_amountOfLights).c_str());
		ImGui::SameLine();
		bool removeLight = ImGui::Button("-");
		ImGui::SameLine();
		bool addLight = ImGui::Button("+");

		ImGui::Text(Stringf("Selected Light: %d", imGuiSettings.m_selectedLight).c_str());
		ImGui::SameLine();
		bool prevLight = ImGui::ArrowButton("leftArrow", 0);
		ImGui::SameLine();
		bool nextLight = ImGui::ArrowButton("rightArrow", 1);

		if (nextLight) imGuiSettings.m_selectedLight++;
		if (prevLight) imGuiSettings.m_selectedLight--;

		if (imGuiSettings.m_selectedLight < 0) imGuiSettings.m_selectedLight = imGuiSettings.m_amountOfLights - 1;
		if (imGuiSettings.m_selectedLight < 0 || imGuiSettings.m_selectedLight >= imGuiSettings.m_amountOfLights) imGuiSettings.m_selectedLight = 0;

		if (addLight) {
			if (imGuiSettings.m_amountOfLights < 8) {
				Light& addedLight = imGuiSettings.m_sceneLights[imGuiSettings.m_amountOfLights];
				addedLight.Enabled = true;
				Rgba8::WHITE.GetAsFloats(addedLight.Color);
				addedLight.Position = currentEntity->m_position + Vec3(0.0f, 0.0f, 3.0f);
				imGuiSettings.m_amountOfLights++;
			}
		}

		if (removeLight) {
			if (imGuiSettings.m_amountOfLights > 0) {
				imGuiSettings.m_amountOfLights--;
				imGuiSettings.m_sceneLights[imGuiSettings.m_amountOfLights].Enabled = false;
			}
		}


		ImGui::DragFloat("Distance From Object", &imGuiSettings.m_distFromObject, 0.5f, 0.5f, 10.0f);
		moveLightsTo[0] = ImGui::Button("Front");
		ImGui::SameLine();
		moveLightsTo[1] = ImGui::Button("Back");
		ImGui::SameLine();
		moveLightsTo[2] = ImGui::Button("Left");
		ImGui::SameLine();
		moveLightsTo[3] = ImGui::Button("Right");
		ImGui::SameLine();
		moveLightsTo[4] = ImGui::Button("Top");
		ImGui::SameLine();
		moveLightsTo[5] = ImGui::Button("Bottom");
	}

	Vec3& lightPosition = imGuiSettings.m_sceneLights[imGuiSettings.m_selectedLight].Position;


	if (moveLightsTo[0]) {
		lightPosition = currentEntity->m_position + Vec3(imGuiSettings.m_distFromObject, 0.0f, 0.0f);
	}
	if (moveLightsTo[1]) {
		lightPosition = currentEntity->m_position + Vec3(-imGuiSettings.m_distFromObject, 0.0f, 0.0f);
	}
	if (moveLightsTo[2]) {
		lightPosition = currentEntity->m_position + Vec3(0.0f, imGuiSettings.m_distFromObject, 0.0f);
	}
	if (moveLightsTo[3]) {
		lightPosition = currentEntity->m_position + Vec3(0.0f, -imGuiSettings.m_distFromObject, 0.0f);
	}
	if (moveLightsTo[4]) {
		lightPosition = currentEntity->m_position + Vec3(0.0f, 0.0, imGuiSettings.m_distFromObject);
	}
	if (moveLightsTo[5]) {
		lightPosition = currentEntity->m_position + Vec3(0.0f, 0.0, -imGuiSettings.m_distFromObject);
	}

	if (ImGui::CollapsingHeader("Rendering")) {

		if (m_currentState == GameState::HairTessellation) {
			bool lowInterp = ImGui::Button("Low Interpolation");
			ImGui::SameLine();
			bool mediumInterp = ImGui::Button("Medium Interpolation");
			ImGui::SameLine();
			bool highInterp = ImGui::Button("High Interpolation");

			if (highInterp) {
				m_currentConstants->InterpolationFactor = 10;
				m_currentConstants->InterpolationFactorMultiStrand = 20;
			}

			if (mediumInterp) {
				m_currentConstants->InterpolationFactor = 6;
				m_currentConstants->InterpolationFactorMultiStrand = 15;
			}

			if (lowInterp) {
				m_currentConstants->InterpolationFactor = 3;
				m_currentConstants->InterpolationFactorMultiStrand = 8;
			}
		}


		if (m_currentState == GameState::HairSphereLighting || m_currentState == GameState::HairTessellation) {
			ImGui::Checkbox("Render Sphere Hair", &m_renderHair);
			ImGui::Checkbox("Render Multi-Strand Interpolated Hair", &m_renderMultInterp);
			ImGui::Checkbox("Render Sphere", &m_renderSphere);
		}

		if (ImGui::CollapsingHeader("Marschner")) {
			bool highSpec = ImGui::Button("High Specular");
			ImGui::SameLine();
			bool lowSpec = ImGui::Button("Low Specular");

			if (highSpec) {
				m_currentConstants->SpecularMarchner = 1.0f;
				m_currentConstants->LongitudinalWidth = 5.0f;
			}

			if (lowSpec) {
				m_currentConstants->SpecularMarchner = 0.5;
				m_currentConstants->LongitudinalWidth = 10.0f;
			}

		}

		if (ImGui::CollapsingHeader("Kajiya")) {
			bool highSpec = ImGui::Button("High Specular");
			ImGui::SameLine();
			bool lowSpec = ImGui::Button("Low Specular");
			if (highSpec) {
				m_currentConstants->SpecularCoefficient = 0.6f;
				m_currentConstants->SpecularExponent = 12;
			}

			if (lowSpec) {
				m_currentConstants->SpecularCoefficient = 0.4f;
				m_currentConstants->SpecularExponent = 4;
			}

		}
	}

	if (ImGui::CollapsingHeader("Simulation Algorithm")) {
		bool useDFTL = ImGui::Button("Dynamic Follow-The-Leader");
		bool useMassCurly = ImGui::Button("Mass-Spring Systems Curly Hair");
		bool useMassStraight = ImGui::Button("Mass-Spring Systems Straight Hair");

		if ((SimulAlgorithm)m_currentConstants->SimulationAlgorithm != SimulAlgorithm::DFTL) {
			if (useDFTL) {
				m_currentConstants->SimulationAlgorithm = (unsigned int)SimulAlgorithm::DFTL;
				imGuiSettings.m_restartRequested = true;
				m_currentConstants->DampingCoefficient = 0.925f;
			}
		}

		if (useMassCurly) {
			m_currentConstants->SimulationAlgorithm = (unsigned int)SimulAlgorithm::MASS_SPRINGS;
			m_currentConstants->InterpolationFactor = 1;
			m_currentConstants->InterpolationFactorMultiStrand = 15;
			m_currentConstants->TessellationFactor = 18.0f;
			m_currentConstants->DampingCoefficient = 0.5f;
			m_currentConstants->StrainLimitingCoefficient = 1.0f;
			m_currentConstants->IsHairCurly = true;
			m_currentConstants->Gravity = -9.8f;
			m_currentConstants->SegmentLength = HairGuide::HairSegmentLength;
			m_currentConstants->BendInitialLength = HairGuide::HairSegmentLength * 1.2f;
			m_currentConstants->TorsionInitialLength = HairGuide::HairSegmentLength * 1.44f;
			m_currentConstants->EdgeStiffness = 1800.0f;
			m_currentConstants->BendStiffness = 1800.0f;
			m_currentConstants->TorsionStiffness = 1500.0f;
			imGuiSettings.m_restartRequested = true;
		}
		if (useMassStraight) {
			m_currentConstants->SimulationAlgorithm = (unsigned int)SimulAlgorithm::MASS_SPRINGS;
			m_currentConstants->InterpolationFactor = 2;
			m_currentConstants->InterpolationRadius = 0.25f;
			m_currentConstants->TessellationFactor = 10.0f;
			m_currentConstants->InterpolationFactorMultiStrand = 6;
			m_currentConstants->DampingCoefficient = 0.1f;
			m_currentConstants->StrainLimitingCoefficient = 0.7f;
			m_currentConstants->SegmentLength = HairGuide::HairSegmentLength;
			m_currentConstants->Gravity = -17.0f;
			m_currentConstants->IsHairCurly = false;
			m_currentConstants->BendInitialLength = HairGuide::HairSegmentLength * 4.0f;
			m_currentConstants->TorsionInitialLength = HairGuide::HairSegmentLength * 5.0f;
			m_currentConstants->EdgeStiffness = 1800.0f;
			m_currentConstants->BendStiffness = 1600.0f;
			m_currentConstants->TorsionStiffness = 1200.0f;
			imGuiSettings.m_restartRequested = true;
		}
	}



	ImGui::End();
}



Vec3 Game::GetPlayerPosition() const
{
	return m_player->m_position;
}

Shader* Game::GetSimulationShader() const
{
	SimulAlgorithm asEnum = (SimulAlgorithm)m_currentConstants->SimulationAlgorithm;

	switch (asEnum)
	{
	case SimulAlgorithm::DFTL:
		return m_DFTLCShader;
		break;
	case SimulAlgorithm::MASS_SPRINGS:
		if (m_currentConstants->IsHairCurly) {
			return m_MassSpringsCurlyCShader;
		}
		else {
			return m_MassSpringsStraightShader;
		}
		break;
	}
	return m_DFTLCShader;
}


Rgba8 const Game::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

bool Game::DebugSpawnWorldWireSphere(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldWireSphere(pointerToSelf->m_player->m_position, 1, 5, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnWorldLine3D(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugAddWorldLine(pointerToSelf->m_player->m_position, Vec3::ZERO, 0.125f, 5.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::XRAY);
	return false;
}

bool Game::DebugClearShapes(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	DebugRenderClear();
	return false;
}

bool Game::DebugToggleRenderMode(EventArgs& eventArgs)
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

bool Game::DebugSpawnPermanentBasis(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	Mat44 invertedView = pointerToSelf->m_worldCamera.GetViewMatrix();
	DebugAddWorldBasis(invertedView.GetOrthonormalInverse(), -1.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
	return false;
}

bool Game::DebugSpawnWorldWireCylinder(EventArgs& eventArgs)
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

bool Game::DebugSpawnBillboardText(EventArgs& eventArgs)
{
	Player* const& player = pointerToSelf->m_player;
	std::string playerInfo = Stringf("Position: (%f, %f, %f)\nOrientation: (%f, %f, %f)", player->m_position.x, player->m_position.y, player->m_position.z, player->m_orientation.m_yawDegrees, player->m_orientation.m_pitchDegrees, player->m_orientation.m_rollDegrees);
	DebugAddWorldBillboardText(playerInfo, player->m_position, 0.25f, Vec2(0.5f, 0.5f), 10.0f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USEDEPTH);
	UNUSED(eventArgs);
	return false;
}

bool Game::GetControls(EventArgs& eventArgs)
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

bool Game::ImportMesh(EventArgs& eventArgs)
{
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	MeshImportOptions meshImportOptions;

	std::string modelPath = eventArgs.GetValue("path", "unknown");
	if (modelPath == "unknown") {
		ERROR_RECOVERABLE("COULD NOT FIND MESH FILE TO LOAD");
	}
	else {
		float scale = eventArgs.GetValue("scale", 1.0f);
		std::string basis = eventArgs.GetValue("basis", "i,j,k");
		Rgba8 color = eventArgs.GetValue("color", Rgba8::WHITE);
		bool reverseWindingOrder = eventArgs.GetValue("reverseWinding", false);

		bool invertV = eventArgs.GetValue("invertV", false);

		std::string memoryUsageStr = eventArgs.GetValue("memoryUsage", "dynamic");
		MemoryUsage memoryUsage = (!_stricmp(memoryUsageStr.c_str(), "dynamic")) ? MemoryUsage::Dynamic : MemoryUsage::Default;

		Strings basisSplit = SplitStringOnDelimiter(basis, ',');
		Vec3 i, j, k;

		i.SetFromNotation(basisSplit[0]);
		j.SetFromNotation(basisSplit[1]);
		k.SetFromNotation(basisSplit[2]);


		meshImportOptions.m_transform.SetIJK3D(i, j, k);
		meshImportOptions.m_scale = scale;
		meshImportOptions.m_color = color;
		meshImportOptions.m_reverseWindingOrder = reverseWindingOrder;
		meshImportOptions.m_memoryUsage = memoryUsage;
		meshImportOptions.m_invertUV = invertV;

		if (meshBuilder) {
			meshBuilder->m_importOptions = meshImportOptions;
			meshBuilder->m_vertexes.clear();
			meshBuilder->m_indexes.clear();
		}
		else {
			meshBuilder = new MeshBuilder(meshImportOptions);
		}

		meshBuilder->ImportFromObj(modelPath);

	}


	return true;
}

bool Game::ScaleMesh(EventArgs& eventArgs)
{
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	if (meshBuilder) {
		float scale = eventArgs.GetValue("scale", 1.0f);
		meshBuilder->Scale(scale);
	}


	return true;
}

bool Game::TransformMesh(EventArgs& eventArgs)
{
	if (pointerToSelf->m_meshBuilder) {
		std::string basis = eventArgs.GetValue("basis", "i,j,k");
		Strings basisSplit = SplitStringOnDelimiter(basis, ',');
		Vec3 i, j, k;

		i.SetFromNotation(basisSplit[0]);
		j.SetFromNotation(basisSplit[1]);
		k.SetFromNotation(basisSplit[2]);


		Mat44 newTransform(i, j, k, Vec3::ZERO);

		pointerToSelf->m_meshBuilder->Transform(newTransform);
	}

	return true;
}

bool Game::ReverseWindingOrder(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	if (pointerToSelf->m_meshBuilder) {
		pointerToSelf->m_meshBuilder->ReverseWindingOrder();
	}
	return false;
}

bool Game::SaveToBinary(EventArgs& eventArgs)
{
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	std::string fileName = eventArgs.GetValue("fileName", "Unknown");

	if (meshBuilder && !AreStringsEqualCaseInsensitive(fileName, "unknown")) {
		meshBuilder->WriteToFile(fileName);
	}

	return false;
}

bool Game::LoadFromBinary(EventArgs& eventArgs)
{
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	std::string fileName = eventArgs.GetValue("fileName", "Unknown");

	if (!meshBuilder) {
		meshBuilder = new MeshBuilder(MeshImportOptions());
	}

	if (!AreStringsEqualCaseInsensitive(fileName, "unknown")) {
		meshBuilder->ReadFromFile(fileName);
	}

	return false;
}

bool Game::InvertUV(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	if (meshBuilder) {
		meshBuilder->InvertUV();
	}
	return false;
}
