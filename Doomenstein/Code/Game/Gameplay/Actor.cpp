#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Game/Gameplay/Actor.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"
#include "Game/Gameplay/SpawnInfo.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Gameplay/AI.hpp"
#include "Game/Gameplay/Player.hpp"
#include "Game/Gameplay/Weapon.hpp"


Actor::Actor(Map* pointerToMap, SpawnInfo const& spawnInfo) :
	m_map(pointerToMap),
	m_position(spawnInfo.m_position),
	m_orientation(spawnInfo.m_orientation),
	m_velocity(spawnInfo.m_velocity)
{
	m_definition = spawnInfo.m_definition;
	m_physicsHeight = m_definition->m_physicsHeight;
	m_physicsRadius = m_definition->m_physicsRadius;
	m_health = m_definition->m_health;

	if (m_definition->m_dieOnSpawn) {
		Die();
	}

	CreateWeapons();
	m_equippedWeaponIndex = 0;

	if (m_definition->m_visible && !m_isDead) {
		PlayAnimation(m_currentAnimation);
	}
}

void Actor::CreateWeapons()
{
	std::vector<WeaponDefinition const*>const& weaponDefs = m_definition->m_weaponDefinitions;

	for (int weaponDefIndex = 0; weaponDefIndex < weaponDefs.size(); weaponDefIndex++) {
		Weapon* newWeapon = new Weapon(weaponDefs[weaponDefIndex]);
		m_weapons.push_back(newWeapon);
	}
}

void Actor::PlayAnimation(ActorAnimationName animationName)
{
	bool isDifferentAnim = animationName != m_currentAnimation;
	std::string action = "";
	if (m_definition->m_is3DModel) {
		PlayAnimation3DModel(animationName);
		action = GetAnimationAsString(animationName);
	}
	else {
		if (!m_animationStopwatch.HasDurationElapsed()) {
			if (m_currentAnimation > animationName) return;
		}
		PlayAnimationBillboardSprite(animationName);
		action = GetCurrentAnimationAsString();
	}
	std::string soundSource = m_definition->GetSoundSourceForAction(action);

	if (soundSource.empty()) return;
	GAME_SOUND actionSoundID = GetSoundIDForSource(soundSource);
	if (actionSoundID == GAME_SOUND::UNDEFINED) return;
	Vec3 aLittleBitForward = m_orientation.GetXForward() * 0.1f;

	if (!isDifferentAnim) {
		if (m_lastTimePlayedSound.HasDurationElapsed()) {
			PlaySoundAt(actionSoundID, m_position + aLittleBitForward);
			m_lastTimePlayedSound.Start(&m_map->GetGame()->m_clock, 1.5f);
		}
	}
	else {
		PlaySoundAt(actionSoundID, m_position + aLittleBitForward);
		m_lastTimePlayedSound.Start(&m_map->GetGame()->m_clock, 1.5f);
	}
}

Vec3 const Actor::GetHealthAsArbitraryVector() const
{
	if (m_health > m_definition->m_health * 0.9f) {
		return Vec3(1.0f, 0.0f, 0.0f);
	}
	else if (m_health > m_definition->m_health * 0.7f) {
		return Vec3(1.0f, 1.0f, 0.0f);
	}
	else if (m_health > m_definition->m_health * 0.5f) {
		return Vec3(0.0f, 1.0f, 0.0f);
	}
	else if (m_health > m_definition->m_health * 0.3f) {
		return Vec3(-1.0f, 1.0f, 0.0f);
	}
	else {
		return Vec3(-1.0f, 0.0f, 0.0f);
	}
}

