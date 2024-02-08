#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Math/Easing.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Actor.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/Weapon.hpp"
#include "Game/Gameplay/WeaponDefinition.hpp"
#include "Game/Gameplay/SpawnInfo.hpp"

Player::Player(Map* pointerToMap, Camera* worldCamera, Camera* screenCamera, int playerIndex, int controllerIndex) :
	m_worldCamera(worldCamera),
	m_screenCamera(screenCamera),
	m_playerIndex(playerIndex),
	m_controllerIndex(controllerIndex)
{
	m_map = pointerToMap;


	tinyxml2::XMLDocument doomFaceAnims;
	XMLError animLoadStatus = doomFaceAnims.LoadFile("Data/Definitions/DoomFaceAnimation.xml");
	GUARANTEE_OR_DIE(animLoadStatus == XMLError::XML_SUCCESS, "DOOM FACE ANIMS XML DOES NOT EXIST OR CANNOT BE FOUND");

	m_doomFace = new SpriteAnimGroupDefinition(g_textures[(int)GAME_TEXTURE::Doom_Face]);
	m_doomFace->LoadAnimationsFromXML(*doomFaceAnims.FirstChildElement());
}

Player::~Player()
{
	delete m_doomFace;
	m_doomFace = nullptr;
}

void Player::Update(float deltaSeconds)
{
	Actor* actor = GetActor();
	if (actor) {
		m_killCount = actor->m_killCount;
	}
	if (!m_freeFlyCameraMode) {
		UpdatePossessed(deltaSeconds);
	}
	else {
		UpdateFreeFly(deltaSeconds);
	}

	UpdateDeveloperCheatCodes();

	float& pitch = m_orientation.m_pitchDegrees;
	float& roll = m_orientation.m_rollDegrees;

	pitch = Clamp(pitch, -m_pitchApertureAngleRotation, m_pitchApertureAngleRotation);
	roll = Clamp(roll, -m_rollApertureAngleRotation, m_rollApertureAngleRotation);

	if (actor) {

		Weapon* currentWeapon = actor->GetEquippedWeapon();
		if (m_isAttacking && (currentWeapon->m_refireStopwatch.GetElapsedTime() >= m_currentAnim->GetDuration())) {
			m_isAttacking = false;
			m_currentAnim = currentWeapon->m_definition->m_idleAnimationDefinition;
		}

		if (!m_currentAnim) {
			m_currentAnim = currentWeapon->m_definition->m_idleAnimationDefinition;
		}

		if (actor->m_gotHurt) {
			m_healthRegenStopwatch.Start(&m_map->GetGame()->m_clock, m_healthRegenTime);
			actor->m_gotHurt = false;
		}

		if (m_healthRegenStopwatch.HasDurationElapsed() && m_healthRegenStopwatch.m_duration != 0.0) {
			if (actor->m_health < actor->m_definition->m_health) {
				actor->m_health += deltaSeconds * m_healthRegenRate;
				if (actor->m_health > actor->m_definition->m_health) {
					actor->m_health = actor->m_definition->m_health;
				}
			}
		}
	}
	else {
		if (m_respawnStopWatch.HasDurationElapsed() && !m_map->IsHordeMode()) {
			Actor const* randomSpawn = m_map->GetRandomSpawn();

			SpawnInfo marineSpawnInfo = {};
			marineSpawnInfo.m_definition = ActorDefinition::GetByName("Marine");
			marineSpawnInfo.m_position = randomSpawn->m_position;
			marineSpawnInfo.m_orientation = randomSpawn->m_orientation;

			Actor* spawnedMarine = m_map->SpawnActor(marineSpawnInfo);
			spawnedMarine->m_killCount = m_killCount;
			Possess(spawnedMarine);
		}
	}

	if (!m_map->IsActorAlive(actor) && m_respawnStopWatch.HasDurationElapsed()) {
		m_respawnStopWatch.Start(&m_map->GetGame()->m_clock, m_respawnTime);
		m_playerDied = true;
	}

	//Light spotLight = {};
	//spotLight.Position = m_position;
	//Rgba8::WHITE.GetAsFloats(spotLight.Color);
	//spotLight.ConstantAttenuation = 0.05f;
	//spotLight.LinearAttenuation = 0.05f;
	//spotLight.QuadraticAttenuation = 0.05f;
	//spotLight.Direction = m_orientation.GetXForward();
	//spotLight.LightType = 1;
	//spotLight.SpotAngle = 25.0f;
	//spotLight.Enabled = true;

	//g_theRenderer->SetLight(spotLight, 2);
}


