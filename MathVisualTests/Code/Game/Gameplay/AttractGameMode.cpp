#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/AttractGameMode.hpp"

AttractGameMode::AttractGameMode(Game* pointerToGame, Vec2 const& cameraSize, Vec2 const& UICameraSize):
	GameMode(pointerToGame, cameraSize, UICameraSize)
{
}

void AttractGameMode::Startup()
{
	m_currentText = "MathVisualTests";
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;

}

void AttractGameMode::Shutdown()
{
}

void AttractGameMode::Update(float deltaSeconds)
{
	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = "MathVisualTests";
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}
	m_worldCamera.SetOrthoView(Vec2(0.0f, 0.0f), Vec2(m_UICameraSize.x, m_UICameraSize.y));

	DebugAddMessage(Stringf("FPS: %.2f", 1.0f / deltaSeconds), 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	UpdateInput(deltaSeconds);
	m_timeAlive += deltaSeconds;

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICameraCenter.x, m_UICameraCenter.y * m_textAnimationPosPercentageTop), m_textCellHeight);
	}

	GameMode::Update(deltaSeconds);
}

void AttractGameMode::Render() const
{
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 1));

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	RenderTextAnimation();

	g_theRenderer->EndCamera(m_worldCamera);

	DebugRenderScreen(m_UICamera);

}

void AttractGameMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	XboxController controller = g_theInput->GetController(0);

	bool exitAttractMode = false;
	exitAttractMode = g_theInput->WasKeyJustPressed(KEYCODE_SPACE);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::Start);
	exitAttractMode = exitAttractMode || controller.WasButtonJustPressed(XboxButtonID::A);
}

void AttractGameMode::UpdateInputFromKeyboard()
{
}

void AttractGameMode::UpdateInputFromController()
{
}
