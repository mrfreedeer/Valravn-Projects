#include "Engine/Renderer/Mesh.hpp"
#include "Engine/Core/BufferUtils.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/UnorderedAccessBuffer.hpp"
#include "Engine/Math/Sampling.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game/Gameplay/Prop.hpp"
#include "Game/Gameplay/GameMode.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
#include <ThirdParty/ImGUI/imgui.h>
#include "Game/Gameplay/ConvexPoly3D.hpp"
#include "Game/Gameplay/ConvexPoly3DShape.hpp"

struct ImGuiRenderInfo {
	Light m_sceneLights[8];
	int m_selectedLight = 0;
	int m_amountOfLights = 0;
	bool m_moveLights = true;
	Entity* m_selectedEntity = nullptr;
	float m_manualDepthBias = 0.0f;
	Rgba8 m_ambientLight = Rgba8(0.1f);
	int m_rasterizerDepthBias = 0;
	float m_depthBiasClamp = 0.0f;
	float m_slopeScaledDepthBias = 0.0f;
	bool m_useBoxSampling = false;
	unsigned int m_PCFSampleCount = 0;
	bool m_usePCF = true;
	bool m_refreshModels = false;
	bool m_useSecondDepth = false;
	bool m_useMidpointDepth = false;
};

GameMode* pointerToSelf = nullptr;
ImGuiRenderInfo imGuiInfo = {};

GameMode::GameMode(Game* pointerToGame, Vec2 const& UICameraSize) :
	m_game(pointerToGame),
	m_UICameraSize(UICameraSize),
	m_UICameraCenter(m_UICameraSize * 0.5f)
{
	pointerToSelf = this;
	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);

	m_worldCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system
}

GameMode::~GameMode()
{
}

void GameMode::Startup()
{
	m_depthOnlyShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDepthOnly.hlsl");
	m_defaultShadowsShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuseShadows.hlsl");
	m_secondDepthShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuseShadowsSec.hlsl");
	m_midpointShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuseMidpointShadows.hlsl");
	m_projectiveTextureShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DiffuseProjectiveTexture.hlsl");
	m_linearDepthShader = g_theRenderer->CreateOrGetShader("Data/Shaders/LinearDepthVisualizer.hlsl");

	//AddLight();
	if (!m_shadowMapCBuffer) {
		m_shadowMapCBuffer = new ConstantBuffer(g_theRenderer->m_device, sizeof(ShadowsConstants));
	}

	m_shadowMapsConstants = new ShadowsConstants();
	g_theRenderer->SetDirectionalLightIntensity(Rgba8(0, 0, 0, 0));

	m_modelImportOptions = new ModelLoadingData();
	m_modelImportOptions->m_name = "Model to load";
	m_modelImportOptions->m_basis = "i,j,k";
	FetchAvailableModels();

	SubscribeEventCallbackFunction("SaveScene", this, &GameMode::SaveSceneToFile);
	SubscribeEventCallbackFunction("LoadScene", this, &GameMode::LoadSceneFromFile);

	imGuiInfo.m_PCFSampleCount = m_poissonSamplesAmount;
	PopulatePoissonPoints(m_poissonSamplesAmount);
}

void GameMode::Shutdown()
{
	if (m_shadowMapCBuffer) {
		delete m_shadowMapCBuffer;
	}

	if (m_shadowMapsConstants) {
		delete m_shadowMapsConstants;
	}

	if (m_poissonSampleBuffer) {
		delete m_poissonSampleBuffer;
	}

	UnsubscribeAllEventCallbackFunctions(this);
}


void GameMode::FetchAvailableModels()
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
void GameMode::Update(float deltaSeconds)
{
	if (m_poissonSampleBuffer) {
		if (m_poissonSamplesAmount != imGuiInfo.m_PCFSampleCount) {
			PopulatePoissonPoints(m_poissonSamplesAmount);
			imGuiInfo.m_PCFSampleCount = m_poissonSamplesAmount;
		}
	}
	CheckIfWindowHasFocus();
	if (imGuiInfo.m_refreshModels) {
		FetchAvailableModels();
		imGuiInfo.m_refreshModels = false;
	}

	float gameDeltaSeconds = static_cast<float>(m_clock.GetDeltaTime());
	m_timeAlive += gameDeltaSeconds;

	for (Entity* entity : m_allEntities) {
		if (entity) {
			entity->Update(deltaSeconds);
			if (m_controlLightOrientation && (entity == m_player)) {
				entity->m_orientation = m_savedPlayerOrientation;
				m_worldCamera.SetTransform(m_player->m_position, m_player->m_orientation);
			}
		}
	}

	if (m_controlLightOrientation) {
		Vec2 deltaMouseInput = g_theInput->GetMouseClientDelta();

		deltaMouseInput *= m_player->m_mouseNormalSpeed;
		EulerAngles delta = EulerAngles(deltaMouseInput.x, -deltaMouseInput.y, 0.0f);
		Light& selectedLight = imGuiInfo.m_sceneLights[imGuiInfo.m_selectedLight];
		EulerAngles lightOrientation = EulerAngles::CreateEulerAngleFromForward(selectedLight.Direction);
		lightOrientation += delta;
		selectedLight.Direction = lightOrientation.GetXForward();

	}

	UpdateConstantBuffers();
}

