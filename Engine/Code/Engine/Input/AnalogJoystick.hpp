#pragma once
#include "Engine/Math/Vec2.hpp"

struct Vec2;
constexpr float JOYSTICK_INNER_DEADZONE_FRACTION = 0.3f;
constexpr float JOYSTICK_OUTER_DEADZONE_FRACTION = 0.95f;

class AnalogJoystick {
public:
	Vec2 GetPosition() const;
	float GetMagnitude() const;
	float GetOrientationDegrees() const;

	Vec2 GetRawUncorrectedPosition() const;
	float GetInnerDeadZoneFraction() const;
	float GetOuterDeadZoneFraction() const;

	void Reset();
	void SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold);
	void UpdatePosition(float rawNormalizedX, float rawNormalizedY);

protected:
	Vec2 m_rawPosition;
	Vec2 m_correctedPosition;
	float m_innerDeadZoneFraction = JOYSTICK_INNER_DEADZONE_FRACTION;
	float m_outerDeadZoneFraction = JOYSTICK_OUTER_DEADZONE_FRACTION;
};