SpriteAnimGroupDefinition const* Actor::GetCurrentAnimationGroup() const
{
	std::string currentAnimationName = GetCurrentAnimationAsString();
	for (int animationGroupIndex = 0; animationGroupIndex < m_definition->m_spriteAnimationGroups.size(); animationGroupIndex++) {
		SpriteAnimGroupDefinition const* spriteAnimGroup = m_definition->m_spriteAnimationGroups[animationGroupIndex];
		if (spriteAnimGroup->GetName() == currentAnimationName) {
			return spriteAnimGroup;
		}
	}

	return nullptr;
}

std::string Actor::GetCurrentAnimationAsString() const
{
	switch (m_currentAnimation)
	{
	case ActorAnimationName::WALK:
		return "Walk";
		break;
	case ActorAnimationName::ATTACK:
		return "Attack";
		break;
	case ActorAnimationName::PAIN:
		return "Pain";
		break;
	case ActorAnimationName::DEATH:
		return "Death";
		break;

	}
	return "Unknown";
}

std::string Actor::GetAnimationAsString(ActorAnimationName animationName) const
{

	switch (animationName)
	{
	case ActorAnimationName::WALK:
		return "Walk";
		break;
	case ActorAnimationName::ATTACK:
		return "Attack";
		break;
	case ActorAnimationName::PAIN:
		return "Pain";
		break;
	case ActorAnimationName::DEATH:
		return "Death";
		break;

	}
	return "Unknown";

}

Actor::~Actor()
{
	for (int weaponIndex = 0; weaponIndex < m_weapons.size(); weaponIndex++) {
		Weapon*& weapon = m_weapons[weaponIndex];
		if (weapon) {
			delete weapon;
		}
	}
}

void Actor::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	if ((m_lifetimeStopwatch.m_duration != 0) && m_lifetimeStopwatch.HasDurationElapsed()) {
		m_isPendingDelete = true;
	}

	if (m_currentAnimScaledBySpeed) {
		float maxSpeed = m_definition->m_walkSpeed;
		float currentSpeed = m_velocity.GetLength();

		if (m_currentAnimation == ActorAnimationName::WALK) {
			m_animationClock.SetTimeDilation(currentSpeed / maxSpeed);
		}
		else {
			m_animationClock.SetTimeDilation(1.0f);
		}
	}


	if (!m_isDead && m_animationStopwatch.HasDurationElapsed()) {
		if (m_currentAnimation != ActorAnimationName::WALK) {
			PlayAnimation(ActorAnimationName::WALK);
		}
	}
	if (m_isDead && m_definition->m_is3DModel) {
		m_animationClock.SetTimeDilation(1.0f);
	}
	if (!m_definition->m_spawnCreatureName.empty()) {
		UpdateSpawning();
	}

	if (m_isDead) {
		m_countedAsKill = true;
	}

	if (m_currentAnimation == ActorAnimationName::WALK) {
		std::string soundSource = m_definition->GetSoundSourceForAction("Walk");
		Vec3 aLittleBitForward = m_orientation.GetXForward() * 0.1f;

		if (soundSource.empty()) return;
		GAME_SOUND actionSoundID = GetSoundIDForSource(soundSource);
		if (actionSoundID != GAME_SOUND::UNDEFINED) {
			if (m_lastTimePlayedSound.HasDurationElapsed()) {
				PlaySoundAt(actionSoundID, m_position + aLittleBitForward);
				m_lastTimePlayedSound.Start(&m_animationClock, 1.5f);
			}
		}
	}

}

void Actor::UpdatePhysics(float deltaSeconds)
{


	if (!m_definition->m_simulated) return;
	if (m_isDead) {
		UpdateGravityEffect();
		return;
	}

	float drag = m_definition->m_drag;

	if (!m_definition->m_flying) {
		AddForce(-m_velocity * drag);
	}

	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;

	if (!m_definition->m_flying) {
		m_position.z = 0.0f;
	}

	m_acceleration = Vec3::ZERO;


}

void Actor::Render(Camera const& camera) const
{
	if (m_definition->m_is3DModel) {
		Render3DModel(camera);
	}
	else {
		Render2DSprite(camera);
	}

}

