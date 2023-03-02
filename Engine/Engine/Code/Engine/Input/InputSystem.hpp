#pragma once
#include "Engine/Input/XboxController.hpp"

extern unsigned char const KEYCODE_F2;
extern unsigned char const KEYCODE_F1;
extern unsigned char const KEYCODE_F3;
extern unsigned char const KEYCODE_F4;
extern unsigned char const KEYCODE_F5;
extern unsigned char const KEYCODE_F6;
extern unsigned char const KEYCODE_F7;
extern unsigned char const KEYCODE_F8;
extern unsigned char const KEYCODE_F9;
extern unsigned char const KEYCODE_F10;
extern unsigned char const KEYCODE_F11;
extern unsigned char const KEYCODE_ESC;
extern unsigned char const KEYCODE_UPARROW;
extern unsigned char const KEYCODE_DOWNARROW;
extern unsigned char const KEYCODE_LEFTARROW;
extern unsigned char const KEYCODE_RIGHTARROW;
extern unsigned char const KEYCODE_SPACE;
extern unsigned char const KEYCODE_ENTER;
extern unsigned char const KEYCODE_LEFT_MOUSE;
extern unsigned char const KEYCODE_RIGHT_MOUSE;
extern unsigned char const KEYCODE_SHIFT;
extern unsigned char const KEYCODE_CTRL;
extern unsigned char const KEYCODE_TILDE;
extern unsigned short const KEYCODE_MOUSEWHEEL_UP;
extern unsigned short const KEYCODE_MOUSEWHEEL_DOWN;

constexpr int NUM_KEYCODES = 258;
constexpr int NUM_XBOX_CONTROLLERS = 4;

class Window;

struct InputSystemConfig {

};

struct MouseState {
	bool m_isHidden = false;
	bool m_isClipped = false;
	bool m_isRelative = false;

	Vec2 m_prevCursorPosition = Vec2::ZERO;
	Vec2 m_currentCursorPosition = Vec2::ZERO;
	Vec2 m_clientCenter = Vec2::ZERO;
	short m_mouseWheelDelta = 0;
};

class InputSystem {

public:
	InputSystem(InputSystemConfig const& config);
	~InputSystem();
	
	void Startup();
	void BeginFrame();
	void EndFrame();
	void ShutDown();
	bool WasKeyJustPressed(unsigned short keyCode);
	bool WasKeyJustReleased(unsigned short keyCode);

	bool IsKeyDown(unsigned short keyCode) { return m_keyStates[keyCode].m_isPressed; }

	bool HandleKeyPressed(unsigned short keyCode);
	void HandleKeyReleased(unsigned short keyCode);
	
	
	bool HandleCharInput(int charCode);

	const XboxController& GetController(int controllerID) { return m_xboxControllers[controllerID]; }
	
	void ResetKeyStates();
	void ConsumeKeyJustPressed(unsigned short keyCode);
	void ConsumeAllKeysJustPressed();

	void SetMouseMode(MouseState const& newMouseMode);
	void SetMouseMode(bool hidden, bool clipped, bool relative);
	Vec2 const GetMouseClientPosition() const;
	Vec2 const GetMouseClientDelta() const;
	void ResetMouseClientDelta();

	short GetMouseWheelDelta() const;
	float GetMouseWheelDeltaNormalized() const;
	void ResetMouseWheelDelta();
	void HandleMouseWheelDelta(short delta);
	MouseState GetMouseState() const;
private:
	void SetCursorVisibility(bool hidden);
	void SetRelativeCursorPosition();

protected:

	KeyButtonState m_keyStates[NUM_KEYCODES];
	XboxController m_xboxControllers[NUM_XBOX_CONTROLLERS];
	InputSystemConfig m_config;

	MouseState m_mouseState = {false, false, false};

	bool m_pasteCommand = false;
};