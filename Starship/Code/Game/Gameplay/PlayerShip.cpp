#include "Game/Gameplay/PlayerShip.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Framework/App.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"

extern App* g_theApp;


PlayerShip::PlayerShip(Game* gamePointer, const Vec2& startingPosition) :
	Entity(gamePointer, startingPosition)
{
	m_health = 3;

	m_color = Rgba8(102, 153, 204, 255);
	m_angularVelocity = PLAYER_SHIP_TURN_SPEED;
	m_physicsRadius = PLAYER_SHIP_PHYSICS_RADIUS;
	m_cosmeticRadius = PLAYER_SHIP_COSMETIC_RADIUS;
	m_shieldRadius = m_physicsRadius * SHIELD_PHYSICS_RADIUS_PERCENTAGE;
	InitializeLocalVerts();
}

PlayerShip::~PlayerShip()
{
	m_game = nullptr;
	g_theAudio->StopSound(m_flameSound);
}

void PlayerShip::Update(float deltaSeconds)
{
	if (m_isSlowedDown) {
		deltaSeconds *= 0.5f;
		m_clockSoundSpeed = 0.5f;
	}

	if (m_isSpedUp) {
		deltaSeconds *= 2.0f;
		m_clockSoundSpeed = 1.0f;
	}



	UpdateFromKeyboard(deltaSeconds);
	UpdateFromController(deltaSeconds);

	m_position += m_velocity * deltaSeconds;

	BounceOffWalls();

	if (m_health == 0) 
		m_playerDeathTime += deltaSeconds;
	

	HandleSound();

	m_isSpedUp = false;
	m_isSlowedDown = false;

	if (m_renderShieldHit) {
		m_shieldHitEffectTimer += deltaSeconds;
		if (m_shieldHitEffectTimer >= SHIELD_TIME_HIT_EFFECT) {
			m_renderShieldHit = false;
			m_shieldHitEffectTimer = 0.0f;
		}
	}

}

void PlayerShip::HandleSound() {
	// Clock Sounds
	bool timeWarpXORCondition = (!m_isSlowedDown && m_isSpedUp) || (m_isSlowedDown && !m_isSpedUp);
	if (!m_playingClockSound && timeWarpXORCondition) {
		m_clockTickingSound = g_theAudio->StartSound(clockTickingSound, true, 20.0f, 0.0f, m_clockSoundSpeed);
		m_playingClockSound = true;
	}
	if (!timeWarpXORCondition) {
		if (m_playingClockSound) {
			g_theAudio->StopSound(m_clockTickingSound);
			m_playingClockSound = false;
		}
	}

	// Flame sounds
	if (m_thrustForce > 0.0f) {
		if (!m_playingFlameSound) {
			m_flameSound = g_theAudio->StartSound(playerFlameSound, true);
			m_playingFlameSound = true;
		}
		g_theAudio->SetSoundPlaybackVolume(m_flameSound, m_thrustForce);
	}
	else {
		g_theAudio->StopSound(m_flameSound);
		m_playingFlameSound = false;
	}
}

void PlayerShip::UpdateFromKeyboard(float deltaSeconds)
{

	if (m_isDead) {
		if (g_theInput->WasKeyJustPressed('N') && m_health > 0) {
			Respawn();
		}
		else {
			return;
		}
	}
	if (g_theInput->IsKeyDown('S')) { // S key
		m_orientationDegrees += m_angularVelocity * deltaSeconds;
	}
	if (g_theInput->IsKeyDown('F')) { // F key
		m_orientationDegrees -= m_angularVelocity * deltaSeconds;
	}

	if (g_theInput->IsKeyDown('E')) { // E key
		const Vec2& forwardNormal = GetForwardNormal();
		m_velocity += PLAYER_SHIP_ACCELERATION * deltaSeconds * forwardNormal;
		m_thrustForce = 1.0f;
	}
	else {
		m_thrustForce = 0.0f;
	}

	if (g_theInput->WasKeyJustPressed(' ') && !Clock::GetSystemClock().IsPaused()) {
		Shoot();
	}
}

void PlayerShip::UpdateFromController(float deltaSeconds)
{

	if (Clock::GetSystemClock().IsPaused()) return;
	XboxController controller = g_theInput->GetController(0);

	if (m_isDead) {
		if (controller.WasButtonJustPressed(XboxButtonID::Start) && m_health > 0) {
			Respawn();
		}
		else {
			return;
		}
	}

	if (controller.WasButtonJustPressed(XboxButtonID::A) && !Clock::GetSystemClock().IsPaused()) {
		Shoot();
	}

	AnalogJoystick joystick = controller.GetLeftStick();
	float joystickMagnitude = joystick.GetMagnitude();
	if (joystickMagnitude > 0.0f) {
		m_orientationDegrees = joystick.GetOrientationDegrees();
		m_thrustForce = joystickMagnitude;

		m_velocity += GetForwardNormal() * m_thrustForce * PLAYER_SHIP_ACCELERATION * deltaSeconds;
	}

}

