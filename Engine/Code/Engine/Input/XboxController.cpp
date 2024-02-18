#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>
#include <Xinput.h>
#include "Engine/Input/XboxController.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"

#pragma comment( lib, "xinput9_1_0" ) // Xinput 1_4 doesn't work in older Windows versions; use XInput 9_1_0 explicitly for best compatibility

XboxController::XboxController()
{
}

XboxController::~XboxController()
{
}

bool XboxController::IsConnected() const
{
	return m_isConnected;
}

void XboxController::CheckConnection()
{
	XINPUT_STATE xboxControllerState = {};
	DWORD errorStatus = XInputGetState(m_id, &xboxControllerState);
	m_isConnected = (errorStatus == ERROR_SUCCESS);
}

int XboxController::GetControllerId() const
{
	return m_id;
}

const AnalogJoystick& XboxController::GetLeftStick() const
{
	return m_leftStick;
}

const AnalogJoystick& XboxController::GetRightStick() const
{
	return m_rightStick;
}

float XboxController::GetLeftTrigger() const
{
	return m_leftTrigger;
}

float XboxController::GetRightTrigger() const
{
	return m_rightTrigger;
}

const KeyButtonState& XboxController::GetButton(XboxButtonID buttonID) const
{
	return m_buttons[static_cast<int>(buttonID)];
}

bool XboxController::IsButtonDown(XboxButtonID buttonID) const
{
	KeyButtonState button = m_buttons[static_cast<int>(buttonID)];
	return button.m_isPressed;
}

bool XboxController::WasButtonJustPressed(XboxButtonID buttonID) const
{
	KeyButtonState button = m_buttons[static_cast<int>(buttonID)];
	return !button.m_wasPressedLastFrame && button.m_isPressed;
}

bool XboxController::WasButtonJustReleased(XboxButtonID buttonID) const
{
	KeyButtonState button = m_buttons[static_cast<int>(buttonID)];
	return button.m_wasPressedLastFrame && !button.m_isPressed;
}

void XboxController::ResetKeys()
{
	for (int buttonIndex = 0; buttonIndex < XboxButtonID::NUM; buttonIndex++) {
		KeyButtonState& button = m_buttons[buttonIndex];
		button.m_isPressed = false;
		button.m_wasPressedLastFrame = false;
	}
}

void XboxController::Vibrate(int leftVibrationVal, int rightVibrationVal)
{
	XINPUT_VIBRATION vibration = {};

	vibration.wLeftMotorSpeed = static_cast<WORD>(leftVibrationVal);
	vibration.wRightMotorSpeed = static_cast<WORD>(rightVibrationVal);

	XInputSetState(m_id, &vibration);

}

void XboxController::Update()
{
	if (m_isConnected) {
		XINPUT_STATE xboxControllerState = {};
		XInputGetState(m_id, &xboxControllerState);
		XINPUT_GAMEPAD& gamepad = xboxControllerState.Gamepad;
		WORD& buttonStates = gamepad.wButtons;

		UpdateButton(XboxButtonID::A, buttonStates, XINPUT_GAMEPAD_A);
		UpdateButton(XboxButtonID::B, buttonStates, XINPUT_GAMEPAD_B);
		UpdateButton(XboxButtonID::Y, buttonStates, XINPUT_GAMEPAD_Y);
		UpdateButton(XboxButtonID::X, buttonStates, XINPUT_GAMEPAD_X);
		UpdateButton(XboxButtonID::LeftShoulder, buttonStates, XINPUT_GAMEPAD_LEFT_SHOULDER);
		UpdateButton(XboxButtonID::RightShoulder, buttonStates, XINPUT_GAMEPAD_RIGHT_SHOULDER);
		UpdateButton(XboxButtonID::Up, buttonStates, XINPUT_GAMEPAD_DPAD_UP);
		UpdateButton(XboxButtonID::Down, buttonStates, XINPUT_GAMEPAD_DPAD_DOWN);
		UpdateButton(XboxButtonID::Left, buttonStates, XINPUT_GAMEPAD_DPAD_LEFT);
		UpdateButton(XboxButtonID::Right, buttonStates, XINPUT_GAMEPAD_DPAD_RIGHT);
		UpdateButton(XboxButtonID::Back, buttonStates, XINPUT_GAMEPAD_BACK);
		UpdateButton(XboxButtonID::Start, buttonStates, XINPUT_GAMEPAD_START);
		UpdateButton(XboxButtonID::LeftThumb, buttonStates, XINPUT_GAMEPAD_LEFT_THUMB);
		UpdateButton(XboxButtonID::RightThumb, buttonStates, XINPUT_GAMEPAD_RIGHT_THUMB);
		UpdateTrigger(m_leftTrigger, gamepad.bLeftTrigger);
		UpdateTrigger(m_rightTrigger, gamepad.bRightTrigger);

		UpdateJoystick(m_leftStick, gamepad.sThumbLX, gamepad.sThumbLY);
		UpdateJoystick(m_rightStick, gamepad.sThumbRX, gamepad.sThumbRY);
	}
	else {
		Reset();
		CheckConnection();
	}
}

void XboxController::Reset()
{
	ResetKeys();

	m_isConnected = false;
	m_leftTrigger = 0.0f;
	m_rightTrigger = 0.0f;
	m_leftStick.Reset();
	m_rightStick.Reset();

}

void XboxController::UpdateJoystick(AnalogJoystick& outJoystick, short rawX, short rawY)
{
	float rawNormalizedX = RangeMap(rawX, MIN_JOYSTICK_VALUE, MAX_JOYSTICK_VALUE, -1.0f, 1.0f);
	float rawNormalizedY = RangeMap(rawY, MIN_JOYSTICK_VALUE, MAX_JOYSTICK_VALUE, -1.0f, 1.0f);

	outJoystick.UpdatePosition(rawNormalizedX, rawNormalizedY);
}

void XboxController::UpdateTrigger(float& outTriggerValue, unsigned char rawValue)
{
	outTriggerValue = RangeMap(rawValue, 0.0f, 255.0f, 0.0f, 1.0f);
}

void XboxController::UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonMask)
{
	KeyButtonState& button = m_buttons[static_cast<int>(buttonID)];
	button.m_wasPressedLastFrame = button.m_isPressed;
	button.m_isPressed = (buttonFlags & buttonMask) == buttonMask;

}
