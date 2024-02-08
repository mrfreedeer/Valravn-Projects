#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/BufferUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game/Gameplay/BasicShapesMode.hpp"

#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/ConvexPoly3DShape.hpp"
#include "Game/Gameplay/TetrahedronShape.hpp"

BasicShapesMode::BasicShapesMode(Game* pointerToGame, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, UICameraSize)
{


}

BasicShapesMode::~BasicShapesMode()
{

}

void BasicShapesMode::Startup()
{
	GameMode::Startup();
	m_worldCamera.SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60, 0.1f, 100.0f);
	m_UICamera.SetOrthoView(Vec2::ZERO, m_UICameraSize);
	m_gameModeName = "Basic Shapes";

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

	std::vector<Vec3> polyVerts = {
		// Clockwise Bottoms
		Vec3(-1.0f, -0.5f, -0.5f), // 0
		Vec3(0.0f,	-1.0f, -0.5f), // 1
		Vec3(1.0f,  -0.5f, -0.5f), // 2
		Vec3(1.0f,	 0.5f, -0.5f), // 3
		Vec3(0.0f,	 1.0f, -0.5f), // 4
		Vec3(-1.0f,  0.5f, -0.5f), // 5

		// Clockwise Top
		Vec3(-1.0f,-0.5f, 0.5f), // 6
		Vec3(0.0f, -1.0f, 0.5f), // 7
		Vec3(1.0f, -0.5f, 0.5f), // 8
		Vec3(1.0f,  0.5f, 0.5f), // 9
		Vec3(0.0f,  1.0f, 0.5f), // 10
		Vec3(-1.0f, 0.5f, 0.5f), // 11

	};

	std::vector<Face> polyFaces = {
		// Bottom
		Face{std::vector<int>({5,4,3,2,1,0})},
		// Top
		Face{std::vector<int>({6,7,8,9,10,11})},

		// Sides
		Face{std::vector<int>({0,1,7,6})},
		Face{std::vector<int>({1,2,8,7})},
		Face{std::vector<int>({2,3,9,8})},
		Face{std::vector<int>({3,4,10,9})},
		Face{std::vector<int>({4,5,11,10})},
		Face{std::vector<int>({5,0,6,11})}
	};

	ConvexPoly3D convexPoly(polyVerts, polyFaces);

	ConvexPoly3DShape* newConvexPolyShape = new ConvexPoly3DShape(m_game, Vec3(2.0f, 2.0f, 2.0f), convexPoly);
	m_allEntities.push_back(newConvexPolyShape);
	m_movableEntities.push_back(newConvexPolyShape);



	TetrahedronShape* newTetrahedronShape = new TetrahedronShape(m_game, Vec3(-1.0f, -1.0f, 6.0f), 2.0f);
	m_allEntities.push_back(newTetrahedronShape);
	m_movableEntities.push_back(newTetrahedronShape);

	Window* window = Window::GetWindowContext();

	m_depthTarget = CreateOrReplaceShadowMapTexture(window->GetClientDimensions());
	m_secondDepthTarget = CreateOrReplaceShadowMapTexture(window->GetClientDimensions());

	g_theConsole->ExecuteXmlCommandScriptFile("Data/LoadDefaultScene.xml");
}

void BasicShapesMode::Shutdown()
{
	g_theRenderer->DestroyTexture(m_depthTarget);
	g_theRenderer->DestroyTexture(m_secondDepthTarget);
	GameMode::Shutdown();
}

void BasicShapesMode::Update(float deltaSeconds)
{
	Vec2 mouseClientDelta = g_theInput->GetMouseClientDelta();

	UpdateInput(deltaSeconds);

	m_helperText = m_baseHelperText;
	m_helperText += Stringf(" FPS: %.2f", 1.0f / deltaSeconds);



	GameMode::Update(deltaSeconds);
	GameMode::SetLightConstants();
}

void BasicShapesMode::Render() const
{
	int rasterizedDepthBias = 0;
	float depthBiasClamp = 0.0f;
	float slopeScaledBias = 0.0f;

	GetRasterizerDepthSettings(rasterizedDepthBias, depthBiasClamp, slopeScaledBias);

	{
		Camera depthCamera;
		if (AreThereLights()) {
			depthCamera = GetLightCamera(0);
		}
		else {
			depthCamera = GetCameraForDepthPass(m_worldCamera.GetViewPosition(), m_worldCamera.GetViewOrientation());
		}

		depthCamera.SetDepthTarget(m_depthTarget);

		g_theRenderer->BeginDepthOnlyCamera(depthCamera);
		g_theRenderer->ClearDepth();

		g_theRenderer->BindShader(m_depthOnlyShader);
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);



		//g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
		g_theRenderer->SetRasterizerState(CullMode::BACK, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE, rasterizedDepthBias, depthBiasClamp, slopeScaledBias);

		g_theRenderer->SetDepthStencilState(DepthTest::LESSEQUAL, true);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

		RenderEntities();

		g_theRenderer->EndCamera(depthCamera);

	}

	{
		Shader* usedShader = (g_drawDebug) ? m_projectiveTextureShader : m_defaultShadowsShader;
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
		g_theRenderer->BindConstantBuffer(4, m_shadowMapCBuffer);
		//if (m_useTextAnimation) {
		//	RenderTextAnimation();
		//}
		RenderEntities(true);

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
}

Vec3 BasicShapesMode::GetPlayerPosition() const
{
	return m_player->m_position;
}



void BasicShapesMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	GameMode::UpdateInput(deltaSeconds);
	UpdateInputFromController();
	UpdateInputFromKeyboard();
}

void BasicShapesMode::UpdateInputFromKeyboard()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) {
		SetNextGameMode((int)GameState::AttractScreen);
	}
}

void BasicShapesMode::UpdateInputFromController()
{
}

bool BasicShapesMode::IsEntityMovable(Entity* entity) const
{
	UNUSED(entity);
	return true;
	//return dynamic_cast<ConvexPoly3DShape*>(entity);
}