void PlayerShip::BounceOffWalls()
{
	if (m_position.x > WORLD_SIZE_X - m_physicsRadius) {
		m_position.x = WORLD_SIZE_X - m_physicsRadius;
		m_velocity.x *= -1;
	}

	if (m_position.x < m_physicsRadius) {
		m_position.x = m_physicsRadius;
		m_velocity.x *= -1;
	}

	if (m_position.y > WORLD_SIZE_Y - m_physicsRadius) {
		m_position.y = WORLD_SIZE_Y - m_physicsRadius;
		m_velocity.y *= -1;
	}

	if (m_position.y < m_physicsRadius) {
		m_position.y = m_physicsRadius;
		m_velocity.y *= -1;
	}

}

void PlayerShip::Respawn()
{
	m_position = Vec2(WORLD_CENTER_X, WORLD_CENTER_Y);
	m_orientationDegrees = 0.f;
	m_velocity = Vec2();
	m_isDead = false;
	m_game->StopShakeScreen();
	g_theAudio->StopSound(m_deathSound);
	m_respawnSound = g_theAudio->StartSound(respawnSound);
}

void PlayerShip::StopAllSounds()
{
	g_theAudio->StopSound(m_deathSound);
	g_theAudio->StopSound(m_currentShootingSound);
	g_theAudio->StopSound(m_clockTickingSound);
	g_theAudio->StopSound(m_flameSound);
	g_theAudio->StopSound(m_respawnSound);
}

void PlayerShip::DrawPlayerShip(const Vec2& position, float orientationDegrees, float scale, float verticalOffset)
{
	Vertex_PCU vertArray[PLAYER_SHIP_VERTEX_AMOUNT] = {};
	Rgba8 blueGrey(102, 153, 204, 255);
	GetLocalVerts(vertArray, blueGrey, verticalOffset);
	TransformVertexArrayXY3D(PLAYER_SHIP_VERTEX_AMOUNT, vertArray, scale, orientationDegrees, position);

	g_theRenderer->DrawVertexArray(PLAYER_SHIP_VERTEX_AMOUNT, vertArray);
}



void PlayerShip::Render() const
{
	if (m_isDead) return;

	float tailFlickerAmount = 0.5f;

	Vertex_PCU world_verts[PLAYER_SHIP_VERTEX_AMOUNT] = {};
	for (int vertIndex = 0; vertIndex < PLAYER_SHIP_VERTEX_AMOUNT; vertIndex++) {
		world_verts[vertIndex] = m_verts[vertIndex];
	}

	// Flame tail vertex
	if (m_thrustForce > 0.0f) {
		Vertex_PCU& flameVertex = world_verts[16];
		flameVertex.m_position.x = RangeMap(m_thrustForce, 0.0f, 1.0f, -2.0f, -5.0f);
		flameVertex.m_position.x += rng.GetRandomFloatInRange(-tailFlickerAmount, tailFlickerAmount);
		flameVertex.m_color.a = static_cast<unsigned char>(rng.GetRandomIntInRange(0, 255));
	}

	TransformVertexArrayXY3D(PLAYER_SHIP_VERTEX_AMOUNT, world_verts, 1.f, m_orientationDegrees, m_position);

	g_theRenderer->DrawVertexArray(PLAYER_SHIP_VERTEX_AMOUNT, world_verts);

	RenderShield();

}

void PlayerShip::RenderLives() const
{
	float livesY = UI_SIZE_Y - PLAYER_SHIP_COSMETIC_RADIUS - (UI_DRAWING_SPACE_BUFFER * .5f);
	float livesX = PLAYER_SHIP_COSMETIC_RADIUS + UI_DRAWING_SPACE_BUFFER;

	for (int liveIndex = 0; liveIndex < m_health; liveIndex++, livesX += PLAYER_SHIP_COSMETIC_RADIUS + UI_DRAWING_SPACE_BUFFER) {
		DrawPlayerShip(Vec2(livesX, livesY), 90.0f, 5.0f);
	}
}

void PlayerShip::Die()
{
	m_health--;
	m_isDead = true;
	SpawnDeathDebrisCluster();
	m_deathSound = g_theAudio->StartSound(playerExplosionSound);
	m_thrustForce = 0.0f;
	g_theAudio->StopSound(m_clockTickingSound);

	if (m_health <= 0) {
		m_game->m_useTextAnimation = true;
		m_game->m_currentText = "YOU DIED"; 
		m_game->m_textAnimationColor = Rgba8(255, 0, 0, 255);
		m_game->m_loseGameSoundID = g_theAudio->StartSound(loseGameSound, false, 2.0f);
	}
}