void Actor::Render2DSprite(Camera const& camera) const
{
	if (m_definition->m_name == "PlasmaProjectile") {
		2 + 2;
	}

	if (!m_definition->m_visible) return;
	Player* controllerAsPlayer = dynamic_cast<Player*>(m_controller);
	if (controllerAsPlayer && !controllerAsPlayer->m_freeFlyCameraMode) {
		if (&camera == controllerAsPlayer->m_worldCamera) return;
	}

	g_theRenderer->SetModelMatrix(GetModelMatrix(camera));
	SpriteAnimGroupDefinition const* currentAnimGroup = GetCurrentAnimationGroup();

	

	if(currentAnimGroup) {
		Mat44 actorModelMat = m_orientation.GetMatrix_XFwd_YLeft_ZUp().GetOrthonormalInverse();
		Vec3 dispCameraToActor = m_position - camera.GetViewPosition();
		dispCameraToActor.z = 0.0f;
		dispCameraToActor.Normalize();

		// Converting the displacement to the actor local world
		Vec3 direction = actorModelMat.TransformVectorQuantity3D(dispCameraToActor);

		float elapsedTime = (float)m_animationStopwatch.GetElapsedTime();
		SpriteDefinition const* currentAnimSprite = currentAnimGroup->GetSpriteAtDefTime(elapsedTime, direction);
		if (!currentAnimSprite) {
			return;
		}

		Vec3 bottomLeftSprite = Vec3::ZERO;
		Vec3 bottomRightSprite = Vec3(0.0f, m_definition->m_appearanceSize.x, 0.0f);

		Vec3 topLeftSprite = Vec3(0.0f, 0.0f, m_definition->m_appearanceSize.y);
		Vec3 topRightSprite = Vec3(0.0f, m_definition->m_appearanceSize.x, m_definition->m_appearanceSize.y);

		bottomLeftSprite -= Vec3(0.0f, (m_definition->m_appearanceSize.x * m_definition->m_pivot.x), (m_definition->m_appearanceSize.y * m_definition->m_pivot.y));
		bottomRightSprite -= Vec3(0.0f, (m_definition->m_appearanceSize.x * m_definition->m_pivot.x), (m_definition->m_appearanceSize.y * m_definition->m_pivot.y));
		topLeftSprite -= Vec3(0.0f, (m_definition->m_appearanceSize.x * m_definition->m_pivot.x), (m_definition->m_appearanceSize.y * m_definition->m_pivot.y));
		topRightSprite -= Vec3(0.0f, (m_definition->m_appearanceSize.x * m_definition->m_pivot.x), (m_definition->m_appearanceSize.y * m_definition->m_pivot.y));


		std::vector<Vertex_PNCU> animVertsLit;
		animVertsLit.reserve(12);
		std::vector<Vertex_PCU> animVerts;
		animVertsLit.reserve(12);

		SpriteAnimGroupDefinition const* spriteAnimGroup = m_definition->m_spriteAnimationGroups.at(0);
		std::string shaderName = spriteAnimGroup->GetMatName();
		g_theRenderer->BindMaterialByPath(shaderName.c_str());

		Vec3 worldPivotPoint = m_position;
		worldPivotPoint.y -= m_definition->m_appearanceSize.x * m_definition->m_pivot.x;
		worldPivotPoint.z -= (m_definition->m_appearanceSize.y * m_definition->m_pivot.y);


		//DebugAddWorldPoint(m_position, m_physicsRadius, 0.0f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::USEDEPTH);

		//g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindTexture(&currentAnimSprite->GetTexture());
		if (m_definition->m_renderLit) {
			if (m_definition->m_renderRoundedNormals) {
				AddVertsForRoundedQuad3D(animVertsLit, bottomLeftSprite, bottomRightSprite, topRightSprite, topLeftSprite, Rgba8::WHITE, currentAnimSprite->GetUVs());
			}
			else {
				AddVertsForQuad3D(animVertsLit, bottomLeftSprite, bottomRightSprite, topRightSprite, topLeftSprite, Rgba8::WHITE, currentAnimSprite->GetUVs());
			}
			g_theRenderer->DrawVertexArray(animVertsLit);
		}
		else {
			AddVertsForQuad3D(animVerts, bottomLeftSprite, bottomRightSprite, topRightSprite, topLeftSprite, Rgba8::WHITE, currentAnimSprite->GetUVs());
			g_theRenderer->DrawVertexArray(animVerts);
		}

	}

	if (m_definition->m_renderDepth) {
		g_theRenderer->SetDepthStencilState(DepthFunc::LESSEQUAL, true);
	}
	else {
		g_theRenderer->SetDepthStencilState(DepthFunc::ALWAYS, false);
	}
}