void Player::Render() const
{
}

void Player::Possess(Actor* actor)
{
	if (!actor) {
		Actor* previouslyPossessed = m_map->GetActorByUID(m_actorUID);
		if (previouslyPossessed) {

			previouslyPossessed->OnUnpossessed(this);
		}
	}
	else {
		Controller::Possess(actor);
		m_orientation = actor->m_orientation;
	}
}

void Player::ToggleInvincibility()
{
	Actor* possessedActor = GetActor();
	if (possessedActor) {
		possessedActor->m_isInvincible = !possessedActor->m_isInvincible;
	}
}

Mat44 Player::GetModelMatrix() const
{
	Mat44 model = m_orientation.GetMatrix_XFwd_YLeft_ZUp();
	model.SetTranslation3D(m_position);
	return model;
}

void Player::UpdateFreeFly(float deltaSeconds)
{
	UpdateInputFreeFly(deltaSeconds);
	m_worldCamera->SetPerspectiveView(g_theWindow->GetConfig().m_clientAspect, 60.0f, 0.1f, 100.0f);
}

void Player::UpdatePossessed(float deltaSeconds)
{
	Actor* actor = GetActor();

	if (actor && actor->m_isInvincible) {
		DebugAddMessage("Invincible", 0.0f, Rgba8::RED, Rgba8::RED);
	}

	if (!m_map->IsActorAlive(actor)) return;
	UpdateInput(deltaSeconds);

	m_position = actor->m_position;
	m_position.z = actor->m_definition->m_eyeHeight;


}

void Player::UpdateInput(float deltaSeconds)
{
	if (m_controllerIndex == -1) {
		UpdateInputFromKeyboard(deltaSeconds);
	}
	else {
		UpdateInputFromController(deltaSeconds);
	}
}

void Player::UpdateInputFreeFly(float deltaSeconds)
{
	UpdateInputFromKeyboardFreeFly(deltaSeconds);
	UpdateInputFromControllerFreeFly(deltaSeconds);
}

void Player::UpdateInputFromKeyboard(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	Actor* actor = GetActor();
	if (!actor) return;

	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	Vec2 deltaMouseInput = g_theInput->GetMouseClientDelta();

	float speedUsed = actor->m_definition->m_walkSpeed;
	float mouseSpeed = m_mouseNormalSpeed;


	Vec3 forceToAdd = Vec3::ZERO;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
		mouseSpeed = m_mouseSprintSpeed;
		speedUsed = actor->m_definition->m_runSpeed;
	}

	if (g_theInput->IsKeyDown('W')) {
		forceToAdd += iBasis;
	}

	if (g_theInput->IsKeyDown('S')) {
		forceToAdd -= iBasis;
	}

	if (g_theInput->IsKeyDown('A')) {
		forceToAdd += jBasis;
	}

	if (g_theInput->IsKeyDown('D')) {
		forceToAdd -= jBasis;
	}

	if (g_theInput->IsKeyDown('H')) {
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles();
	}

	forceToAdd.z = 0.0f;
	forceToAdd.Normalize();

	forceToAdd *= actor->m_definition->m_drag * speedUsed;

	actor->AddForce(forceToAdd);

	deltaMouseInput *= mouseSpeed;

	m_orientation += EulerAngles(deltaMouseInput.x, -deltaMouseInput.y, 0.0f);
	actor->m_orientation += EulerAngles(deltaMouseInput.x, -deltaMouseInput.y, 0.0f);

	if (!actor) return;

	if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) {
		actor->Attack();
		m_isAttacking = true;
		m_currentAnim = actor->GetEquippedWeapon()->m_definition->m_attackAnimationDefinition;
	}

	bool changedWeapon = false;
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFTARROW)) {
		changedWeapon = true;
		actor->EquipPreviousWeapon();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHTARROW)) {
		actor->EquipNextWeapon();
		changedWeapon = true;
	}

	if (g_theInput->WasKeyJustPressed('1')) {
		actor->EquipWeapon(0);
		changedWeapon = true;
	}

	if (g_theInput->WasKeyJustPressed('2')) {
		changedWeapon = true;
		actor->EquipWeapon(1);
	}

	if (g_theInput->WasKeyJustPressed('3')) {
		changedWeapon = true;
		actor->EquipWeapon(2);
	}

	if (changedWeapon) {
		m_currentAnim = actor->GetEquippedWeapon()->m_definition->m_idleAnimationDefinition;
	}
}

