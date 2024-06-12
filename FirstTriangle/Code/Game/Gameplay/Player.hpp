#pragma once
#include "Game/Gameplay/Entity.hpp"

class Camera;

class Player : public Entity {
public:
	Player(Game* pointerToGame, Vec3 const& startingWorldPosition, Camera* pointerToCamera);
	~Player();

	void Update(float deltaSeconds);
	void Render() const;


private:
	void UpdateInput();

	void UpdateInputFromKeyboard();
	void UpdateInputFromController();
private:
	Camera* m_camera = nullptr;

	float m_pitchApertureAngleRotation = g_gameConfigBlackboard.GetValue("PITCH_APERTURE_ANGLE_ROTATION", 85.0f);
	float m_rollApertureAngleRotation = g_gameConfigBlackboard.GetValue("ROLL_APERTURE_ANGLE_ROTATION", 45.0f);
	float m_mouseNormalSpeed = g_gameConfigBlackboard.GetValue("MOUSE_NORMAL_SPEED", 0.05f);
	float m_mouseSprintSpeed = g_gameConfigBlackboard.GetValue("MOUSE_SPRINT_SPEED", 0.1f);

	float m_playerNormalSpeed = g_gameConfigBlackboard.GetValue("PLAYER_NORMAL_SPEED", 10.0f);
	float m_playerSprintSpeed = g_gameConfigBlackboard.GetValue("PLAYER_SPRINT_SPEED", 20.0f);
	float m_playerZSpeed = g_gameConfigBlackboard.GetValue("PLAYER_Z_SPEED", 5.0f);
	float m_playerZSprintSpeed = g_gameConfigBlackboard.GetValue("PLAYER_Z_SPRINT_SPEED", 10.0f);
	float m_playerRollSpeed = g_gameConfigBlackboard.GetValue("PLAYER_ROLL_SPEED", 45.0f);

	float m_controllerCameraSpeed = g_gameConfigBlackboard.GetValue("PLAYER_CONTROLLER_CAMERA_SPEED", 90.0f);
};