#pragma once
#include "Engine/Core/EventSystem.hpp"

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
	static bool s_isQuitting;
private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

};