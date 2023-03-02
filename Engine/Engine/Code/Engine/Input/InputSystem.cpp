#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"
#include <string>
#include <windows.h>


unsigned char const KEYCODE_F1 = VK_F1;
unsigned char const KEYCODE_F2 = VK_F2;
unsigned char const KEYCODE_F3 = VK_F3;
unsigned char const KEYCODE_F4 = VK_F4;
unsigned char const KEYCODE_F5 = VK_F5;
unsigned char const KEYCODE_F6 = VK_F6;
unsigned char const KEYCODE_F7 = VK_F7;
unsigned char const KEYCODE_F8 = VK_F8;
unsigned char const KEYCODE_F9 = VK_F9;
unsigned char const KEYCODE_F10 = VK_F10;
unsigned char const KEYCODE_F11 = VK_F11;
unsigned char const KEYCODE_ESC = VK_ESCAPE;
unsigned char const KEYCODE_UPARROW = VK_UP;
unsigned char const KEYCODE_DOWNARROW = VK_DOWN;
unsigned char const KEYCODE_LEFTARROW = VK_LEFT;
unsigned char const KEYCODE_RIGHTARROW = VK_RIGHT;
unsigned char const KEYCODE_SPACE = VK_SPACE;
unsigned char const KEYCODE_ENTER = VK_RETURN;
unsigned char const KEYCODE_LEFT_MOUSE = VK_LBUTTON;
unsigned char const KEYCODE_RIGHT_MOUSE = VK_RBUTTON;
unsigned char const KEYCODE_SHIFT = VK_SHIFT;
unsigned char const KEYCODE_CTRL = VK_CONTROL;
unsigned char const KEYCODE_TILDE = 192;

unsigned short const KEYCODE_MOUSEWHEEL_UP = 256;
unsigned short const KEYCODE_MOUSEWHEEL_DOWN = 257;

InputSystem::InputSystem(InputSystemConfig const& config) :
	m_config(config)
{
}

InputSystem::~InputSystem()
{
}

void InputSystem::Startup()
{
	for (int xboxControllerIndex = 0; xboxControllerIndex < NUM_XBOX_CONTROLLERS; xboxControllerIndex++) {
		m_xboxControllers[xboxControllerIndex].m_id = xboxControllerIndex;
	}

}

void InputSystem::BeginFrame()
{
	for (int xboxControllerIndex = 0; xboxControllerIndex < NUM_XBOX_CONTROLLERS; xboxControllerIndex++) {
		m_xboxControllers[xboxControllerIndex].Update();
	}

}

void InputSystem::EndFrame()
{
	Window* window = Window::GetWindowContext();
	HWND windowHandle = (HWND)window->m_osWindowHandle;

	for (int keyIndex = 0; keyIndex < NUM_KEYCODES; keyIndex++) {
		KeyButtonState& button = m_keyStates[keyIndex];
		button.m_wasPressedLastFrame = button.m_isPressed;
	}

	if (m_keyStates[KEYCODE_MOUSEWHEEL_UP].m_wasPressedLastFrame) {
		m_keyStates[KEYCODE_MOUSEWHEEL_UP].m_isPressed = false;
	}

	if (m_keyStates[KEYCODE_MOUSEWHEEL_DOWN].m_wasPressedLastFrame) {
		m_keyStates[KEYCODE_MOUSEWHEEL_DOWN].m_isPressed = false;
	}

	m_mouseState.m_mouseWheelDelta = 0;

	if (m_pasteCommand) {
		if (::OpenClipboard(windowHandle)) {
			HANDLE clipboardData;

			clipboardData = GetClipboardData(CF_TEXT);

			if (clipboardData) {
				char* textToParse = static_cast<char*>(::GlobalLock(clipboardData));

				std::string text;
				if (textToParse) {
					text = std::string(textToParse);
				}

				GlobalUnlock(clipboardData);
				EventArgs pasteArgs;

				pasteArgs.SetValue("PasteCmdText", text);

				g_theEventSystem->FireEvent("PasteText", pasteArgs);

			}

		}
		::CloseClipboard();
	}

	m_pasteCommand = false;
}