void GameMode::RenderMeshes() const
{
	if (!m_meshBuilder) return;
	{
		g_theRenderer->BeginCamera(m_worldCamera);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);

		Shader* normalShader = g_theRenderer->CreateOrGetShader("Data/Shaders/DefaultDiffuse");
		g_theRenderer->BindShader(normalShader);
		g_theRenderer->CopyAndBindLightConstants();
		g_theRenderer->DrawVertexArray(m_meshBuilder->m_vertexes);

		g_theRenderer->EndCamera(m_worldCamera);
	}
}

void GameMode::Render() const
{
	//RenderMeshes();
	RenderUI();

	for (Light const& currentLight : imGuiInfo.m_sceneLights) {
		if (currentLight.Enabled) {
			DebugAddWorldPoint(currentLight.Position, 0.1f, 0.0f, Rgba8::BLUE, Rgba8::BLUE, DebugRenderMode::USEDEPTH);
			DebugAddWorldArrow(currentLight.Position, currentLight.Position + currentLight.Direction * 0.5f, 0.09f, 0.0f, Rgba8::RED, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);
		}
	}

	DebugRenderWorld(m_worldCamera);
	DebugRenderScreen(m_UICamera);

	AddImGuiDebugControls();


}

void GameMode::RenderEntities(bool useDiffuse) const
{
	Entity* selectedShape = GetSelectedEntity();
	for (Entity const* entity : m_allEntities) {
		if (entity) {
			if (useDiffuse) {
				entity->RenderDiffuse();
			}
			else {
				entity->Render();
			}
		}
		if (selectedShape && (entity == selectedShape)) {
			DebugAddWorldWireSphere(entity->m_position, 1.5f, 0.0f, Rgba8::ORANGE, Rgba8::ORANGE, DebugRenderMode::USEDEPTH, 4, 8);
		}
	}
}

void GameMode::RenderUI() const
{
	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	std::string gameModeAsText = GetCurrentGameStateAsText();
	std::string gameModeHelperText = m_helperText;

	std::vector<Vertex_PCU> titleVerts;
	Vec2 titlePos(m_UICameraSize.x * 0.01f, m_UICameraSize.y * 0.98f);
	Vec2 helpTextPos(m_UICameraSize.x * 0.01f, titlePos.y * 0.97f);

	g_squirrelFont->AddVertsForText2D(titleVerts, titlePos, m_textCellHeight, gameModeAsText, Rgba8::YELLOW, 0.6f);
	g_squirrelFont->AddVertsForText2D(titleVerts, helpTextPos, m_textCellHeight, gameModeHelperText, Rgba8::GREEN, 0.6f);
	g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
	g_theRenderer->DrawVertexArray(titleVerts);

	if (m_useTextAnimation) {

		RenderTextAnimation();
	}
	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());

	g_theConsole->Render(devConsoleBounds);

	g_theRenderer->EndCamera(m_UICamera);
}

void GameMode::RenderTextAnimation() const
{
	if (m_textAnimTriangles.size() > 0) {
		g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
		g_theRenderer->DrawVertexArray(m_textAnimTriangles);
	}
}

Vec3 GameMode::GetPlayerPosition() const
{
	return Vec3();
}

Camera GameMode::GetWorldCamera() const
{
	return m_worldCamera;
}

Player* GameMode::GetPlayer() const
{
	return m_player;
}

Rgba8 const GameMode::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
}

void GameMode::GetRasterizerDepthSettings(int& out_depthBias, float& out_depthBiasClamp, float& out_slopeScaledBias) const
{
	out_depthBias = imGuiInfo.m_rasterizerDepthBias;
	out_depthBiasClamp = imGuiInfo.m_depthBiasClamp;
	out_slopeScaledBias = imGuiInfo.m_slopeScaledDepthBias;
}

Texture* GameMode::CreateOrReplaceShadowMapTexture(IntVec2 const& dimensions) const
{
	TextureCreateInfo info;
	info.m_name = "BasicShapesDepth";
	info.m_dimensions = dimensions;
	info.m_format = TextureFormat::R24G8_TYPELESS;
	info.m_bindFlags = TextureBindFlagBit::TEXTURE_BIND_DEPTH_STENCIL_BIT | TextureBindFlagBit::TEXTURE_BIND_SHADER_RESOURCE_BIT;
	info.m_memoryUsage = MemoryUsage::GPU;

	return g_theRenderer->CreateTexture(info);
}

void GameMode::AddBasicProps()
{
	m_props.clear();
	Player* player = new Player(this->m_game, Vec3::ZERO, &m_worldCamera);
	m_player = player;
	m_allEntities.push_back(player);

	Prop* cubeProp = new Prop(this->m_game, Vec3(-2.0f, 2.0f, 0.0f));
	cubeProp->m_angularVelocity.m_yawDegrees = 45.0f;

	//Prop* gridProp = new Prop(this->m_game, Vec3::ZERO, PropRenderType::GRID);

	Prop* sphereProp = new Prop(this->m_game, Vec3(10.0f, -5.0f, 1.0f), 1.0f, PropRenderType::SPHERE);
	sphereProp->m_angularVelocity.m_pitchDegrees = 20.0f;
	sphereProp->m_angularVelocity.m_yawDegrees = 20.0f;

	m_allEntities.push_back(cubeProp);
	//m_allEntities.push_back(gridProp);
	m_allEntities.push_back(sphereProp);

	m_movableEntities.push_back(cubeProp);
	m_movableEntities.push_back(sphereProp);

	m_props.push_back(cubeProp);
	m_props.push_back(sphereProp);

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
}

