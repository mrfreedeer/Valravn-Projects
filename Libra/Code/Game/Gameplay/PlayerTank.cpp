#include "Engine/Renderer/Renderer.hpp"
#include "Game/Gameplay/PlayerTank.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Map.hpp"

extern Renderer* g_theRenderer;

PlayerTank::PlayerTank(Map* mapPointer, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type) :
	Entity(mapPointer, startingPosition, orientation, faction, type)
{
	m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankBase.png");
	m_turretTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/PlayerTankTop.png");

	m_physicsRadius = g_gameConfigBlackboard.GetValue("PLAYER_TANK_PHYSICS_RADIUS", 0.3f);
	m_cosmeticsRadius = g_gameConfigBlackboard.GetValue("PLAYER_TANK_COSMETIC_RADIUS", 0.35f);
	m_health = g_gameConfigBlackboard.GetValue("PLAYER_TANK_HEALTH", 5);
	m_maxHealth = m_health;
	m_turnSpeed = g_gameConfigBlackboard.GetValue("PLAYER_TANK_TURN_SPEED", 180.0f);
	m_speed = g_gameConfigBlackboard.GetValue("PLAYER_TANK_SPEED", 1.0f);

	int flamethrowerBulletsPerSecond = g_gameConfigBlackboard.GetValue("FLAMETHROWER_BULLETS_PER_SECOND", 20);
	m_flameThrowerFireCoolDown = 1.0f / (static_cast<float>(flamethrowerBulletsPerSecond));
}

PlayerTank::~PlayerTank()
{
	g_theAudio->StopSound(g_soundPlaybackIDs[GAME_SOUND::PLAYER_SHOOTING]);
	g_theAudio->StopSound(g_soundPlaybackIDs[GAME_SOUND::PLAYER_HIT]);
}

void PlayerTank::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);

	Entity::Update(deltaSeconds);

	if (m_isCrossingMaps) {
		UpdateCrossingAnimation(deltaSeconds);
	}

}

void PlayerTank::UpdateInput(float deltaSeconds)
{
	if (!m_isAlive) return;
	m_velocity = Vec2::ZERO;

	static float bulletPhysicsRadius = g_gameConfigBlackboard.GetValue("BULLET_PHYSICS_RADIUS", 0.1f);

	UpdateBaseQuadOrientation(deltaSeconds);
	UpdateTurretQuadOrientation(deltaSeconds);

	float usedFireCoolDown = m_regularFireCoolDown;
	if (m_weaponType == PlayerWeapon::FLAMETHROWER) {
		usedFireCoolDown = m_flameThrowerFireCoolDown;
	}

	XboxController controller = g_theInput->GetController(0);
	controller.Vibrate(0, 0);

	bool firedWeaponWithKeyboard = g_theInput->WasKeyJustPressed(' ') || g_theInput->IsKeyDown(' ');
	bool firedWeaponWithController = controller.WasButtonJustPressed(XboxButtonID::A) || controller.IsButtonDown(XboxButtonID::A);

	if ((firedWeaponWithController || firedWeaponWithKeyboard) && m_timeLeftToShoot <= 0) {
		m_timeLeftToShoot = usedFireCoolDown;
		Vec2 bulletPosition = Vec2::MakeFromPolarDegrees(m_turretOrientation) * (m_cosmeticsRadius - bulletPhysicsRadius);
		bulletPosition += m_position;

		if (m_weaponType == PlayerWeapon::REGULAR) {

			m_map->SpawnNewEntity(EntityType::BULLET, EntityFaction::GOOD, bulletPosition, m_turretOrientation);
			Vec2 explosionPosition = Vec2::MakeFromPolarDegrees(m_turretOrientation) * (m_cosmeticsRadius)+m_position;

			m_map->SpawnNewEntity(EntityType::EXPLOSION, EntityFaction::NEUTRAL, explosionPosition, m_turretOrientation);
		}
		else {

			bulletPosition = Vec2::MakeFromPolarDegrees(m_turretOrientation) * (m_cosmeticsRadius)+m_position;
			float randOrientation = rng.GetRandomFloatInRange(-m_flameThrowerHalfAngleVariance, m_flameThrowerHalfAngleVariance);
			m_map->SpawnNewEntity(EntityType::FLAMETHROWER_BULLET, EntityFaction::GOOD, bulletPosition, m_turretOrientation + randOrientation);
		}
		PlaySound(GAME_SOUND::PLAYER_SHOOTING);
	}

	bool switchedWeaponWithKeyboard = g_theInput->WasKeyJustPressed('Q');
	bool switchedWeaponWithController = controller.WasButtonJustPressed(XboxButtonID::Y);

	if (switchedWeaponWithController || switchedWeaponWithKeyboard) {
		m_weaponType = (PlayerWeapon)(((int)m_weaponType + 1) % ((int)PlayerWeapon::NUM_WEAPONS));
	}


	if (m_rubbleSpeedPenalty > 0) {
		float movementStrength = controller.GetLeftStick().GetMagnitude();

		// 65535 is the WORD limit for vibration
		float vibrationStrength = RangeMapClamped(movementStrength, 0.0f, 1.0f, 0.0f, 65535.0f);
		int vibrationStrengthAsInt = RoundDownToInt(vibrationStrength);

		controller.Vibrate(vibrationStrengthAsInt, vibrationStrengthAsInt);
	}

	m_timeLeftToShoot -= deltaSeconds;
}

