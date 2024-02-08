#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/BufferUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game/Gameplay/ModelsMode.hpp"

#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/ConvexPoly3DShape.hpp"
#include "Engine/Renderer/Mesh.hpp"
#include <Engine/Math/Sampling.hpp>

ModelsMode::ModelsMode(Game* pointerToGame, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, UICameraSize)
{
}

ModelsMode::~ModelsMode()
{

}

void ModelsMode::Startup()
{
	GameMode::Startup();
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);
	m_UICamera.SetOrthoView(Vec2::ZERO, m_UICameraSize);
	m_gameModeName = "3D Models";

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);


	AddBasicProps();

	m_isCursorHidden = true;
	m_isCursorClipped = true;
	m_isCursorRelative = true;

	g_theInput->ResetMouseClientDelta();


	std::vector<Vec3>planePositions = {
	Vec3(100.0f, 100.0f, 0.0f),
	Vec3(-100.0f, 100.0f, 0.0f),
	Vec3(-100.0f, -100.0f, 0.0f),
	Vec3(100.0f, -100.0f, 0.0f)
	};

	std::vector<Face> planeFaces{
		Face{std::vector<int>({0,1,2,3})}
	};
	ConvexPoly3D planePoly(planePositions, planeFaces);

	ConvexPoly3DShape* newPlaneShape = new ConvexPoly3DShape(m_game, Vec3::ZERO, planePoly);
	m_allEntities.push_back(newPlaneShape);
	m_movableEntities.push_back(newPlaneShape);

	Window* window = Window::GetWindowContext();

	m_depthTarget = CreateOrReplaceShadowMapTexture(window->GetClientDimensions());
	m_secondDepthTarget = CreateOrReplaceShadowMapTexture(window->GetClientDimensions());

	g_theConsole->ExecuteXmlCommandScriptFile("Data/LoadDefaultScene.xml");

	//PoissonDisc2D poissonDisc(Vec2(window->GetClientDimensions()), 50);
	//poissonDisc.GetSamplingPoints(m_poissonPoints);
	
}

void ModelsMode::Shutdown()
{
	g_theRenderer->DestroyTexture(m_depthTarget);
	g_theRenderer->DestroyTexture(m_secondDepthTarget);
	GameMode::Shutdown();
}

void ModelsMode::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);

	m_helperText = m_baseHelperText;
	m_helperText += Stringf(" FPS: %.2f", 1.0f / deltaSeconds);

	GameMode::Update(deltaSeconds);
	GameMode::SetLightConstants();
}

void ModelsMode::Render() const
{
	int rasterizedDepthBias = 0;
	float depthBiasClamp = 0.0f;
	float slopeScaledBias = 0.0f;

	GetRasterizerDepthSettings(rasterizedDepthBias, depthBiasClamp, slopeScaledBias);


	if (IsMidpointDepthEnabled()) { // Midpoint averages both generated shadow maps
		CreateShadowMap(m_depthTarget, CullMode::BACK);
		CreateShadowMap(m_secondDepthTarget, CullMode::FRONT);
	}
	else {
		// For second depth, the cull mode is front
		CullMode cullMode = (IsSecondDepthEnabled()) ? CullMode::FRONT : CullMode::BACK;
		CreateShadowMap(m_depthTarget, cullMode);
	}

	{
		Shader* shadowMapShader = m_defaultShadowsShader;
		Shader* usedShader = nullptr;

		if (IsSecondDepthEnabled()) shadowMapShader = m_secondDepthShader;
		if (IsMidpointDepthEnabled()) shadowMapShader = m_midpointShader;

		usedShader = (g_drawDebug) ? m_projectiveTextureShader : shadowMapShader;

		Texture* usedTexture = (g_drawDebug) ? g_textures[(int)GAME_TEXTURE::CompanionCube] : nullptr;

		g_theRenderer->BeginCamera(m_worldCamera);
		g_theRenderer->ClearScreen(Rgba8::BLACK);

		g_theRenderer->BindShader(usedShader);
		g_theRenderer->CopyAndBindLightConstants();
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		//g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE, rasterizedDepthBias, depthBiasClamp, slopeScaledBias);

		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);
		g_theRenderer->SetShadowsSamplerMode(SamplerMode::SHADOWMAPS);
		g_theRenderer->BindSamplerState();
		g_theRenderer->BindTexture(usedTexture, 0);
		g_theRenderer->BindTexture(m_depthTarget, 1);
		g_theRenderer->SetShaderResource(m_poissonSampleBuffer, 3);

		if (IsMidpointDepthEnabled()) {
			g_theRenderer->BindTexture(m_secondDepthTarget, 2);
		}
		g_theRenderer->BindConstantBuffer(4, m_shadowMapCBuffer);

		RenderEntities(true);
		if (m_meshBuilder) {

			g_theRenderer->BindTexture(nullptr);
			g_theRenderer->DrawVertexArray(m_meshBuilder->m_vertexes);
		}
		g_theRenderer->EndCamera(m_worldCamera);
	}

	{
		AABB2 uisize(Vec2::ZERO, m_UICameraSize);
		AABB2 depthTargetBox = uisize.GetBoxWithin(0.70f, 0.70f, 0.95f, 0.95f);
		g_theRenderer->BeginCamera(m_UICamera);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
		g_theRenderer->BindShader(m_linearDepthShader);

		std::vector<Vertex_PCU> depthBoxVerts;
		// Default DX11 texture uvs are flipped vertically
		AddVertsForAABB2D(depthBoxVerts, depthTargetBox, Rgba8::WHITE, Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f));

		g_theRenderer->BindTexture(m_depthTarget);
		g_theRenderer->DrawVertexArray(depthBoxVerts);

		g_theRenderer->EndCamera(m_UICamera);
	}

	GameMode::Render();

	//{
	//	g_theRenderer->BeginCamera(m_UICamera);
	//	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	//	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	//	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);
	//	std::vector<Vertex_PCU> poissonVerts;

	//	for (Vec2 const& point : m_poissonPoints) {
	//		AddVertsForDisc2D(poissonVerts, point, 4.0f, Rgba8::RED, 4);
	//	}

	//	g_theRenderer->DrawVertexArray(poissonVerts);
	//	g_theRenderer->EndCamera(m_UICamera);
	//}
}



Vec3 ModelsMode::GetPlayerPosition() const
{
	return m_player->m_position;

}

void ModelsMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	GameMode::UpdateInput(deltaSeconds);
	UpdateInputFromController();
	UpdateInputFromKeyboard();
}

void ModelsMode::UpdateInputFromKeyboard()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) {
		SetNextGameMode((int)GameState::AttractScreen);
	}
}

void ModelsMode::UpdateInputFromController()
{
}

bool ModelsMode::IsEntityMovable(Entity* entity) const
{
	UNUSED(entity);
	return true;
}

