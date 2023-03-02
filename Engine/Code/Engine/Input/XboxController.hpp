#pragma once
#include "Engine/Input/AnalogJoystick.hpp"
#include "Engine/Input/XboxButtonIDEnum.hpp"
#include "Engine/Input/KeyButtonState.hpp"

constexpr float MIN_JOYSTICK_VALUE = -32678.0f;
constexpr float MAX_JOYSTICK_VALUE = 32677.0f;

class XboxController {
	friend class InputSystem;
public:
	XboxController();
	~XboxController();
	bool IsConnected() const;

	void CheckConnection();
	int GetControllerId() const;
	const AnalogJoystick& GetLeftStick() const;
	const AnalogJoystick& GetRightStick() const;
	float GetLeftTrigger() const;
	float GetRightTrigger() const;
	const KeyButtonState& GetButton(XboxButtonID buttonID) const;
	bool IsButtonDown(XboxButtonID buttonID) const;
	bool WasButtonJustPressed(XboxButtonID buttonID) const;
	bool WasButtonJustReleased(XboxButtonID buttonID) const;
	void ResetKeys();
	void Vibrate(int leftVibrationVal, int rightVibrationVal);

private:
	void Update();
	void Reset();
	void UpdateJoystick(AnalogJoystick& outJoystick, short rawX, short rawY);
	void UpdateTrigger(float& outTriggerValue, unsigned char rawValue);
	void UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonMask);

	int m_id = -1;
	bool m_isConnected = false;
	float m_leftTrigger = 0.f;
	float m_rightTrigger = 0.f;
	KeyButtonState m_buttons[(int)XboxButtonID::NUM];
	AnalogJoystick m_leftStick;
	AnalogJoystick m_rightStick;
	
};