std::string GameMode::GetCurrentGameStateAsText() const
{
	std::string fullTitleText("Mode (F6/F7 for prev/next): ");
	fullTitleText += m_gameModeName;
	return fullTitleText;
}

void GameMode::CheckIfWindowHasFocus()
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

void GameMode::SetNextGameMode(int nextGameState)
{
	m_game->m_nextState = (GameState)nextGameState;
}

Camera GameMode::GetLightCamera(Light const& light) const
{
	EulerAngles viewOrientation = EulerAngles::CreateEulerAngleFromForward(light.Direction);
	Camera depthCamera = GetCameraForDepthPass(light.Position, viewOrientation);

	depthCamera.SetPerspectiveView(Window::GetWindowContext()->GetConfig().m_clientAspect, light.SpotAngle, 0.01f, 100.0f);
	return depthCamera;
}

Camera GameMode::GetLightCamera(int lightIndex) const
{
	return GetLightCamera(imGuiInfo.m_sceneLights[lightIndex]);
}

Camera GameMode::GetCameraForDepthPass(Vec3 const& cameraPosition, EulerAngles const& viewOrientation) const
{
	Camera depthCamera;
	static const Vec3 iBasis(0.0f, -1.0f, 0.0f);
	static const Vec3 jBasis(0.0f, 0.0f, 1.0f);
	static const Vec3 kBasis(1.0f, 0.0f, 0.0f);


	depthCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis);
	depthCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);
	depthCamera.SetTransform(cameraPosition, viewOrientation);

	return depthCamera;
}

bool GameMode::AreThereLights() const
{
	return imGuiInfo.m_amountOfLights > 0;
}

