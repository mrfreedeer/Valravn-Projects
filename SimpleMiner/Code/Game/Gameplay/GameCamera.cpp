#include "Game/Gameplay/GameCamera.hpp"
#include "Game/Gameplay/World.hpp"
#include "Game/Gameplay/Game.hpp"

GameCamera::GameCamera(Game* pointerToGame, Entity* player) :
	m_player(player),
	Entity(pointerToGame, player->m_position, player->m_velocity, player->m_orientation)
{
	m_usedDrag = m_airDrag;
	m_height = 0.1f; // Camera is getting caught on surfaces when on First Person view
	m_width = 0.1f;

}

void GameCamera::Update(float deltaSeconds)
{
	switch (m_mode)
	{
	case CameraMode::SPECTATOR:
		UpdateSpectator(deltaSeconds);
		break;
	case CameraMode::INDEPENDENT: // Camera does nothing here, just stays put
		break;
	case CameraMode::FIRST_PERSON:
		UpdateFirstPerson();
		break;
	case CameraMode::FIXED_ANGLE_TRACKING:
		UpdateFixedAngle();
		break;
	case CameraMode::OVERSHOULDER:
		UpdateOverShoulder();
		break;
	}
}

void GameCamera::SetNextCameraMode()
{
	m_mode = CameraMode(((int)m_mode + 1) % (int)CameraMode::NUM_CAMERA_MODES);
	if ((m_mode == CameraMode::FIRST_PERSON) || (m_mode == CameraMode::SPECTATOR)) {
		m_player->m_renderMesh = false;
	}
	else {
		m_player->m_renderMesh = true;
	}

	if (m_mode == CameraMode::SPECTATOR) {
		m_position = m_player->m_position;
		m_isPreventativeEnabled = false;
		m_player->m_isPreventativeEnabled = false;
		m_movementMode = MovementMode::NOCLIP;
	}
	else {
		m_isPreventativeEnabled = true;
		m_player->m_isPreventativeEnabled = true;
	}
}

void GameCamera::SetCameraMode(CameraMode const& newCameraMode)
{
	m_mode = newCameraMode;

	if (m_mode == CameraMode::SPECTATOR) {
		m_position = m_player->m_position;
		m_isPreventativeEnabled = false;
		m_player->m_isPreventativeEnabled = false;
		m_movementMode = MovementMode::NOCLIP;
	}
	else {
		m_isPreventativeEnabled = true;
		m_player->m_isPreventativeEnabled = true;
	}
}

std::string GameCamera::GetCurrentCameraModeAsText() const
{
	switch (m_mode)
	{
	case CameraMode::SPECTATOR:
		return "Spectator";
		break;
	case CameraMode::INDEPENDENT: // Camera does nothing here, just stays put
		return "Independent";
		break;
	case CameraMode::FIRST_PERSON:
		return "First Person";
		break;
	case CameraMode::FIXED_ANGLE_TRACKING:
		return "Fixed Angle";
		break;
	case CameraMode::OVERSHOULDER:
		return "Over Shoulder";
		break;
	}

	return "UNKNOWN";
}

void GameCamera::UpdateSpectator(float deltaSeconds)
{
	AddForce(-m_velocity * m_usedDrag);
	m_velocity += m_acceleration * deltaSeconds;
	m_orientation += m_angularVelocity * deltaSeconds;
	m_position += m_velocity * deltaSeconds;

	m_acceleration = Vec3::ZERO;

	m_player->m_position = m_position;
	m_player->m_orientation = m_orientation;

}

void GameCamera::UpdateFirstPerson()
{
	m_position = m_player->m_position; // Player is at the middle position
	m_position.z += m_eyeHeight - (m_player->m_height * 0.5f);
	m_orientation = m_player->m_orientation;
}

void GameCamera::UpdateFixedAngle()
{
	m_orientation = EulerAngles(40.0f, 30.0f, 0.0f);
	Vec3 cameraFwd = m_orientation.GetXForward();
	m_position = m_player->m_position - (cameraFwd * 10.0f);

}

void GameCamera::UpdateOverShoulder()
{
	Vec3 playerForwardNegZ = m_player->m_orientation.GetXForward();
	//playerForwardNegZ.z *= -1.0f;


	Vec3 playerEyeView = m_player->m_position;
	playerEyeView.z += m_eyeHeight - (m_player->m_height * 0.5f);

	World* world = m_game->GetWorld();
	SimpleMinerRaycast raycastToPos = world->RaycastVsTiles(playerEyeView, -playerForwardNegZ, 4.0f);

	if (raycastToPos.m_didImpact) {
		m_position = playerEyeView - (playerForwardNegZ * (raycastToPos.m_impactDist - 0.25f));
	}
	else {
		m_position = playerEyeView - (playerForwardNegZ * 4.0f);
	}

	m_orientation = m_player->m_orientation;

}