bool InputSystem::WasKeyJustPressed(unsigned short keyCode)
{
	KeyButtonState& button = m_keyStates[keyCode];
	return button.m_isPressed && !button.m_wasPressedLastFrame;
}

bool InputSystem::WasKeyJustReleased(unsigned short keyCode)
{
	KeyButtonState& button = m_keyStates[keyCode];
	return !button.m_isPressed && button.m_wasPressedLastFrame;
}

bool InputSystem::HandleKeyPressed(unsigned short keyCode)
{
	m_keyStates[keyCode].m_isPressed = true;

	EventArgs eventArgs;
	eventArgs.SetValue("inputChar", keyCode);
	FireEvent("HandleKeyPressedDev", eventArgs);

	return true;
}

void InputSystem::HandleKeyReleased(unsigned short keyCode)
{
	m_keyStates[keyCode].m_isPressed = false;

	EventArgs eventArgs;
	eventArgs.SetValue("inputChar", keyCode);
	FireEvent("HandleKeyReleasedDev", eventArgs);
}

void InputSystem::ShutDown()
{
}

void InputSystem::ResetKeyStates()
{
	for (int keyIndex = 0; keyIndex < NUM_KEYCODES; keyIndex++) {
		m_keyStates[keyIndex] = KeyButtonState();
	}
	for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; controllerIndex++) {
		m_xboxControllers[controllerIndex].ResetKeys();
	}
}

void InputSystem::ConsumeKeyJustPressed(unsigned short keyCode)
{
	m_keyStates[keyCode].m_isPressed = false;
}

void InputSystem::ConsumeAllKeysJustPressed()
{
	for (int keycode = 0; keycode < 256; keycode++) {
		if (keycode == (int)KEYCODE_TILDE) continue;
		m_keyStates[keycode].m_isPressed = false;
	}
}

void InputSystem::SetMouseMode(MouseState const& newMouseMode)
{
	SetMouseMode(newMouseMode.m_isHidden, newMouseMode.m_isClipped, newMouseMode.m_isRelative);
}

void InputSystem::SetMouseMode(bool hidden, bool clipped, bool relative)
{
	Window* currentWindow = Window::GetWindowContext();
	HWND windowHandle = (HWND)currentWindow->m_osWindowHandle;

	if (hidden != m_mouseState.m_isHidden) {
		SetCursorVisibility(hidden);
	}

	if (clipped || relative) {
		POINT origin = { 0,0 };
		RECT clientRect;
		::GetClientRect(windowHandle, &clientRect);
		::ClientToScreen(windowHandle, &origin);

		clientRect.bottom += origin.y;
		clientRect.top += origin.y;
		clientRect.left += origin.x;
		clientRect.right += origin.x;

		//::ClipCursor(&clientRect);
	}
	else {
		::ClipCursor(nullptr);
	}

	if (relative) {
		RECT clientRect;
		::GetClientRect(windowHandle, &clientRect);

		float clientCenterX = (float)((clientRect.left + clientRect.right) * 0.5f);
		float clientCenterY = (float)((clientRect.bottom + clientRect.top) * 0.5f);

		m_mouseState.m_clientCenter = Vec2(clientCenterX, clientCenterY);

		SetRelativeCursorPosition();
	}

	m_mouseState.m_isHidden = hidden;
	m_mouseState.m_isClipped = clipped;
	m_mouseState.m_isRelative = relative;


}

Vec2 const InputSystem::GetMouseClientPosition() const
{
	return m_mouseState.m_currentCursorPosition;
}

Vec2 const InputSystem::GetMouseClientDelta() const
{
	Vec2 delta = m_mouseState.m_currentCursorPosition - m_mouseState.m_prevCursorPosition;
	return delta;
}

