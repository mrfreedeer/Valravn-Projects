#pragma once
#include "Engine/Renderer/Camera.hpp"
#include "Game/Gameplay/Entity.hpp"


enum class GameState {
	EngineLogo = -2,
	AttractScreen,
	Play
};

class App;
class Player;
class Material;
class GameMode;

class Game {

public:
	Game(App* appPointer);
	~Game();
	bool SuperTest(EventArgs&);
	bool AnotherSuperTest(EventArgs&);
	void Startup();
	void ShutDown();
	void LoadAssets();
	void LoadTextures();
	void LoadSoundFiles();

	void Update();

	void Render();

	void ExitToAttractScreen();
	void EnterGameMode();
	void HandleQuitRequested();

private:
	App* m_theApp = nullptr;

	void StartupLogo();
	void StartupAttractScreen();
	void StartupPlay();

	void UpdateGameState();

	void UpdateLogo(float deltaSeconds);
	void UpdateInputLogo(float deltaSeconds);

	void RenderLogo() const;
private:
	Clock m_clock;

	Texture* m_logoTexture = nullptr;
	double m_timeShowingLogo = 0.0;
	double m_engineLogoLength = g_gameConfigBlackboard.GetValue("ENGINE_LOGO_LENGTH", 2.0);
	bool m_showEngineLogo = g_gameConfigBlackboard.GetValue("SHOW_ENGINE_LOGO", true);

	bool m_loadedAssets = false;
	GameState m_currentState = GameState::AttractScreen;
	GameState m_nextState = GameState::AttractScreen;
	GameMode* m_currentGameMode = nullptr;
	
	Vec2 m_UISize = g_gameConfigBlackboard.GetValue("UI_SIZE", Vec2(1600.0f, 800.0f));

};