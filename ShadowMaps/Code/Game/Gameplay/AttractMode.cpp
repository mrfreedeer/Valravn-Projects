#include "Game/Gameplay/AttractMode.hpp"
#include "Game/Gameplay/Game.hpp"

AttractMode::AttractMode(Game* pointerToGame, Vec2 const& UICameraSize) :
	GameMode(pointerToGame, UICameraSize)
{
}

void AttractMode::Startup()
{
	m_currentText = "Shadow Maps";
	m_startTextColor = GetRandomColor();
	m_endTextColor = GetRandomColor();
	m_transitionTextColor = true;
	m_UICamera.SetOrthoView(Vec2::ZERO, m_UICameraSize);
	m_worldCamera.SetOrthoView(Vec2::ZERO, m_UICameraSize);
	m_useTextAnimation = true;


	if (g_sounds[GAME_SOUND::CLAIRE_DE_LUNE] != -1) {
		g_soundPlaybackIDs[GAME_SOUND::CLAIRE_DE_LUNE] = g_theAudio->StartSound(g_sounds[GAME_SOUND::CLAIRE_DE_LUNE]);
	}
}

void AttractMode::Shutdown()
{
}

void AttractMode::Update(float deltaSeconds)
{
	if (!m_useTextAnimation) {
		m_useTextAnimation = true;
		m_currentText = "Shadow Maps";
		m_transitionTextColor = true;
		m_startTextColor = GetRandomColor();
		m_endTextColor = GetRandomColor();
	}

	UpdateInput(deltaSeconds);

	if (m_useTextAnimation) {
		UpdateTextAnimation(deltaSeconds, m_currentText, Vec2(m_UICameraCenter.x, m_UICameraSize.y * m_textAnimationPosPercentageTop), m_textCellHeightAttractScreen);
	}

	GameMode::Update(deltaSeconds);
}

void AttractMode::Render() const
{
	g_theRenderer->BeginCamera(m_UICamera);
	g_theRenderer->ClearScreen(Rgba8::BLACK);

	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerState(CullMode::NONE, FillMode::SOLID, WindingOrder::COUNTERCLOCKWISE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	RenderTextAnimation();

	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	std::vector<Vertex_PCU> testTextVerts;
	AABB2 testTextureAABB2(740.0f, 150.0f, 1040.0f, 450.f);
	AddVertsForAABB2D(testTextVerts, testTextureAABB2, Rgba8());
	g_theRenderer->BindTexture(testTexture);
	g_theRenderer->DrawVertexArray((int)testTextVerts.size(), testTextVerts.data());
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->EndCamera(m_UICamera);

	//GameMode::Render();
}

void AttractMode::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	UpdateInputFromController();
	UpdateInputFromKeyboard();
}

void AttractMode::UpdateInputFromKeyboard()
{
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) {
		m_game->Quit();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_SPACE)) {
		SetNextGameMode(0); // Whichever is first
	}
}

void AttractMode::UpdateInputFromController()
{
}
