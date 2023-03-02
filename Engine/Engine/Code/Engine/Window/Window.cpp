#include "Engine/Window/Window.hpp"
#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"

Window* Window::s_mainWindow = nullptr;

//-----------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called by Windows whenever we ask it for notifications

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam)
{

	if (ImGui_ImplWin32_WndProcHandler(windowHandle, wmMessageCode, wParam, lParam)) {
		return true;
	}

	Window* windowContext = Window::GetWindowContext();
	GUARANTEE_OR_DIE(windowContext != nullptr, "WindowContext is Null!");
	InputSystem* input = windowContext->GetConfig().m_inputSystem;
	GUARANTEE_OR_DIE(input != nullptr, "No input System!!!");

	switch (wmMessageCode)
	{
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
	case WM_CLOSE:
	{
		FireEvent("QuitRequested");
		return 0;
	}

	// Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
	case WM_KEYDOWN:
	{
		unsigned char asKey = (unsigned char)wParam;
		bool wasConsumed = false;

		if (input) {
			wasConsumed = input->HandleKeyPressed(asKey);

			if (wasConsumed) {
				return 0;
			}

		}

		break;
	}

	// Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
	case WM_KEYUP:
	{
		unsigned char asKey = (unsigned char)wParam;
		if (input) {
			input->HandleKeyReleased(asKey);
			return 0;
		}
		//			#SD1ToDo: Tell the App and InputSystem about this key-released event...
		break;
	}

	// Left mouse button pressed
	case WM_LBUTTONDOWN:
	{
		unsigned char keyCode = KEYCODE_LEFT_MOUSE;
		if (input) {
			input->HandleKeyPressed(keyCode);
		}

		break;
	}

	// Left mouse button released
	case WM_LBUTTONUP:
	{
		unsigned char keyCode = KEYCODE_LEFT_MOUSE;
		if (input) {
			input->HandleKeyReleased(keyCode);
		}

		break;
	}

	// Right mouse button pressed
	case WM_RBUTTONDOWN:
	{
		unsigned char keyCode = KEYCODE_RIGHT_MOUSE;
		if (input) {
			input->HandleKeyPressed(keyCode);
		}

		break;
	}

	// Right mouse button released
	case WM_RBUTTONUP:
	{
		unsigned char keyCode = KEYCODE_RIGHT_MOUSE;
		if (input) {
			input->HandleKeyReleased(keyCode);
		}

		break;
	}

	case WM_CHAR:
	{
		int asKey = (int)wParam;
		if (input) {
			input->HandleCharInput(asKey);
		}
		break;
	}

	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		if (input) {
			input->HandleMouseWheelDelta(wheelDelta);

			if (input->WasKeyJustPressed(KEYCODE_MOUSEWHEEL_UP)) {
				input->HandleKeyReleased(KEYCODE_MOUSEWHEEL_UP);
			}

			if (input->WasKeyJustPressed(KEYCODE_MOUSEWHEEL_DOWN)) {
				input->HandleKeyReleased(KEYCODE_MOUSEWHEEL_UP);
			}


			if (wheelDelta > 0) {
				input->HandleKeyPressed(KEYCODE_MOUSEWHEEL_UP);
			}
			else {
				input->HandleKeyPressed(KEYCODE_MOUSEWHEEL_DOWN);

			}
		}


		break;

	}

	}


	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}
Window::Window(WindowConfig const& config) :
	m_config(config)
{
	s_mainWindow = this;
}

Window::~Window()
{
}

void Window::Startup()
{
	CreateOSWindow();
}

void Window::BeginFrame()
{
	RunMessagePump();
}

void Window::EndFrame()
{
}

void Window::Shutdown()
{
}

WindowConfig const& Window::GetConfig() const
{
	return m_config;
}

Window* Window::GetWindowContext()
{
	return s_mainWindow;
}

Vec2 Window::GetNormalizedCursorPos() const
{
	HWND windowHandle = HWND(m_osWindowHandle);
	POINT cursorCoords;
	RECT clientRect;

	::GetCursorPos(&cursorCoords);
	::ScreenToClient(windowHandle, &cursorCoords);
	::GetClientRect(windowHandle, &clientRect);

	float cursorX = float(cursorCoords.x) / float(clientRect.right);
	float cursorY = float(cursorCoords.y) / float(clientRect.bottom);

	return Vec2(cursorX, 1.0f - cursorY);
}

IntVec2 Window::GetClientDimensions() const
{
	return m_config.m_clientDimensions;
}

bool Window::HasFocus() const
{
	return ::GetActiveWindow() == (HWND)m_osWindowHandle;
}

void Window::CreateOSWindow()
{
	HMODULE applicationInstanceHandle = ::GetModuleHandle(NULL);

	// Define a window style/class
	WNDCLASSEX windowClassDescription;
	memset(&windowClassDescription, 0, sizeof(windowClassDescription));
	windowClassDescription.cbSize = sizeof(windowClassDescription);
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
	windowClassDescription.hInstance = applicationInstanceHandle;
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT("Simple Window Class");
	RegisterClassEx(&windowClassDescription);

	// #SD1ToDo: Add support for full screen mode (requires different window style flags than windowed mode)
	const DWORD windowStyleFlags = WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_OVERLAPPED;
	const DWORD windowStyleExFlags = WS_EX_APPWINDOW;

	// Get desktop rect, dimensions, aspect
	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &desktopRect);
	float desktopWidth = (float)(desktopRect.right - desktopRect.left);
	float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);


	float desktopAspect = desktopWidth / desktopHeight;

	// Calculate maximum client size (as some % of desktop size)
	constexpr float maxClientFractionOfDesktop = 1.0f;
	float clientWidth = desktopWidth * maxClientFractionOfDesktop;
	float clientHeight = desktopHeight * maxClientFractionOfDesktop;

	if (m_config.m_clientAspect > desktopAspect)
	{
		// Client window has a wider aspect than desktop; shrink client height to match its width
		clientHeight = clientWidth / m_config.m_clientAspect;
	}
	else
	{
		// Client window has a taller aspect than desktop; shrink client width to match its height
		clientWidth = clientHeight * m_config.m_clientAspect;
	}

	m_config.m_clientDimensions = IntVec2((int)clientWidth, (int)clientHeight);

	// Calculate client rect bounds by centering the client area
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);
	RECT clientRect;
	clientRect.left = (int)clientMarginX;
	clientRect.right = clientRect.left + (int)clientWidth;
	clientRect.top = (int)clientMarginY;
	clientRect.bottom = clientRect.top + (int)clientHeight;

	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;
	AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

	WCHAR windowTitle[1024];
	MultiByteToWideChar(GetACP(), 0, m_config.m_windowTitle.c_str(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));
	HWND hwnd = CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		applicationInstanceHandle,
		NULL);

	m_osWindowHandle = hwnd;

	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);


	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	SetCursor(cursor);
}

void Window::RunMessagePump()
{
	MSG queuedMessage;
	for (;; )
	{
		const BOOL wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
	}
}




