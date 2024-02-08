#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"

constexpr float playButtonHeight = 10.0f;
constexpr float playButtonWidth = 10.0f;
class App {
public:
	App();
	~App();

	void Startup();
	void Shutdown();
	void RunFrame();

	bool IsQuitting() const { return s_isQuitting; };
	
	void HandleQuitRequested();
	static bool QuitRequestedEvent(EventArgs& args);
private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

private:
	static bool s_isQuitting;
	double m_lastSavedTime = 0;
	
};