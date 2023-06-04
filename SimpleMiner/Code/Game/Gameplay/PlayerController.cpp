#include "Game/Gameplay/PlayerController.hpp"
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/GameCamera.hpp"

PlayerController::PlayerController(Game* pointerToGame) :
	Controller(pointerToGame)
{
}

void PlayerController::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	UpdateInput();
}

void PlayerController::UpdateInput()
{
	if (!m_possessedEntity) return;
	m_possessedEntity->m_angularVelocity = EulerAngles();

	UpdateInputFromKeyboard();
	UpdateInputFromController();

	float& pitch = m_possessedEntity->m_orientation.m_pitchDegrees;
	float& roll = m_possessedEntity->m_orientation.m_rollDegrees;

	pitch = Clamp(pitch, -m_pitchApertureAngleRotation, m_pitchApertureAngleRotation);
	roll = Clamp(roll, -m_rollApertureAngleRotation, m_rollApertureAngleRotation);
}

void PlayerController::UpdateInputFromKeyboard()
{
	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_possessedEntity->m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);


	iBasis.z = 0.0f;
	jBasis.z = 0.0f;

	iBasis.Normalize();
	jBasis.Normalize();

	Vec2 deltaMouseInput = g_theInput->GetMouseClientDelta();

	float speedUsed = m_playerNormalSpeed;
	float mouseSpeed = m_mouseNormalSpeed;
	float zSpeedUsed = m_playerZSpeed;

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
		mouseSpeed = m_mouseSprintSpeed;
		speedUsed = m_playerSprintSpeed;
		zSpeedUsed = m_playerZSprintSpeed;
	}

	if (g_theInput->IsKeyDown(KEYCODE_SPACE)) {
		m_possessedEntity->Jump();
	}

	Vec3 moveDirection = Vec3::ZERO;

	if (g_theInput->IsKeyDown('W')) {
		moveDirection += iBasis;
	}

	if (g_theInput->IsKeyDown('S')) {
		moveDirection -= iBasis;
	}

	if (g_theInput->IsKeyDown('A')) {
		moveDirection += jBasis;
	}

	if (g_theInput->IsKeyDown('D')) {
		moveDirection -= jBasis;
	}

	GameCamera* entityAsGameCam = dynamic_cast<GameCamera*>(m_possessedEntity);

	if (entityAsGameCam || m_possessedEntity->GetPhysicsMode() != MovementMode::WALKING) {
		if (g_theInput->IsKeyDown('Z') || g_theInput->IsKeyDown('Q')) {
			m_possessedEntity->MoveInDirection(Vec3(0.0f, 0.0f, 1.0f), zSpeedUsed);
		}

		if (g_theInput->IsKeyDown('C') || g_theInput->IsKeyDown('E')) {
			m_possessedEntity->MoveInDirection(Vec3(0.0f, 0.0f, -1.0f), zSpeedUsed);
		}
	}


	if (g_theInput->IsKeyDown('H')) {
		m_possessedEntity->m_position = Vec3::ZERO;
		m_possessedEntity->m_orientation = EulerAngles();
	}

	m_possessedEntity->MoveInDirection(moveDirection, speedUsed);

	deltaMouseInput *= mouseSpeed;
	m_possessedEntity->m_orientation += EulerAngles(deltaMouseInput.x, -deltaMouseInput.y, 0.0f);
}

void PlayerController::UpdateInputFromController()
{
	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_possessedEntity->m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	iBasis.Normalize();
	jBasis.Normalize();

	XboxController controller = g_theInput->GetController(0);
	float speedUsed = m_playerNormalSpeed;
	float zSpeedUsed = m_playerZSpeed;

	//if (controller.IsButtonDown(XboxButtonID::A)) {
	//	speedUsed = m_playerSprintSpeed;
	//	zSpeedUsed = m_playerZSprintSpeed;
	//}


	if (controller.IsButtonDown(XboxButtonID::A)) {
		m_possessedEntity->Jump();
	}
	if (controller.IsButtonDown(XboxButtonID::RightShoulder)) {
		m_possessedEntity->m_velocity.z += zSpeedUsed;
	}

	if (controller.IsButtonDown(XboxButtonID::LeftShoulder)) {
		m_possessedEntity->m_velocity.z += -zSpeedUsed;
	}

	AnalogJoystick leftStick = controller.GetLeftStick();
	float leftJoyStickMagnitude = leftStick.GetMagnitude();
	if (leftJoyStickMagnitude > 0.0f) {
		Vec2 stickPos = leftStick.GetPosition();
		iBasis *= stickPos.y;
		jBasis *= -stickPos.x;


		Vec3 moveDirection = iBasis;
		moveDirection += jBasis;

		m_possessedEntity->MoveInDirection(moveDirection, speedUsed);
	}

	AnalogJoystick rightStick = controller.GetRightStick();
	float rightJoyStickMagnitude = rightStick.GetMagnitude();
	if (rightJoyStickMagnitude > 0.0f) {
		Vec2 cameraAngle = rightStick.GetPosition();
		cameraAngle *= rightJoyStickMagnitude;
		m_possessedEntity->m_angularVelocity = EulerAngles(-cameraAngle.x * m_controllerCameraSpeed, -cameraAngle.y * m_controllerCameraSpeed, m_possessedEntity->m_angularVelocity.m_rollDegrees);
	}


}