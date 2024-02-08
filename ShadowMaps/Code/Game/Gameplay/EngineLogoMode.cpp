#include "Game/Gameplay/EngineLogoMode.hpp"
#include "Game/Gameplay/Game.hpp"

EngineLogoMode::EngineLogoMode(Game* pointerToGame, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, UICameraSize)
{
}

void EngineLogoMode::Startup()
{
	m_currentText = "Shadow Maps";
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;
	m_UICamera.SetOrthoView(Vec2::ZERO, m_UICameraSize);
	m_worldCamera.SetOrthoView(Vec2::ZERO, m_UICameraSize);
	m_logoTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Engine Logo.png");
}

void EngineLogoMode::Shutdown()
{
}

void EngineLogoMode::Update(float deltaSeconds)
{
	m_timeShowingLogo += deltaSeconds;
	UpdateInput(deltaSeconds);

	if (m_timeShowingLogo > m_engineLogoLength) {
		SetNextGameMode((int) GameState::AttractScreen);
	}
	GameMode::Update(deltaSeconds);
}

void EngineLogoMode::Render() const
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

	g_theRenderer->BeginCamera(m_UICamera); {

		g_theRenderer->ClearScreen(Rgba8::BLACK);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(whiteBackgroundVerts);
		g_theRenderer->BindTexture(m_logoTexture);
		g_theRenderer->DrawVertexArray(logoVerts);
	}
	g_theRenderer->EndCamera(m_UICamera);

}

void EngineLogoMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	UpdateInputFromController();
	UpdateInputFromKeyboard();
}

void EngineLogoMode::UpdateInputFromKeyboard()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE)) {
		SetNextGameMode((int)GameState::AttractScreen);
	}
}

void EngineLogoMode::UpdateInputFromController()
{
	XboxController controller = g_theInput->GetController(0);

	if (controller.WasButtonJustPressed(XboxButtonID::A)) {
		SetNextGameMode((int)GameState::AttractScreen);
	}
}