void Player::UpdateInputFromKeyboardFreeFly(float deltaSeconds)
{
	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	Vec2 deltaMouseInput = g_theInput->GetMouseClientDelta();

	float speedUsed = m_playerNormalSpeed;
	float mouseSpeed = m_mouseNormalSpeed;
	float zSpeedUsed = m_playerZSpeed;

	Vec3 velocity = Vec3::ZERO;

	if (g_theInput->IsKeyDown(KEYCODE_SHIFT)) {
		mouseSpeed = m_mouseSprintSpeed;
		speedUsed = m_playerSprintSpeed;
		zSpeedUsed = m_playerZSprintSpeed;
	}

	if (g_theInput->IsKeyDown('W')) {
		velocity += iBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('S')) {
		velocity -= iBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('A')) {
		velocity += jBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('D')) {
		velocity -= jBasis * speedUsed;
	}

	if (g_theInput->IsKeyDown('Z')) {
		velocity.z += zSpeedUsed;
	}

	if (g_theInput->IsKeyDown('C')) {
		velocity.z += -zSpeedUsed;
	}

	if (g_theInput->IsKeyDown('H')) {
		m_position = Vec3::ZERO;
		m_orientation = EulerAngles();
	}

	deltaMouseInput *= mouseSpeed;
	m_orientation += EulerAngles(deltaMouseInput.x, -deltaMouseInput.y, 0.0f);
	m_position += velocity * deltaSeconds;
}

void Player::UpdateInputFromController(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Actor* actor = GetActor();
	if (!actor) return;

	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;
	m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	iBasis.Normalize();
	jBasis.Normalize();

	XboxController controller = g_theInput->GetController(m_controllerIndex);
	float speedUsed = actor->m_definition->m_walkSpeed;

	if (controller.IsButtonDown(XboxButtonID::A)) {
		speedUsed = actor->m_definition->m_runSpeed;;
	}



	Vec3 forceToAdd = Vec3::ZERO;


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

		forceToAdd.x = 0.0f;
		forceToAdd.y = 0.0f;

		forceToAdd += iBasis;
		forceToAdd += jBasis;
	}

	forceToAdd.z = 0.0f;
	forceToAdd.Normalize();

	forceToAdd *= actor->m_definition->m_drag * speedUsed;

	actor->AddForce(forceToAdd);

	AnalogJoystick rightStick = controller.GetRightStick();
	float rightJoyStickMagnitude = rightStick.GetMagnitude();
	if (rightJoyStickMagnitude > 0.0f) {
		Vec2 cameraAngle = rightStick.GetPosition();
		cameraAngle *= rightJoyStickMagnitude;
		m_orientation += EulerAngles(-cameraAngle.x * m_controllerCameraSpeed, -cameraAngle.y * m_controllerCameraSpeed, 0.0f) * deltaSeconds;
		actor->m_orientation += EulerAngles(-cameraAngle.x * m_controllerCameraSpeed, -cameraAngle.y * m_controllerCameraSpeed, 0.0f) * deltaSeconds;
	}

	if (!actor)return;

	if (controller.GetRightTrigger() > 0.0f) {
		actor->Attack();
		m_isAttacking = true;
		m_currentAnim = actor->GetEquippedWeapon()->m_definition->m_attackAnimationDefinition;
	}

	bool changedWeapon = false;

	if (controller.WasButtonJustPressed(XboxButtonID::Right)) {
		actor->EquipNextWeapon();
		changedWeapon = true;
	}

	if (controller.WasButtonJustPressed(XboxButtonID::Left)) {
		actor->EquipPreviousWeapon();
		changedWeapon = true;
	}

	if (changedWeapon) {
		m_currentAnim = actor->GetEquippedWeapon()->m_definition->m_idleAnimationDefinition;
	}
}