void InputSystem::ResetMouseClientDelta() 
{
	RECT clientRect;


	Window* window = Window::GetWindowContext();
	HWND windowHandle = (HWND)window->m_osWindowHandle;
	::GetClientRect(windowHandle, &clientRect);

	float clientCenterX = (float)((clientRect.left + clientRect.right) * 0.5f);
	float clientCenterY = (float)((clientRect.bottom + clientRect.top) * 0.5f);
	m_mouseState.m_clientCenter = Vec2(clientCenterX, clientCenterY);
	
	POINT mousePos;
	::GetCursorPos(&mousePos);
	ScreenToClient(windowHandle, &mousePos);

	m_mouseState.m_currentCursorPosition = Vec2((float)mousePos.x, (float)mousePos.y);
	m_mouseState.m_prevCursorPosition = m_mouseState.m_currentCursorPosition;

	int roundedDownXCenter = RoundDownToInt(m_mouseState.m_clientCenter.x);
	int roundedDownYCenter = RoundDownToInt(m_mouseState.m_clientCenter.y);

	POINT clientCenterOnScreen = { roundedDownXCenter, roundedDownYCenter };
	::ClientToScreen(windowHandle, &clientCenterOnScreen);

	::SetCursorPos(clientCenterOnScreen.x, clientCenterOnScreen.y);
}

short InputSystem::GetMouseWheelDelta() const
{
	return m_mouseState.m_mouseWheelDelta;
}

float InputSystem::GetMouseWheelDeltaNormalized() const
{
	float delta = static_cast<float>(-m_mouseState.m_mouseWheelDelta);

	static float baseWheelDelta = static_cast <float>(WHEEL_DELTA);

	return delta / baseWheelDelta;
}

void InputSystem::ResetMouseWheelDelta()
{
	m_mouseState.m_mouseWheelDelta = 0;
}

void InputSystem::HandleMouseWheelDelta(short delta)
{
	m_mouseState.m_mouseWheelDelta = delta;
}

MouseState InputSystem::GetMouseState() const
{
	return m_mouseState;
}


void InputSystem::SetCursorVisibility(bool hidden)
{
	if (hidden) {
		while (::ShowCursor(FALSE) >= 0);
	}
	else {
		while (::ShowCursor(TRUE) <0);
	}
}


void InputSystem::SetRelativeCursorPosition()
{
	Window* currentWindow = Window::GetWindowContext();
	HWND windowHandle = (HWND)currentWindow->m_osWindowHandle;
	POINT displacedCursorPos;
	::GetCursorPos(&displacedCursorPos);

	::ScreenToClient(windowHandle, &displacedCursorPos);

	m_mouseState.m_prevCursorPosition = Vec2((float)displacedCursorPos.x, (float)displacedCursorPos.y);
	int roundedDownXCenter = RoundDownToInt(m_mouseState.m_clientCenter.x);
	int roundedDownYCenter = RoundDownToInt(m_mouseState.m_clientCenter.y);

	POINT clientCenterOnScreen = {roundedDownXCenter, roundedDownYCenter};
	::ClientToScreen(windowHandle, &clientCenterOnScreen);

	::SetCursorPos(clientCenterOnScreen.x, clientCenterOnScreen.y);

	POINT currentCursorPos;
	::GetCursorPos(&currentCursorPos);
	::ScreenToClient(windowHandle, &currentCursorPos);

	m_mouseState.m_currentCursorPosition = Vec2((float)currentCursorPos.x, (float)currentCursorPos.y);
}

bool InputSystem::HandleCharInput(int charCode)
{
	EventArgs eventArgs;
	eventArgs.SetValue("inputChar", charCode);
	FireEvent("HandleCharInputDev", eventArgs);

	if (charCode == 22) {
		m_pasteCommand = true;
	}
	

	return true;
}