void PlayerShip::TakeAHit(Vec2 const& newHitPosition)
{
	m_hitPosition = newHitPosition;
	if (m_shieldHealth > 0) {
		m_shieldHealth--;
		m_renderShieldHit = true;
		m_game->ShakeScreenCollision();
	}
	else {
		if (!m_isDead) {
			Die();
			m_game->ShakeScreenPlayerDeath();
		}
	}
}

void PlayerShip::SpawnDeathDebrisCluster() const {
	float debrisMinSpeed = m_velocity.GetLength() * 2.0f;
	float debrisMaxSpeed = debrisMinSpeed * 1.5f;

	m_game->SpawnDebrisCluster(PLAYER_DEBRIS_AMOUNT, m_position, m_color, m_velocity, DEBRIS_MIN_SCALE_DEATH, DEBRIS_MAX_SCALE_DEATH, debrisMinSpeed, debrisMaxSpeed);
}

void PlayerShip::Shoot()
{
	const Vec2& forwardNormal = GetForwardNormal();
	m_game->SpawnBullet(m_position + forwardNormal, m_orientationDegrees);
	m_currentShootingSound = g_theAudio->StartSound(playerShootingSound);

}

void PlayerShip::InitializeLocalVerts()
{

	GetLocalVerts(m_verts, m_color);
}

void PlayerShip::GetLocalVerts(Vertex_PCU* storeArray, Rgba8& color, float verticalOffset) {
	storeArray[0] = Vertex_PCU(Vec3(-2.0f, 1.0f, 0.0f), color, Vec2()); // left triangle A vertex
	storeArray[1] = Vertex_PCU(Vec3(2.0f, 1.0f, 0.0f), color, Vec2()); // right triangle A vertex
	storeArray[2] = Vertex_PCU(Vec3(0, 2.0f, 0.0f), color, Vec2()); // top triangle A vertex

	storeArray[3] = Vertex_PCU(Vec3(-2.0f, 1.0f, 0.0f), color, Vec2()); // top left triangle B vertex
	storeArray[4] = Vertex_PCU(Vec3(-2.0f, -1.0f, 0.0f), color, Vec2()); // bottom left triangle B vertex
	storeArray[5] = Vertex_PCU(Vec3(0, 1.0f, 0.0f), color, Vec2()); // right triangle B vertex

	storeArray[6] = Vertex_PCU(Vec3(-2.0f, -1.0f, 0.0f), color, Vec2()); // left triangle C vertex
	storeArray[7] = Vertex_PCU(Vec3(0, -1.0f, 0.0f), color, Vec2()); // bottom right triangle C vertex
	storeArray[8] = Vertex_PCU(Vec3(0, 1.0f, 0.0f), color, Vec2()); // top right triangle C vertex

	storeArray[9] = Vertex_PCU(Vec3(0, -1.0f, 0.0f), color, Vec2()); // bottom left triangle D vertex
	storeArray[10] = Vertex_PCU(Vec3(1.0f, 0, 0.0f), color, Vec2()); // right triangle D vertex
	storeArray[11] = Vertex_PCU(Vec3(0, 1.0f, 0.0f), color, Vec2()); // top left triangle D vertex

	storeArray[12] = Vertex_PCU(Vec3(-2.0f, -1.0f, 0.0f), color, Vec2()); // left triangle E vertex
	storeArray[13] = Vertex_PCU(Vec3(0, -2.0f, 0.0f), color, Vec2()); // bottom triangle E vertex
	storeArray[14] = Vertex_PCU(Vec3(2.0f, -1.0f, 0.0f), color, Vec2()); // right triangle E vertex

	storeArray[15] = Vertex_PCU(Vec3(-2.0f, -1.0f, 0.0f), Rgba8(255, 0, 0, 255), Vec2()); // flame
	storeArray[16] = Vertex_PCU(Vec3(-2.0f, 0.0f, 0.0f), Rgba8(255, 0, 0, 255), Vec2());
	storeArray[17] = Vertex_PCU(Vec3(-2.0f, 1.0f, 0.0f), Rgba8(255, 0, 0, 255), Vec2());

	if (verticalOffset > 0) {
		for (int storeIndex = 0; storeIndex < PLAYER_SHIP_VERTEX_AMOUNT; storeIndex++)
		{
			storeArray[storeIndex].m_position.y += verticalOffset;
		}
	}
}