void Actor::Render3DModel(Camera const& camera) const
{
	Mat44 modelMat = GetModelMatrix(camera);
	std::string currentAction = GetCurrentAnimationAsString();

	std::map<std::string, ModelAnimation>::const_iterator currentModelAnimPair = m_definition->m_modelAnimationByAction.find(currentAction);

	if (currentModelAnimPair == m_definition->m_modelAnimationByAction.end()) return;
	ModelAnimation const& currentModelAnim = currentModelAnimPair->second;

	float elapsedTime = (float)m_animationStopwatch.GetElapsedTime();
	float timeWithinAnimationDuration = fmodf(elapsedTime, (float)m_animationStopwatch.m_duration) / (float)m_animationStopwatch.m_duration;
	int amountOfFrames = currentModelAnim.m_amountOfFrames;

	int animIndex = RoundDownToInt(timeWithinAnimationDuration * amountOfFrames);
	if (animIndex == amountOfFrames) animIndex = 0;

	IndexBuffer* const& currentIndexBuffer = currentModelAnim.m_indexBuffers[animIndex];
	VertexBuffer* const& currentVertexBuffer = currentModelAnim.m_vertexBuffers[animIndex];

	g_theRenderer->BindMaterial(currentModelAnim.m_material);
	g_theRenderer->BindTexture(currentModelAnim.m_texture);
	g_theRenderer->SetModelMatrix(modelMat);
	//g_theRenderer->CopyAndBindModelConstants();
	g_theRenderer->DrawIndexedVertexBuffer(currentVertexBuffer, currentIndexBuffer, currentModelAnim.m_amountOfIndexes[animIndex]);

}

void Actor::PlayAnimationBillboardSprite(ActorAnimationName animationName)
{
	if (m_currentAnimation == animationName) {
		if (!m_animationStopwatch.HasDurationElapsed()) return;
	}
	if (m_isDead && animationName != ActorAnimationName::DEATH) return;



	m_currentAnimation = animationName;
	SpriteAnimGroupDefinition const* currentAnimGroup = GetCurrentAnimationGroup();

	if (!currentAnimGroup) return;

	float animationLengthTime = currentAnimGroup->GetTotalLengthTime();
	m_animationStopwatch.Start(&m_animationClock, animationLengthTime);
	m_currentAnimScaledBySpeed = currentAnimGroup->IsScaledBySpeed();
	m_animationClock.SetTimeDilation(1.0f);

}

void Actor::PlayAnimation3DModel(ActorAnimationName animationName)
{
	std::string currentAction = GetCurrentAnimationAsString();

	if (m_currentAnimation == animationName) {
		if (!m_animationStopwatch.HasDurationElapsed()) return;
	}
	if (m_isDead && animationName != ActorAnimationName::DEATH) return;
	std::map<std::string, ModelAnimation>::const_iterator currentModelAnimPair = m_definition->m_modelAnimationByAction.find(currentAction);

	if (currentModelAnimPair == m_definition->m_modelAnimationByAction.end()) return;
	ModelAnimation const& currentModelAnim = currentModelAnimPair->second;

	double animationDuration = (double)(currentModelAnim.m_secondsPerFrame * (float)currentModelAnim.m_amountOfFrames);
	m_animationStopwatch.Start(&m_animationClock, animationDuration);

	m_currentAnimScaledBySpeed = currentModelAnim.m_scaledBySpeed;
	m_animationClock.SetTimeDilation(1.0f);
}

