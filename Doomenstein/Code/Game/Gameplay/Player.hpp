#pragma once
#include "Game/Gameplay/Controller.hpp"

class Camera;
class Map;
class SpriteAnimDefinition;

class Player : public Controller {
public:
	Player(Map* pointerToMap, Camera* worldCamera, Camera* screenCamera, int playerIndex, int controllerIndex = -1);
	virtual ~Player();

	void Update(float deltaSeconds) override;
	void UpdateCameras();
	void Render() const;
	void RenderHUD(Camera const& camera) const;
	void Possess(Actor* actor) override;
	void ToggleInvincibility();

	Mat44 GetModelMatrix() const;

public:
	int m_playerIndex = 0;
	int m_controllerIndex = -1;
	Vec3 m_position = Vec3::ZERO;
	EulerAngles m_orientation = {};
	bool m_freeFlyCameraMode = false;
	Camera* m_worldCamera = nullptr;
	Camera* m_screenCamera = nullptr;
	bool m_playerDied = false;
private:
	void UpdateFreeFly(float deltaSeconds);
	void UpdatePossessed(float deltaSeconds);

	void UpdateInput(float deltaSeconds);
	void UpdateInputFreeFly(float deltaSeconds);

	void UpdateInputFromKeyboard(float deltaSeconds);
	void UpdateInputFromKeyboardFreeFly(float deltaSeconds);
	void UpdateInputFromController(float deltaSeconds);
	void UpdateInputFromControllerFreeFly(float deltaSeconds);

	void UpdateDeveloperCheatCodes();

private:

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
	float m_healthRegenTime = g_gameConfigBlackboard.GetValue("HEALTH_REGEN_TIME_WAIT", 10.0f);
	float m_healthRegenRate = g_gameConfigBlackboard.GetValue("HEALTH_REGEN_RATE", 10.0f);

	bool m_isAttacking = false;

	SpriteAnimGroupDefinition* m_doomFace = nullptr;
	SpriteAnimDefinition const* m_currentAnim = nullptr;
	Stopwatch m_respawnStopWatch;
	Stopwatch m_healthRegenStopwatch;
	float m_respawnTime = g_gameConfigBlackboard.GetValue("RESPAWN_TIME", 2.0f);
	int m_killCount = 0;
};