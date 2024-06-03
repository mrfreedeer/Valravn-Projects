#include "Game/Gameplay/AttractScreenMode.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"

AttractScreenMode::~AttractScreenMode()
{

}

void AttractScreenMode::Startup()
{
	GameMode::Startup();
	m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "Protogame3D");
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;

	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}

	TextureCreateInfo secRtvDesc = {};
	secRtvDesc.m_bindFlags = RESOURCE_BIND_RENDER_TARGET_BIT;
	secRtvDesc.m_clearColour = Rgba8::BLACK;
	secRtvDesc.m_clearFormat = TextureFormat::R32_FLOAT;
	secRtvDesc.m_dimensions = Window::GetWindowContext()->GetClientDimensions();
	secRtvDesc.m_format = TextureFormat::R32_FLOAT;
	secRtvDesc.m_initialData = nullptr;
	secRtvDesc.m_name = "FLOAT RT";
	secRtvDesc.m_owner = g_theRenderer;

	m_floatRT = g_theRenderer->CreateTexture(secRtvDesc);

}

void AttractScreenMode::Update(float deltaSeconds)
{
	GameMode::Update(deltaSeconds);

	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = g_gameConfigBlackboard.GetValue("GAME_TITLE", "FirstTriangle");
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}

	UpdateInput(deltaSeconds);
	m_timeAlive += deltaSeconds;
	m_UICamera.SetOrthoView(Vec2(-1.0f, -1.0f), Vec2(1.0f, 1.0f));
}

void AttractScreenMode::Render() const
{
	float aspectRatio = Window::GetWindowContext()->GetConfig().m_clientAspect;
	Vertex_PCU triangleVertices[] =
	{
		Vertex_PCU(Vec3(0.0f, 0.25f * aspectRatio, 0.0f), Rgba8(255, 0, 0, 255), Vec2(0.5f, 0.0f)),
		Vertex_PCU(Vec3(0.25f, -0.25f * aspectRatio, 0.0f), Rgba8(0, 255, 0, 255), Vec2(1.0f, 1.0f)),
		Vertex_PCU(Vec3(-0.25f, -0.25f * aspectRatio, 0.0f), Rgba8(0, 0, 255, 255), Vec2(0.0f, 1.0f))
	};

	Material* firstTriangleMat = g_theRenderer->GetMaterialForName("FirstTriangle");
	g_theRenderer->BeginCamera(m_UICamera);
	{	
		g_theRenderer->BindMaterial(firstTriangleMat);
		g_theRenderer->SetRenderTarget(m_floatRT, 1);
		g_theRenderer->DrawVertexArray(3, triangleVertices);
	}
	g_theRenderer->EndCamera(m_UICamera);

}


void AttractScreenMode::UpdateInput(float deltaSeconds)
{
	UpdateDeveloperCheatCodes(deltaSeconds);

	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed(KEYCODE_SPACE);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::A);

	if (exitAttractMode) {
		m_game->EnterGameMode();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || controller.WasButtonJustPressed(XboxButtonID::Back)) {
		m_game->HandleQuitRequested();
	}
}