void GameMode::AddImGuiDebugControls() const
{
	ImGuiTreeNodeFlags_ presetsFlags = ImGuiTreeNodeFlags_DefaultOpen;
	ImGui::Begin("Debug Controls", NULL, presetsFlags);

	if (ImGui::CollapsingHeader("Models")) {

		imGuiInfo.m_refreshModels = ImGui::Button("Refresh");

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
					ImportMesh(loadArgs);
				}
			}
			ImGui::EndCombo();
		}
	}

	if (imGuiInfo.m_moveLights) {
		ImGui::Text("Lights are being moved");
	}
	else {
		ImGui::Text("Objects are being moved");
	}

	ImGui::Text("What should move?");
	ImGui::SameLine();
	bool areLightsChosen = ImGui::Button("Lights");
	ImGui::SameLine();
	bool areObjectsChosen = ImGui::Button("Objects");


	if (areLightsChosen) {
		imGuiInfo.m_moveLights = true;
		imGuiInfo.m_selectedEntity = nullptr;
	}
	else if (areObjectsChosen) {
		imGuiInfo.m_moveLights = false;
	}

	if (imGuiInfo.m_moveLights) {
		ImGui::Text(Stringf("Num Lights: %d", imGuiInfo.m_amountOfLights).c_str());
		ImGui::SameLine();
		bool removeLight = ImGui::Button("Remove");
		ImGui::SameLine();
		bool addLight = ImGui::Button("Add");

		if (imGuiInfo.m_amountOfLights > 0) {
			ImGui::Text(Stringf("Selected Light: %d", imGuiInfo.m_selectedLight + 1).c_str());
		}
		else {
			ImGui::Text(Stringf("Selected Light: %d", 0).c_str());
		}
		ImGui::SameLine();
		bool prevLight = ImGui::ArrowButton("leftArrow", 0);
		ImGui::SameLine();
		bool nextLight = ImGui::ArrowButton("rightArrow", 1);

		if (nextLight) imGuiInfo.m_selectedLight++;
		if (prevLight) imGuiInfo.m_selectedLight--;

		if (imGuiInfo.m_selectedLight < 0) imGuiInfo.m_selectedLight = imGuiInfo.m_amountOfLights - 1;
		if (imGuiInfo.m_selectedLight < 0 || imGuiInfo.m_selectedLight >= imGuiInfo.m_amountOfLights) imGuiInfo.m_selectedLight = 0;

		if (addLight) {
			AddLight();
		}

		if (removeLight) {
			if (imGuiInfo.m_amountOfLights > 0) {
				imGuiInfo.m_amountOfLights--;
				imGuiInfo.m_sceneLights[imGuiInfo.m_amountOfLights].Enabled = false;
			}
		}

		Light& selectedLight = imGuiInfo.m_sceneLights[imGuiInfo.m_selectedLight];
		ImGui::DragFloat("Spotlight Angle", &selectedLight.SpotAngle, 1.0f, 15.0f, 180.0f);
	}
	else {
		bool prevObject = ImGui::Button("Previous");
		ImGui::SameLine();
		bool nextObject = ImGui::Button("Next");
		ImGui::SameLine();
		bool clearSelection = ImGui::Button("Clear");

		if (clearSelection) imGuiInfo.m_selectedEntity = nullptr;

		if (prevObject) imGuiInfo.m_selectedEntity = GetPrevMovableEntity();
		if (nextObject) imGuiInfo.m_selectedEntity = GetNextMovableEntity();

	}

	Window* window = Window::GetWindowContext();
	IntVec2 const& windowDims = window->GetClientDimensions();

	IntVec2 const& currentDims = m_depthTarget->GetDimensions();
	float currentMult = static_cast<float>(currentDims.x) / static_cast<float>(windowDims.x);
	float newMult = currentMult;
	std::string clientDimsStr = Stringf("Viewport Dimensions (%d,%d)", windowDims.x, windowDims.y);
	std::string shadowMapDimStr = Stringf("Shadow Map Dimensions (%d,%d)", currentDims.x, currentDims.y);
	ImGui::Text(clientDimsStr.c_str());
	ImGui::Text(shadowMapDimStr.c_str());
	ImGui::Text(Stringf("Current Texture Dims Multiplier: %.2f", currentMult).c_str());
	ImGui::SameLine();
	bool decrease = ImGui::Button("-");
	ImGui::SameLine();
	bool increase = ImGui::Button("+");

	float multiplier = 1.0f;
	float adder = 0.0f;

	if (increase) {
		multiplier = 2.0f;
		adder = 1.0f;
	}
	else if (decrease) {
		multiplier = 0.5f;
		adder = -1.0f;
	}

	if (newMult > 1.0f) newMult += adder;
	else if (increase && (newMult == 1.0f))newMult += adder;
	else {
		newMult *= multiplier;
	}
	if (newMult < 0.25f) newMult = 0.25f;


	if (newMult != currentMult) {
		newMult = (newMult <= 0) ? 1.0f : newMult;
		Vec2 dimsAsFloat = Vec2(windowDims);

		Vec2 newDimensionsAsFloat = dimsAsFloat * newMult;
		IntVec2 newDimensions = IntVec2(newDimensionsAsFloat);
		bool isXBeyondMax = (newDimensions.x > 16384);
		bool isYBeyondMax = (newDimensions.y > 16384);

		if (!(isXBeyondMax || isYBeyondMax)) {
			g_theRenderer->DestroyTexture(m_depthTarget);
			g_theRenderer->DestroyTexture(m_secondDepthTarget);
			m_depthTarget = CreateOrReplaceShadowMapTexture(newDimensions);
			m_secondDepthTarget = CreateOrReplaceShadowMapTexture(newDimensions);
		}

	}

	if (!imGuiInfo.m_useSecondDepth) {
		ImGui::Checkbox("Use Midpoint Depth", &imGuiInfo.m_useMidpointDepth);
	}
	if (!imGuiInfo.m_useMidpointDepth) {
		ImGui::Checkbox("Use Second Depth", &imGuiInfo.m_useSecondDepth);
	}
	if (!(imGuiInfo.m_useSecondDepth || imGuiInfo.m_useMidpointDepth)) {
		if (!imGuiInfo.m_usePCF) {
			ImGui::Checkbox("Use Box Sampling", &imGuiInfo.m_useBoxSampling);
		}
	}

	if (!imGuiInfo.m_useBoxSampling) {
		ImGui::Checkbox("Use PCF", &imGuiInfo.m_usePCF);
		ImGui::Text(Stringf("PCF Sample Count: %d", imGuiInfo.m_PCFSampleCount).c_str());
		ImGui::SameLine();
		bool decreasePCF = ImGui::Button("Decrease");
		ImGui::SameLine();
		bool increasePCF = ImGui::Button("Increase");

		if (decreasePCF) {
			m_poissonSamplesAmount /= 2;
		}
		if (increasePCF) {
			m_poissonSamplesAmount *= 2;
		}

		if (m_poissonSamplesAmount < 4) m_poissonSamplesAmount = 4;
		if (m_poissonSamplesAmount > 64) m_poissonSamplesAmount = 64;

	}
	if (imGuiInfo.m_usePCF) imGuiInfo.m_useBoxSampling = false;
	if (imGuiInfo.m_useBoxSampling) imGuiInfo.m_usePCF = false;

	bool resetBiases = ImGui::Button("Reset Biases");
	ImGui::DragFloat("Depth Bias", &imGuiInfo.m_manualDepthBias, 0.00005f, 0.0f, 1.0f, "%.7f", ImGuiSliderFlags_Logarithmic);
	ImGui::DragInt("Rasterizer Depth Bias", &imGuiInfo.m_rasterizerDepthBias, 1, 0, 100, "%d", ImGuiSliderFlags_Logarithmic);
	ImGui::DragFloat("Depth Bias Clamp", &imGuiInfo.m_depthBiasClamp, 0.005f, 0.0f, 1.0f, "%.7f", ImGuiSliderFlags_Logarithmic);
	ImGui::DragFloat("Slope Scoped Depth Bias", &imGuiInfo.m_slopeScaledDepthBias, 0.05f, 0.0f, 100.0f, "%.7f", ImGuiSliderFlags_Logarithmic);

	if (resetBiases) {
		imGuiInfo.m_manualDepthBias = 0.0f;
		imGuiInfo.m_rasterizerDepthBias = 0;
		imGuiInfo.m_depthBiasClamp = 0.0f;
		imGuiInfo.m_slopeScaledDepthBias = 0.0f;
	}

	float ambientColor[4] = {};
	imGuiInfo.m_ambientLight.GetAsFloats(ambientColor);

	float currentAmbient = ambientColor[0];

	ImGui::SliderFloat("Ambient Intensity", &currentAmbient, 0.0f, 1.0f);


	ImGui::ColorPicker3("Light Color", imGuiInfo.m_sceneLights[imGuiInfo.m_selectedLight].Color);



	imGuiInfo.m_ambientLight = Rgba8(currentAmbient);

	//ImGui::DragFloat("Ambient Light", &imGuiInfo.m_ambientLight, 0.001f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_Logarithmic);

	g_theRenderer->SetAmbientIntensity(Rgba8(imGuiInfo.m_ambientLight));
	ImGui::End();
}

