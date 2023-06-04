#pragma once
#include "Game/Gameplay/Entity.hpp"

enum class CameraMode {
	SPECTATOR,
	INDEPENDENT,
	FIRST_PERSON,
	FIXED_ANGLE_TRACKING,
	OVERSHOULDER,
	NUM_CAMERA_MODES
};

class GameCamera : public Entity {
public:
	GameCamera(Game* pointerToGame, Entity* player);

	void Update(float deltaSeconds);
	void Render() const {};

	void Jump() override {};

	void SetNextCameraMode();
	void SetCameraMode(CameraMode const& newCameraMode);
	CameraMode GetMode() { return m_mode; }
	std::string GetCurrentCameraModeAsText() const;

private:
	void UpdateSpectator(float deltaSeconds);
	void UpdateFirstPerson();
	void UpdateFixedAngle();
	void UpdateOverShoulder();
private:
	Entity* m_player = nullptr;
	CameraMode m_mode = CameraMode::FIRST_PERSON;
	float m_eyeHeight = g_gameConfigBlackboard.GetValue("CAMERA_HEIGHT", 1.6f);
};