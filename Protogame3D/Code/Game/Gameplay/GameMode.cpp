#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Gameplay/GameMode.hpp"
#include "Game/Framework/GameCommon.hpp"

GameMode::~GameMode()
{

}

void GameMode::Startup()
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);

	m_isCursorHidden = false;
	m_isCursorClipped = false;
	m_isCursorRelative = false;

	Vec3 iBasis(0.0f, -1.0f, 0.0f);
	Vec3 jBasis(0.0f, 0.0f, 1.0f);
	Vec3 kBasis(1.0f, 0.0f, 0.0f);
	m_worldCamera.SetViewToRenderTransform(iBasis, jBasis, kBasis); // Sets view to render to match D11 handedness and coordinate system

}

void GameMode::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	CheckIfWindowHasFocus();
}

void GameMode::Render() const
{
	DebugRenderWorld(m_worldCamera);
	RenderUI();
	DebugRenderScreen(m_UICamera);
}

void GameMode::Shutdown()
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity*& entity = m_allEntities[entityIndex];
		if (entity) {
			delete entity;
			entity = nullptr;
		}
	}
	m_allEntities.resize(0);
	m_player = nullptr;

	for (SoundPlaybackID playbackId : g_soundPlaybackIDs) {
		if(playbackId == MISSING_SOUND_ID) continue;

		g_theAudio->StopSound(playbackId);
	}

}

void GameMode::UpdateDeveloperCheatCodes(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	XboxController controller = g_theInput->GetController(0);
	Clock& sysClock = Clock::GetSystemClock();

	if (g_theInput->WasKeyJustPressed('T')) {
		sysClock.SetTimeDilation(0.1);
	}

	if (g_theInput->WasKeyJustPressed('O')) {
		Clock::GetSystemClock().StepFrame();
	}

	if (g_theInput->WasKeyJustPressed('P')) {
		sysClock.TogglePause();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F1)) {
		g_drawDebug = !g_drawDebug;
	}


	if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) {
		Shutdown();
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
}

Rgba8 const GameMode::GetRandomColor() const
{
	unsigned char randomR = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomG = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomB = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	unsigned char randomA = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	return Rgba8(randomR, randomG, randomB, randomA);
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

void GameMode::UpdateEntities(float deltaSeconds)
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		entity->Update(deltaSeconds);
	}
}

void GameMode::RenderEntities() const
{
	for (int entityIndex = 0; entityIndex < m_allEntities.size(); entityIndex++) {
		Entity* entity = m_allEntities[entityIndex];
		entity->Render();
	}
}

void GameMode::RenderUI() const
{
	g_theRenderer->BeginCamera(m_UICamera);

	AABB2 devConsoleBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());
	AABB2 screenBounds(m_UICamera.GetOrthoBottomLeft(), m_UICamera.GetOrthoTopRight());

	Material* default2DMat = g_theMaterialSystem->GetMaterialForName("Default2DMaterial");
	g_theRenderer->BindMaterial(default2DMat);

	std::vector<Vertex_PCU> gameInfoVerts;

	g_theConsole->Render(devConsoleBounds);
	g_theRenderer->EndCamera(m_UICamera);
}