void GameMode::AddLight() const
{
	if (imGuiInfo.m_amountOfLights < 8) {
		Light& addedLight = imGuiInfo.m_sceneLights[imGuiInfo.m_amountOfLights];
		addedLight.Enabled = true;
		addedLight.LightType = 1;
		addedLight.ConstantAttenuation = m_lightConstantAttenuation;
		addedLight.LinearAttenuation = m_lightLinearAttenuation;
		addedLight.QuadraticAttenuation = m_lightQuadAttenuation;
		Rgba8::WHITE.GetAsFloats(addedLight.Color);
		addedLight.Position = m_player->m_position + (m_player->m_orientation.GetXForward());
		addedLight.Direction = Vec3(0.0f, 0.0f, -1.0f);
		imGuiInfo.m_amountOfLights++;
	}
}

Entity* GameMode::GetNextMovableEntity() const
{
	int startingIndex = 0;
	if (imGuiInfo.m_selectedEntity) {
		auto it = std::find(m_movableEntities.begin(), m_movableEntities.end(), imGuiInfo.m_selectedEntity);
		if (it != m_movableEntities.end()) {
			startingIndex = static_cast<int>(it - m_movableEntities.begin());
		}
	}

	for (int index = startingIndex + 1; index < m_movableEntities.size(); index++) {
		Entity* entity = m_movableEntities[index];
		if (!entity) continue;

		if (IsEntityMovable(entity)) return entity;
	}

	for (int index = 0; index < startingIndex; index++) {
		Entity* entity = m_movableEntities[index];
		if (!entity) continue;

		if (IsEntityMovable(entity)) return entity;
	}


	return nullptr;
}

Entity* GameMode::GetPrevMovableEntity() const
{
	int startingIndex = 0;
	if (imGuiInfo.m_selectedEntity) {
		auto it = std::find(m_movableEntities.begin(), m_movableEntities.end(), imGuiInfo.m_selectedEntity);
		if (it != m_movableEntities.end()) {
			startingIndex = static_cast<int>(it - m_movableEntities.begin());
		}
	}

	for (int index = startingIndex - 1; index >= 0; index--) {
		Entity* entity = m_movableEntities[index];
		if (!entity) continue;
		if (IsEntityMovable(entity)) return entity;
	}

	for (int index = (int)m_movableEntities.size() - 1; index > startingIndex; index--) {
		Entity* entity = m_movableEntities[index];
		if (!entity) continue;

		if (IsEntityMovable(entity)) return entity;
	}


	return nullptr;
}

Entity* GameMode::GetSelectedEntity() const
{
	return imGuiInfo.m_selectedEntity;
}

void GameMode::SaveBaseSceneInfo(std::vector<unsigned char>& sceneBuffer) const
{
	BufferWriter bufWriter(sceneBuffer);

	// Save Player Info
	bufWriter.AppendVec3(m_player->m_position);
	bufWriter.AppendEulerAngles(m_player->m_orientation);

	bufWriter.AppendFloat(imGuiInfo.m_manualDepthBias);
	bufWriter.AppendRgba(imGuiInfo.m_ambientLight);
	bufWriter.AppendInt32(imGuiInfo.m_rasterizerDepthBias);
	bufWriter.AppendFloat(imGuiInfo.m_depthBiasClamp);
	bufWriter.AppendFloat(imGuiInfo.m_slopeScaledDepthBias);

	for (Light const& light : imGuiInfo.m_sceneLights) {
		bufWriter.AppendBool(light.Enabled);
		bufWriter.AppendInt32(light.LightType);
		bufWriter.AppendVec3(light.Position);
		bufWriter.AppendVec3(light.Direction);
		bufWriter.AppendFloat(light.SpotAngle);
		bufWriter.AppendFloat(light.Color[0]);
		bufWriter.AppendFloat(light.Color[1]);
		bufWriter.AppendFloat(light.Color[2]);
	}


}


void GameMode::LoadBaseSceneInfo(BufferParser& bufferParser) const
{
	m_player->m_position = bufferParser.ParseVec3();
	m_player->m_orientation = bufferParser.ParseEulerAngles();
	imGuiInfo.m_amountOfLights = 0;
	imGuiInfo.m_manualDepthBias = bufferParser.ParseFloat();
	imGuiInfo.m_ambientLight = bufferParser.ParseRgba();
	imGuiInfo.m_rasterizerDepthBias = bufferParser.ParseInt32();
	imGuiInfo.m_depthBiasClamp = bufferParser.ParseFloat();
	imGuiInfo.m_slopeScaledDepthBias = bufferParser.ParseFloat();

	for (Light& light : imGuiInfo.m_sceneLights) {
		light.ConstantAttenuation = m_lightConstantAttenuation;
		light.LinearAttenuation = m_lightLinearAttenuation;
		light.QuadraticAttenuation = m_lightQuadAttenuation;
		light.Enabled = bufferParser.ParseBool();
		light.LightType = bufferParser.ParseInt32();
		light.Position = bufferParser.ParseVec3();
		light.Direction = bufferParser.ParseVec3();
		light.SpotAngle = bufferParser.ParseFloat();
		light.Color[0] = bufferParser.ParseFloat();
		light.Color[1] = bufferParser.ParseFloat();
		light.Color[2] = bufferParser.ParseFloat();

		//light.Enabled = bufferParser.ParseBool();
		//light.Position = bufferParser.ParseVec3();
		//light.Direction = bufferParser.ParseVec3();
		if (light.Enabled) {
			imGuiInfo.m_amountOfLights++;
		}
	}

}