Mat44 const Actor::GetModelMatrix(Camera const& camera) const
{

	Mat44 modelMat;
	Vec3 actorPivot = Vec3(0.0f, m_definition->m_pivot.y * m_definition->m_appearanceSize.y, m_definition->m_pivot.y * m_definition->m_appearanceSize.x);
	Vec3 iBasis = Vec3::ZERO;
	Vec3 jBasis = Vec3::ZERO;
	Vec3 kBasis = Vec3(0.0f, 0.0f, 1.0f);

	Vec3 actorForward = GetForward().GetNormalized();

	Vec3 const cameraViewPosition = camera.GetViewPosition();

	switch (m_definition->m_billboardType)
	{
	case ActorBillboardType::NONE:
		m_orientation.GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);
		break;
	case ActorBillboardType::FACING:
		iBasis = (cameraViewPosition - m_position).GetNormalized();
		iBasis.z = 0.0f;

		jBasis.x = -iBasis.y;
		jBasis.y = iBasis.x;
		break;
	default:
		iBasis = camera.GetViewOrientation().GetXForward().GetNormalized();
		iBasis *= -1.0f;

		jBasis.x = -iBasis.y;
		jBasis.y = iBasis.x;
		break;
	}

	modelMat.SetIJK3D(iBasis, jBasis, kBasis);
	modelMat.SetTranslation3D(m_position);
	//DebugAddWorldArrow(m_position, m_position + (actorForward * 0.25f), 0.15f, 0.0f, Rgba8::RED, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);
	//DebugAddWorldPoint(-actorPivot + m_position, 0.1f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);
	////DebugAddWorldPoint(m_position, 0.1f, 0.0f, Rgba8::LIGHTBLUE, Rgba8::LIGHTBLUE, DebugRenderMode::ALWAYS);
	//modelMat = m_orientation.GetMatrix_XFwd_YLeft_ZUp();
	//modelMat.SetTranslation3D(m_position);

	return modelMat;
}

Vec3 Actor::GetForward() const
{
	return m_orientation.GetXForward();
}

void Actor::HandleDamage(float damage)
{
	if (m_isInvincible) return;
	m_health -= damage;
	if (m_health < 0.0f) m_health = 0.0f;
	if (m_health <= 0.0f && !m_isDead) {
		Die();
	}
	else {
		if (damage > 0.0f) {
			PlayAnimation(ActorAnimationName::PAIN);
			m_gotHurt = true;
		}
	}
}

void Actor::Die()
{
	PlayAnimation(ActorAnimationName::DEATH);

	m_lifetimeStopwatch.Start(&m_map->GetGame()->m_clock, m_definition->m_corpseLifetime);
	m_isDead = true;
	m_velocity = Vec3::ZERO;
	if (m_definition->m_is3DModel) {
		m_orientation.m_pitchDegrees += 180.0f;
		m_position.z += m_physicsHeight * 0.8f;
	}
}

bool Actor::IsSimulated()
{
	return m_definition->m_simulated;
}

void Actor::AddForce(const Vec3& force)
{
	m_acceleration += force;
}

void Actor::AddImpulse(const Vec3& impulse)
{
	m_velocity += impulse;
}