void PlayerTank::UpdateBaseQuadOrientation(float deltaSeconds) {

	bool move = false;
	Vec2 turnDirection;

	if (g_theInput->IsKeyDown('W')) {
		turnDirection += Vec2(0.0f, 1.0f);
		move = true;
	}

	if (g_theInput->IsKeyDown('A')) {
		turnDirection += Vec2(-1.0f, 0.0f);
		move = true;
	}

	if (g_theInput->IsKeyDown('S')) {
		turnDirection += Vec2(0.0f, -1.0f);
		move = true;
	}

	if (g_theInput->IsKeyDown('D')) {
		turnDirection += Vec2(1.0f, 0.0f);
		move = true;
	}


	XboxController controller = g_theInput->GetController(0);
	AnalogJoystick leftJoystick = controller.GetLeftStick();

	if (leftJoystick.GetMagnitude() > 0.0f) {
		turnDirection = leftJoystick.GetPosition().GetNormalized();
		move = true;
	}

	m_isMoving = false;
	if (move) {
		m_isMoving = true;
		m_goalOrientation = turnDirection.GetOrientationDegrees();

		float resultingOrientation = turnDirection.GetOrientationDegrees();
		float difBetweenTurretAndQuadDeg = m_turretOrientation - m_orientationDegrees;
		m_orientationDegrees = GetTurnedTowardDegrees(m_orientationDegrees, resultingOrientation, m_turnSpeed * deltaSeconds);
		float difBetweentTurretAndGoal = m_goalTurretOrientation - m_turretOrientation;
		m_turretOrientation = m_orientationDegrees + difBetweenTurretAndQuadDeg;
		m_goalTurretOrientation = m_turretOrientation + difBetweentTurretAndGoal;

		m_velocity = GetForwardNormal() * m_speed;

		if (leftJoystick.GetMagnitude() > 0.0f) {
			m_velocity *= leftJoystick.GetMagnitude();
		}
	}

}

void PlayerTank::UpdateTurretQuadOrientation(float deltaSeconds)
{

	bool move = false;
	Vec2 turnDirection;

	if (g_theInput->IsKeyDown('I')) {
		turnDirection += Vec2(0.0f, 1.0f);
		move = true;
	}

	if (g_theInput->IsKeyDown('J')) {
		turnDirection += Vec2(-1.0f, 0.0f);
		move = true;
	}

	if (g_theInput->IsKeyDown('K')) {
		turnDirection += Vec2(0.0f, -1.0f);
		move = true;
	}

	if (g_theInput->IsKeyDown('L')) {
		turnDirection += Vec2(1.0f, 0.0f);
		move = true;
	}

	XboxController controller = g_theInput->GetController(0);
	AnalogJoystick rightJoystick = controller.GetRightStick();


	if (rightJoystick.GetMagnitude() > 0.0f) {
		turnDirection = rightJoystick.GetPosition().GetNormalized();
		move = true;
	}

	if (move) {
		m_goalTurretOrientation = turnDirection.GetOrientationDegrees();
		float resultingOrientation = turnDirection.GetOrientationDegrees();
		m_turretOrientation = GetTurnedTowardDegrees(m_turretOrientation, resultingOrientation, m_turretTurnSpeed * deltaSeconds);
	}
}