void GameMode::SetLightConstants() const
{
	for (int lightIndex = 0; lightIndex < 8; lightIndex++) {
		Light const& light = imGuiInfo.m_sceneLights[lightIndex];
		g_theRenderer->SetLight(light, lightIndex);
	}
}

void GameMode::UpdateConstantBuffers()
{
	if (m_shadowMapsConstants) {
		delete m_prevShadowMapsConstants;
		m_shadowMapsConstants->DepthBias = imGuiInfo.m_manualDepthBias;
		m_shadowMapsConstants->UseBoxSampling = (imGuiInfo.m_useBoxSampling) ? 1 : 0; // Explicitly setting it to uint values
		m_shadowMapsConstants->UsePCF = (imGuiInfo.m_usePCF) ? 1 : 0;
		m_shadowMapsConstants->PCFSampleCount = m_poissonSamplesAmount;

		m_prevShadowMapsConstants = m_shadowMapsConstants;

		m_shadowMapsConstants = new ShadowsConstants(*m_prevShadowMapsConstants);
		g_theRenderer->CopyCPUToGPU(m_shadowMapsConstants, sizeof(ShadowsConstants), m_shadowMapCBuffer);
	}
}

void GameMode::UpdateInput(float deltaSeconds)
{
	m_controlLightOrientation = false;

	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	if (m_player) {
		m_player->m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

		float speedUsed = m_player->m_playerNormalSpeed * 0.25f;
		float zSpeedUsed = m_player->m_playerZSpeed * 0.25f;

		iBasis.z = 0;
		jBasis.z = 0;

		iBasis.Normalize();
		jBasis.Normalize();
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

		if (g_theInput->IsKeyDown('L')) {
			if (!m_controlLightOrientation) {
				m_savedPlayerOrientation = m_player->m_orientation;
			}
			m_controlLightOrientation = true;
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


		if (imGuiInfo.m_moveLights) {
			Light& currentLight = imGuiInfo.m_sceneLights[imGuiInfo.m_selectedLight];
			currentLight.Position += velocity * deltaSeconds;
		}
		else if (imGuiInfo.m_selectedEntity) {
			imGuiInfo.m_selectedEntity->m_position += velocity * deltaSeconds;
		}
	}
}

void GameMode::UpdateTextAnimation(float deltaTime, std::string text, Vec2 const& textLocation, float textCellHeight)
{
	m_timeElapsedTextAnimation += deltaTime;
	m_textAnimTriangles.clear();

	Rgba8 usedTextColor = m_textAnimationColor;
	if (m_transitionTextColor) {

		float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0, m_textAnimationDuration);

		usedTextColor.r = static_cast<unsigned char>(Interpolate(m_startTextColor.r, m_endTextColor.r, timeAsFraction));
		usedTextColor.g = static_cast<unsigned char>(Interpolate(m_startTextColor.g, m_endTextColor.g, timeAsFraction));
		usedTextColor.b = static_cast<unsigned char>(Interpolate(m_startTextColor.b, m_endTextColor.b, timeAsFraction));
		usedTextColor.a = static_cast<unsigned char>(Interpolate(m_startTextColor.a, m_endTextColor.a, timeAsFraction));

	}

	g_squirrelFont->AddVertsForText2D(m_textAnimTriangles, Vec2(), textCellHeight, text, usedTextColor);

	float fullTextWidth = g_squirrelFont->GetTextWidth(textCellHeight, text);
	float textWidth = GetTextWidthForAnim(fullTextWidth);

	Vec2 iBasis = GetIBasisForTextAnim();
	Vec2 jBasis = GetJBasisForTextAnim();

	TransformText(iBasis, jBasis, Vec2(textLocation.x - textWidth * 0.5f, textLocation.y));

	if (m_timeElapsedTextAnimation >= m_textAnimationDuration) {
		m_useTextAnimation = false;
		m_timeElapsedTextAnimation = 0.0f;
	}
}

void GameMode::TransformText(Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
	for (int vertIndex = 0; vertIndex < int(m_textAnimTriangles.size()); vertIndex++) {
		Vec3& vertPosition = m_textAnimTriangles[vertIndex].m_position;
		TransformPositionXY3D(vertPosition, iBasis, jBasis, translation);
	}
}

float GameMode::GetTextWidthForAnim(float fullTextWidth)
{
	float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0.0f, m_textAnimationDuration);
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

Vec2 const GameMode::GetIBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0.0f, m_textAnimationDuration);
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