void Actor::OnCollide(Actor* other)
{
	if (other) {
		Vec3 impulseDir = m_velocity;
		impulseDir.z = 0;
		impulseDir.Normalize();

		other->AddImpulse(impulseDir * m_definition->m_impulseOnCollide);
		float damageDealt = rng.GetRandomFloatInRange(m_definition->m_damageOnCollide.m_min, m_definition->m_damageOnCollide.m_max);
		other->HandleDamage(damageDealt);
		if (m_owner) {
			if (other->m_isDead) {
				m_owner->m_killCount++;
			}
		}
	}

	if (m_definition->m_dieOnCollide && !m_isDead) {
		Die();
	}

}

void Actor::OnPossessed(Controller* controller)
{

	m_prevController = m_controller;
	m_controller = controller;
}

void Actor::OnUnpossessed(Controller* controller)
{
	UNUSED(controller);
	m_controller = m_prevController;
	m_prevController = nullptr;

	if (dynamic_cast<AI*>(m_controller)) {
		m_orientation.m_pitchDegrees = 0.0f;
	}
}

void Actor::MoveInDirection(Vec3 direction, float speed)
{
	AddForce(direction.GetNormalized() * speed * m_definition->m_drag);
}

Weapon* Actor::GetEquippedWeapon()
{
	if (m_weapons.size() == 0) return nullptr;
	if (m_equippedWeaponIndex >= m_weapons.size()) return nullptr;
	return m_weapons[m_equippedWeaponIndex];
}

void Actor::EquipWeapon(int weaponIndex)
{
	m_equippedWeaponIndex = weaponIndex;
}

void Actor::EquipNextWeapon()
{
	m_equippedWeaponIndex++;
	if (m_equippedWeaponIndex >= m_weapons.size()) m_equippedWeaponIndex = 0;
}

void Actor::EquipPreviousWeapon()
{
	m_equippedWeaponIndex--;
	if (m_equippedWeaponIndex < 0) m_equippedWeaponIndex = (int)m_weapons.size() - 1;
}

void Actor::Attack()
{
	if (m_isDead) return;
	Weapon* equippedWeapon = GetEquippedWeapon();

	if (!equippedWeapon) return;

	Vec3 fwd = GetForward();

	equippedWeapon->Fire(m_position, fwd, this);
	PlayAnimation(ActorAnimationName::ATTACK);
}

void Actor::UpdateGravityEffect()
{
	float const& gravityRadius = this->m_definition->m_gravityRadius;
	std::vector<Actor*> const& affectedActors = m_map->GetActorsWithinRadius(this, gravityRadius);
	float gravity = RangeMapClamped((float)m_lifetimeStopwatch.GetElapsedTime(), 0.0f, m_definition->m_corpseLifetime, m_definition->m_gravity);
	float const& deathDamageInterval = m_definition->m_deathDamageInterval;
	float damageOnDeath = rng.GetRandomFloatInRange(m_definition->m_constantDamageOnDeath);
	float damageRadiusSqr = m_definition->m_deathDamageRadius * m_definition->m_deathDamageRadius;

	bool causeExtraDmg = false;
	if (m_isDead) {
		if ((float)m_lifetimeStopwatch.GetElapsedTime() - m_lastDamageOnDeath >= deathDamageInterval) {
			causeExtraDmg = true;
			m_lastDamageOnDeath = (float)m_lifetimeStopwatch.GetElapsedTime();
		}
	}

	for (int affectedActorIndex = 0; affectedActorIndex < affectedActors.size(); affectedActorIndex++) {
		Actor* affectedActor = affectedActors[affectedActorIndex];
		Vec3 dispToActor = (m_position - affectedActor->m_position);
		float distSqrToActor = dispToActor.GetLengthSquared();
		float gravityScale = RangeMapClamped(distSqrToActor, gravityRadius, 0.0f, 1.0f, 0.5f);
		dispToActor.SetLength(gravity * gravityScale);
		affectedActor->AddForce(dispToActor);

		if (causeExtraDmg && distSqrToActor <= damageRadiusSqr) {
			affectedActor->HandleDamage(damageOnDeath);
			if (affectedActor->m_isDead && !affectedActor->m_countedAsKill&& affectedActor->m_definition->m_faction != Faction::NEUTRAL) {
				m_owner->m_killCount++;
			}
		}
	}
}

