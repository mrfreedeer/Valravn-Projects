#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/Framework/GameCommon.hpp"

Player::Player(Game* pointerToGame, Vec3 const& startingWorldPosition, Camera* pointerToCamera) :
	Entity(pointerToGame, startingWorldPosition),
	m_camera(pointerToCamera)
{
}

Player::~Player()
{
}

void Player::Update(float deltaSeconds)
{
	UpdateInput();
	float& pitch = m_orientation.m_pitchDegrees;
	float& roll = m_orientation.m_rollDegrees;

	Entity::Update(deltaSeconds);

	pitch = Clamp(pitch, -m_pitchApertureAngleRotation, m_pitchApertureAngleRotation);
	roll = Clamp(roll, -m_rollApertureAngleRotation, m_rollApertureAngleRotation);
	m_velocity = Vec3::ZERO;
	m_angularVelocity = EulerAngles();
	m_camera->SetTransform(m_position, m_orientation);
}


void Player::Render() const
{
}

void Player::UpdateInput()
{
	UpdateInputFromKeyboard();
	UpdateInputFromController();
}

void Player::UpdateInputFromKeyboard()
{
	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	Vec2 deltaMouseInput = g_theInput->GetMouseClientDelta();

	float speedUsed = m_playerNormalSpeed;
	float mouseSpeed = m_mouseNormalSpeed;
	float zSpeedUsed = m_playerZSpeed;

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
		mouseSpeed = m_mouseSprintSpeed;
		speedUsed = m_playerSprintSpeed;
		zSpeedUsed = m_playerZSprintSpeed;
	}

	if (g_theInput->IsKeyDown('W')) {
		m_velocity += iBasis  * speedUsed;
	}

	if (g_theInput->IsKeyDown('S')) {
		m_velocity -= iBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('A')) {
		m_velocity += jBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('D')) {
		m_velocity -= jBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('Z')) {
		m_velocity.z += zSpeedUsed;
	}

	if (g_theInput->IsKeyDown('C')) {
		m_velocity.z += -zSpeedUsed;
	}

	if (g_theInput->IsKeyDown('Q')) {
		m_angularVelocity.m_rollDegrees = -m_playerRollSpeed;
	}

	if (g_theInput->IsKeyDown('E')) {
		m_angularVelocity.m_rollDegrees = m_playerRollSpeed;
	}

	if (g_theInput->IsKeyDown('H')) {
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles();
	}

	deltaMouseInput *= mouseSpeed;
	m_orientation += EulerAngles(deltaMouseInput.x, -deltaMouseInput.y, 0.0f);
}

void Player::UpdateInputFromController()
{
	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	iBasis.Normalize();
	jBasis.Normalize();

	XboxController controller = g_theInput->GetController(0);
	float speedUsed = m_playerNormalSpeed;
	float zSpeedUsed = m_playerZSpeed;

	if (controller.IsButtonDown(XboxButtonID::A)) {
		speedUsed = m_playerSprintSpeed;
		zSpeedUsed = m_playerZSprintSpeed;
	}

	if (controller.IsButtonDown(XboxButtonID::RightShoulder)) {
		m_velocity.z += zSpeedUsed;
	}

	if (controller.IsButtonDown(XboxButtonID::LeftShoulder)) {
		m_velocity.z += -zSpeedUsed;
	}

	if (controller.GetLeftTrigger() > 0.0f) {
		m_angularVelocity = EulerAngles();
		m_angularVelocity.m_rollDegrees = -m_playerRollSpeed * controller.GetLeftTrigger();
	}

	if (controller.GetRightTrigger() > 0.0f) {
		m_angularVelocity = EulerAngles();
		m_angularVelocity.m_rollDegrees = m_playerRollSpeed * controller.GetRightTrigger();
	}

	if (controller.WasButtonJustPressed(XboxButtonID::Start)) {
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles();
	}

	AnalogJoystick leftStick = controller.GetLeftStick();
	float leftJoyStickMagnitude = leftStick.GetMagnitude();
	if (leftJoyStickMagnitude > 0.0f) {
		Vec2 stickPos = leftStick.GetPosition();
		iBasis *= stickPos.y;
		jBasis *= -stickPos.x;
		
		m_velocity.x = 0.0f;
		m_velocity.y = 0.0f;

		m_velocity += iBasis * speedUsed;
		m_velocity += jBasis * speedUsed;
	}

	AnalogJoystick rightStick = controller.GetRightStick();
	float rightJoyStickMagnitude = rightStick.GetMagnitude();
	if (rightJoyStickMagnitude > 0.0f) {
		Vec2 cameraAngle = rightStick.GetPosition();
		cameraAngle *= rightJoyStickMagnitude;
		m_angularVelocity = EulerAngles(-cameraAngle.x * m_controllerCameraSpeed, -cameraAngle.y * m_controllerCameraSpeed, m_angularVelocity.m_rollDegrees);
	}

	
}