void Player::UpdateInputFromControllerFreeFly(float deltaSeconds)
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


	AnalogJoystick leftStick = controller.GetLeftStick();
	float leftJoyStickMagnitude = leftStick.GetMagnitude();
	if (leftJoyStickMagnitude > 0.0f) {
		Vec2 stickPos = leftStick.GetPosition();
		iBasis *= stickPos.y;
		jBasis *= -stickPos.x;

		Vec3 velocity = Vec3::ZERO;

		velocity += iBasis * speedUsed;
		velocity += jBasis * speedUsed;

		m_position += velocity * deltaSeconds;
	}

	AnalogJoystick rightStick = controller.GetRightStick();
	float rightJoyStickMagnitude = rightStick.GetMagnitude();
	if (rightJoyStickMagnitude > 0.0f) {
		Vec2 cameraAngle = rightStick.GetPosition();
		cameraAngle *= rightJoyStickMagnitude;
		m_orientation += EulerAngles(-cameraAngle.x * m_controllerCameraSpeed, -cameraAngle.y * m_controllerCameraSpeed, 0.0f) * deltaSeconds;
	}


}

void Player::UpdateDeveloperCheatCodes()
{
	if (m_map->GetPlayerAmount() > 1) return;
	Actor* actor = GetActor();

	bool pressedFromController = false;

	if (m_controllerIndex != -1) {
		XboxController controller = g_theInput->GetController(m_controllerIndex);
		pressedFromController = controller.WasButtonJustPressed(XboxButtonID::Y);
	}

	if (g_theInput->WasKeyJustPressed('F') || pressedFromController) {
		m_freeFlyCameraMode = !m_freeFlyCameraMode;
		if (!m_freeFlyCameraMode) {
			m_orientation = actor->m_orientation;
		}
	}
}

void Player::UpdateCameras()
{
	Vec2 cameraSize = Vec2(g_theWindow->GetClientDimensions());
	Vec2 cameraDimensions = m_screenCamera->GetViewport().GetDimensions() * cameraSize;
	float correctAspectRatio = cameraDimensions.x / cameraDimensions.y;
	if (!m_freeFlyCameraMode) {
		Actor* actor = GetActor();
		if (!actor) return;
		float fov = actor->m_definition->m_cameraFOVDegrees;
		m_worldCamera->SetPerspectiveView(correctAspectRatio, fov, 0.1f, 100.0f);

		// Tint lights red depending on remaining health
		float tLight = 1.0f - (actor->m_health / actor->m_definition->m_health);
		Rgba8 directionalLightColor = Rgba8::InterpolateColors(Rgba8(m_map->m_defaultDirectionalLightIntensity), Rgba8::LIGHTRED, tLight);
		Rgba8 ambientLightColor = Rgba8::InterpolateColors(Rgba8(m_map->m_defaultAmbientLightIntensity), Rgba8::LIGHTRED, tLight);
		m_map->SetDirectionalLightIntensity(directionalLightColor, m_playerIndex);
		m_map->SetAmbientLightIntensity(ambientLightColor, m_playerIndex);
	}
	else {
		m_worldCamera->SetPerspectiveView(correctAspectRatio, 60.0f, 0.1f, 100.0f);
	}
	m_worldCamera->SetTransform(m_position, m_orientation);

}