void Actor::UpdateSpawning()
{
	if (m_spawnStopwatch.HasDurationElapsed()) {
		if(!m_map->CanEnemyBeSpawned()) return;
		SpawnInfo actorSpawnInfo = {};
		std::string const& creatureName = m_definition->m_spawnCreatureName;
		actorSpawnInfo.m_definition = ActorDefinition::GetByName(creatureName);
		actorSpawnInfo.m_position = m_position;
		actorSpawnInfo.m_orientation = m_orientation;

		m_map->SpawnActor(actorSpawnInfo);
		float randSpawnTimer = rng.GetRandomFloatInRange(m_definition->m_spawnTimeRange);
		m_spawnStopwatch.Start(&m_map->GetSpawnClock(), randSpawnTimer);
		m_map->m_enemiesSpawned++;
	}

}

void Actor::RenderCollisionCylinder() const
{
	Vec3 cylinderBase;
	Vec3 cylinderTop;
	cylinderTop.z += m_physicsHeight;

	EulerAngles orientationNoPitch = m_orientation;
	orientationNoPitch.m_pitchDegrees = 0.0f;
	Mat44 modelMatNoPitch = orientationNoPitch.GetMatrix_XFwd_YLeft_ZUp();
	modelMatNoPitch.SetTranslation3D(m_position);


	Vec3 beakStart = cylinderBase;
	beakStart.x += (m_definition->m_physicsRadius);
	beakStart.z = m_physicsHeight * 0.85f;

	Vec3 beakEnd = beakStart;
	beakEnd.x += m_physicsRadius * 0.45f;
	beakEnd.z = m_physicsHeight * 0.85f;

	std::vector<Vertex_PCU> cylinderVerts;

	Rgba8 cylinderColor = Rgba8::RED;
	if (m_definition->m_faction == Faction::MARINE) cylinderColor = Rgba8::GREEN;
	if (m_definition->m_faction == Faction::NEUTRAL) cylinderColor = Rgba8::WHITE;

	if (!m_definition->m_canBePossessed) cylinderColor = Rgba8::BLUE;

	if (m_isDead) {
		cylinderColor.r = static_cast<unsigned char>(cylinderColor.r * 0.8f);
		cylinderColor.g = static_cast<unsigned char>(cylinderColor.g * 0.8f);
		cylinderColor.b = static_cast<unsigned char>(cylinderColor.b * 0.8f);
		cylinderColor.a = static_cast<unsigned char>(cylinderColor.a * 0.8f);
	}

	AddVertsForCylinder(cylinderVerts, cylinderBase, cylinderTop, m_physicsRadius, 16, cylinderColor);
	AddVertsForCone3D(cylinderVerts, beakStart, beakEnd, m_physicsRadius * 0.25f, 16, cylinderColor);
	AddVertsForWireCone3D(cylinderVerts, beakStart, beakEnd, m_physicsRadius * 0.251f, 16, Rgba8::WHITE);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetModelMatrix(modelMatNoPitch);
	g_theRenderer->DrawVertexArray(cylinderVerts);


	DebugAddWorldWireCylinder(modelMatNoPitch.TransformPosition3D(cylinderBase), modelMatNoPitch.TransformPosition3D(cylinderTop), m_physicsRadius * 1.01f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
}

bool Actor::CanBePossessed() const
{
	return m_definition->m_canBePossessed;
}

float Actor::GetPhysicsRadius() const
{
	return m_definition->m_physicsRadius;
}

bool Actor::CanCollideWithWorld() const
{
	return m_definition->m_collidesWithWorld;
}

bool Actor::CanCollideWithActors() const
{
	return m_definition->m_collidesWithActors;
}
