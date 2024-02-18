#pragma once
#include "Engine/Core/EngineCommon.hpp"

class InputSystem;

struct WindowConfig {
	InputSystem* m_inputSystem = nullptr;
	std::string m_windowTitle = "Untitled App";
	float m_clientAspect = 2.0f;
	IntVec2 m_clientDimensions = IntVec2::ZERO;
	bool m_isFullScreen = false;
};

class Window {
public:
	Window(WindowConfig const& config);
	~Window();

	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	WindowConfig const& GetConfig() const;
	static Window* GetWindowContext();

	Vec2 GetNormalizedCursorPos() const;
	IntVec2 GetClientDimensions() const;
	bool HasFocus() const;

	void* m_osWindowHandle = nullptr;
protected:
	void CreateOSWindow();
	void RunMessagePump();

private:
	WindowConfig m_config;
	static Window* s_mainWindow;
};