void PlayerTank::UpdateCrossingAnimation(float deltaSeconds)
{
	m_totalTimeCrossingMaps += deltaSeconds;
	if (m_totalTimeCrossingMaps <= m_timeToCrossMaps * 0.5f) {
		float timeCrossingAsFraction = GetFractionWithin(m_totalTimeCrossingMaps, 0.0f, m_timeToCrossMaps * 0.5f);
		float sqrTimeCrossingMaps = timeCrossingAsFraction * timeCrossingAsFraction;

		float animationTurnSpeedFraction = RangeMapClamped(sqrTimeCrossingMaps, 0.0f, 1.0f, 0.0f, 2.0f);
		m_orientationDegrees = m_animationStartingOrientation + m_crossingAnimationTurnSpeed * animationTurnSpeedFraction;
		m_turretOrientation = m_animationStartingOrientation + m_crossingAnimationTurnSpeed * animationTurnSpeedFraction;
	}
	else {
		float timeCrossingAsFraction = GetFractionWithin(m_totalTimeCrossingMaps, m_timeToCrossMaps * 0.5f, m_timeToCrossMaps);
		float sqrTimeCrossingMaps = timeCrossingAsFraction * timeCrossingAsFraction;

		float animationTurnSpeedFraction = RangeMapClamped(sqrTimeCrossingMaps, 0.0f, 1.0f, 2.0f, 0.0f);
		m_orientationDegrees = m_animationStartingOrientation + m_crossingAnimationTurnSpeed * animationTurnSpeedFraction;
		m_turretOrientation = m_animationStartingOrientation + m_crossingAnimationTurnSpeed * animationTurnSpeedFraction;
	}

	if (m_totalTimeCrossingMaps >= m_timeToCrossMaps) {
		m_orientationDegrees = m_animationStartingOrientation;
		m_turretOrientation = m_animationStartingTurrentOrientation;
		m_isCrossingMaps = false;
	}


}

void PlayerTank::Render() const
{
	if (m_isAlive) {
		g_theRenderer->BindTexture(m_baseTexture);
		std::vector<Vertex_PCU> worldVerts;
		worldVerts.reserve(6);

		static float playerTankHalfSize = g_gameConfigBlackboard.GetValue("PLAYER_TANK_HALF_SIZE", 0.5f);
		static float playerTankHalfTurretSize = g_gameConfigBlackboard.GetValue("PLAYER_TANK_HALF_TURRET_SIZE", 0.5f);

		OBB2 worldBaseQuad;
		worldBaseQuad.m_halfDimensions = Vec2(playerTankHalfSize, playerTankHalfSize);
		worldBaseQuad.m_center = m_position;
		worldBaseQuad.RotateAboutCenter(m_orientationDegrees);

		OBB2 worldTurretQuad;
		worldTurretQuad.m_halfDimensions = Vec2(playerTankHalfTurretSize, playerTankHalfTurretSize);
		worldTurretQuad.m_center = m_position;
		worldTurretQuad.RotateAboutCenter(m_turretOrientation);

		if (m_isCrossingMaps) {
			if (m_totalTimeCrossingMaps <= m_timeToCrossMaps * 0.5f) {
				float animationScale = RangeMap(m_totalTimeCrossingMaps, 0.0f, m_timeToCrossMaps * 0.5f, 1.0f, 0.0f);
				worldBaseQuad.m_halfDimensions *= animationScale;
				worldTurretQuad.m_halfDimensions *= animationScale;
			}
			else {
				float animationScale = RangeMap(m_totalTimeCrossingMaps, m_timeToCrossMaps * 0.5f, m_timeToCrossMaps, 0.0f, 1.0f);
				worldBaseQuad.m_halfDimensions *= animationScale;
				worldTurretQuad.m_halfDimensions *= animationScale;
			}
		}

		AddVertsForOBB2D(worldVerts, worldBaseQuad, Rgba8());

		g_theRenderer->DrawVertexArray((int)worldVerts.size(), worldVerts.data());

		worldVerts.clear();

		g_theRenderer->BindTexture(m_turretTexture);

		AddVertsForOBB2D(worldVerts, worldTurretQuad, Rgba8());
		g_theRenderer->DrawVertexArray((int)worldVerts.size(), worldVerts.data());

		g_theRenderer->BindTexture(nullptr);
	}

	if (g_ignorePhysics) {
		DebugDrawRing(m_position, m_cosmeticsRadius * 1.1f, m_debugLineThickness, Rgba8::BLACK);
	}

	if (g_ignoreDamage) {
		DebugDrawRing(m_position, m_cosmeticsRadius * 1.2f, m_debugLineThickness, Rgba8::WHITE);
	}

	if (g_drawDebug) {
		RenderDebug();
	}
}

