#include "Engine/Input/AnalogJoystick.hpp"
#include "Engine/Math/MathUtils.hpp"

Vec2 AnalogJoystick::GetPosition() const
{
	return m_correctedPosition;
}

float AnalogJoystick::GetMagnitude() const
{
	return m_correctedPosition.GetLength();
}

float AnalogJoystick::GetOrientationDegrees() const
{
	return m_correctedPosition.GetOrientationDegrees();
}

Vec2 AnalogJoystick::GetRawUncorrectedPosition() const
{
	return m_rawPosition;
}

float AnalogJoystick::GetInnerDeadZoneFraction() const
{
	return 0.0f;
}

float AnalogJoystick::GetOuterDeadZoneFraction() const
{
	return 0.0f;
}

void AnalogJoystick::Reset()
{
	m_rawPosition = Vec2();
	m_correctedPosition = Vec2();
	m_innerDeadZoneFraction = JOYSTICK_INNER_DEADZONE_FRACTION;
	m_outerDeadZoneFraction = JOYSTICK_OUTER_DEADZONE_FRACTION;
}

void AnalogJoystick::SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold)
{
	m_innerDeadZoneFraction = normalizedInnerDeadzoneThreshold;
	m_outerDeadZoneFraction = normalizedOuterDeadzoneThreshold;
}

void AnalogJoystick::UpdatePosition(float rawNormalizedX, float rawNormalizedY)
{
	Vec2 cartesianPosition = Vec2(rawNormalizedX, rawNormalizedY);
	float orientation = cartesianPosition.GetOrientationDegrees();

	float radius = cartesianPosition.GetLength();

	float clampedRadius = RangeMapClamped(radius, JOYSTICK_INNER_DEADZONE_FRACTION, JOYSTICK_OUTER_DEADZONE_FRACTION, 0.0f, 1.0f);

	Vec2 clampedJoystickPos = Vec2::MakeFromPolarDegrees(orientation);
	clampedJoystickPos.SetLength(clampedRadius);

	m_rawPosition = Vec2(rawNormalizedX, rawNormalizedY);
	m_rawPosition.Normalize();

	m_correctedPosition = clampedJoystickPos;
}