void Player::RenderHUD(Camera const& camera) const
{
	Actor* actor = GetActor();
	AABB2 cameraViewport = camera.GetViewport();
	AABB2 viewport = camera.GetCameraBounds().GetBoxWithin(cameraViewport);

	if (!actor && m_playerDied && m_map->IsHordeMode()) {
		std::vector<Vertex_PCU> deadVerts;
		deadVerts.reserve(6);
		AddVertsForAABB2D(deadVerts, viewport, Rgba8::WHITE);
		g_theRenderer->BindTexture(g_textures[(int)GAME_TEXTURE::Died]);
		g_theRenderer->DrawVertexArray(deadVerts);
		return;
	}

	if(!actor)return;

	if (actor->m_definition->m_faction != Faction::MARINE) return;

	WeaponDefinition const* currentWeaponDef = actor->GetEquippedWeapon()->m_definition;

	Vec2 spriteSize = currentWeaponDef->m_spriteSize;


	Vec2 cameraViewportDims = camera.GetViewport().GetDimensions();
	Vec2 cameraSize = (camera.GetOrthoTopRight() - camera.GetOrthoBottomLeft());
	Vec2 viewportSize = cameraSize * cameraViewport.GetDimensions();


	Vec2 const& bottomLeft = viewport.m_mins;
	Vec2 topRight = viewport.m_maxs;

	topRight.y = bottomLeft.y + (viewportSize.x * 0.1f * cameraViewport.GetDimensions().y);


	float textCellHeight = viewportSize.y * 0.05f;
	float smallestViewportDim = (cameraViewportDims.x < cameraViewportDims.y) ? cameraViewportDims.x : cameraViewportDims.y;

	AABB2 HUDQuad(bottomLeft, topRight);
	AABB2 healthQuad(Vec2::ZERO, Vec2(cameraSize.x * 0.25f, textCellHeight));
	AABB2 killQuad(Vec2::ZERO, Vec2(cameraSize.x * 0.25f, textCellHeight));
	AABB2 weaponQuad(Vec2::ZERO, currentWeaponDef->m_spriteSize * smallestViewportDim);

	std::vector<Vertex_PCU> hudVerts;
	std::vector<Vertex_PCU> reticleVerts;
	std::vector<Vertex_PCU> textInfoVerts;
	std::vector<Vertex_PCU> weaponVerts;

	std::string playerHealth = Stringf("%.0f", actor->m_health);
	std::string playerKillCount = Stringf("%d", actor->m_killCount);

	Vec2 actualReticleSize = currentWeaponDef->m_reticleSize; // Considered reducing reticle in accordance to viewport size. It looks bad. Leaving it here just in case

	Vec2 reticlePos = bottomLeft + ((viewportSize - actualReticleSize) * 0.5f);
	AABB2 reticleQuad(Vec2::ZERO, actualReticleSize);
	reticleQuad.Translate(reticlePos);

	HUDQuad.AlignABB2WithinBounds(healthQuad, Vec2(0.15f, 0.6f));
	HUDQuad.AlignABB2WithinBounds(killQuad, Vec2(0.355f, 0.6f));
	HUDQuad.AlignABB2WithinBounds(weaponQuad, Vec2(0.5f, 1.0f));
	weaponQuad.Translate(Vec2(0.0f, currentWeaponDef->m_spriteSize.y * smallestViewportDim));

	AABB2 doomFaceQuad = HUDQuad.GetBoxWithin(0.455f, 0.0f, 0.55f, 0.9f);
	float animLengthTime = m_doomFace->GetTotalLengthTime();
	float doomFaceT = fmodf((float)m_map->GetGame()->m_clock.GetTotalTime(), animLengthTime) / animLengthTime;
	doomFaceT = CustomEasingFunction(doomFaceT) * animLengthTime;

	SpriteDefinition const* faceSpriteDef = m_doomFace->GetSpriteAtDefTime(doomFaceT, actor->GetHealthAsArbitraryVector().GetNormalized());
	std::vector<Vertex_PCU> doomFaceVerts;
	AddVertsForAABB2D(doomFaceVerts, doomFaceQuad, Rgba8::WHITE, faceSpriteDef->GetUVs());


	if (!m_currentAnim) return;
	SpriteDefinition weaponDef = m_currentAnim->GetSpriteDefAtTime((float)actor->GetEquippedWeapon()->m_refireStopwatch.GetElapsedTime());

	AddVertsForAABB2D(hudVerts, HUDQuad, Rgba8::WHITE);
	AddVertsForAABB2D(reticleVerts, reticleQuad, Rgba8::WHITE);
	AddVertsForAABB2D(weaponVerts, weaponQuad, Rgba8::WHITE, weaponDef.GetUVs());

	g_squirrelFont->AddVertsForTextInBox2D(textInfoVerts, healthQuad, textCellHeight, playerHealth);
	g_squirrelFont->AddVertsForTextInBox2D(textInfoVerts, killQuad, textCellHeight, playerKillCount);

	g_theRenderer->BindTexture(currentWeaponDef->m_hudBaseTexture);
	g_theRenderer->DrawVertexArray(hudVerts);

	g_theRenderer->BindTexture(currentWeaponDef->m_reticleTexture);
	g_theRenderer->DrawVertexArray(reticleVerts);

	g_theRenderer->BindTexture(&g_squirrelFont->GetTexture());
	g_theRenderer->DrawVertexArray(textInfoVerts);

	g_theRenderer->BindTexture(&weaponDef.GetTexture());
	g_theRenderer->DrawVertexArray(weaponVerts);

	g_theRenderer->BindTexture(&faceSpriteDef->GetTexture());
	g_theRenderer->DrawVertexArray(doomFaceVerts);


}