void PlayerTank::Reset()
{
	m_isMoving = false;
	m_velocity = Vec2::ZERO;
	m_health = 5;
	m_isAlive = true;
	m_isGarbage = false;
}

void PlayerTank::StartCrossingMapsAnimation()
{
	m_isCrossingMaps = true;
	m_totalTimeCrossingMaps = 0.0f;
	m_animationStartingOrientation = m_orientationDegrees;
	m_animationStartingTurrentOrientation = m_turretOrientation;
}

void PlayerTank::Die()
{
	if (!m_isAlive) return;
	m_isMoving = false;
	m_velocity = Vec2::ZERO;
	Entity::Die();
}


void PlayerTank::RenderDebug() const
{
	std::vector<Vertex_PCU> debugVerts;
	Vec2 rightDir = Vec2(m_cosmeticsRadius, 0.0f);
	rightDir.SetOrientationDegrees(m_orientationDegrees);
	rightDir += m_position;

	Vec2 leftDir = Vec2(m_cosmeticsRadius, 0.0f);
	leftDir.SetOrientationDegrees(m_orientationDegrees + 90.0f);
	leftDir += m_position;

	Vec2 turretOrientation = Vec2(m_cosmeticsRadius * 1.1f, 0.0f);
	turretOrientation.SetOrientationDegrees(m_turretOrientation);
	turretOrientation += m_position;

	Rgba8 cyan(0, 255, 255, 255);
	Rgba8 magenta(255, 0, 255, 255);
	Rgba8 purple(153, 50, 204, 255);
	Rgba8 red(255, 0, 0, 255);
	Rgba8 green(0, 255, 0, 255);
	Rgba8 yellow(255, 255, 0, 255);

	g_theRenderer->BindTexture(nullptr);

	DebugDrawRing(m_position, m_physicsRadius, m_debugLineThickness, cyan);
	DebugDrawRing(m_position, m_cosmeticsRadius, m_debugLineThickness, magenta);
	DebugDrawLine(m_position, turretOrientation, m_debugLineThickness * 5.0f, purple);
	DebugDrawLine(m_position, rightDir, m_debugLineThickness * 2.0f, red);
	if (m_isMoving) {
		Vec2 movingLine = Vec2(m_cosmeticsRadius * 1.5f, 0.0f);
		movingLine.SetOrientationDegrees(m_orientationDegrees);
		movingLine += m_position;

		DebugDrawLine(m_position, movingLine, m_debugLineThickness, yellow);
	}
	DebugDrawLine(m_position, leftDir, m_debugLineThickness, green);

	OBB2 baseGoalDebug;
	baseGoalDebug.m_center = Vec2(CosDegrees(m_goalOrientation), SinDegrees(m_goalOrientation)) * m_cosmeticsRadius * 1.1f;
	baseGoalDebug.m_center += m_position;
	baseGoalDebug.m_halfDimensions = Vec2(0.1f, 0.015f);
	baseGoalDebug.m_iBasisNormal.SetOrientationDegrees(m_goalOrientation);

	AddVertsForOBB2D(debugVerts, baseGoalDebug, red);

	OBB2 turretGoalDebug;
	turretGoalDebug.m_center = Vec2(CosDegrees(m_goalTurretOrientation), SinDegrees(m_goalTurretOrientation)) * m_cosmeticsRadius * 1.1f;
	turretGoalDebug.m_center += m_position;
	turretGoalDebug.m_halfDimensions = Vec2(0.1f, 0.05f);
	turretGoalDebug.m_iBasisNormal.SetOrientationDegrees(m_goalTurretOrientation);

	AddVertsForOBB2D(debugVerts, turretGoalDebug, purple);

	g_theRenderer->DrawVertexArray((int)debugVerts.size(), debugVerts.data());


}

void PlayerTank::TakeDamage()
{
	if (!g_ignoreDamage) {
		Entity::TakeDamage();
	}
}