Vec2 const GameMode::GetJBasisForTextAnim()
{
	float timeAsFraction = GetFractionWithin(m_timeElapsedTextAnimation, 0.0f, m_textAnimationDuration);
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

void GameMode::SubscribeToEvents()
{
	SubscribeEventCallbackFunction("ImportMesh", ImportMesh);
	SubscribeEventCallbackFunction("ScaleMesh", ScaleMesh);
	SubscribeEventCallbackFunction("TransformMesh", TransformMesh);
	SubscribeEventCallbackFunction("ReverseWinding", ReverseWindingOrder);
	SubscribeEventCallbackFunction("Save", SaveToBinary);
	SubscribeEventCallbackFunction("Load", LoadFromBinary);
	SubscribeEventCallbackFunction("InvertUV", InvertUV);


}

bool GameMode::ImportMesh(EventArgs& eventArgs)
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

		meshImportOptions.m_name = modelPath;
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

bool GameMode::ScaleMesh(EventArgs& eventArgs)
{
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	if (meshBuilder) {
		float scale = eventArgs.GetValue("scale", 1.0f);
		meshBuilder->Scale(scale);
	}


	return true;
}

bool GameMode::TransformMesh(EventArgs& eventArgs)
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

bool GameMode::ReverseWindingOrder(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	if (pointerToSelf->m_meshBuilder) {
		pointerToSelf->m_meshBuilder->ReverseWindingOrder();
	}
	return false;
}

bool GameMode::SaveToBinary(EventArgs& eventArgs)
{
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	std::string fileName = eventArgs.GetValue("fileName", "Unknown");

	if (meshBuilder && !AreStringsEqualCaseInsensitive(fileName, "unknown")) {
		meshBuilder->WriteToFile(fileName);
	}

	return false;
}

bool GameMode::LoadFromBinary(EventArgs& eventArgs)
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

bool GameMode::InvertUV(EventArgs& eventArgs)
{
	UNUSED(eventArgs);
	MeshBuilder*& meshBuilder = pointerToSelf->m_meshBuilder;

	if (meshBuilder) {
		meshBuilder->InvertUV();
	}
	return false;
}

void GameMode::AppendConvexPolyShape3D(BufferWriter const& bufferWriter, ConvexPoly3DShape* convexPolyShape) const
{
	ConvexPoly3D const& convexPoly = convexPolyShape->m_convexPoly;
	bufferWriter.AppendVec3(convexPolyShape->m_position);

	bufferWriter.AppendUint32((unsigned int)convexPoly.m_vertexes.size());
	for (Vec3 const& vertex : convexPoly.m_vertexes) {
		bufferWriter.AppendVec3(vertex);
	}

	bufferWriter.AppendUint32((unsigned int)convexPoly.m_faces.size());
	for (Face const& face : convexPoly.m_faces) {
		bufferWriter.AppendUint32((unsigned int)face.m_faceIndexes.size());
		for (int vertIndex : face.m_faceIndexes) {
			bufferWriter.AppendInt32(vertIndex);
		}
	}
}

bool GameMode::SaveSceneToFile(EventArgs& args)
{
	std::string fileName = args.GetValue("filename", "unnamed");

	std::string fullPath = "Data/Scenes/";
	fullPath += fileName;

	if (AreStringsEqualCaseInsensitive(fileName, "unnamed")) return false;

	SaveScene(fullPath);
	return true;
}

bool GameMode::LoadSceneFromFile(EventArgs& args)
{
	std::string fileName = args.GetValue("filename", "unnamed");

	std::string fullPath = "Data/Scenes/";
	fullPath += fileName;

	if (AreStringsEqualCaseInsensitive(fileName, "unnamed")) return false;

	LoadScene(fullPath);
	return true;
}

void GameMode::SaveScene(std::filesystem::path fileName) const
{
	fileName.replace_extension(".SMS");
	std::vector<unsigned char> sceneBuffer;
	sceneBuffer.reserve(2048);

	SaveBaseSceneInfo(sceneBuffer);

	BufferWriter bufWriter(sceneBuffer);

	int totalMovable = int(m_movableEntities.size() - m_props.size());

	bufWriter.AppendUint32((unsigned int)totalMovable);

	for (Entity* entity : m_movableEntities) {
		if (entity) {
			ConvexPoly3DShape* asPolyShape = dynamic_cast<ConvexPoly3DShape*>(entity);
			if (asPolyShape) {
				AppendConvexPolyShape3D(bufWriter, asPolyShape);
			}
		}
	}

	if (m_meshBuilder) {
		bufWriter.AppendUint32(1); // Amount of models, future proofing
		MeshImportOptions const& meshOptions = m_meshBuilder->m_importOptions;
		bufWriter.AppendStringZeroTerminated(meshOptions.m_name);
		bufWriter.AppendBool(meshOptions.m_reverseWindingOrder);
		bufWriter.AppendBool(meshOptions.m_invertUV);
		bufWriter.AppendFloat(meshOptions.m_scale);
		bufWriter.AppendMat44(meshOptions.m_transform);
		bufWriter.AppendRgba(meshOptions.m_color);
		bufWriter.AppendUint32((unsigned int)meshOptions.m_memoryUsage);
	}

	FileWriteFromBuffer(sceneBuffer, fileName.string());

}

void GameMode::LoadScene(std::filesystem::path fileName)
{
	fileName.replace_extension(".SMS");
	for (Entity* entity : m_allEntities) {
		delete entity;
	}

	if (m_meshBuilder) {
		delete m_meshBuilder;
		m_meshBuilder = nullptr;
	}

	m_allEntities.clear();
	m_movableEntities.clear();
	AddBasicProps();

	std::vector<unsigned char> sceneBuffer;
	FileReadToBuffer(sceneBuffer, fileName.string());

	BufferParser buffParser(sceneBuffer);
	GameMode::LoadBaseSceneInfo(buffParser);

	unsigned int shapeCount = buffParser.ParseUint32();
	for (unsigned int shapeIndex = 0; shapeIndex < shapeCount; shapeIndex++) {
		ConvexPoly3DShape* newShape = ParseConvexPolyShape3D(buffParser);

		m_allEntities.push_back(newShape);
		m_movableEntities.push_back(newShape);
	}

	if (buffParser.GetRemainingSize() > 0) {
		unsigned amountOfModels = buffParser.ParseUint32();
		MeshImportOptions meshOptions = {};
		meshOptions.m_name = "";
		buffParser.ParseStringZeroTerminated(meshOptions.m_name);
		meshOptions.m_reverseWindingOrder = buffParser.ParseBool();
		meshOptions.m_invertUV = buffParser.ParseBool();
		meshOptions.m_scale = buffParser.ParseFloat();
		meshOptions.m_transform = buffParser.ParseMat44();
		meshOptions.m_color = buffParser.ParseRgba();
		meshOptions.m_memoryUsage = (MemoryUsage)buffParser.ParseUint32();

		if (m_meshBuilder) {
			m_meshBuilder->m_importOptions = meshOptions;
		}
		else {
			m_meshBuilder = new MeshBuilder(meshOptions);
		}

		m_meshBuilder->ImportFromObj(meshOptions.m_name);
	}

}

ConvexPoly3DShape* GameMode::ParseConvexPolyShape3D(BufferParser& bufferParser) const
{
	Vec3 position = bufferParser.ParseVec3();
	std::vector<Vec3> vertexes;
	std::vector<Face> faces;

	unsigned int vertexAmount = bufferParser.ParseUint32();
	vertexes.reserve(vertexAmount);

	for (unsigned int vertexInd = 0; vertexInd < vertexAmount; vertexInd++) {
		Vec3 vertex = bufferParser.ParseVec3();
		vertexes.push_back(vertex);
	}

	unsigned int facesAmount = bufferParser.ParseUint32();
	for (unsigned int faceInd = 0; faceInd < facesAmount; faceInd++) {
		unsigned int indexesAmount = bufferParser.ParseUint32();
		Face newFace = {};
		newFace.m_faceIndexes.reserve(indexesAmount);
		for (unsigned int i = 0; i < indexesAmount; i++) {
			int index = bufferParser.ParseInt32();
			newFace.m_faceIndexes.push_back(index);
		}
		faces.push_back(newFace);
	}

	ConvexPoly3D convexPoly(vertexes, faces);

	ConvexPoly3DShape* newShape = new ConvexPoly3DShape(m_game, position, convexPoly);


	return newShape;
}

bool GameMode::IsSecondDepthEnabled() const
{
	return imGuiInfo.m_useSecondDepth;
}

bool GameMode::IsMidpointDepthEnabled() const
{
	return imGuiInfo.m_useMidpointDepth;
}

void GameMode::CreateShadowMap(Texture* shadowMapTex, CullMode cullMode) const
{
	int rasterizedDepthBias = 0;
	float depthBiasClamp = 0.0f;
	float slopeScaledBias = 0.0f;

	GetRasterizerDepthSettings(rasterizedDepthBias, depthBiasClamp, slopeScaledBias);

	// Shadow map construction pass
	{
		Camera depthCamera;
		if (AreThereLights()) {
			depthCamera = GetLightCamera(0);
		}
		else {
			depthCamera = GetCameraForDepthPass(m_worldCamera.GetViewPosition(), m_worldCamera.GetViewOrientation());
		}

		depthCamera.SetDepthTarget(shadowMapTex);

		g_theRenderer->BeginDepthOnlyCamera(depthCamera);
		g_theRenderer->ClearDepth();

		g_theRenderer->BindShader(m_depthOnlyShader);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

		g_theRenderer->SetRasterizerState(cullMode, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE, rasterizedDepthBias, depthBiasClamp, slopeScaledBias);

		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

		if (m_meshBuilder) {
			g_theRenderer->DrawVertexArray(m_meshBuilder->m_vertexes);
		}
		RenderEntities();


		g_theRenderer->EndCamera(depthCamera);

	}
}

void GameMode::PopulatePoissonPoints(unsigned int sampleAmount)
{
	float sqrSampleAmount = sqrtf(float(sampleAmount)) + 1.0f;
	//int sampleWidth = RoundDownToInt(sqrSampleAmount) / 2;
	float poissonSpaceSize = 1000.0f;
	float radius = poissonSpaceSize / float(sqrSampleAmount);

	m_poissonPoints.clear();
	PoissonDisc2D poissonDisc(Vec2(poissonSpaceSize, poissonSpaceSize), radius);
	poissonDisc.GetSamplingPoints(m_poissonPoints, 32);

	Vec2 const poissonSpaceCenter = Vec2(poissonSpaceSize, poissonSpaceSize) * 0.5f;
	for (Vec2& point : m_poissonPoints) {
		point -= poissonSpaceCenter;
		point /= poissonSpaceSize;
	}

	if (m_poissonSampleBuffer) {
		delete m_poissonSampleBuffer;
	}
	m_poissonSampleBuffer = new UnorderedAccessBuffer(g_theRenderer->m_device, m_poissonPoints.data(), m_poissonPoints.size(), sizeof(Vec2), UAVType::STRUCTURED, TextureFormat::R32G32_FLOAT);
}
