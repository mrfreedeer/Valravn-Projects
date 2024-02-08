#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRendererSystem.hpp"
#include "Game/Gameplay/Weapon.hpp"
#include "Game/Gameplay/WeaponDefinition.hpp"
#include "Game/Gameplay/SpawnInfo.hpp"
#include "Game/Gameplay/Actor.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Framework/GameCommon.hpp"

Weapon::Weapon(const WeaponDefinition* definition) :
	m_definition(definition)
{
}

Weapon::~Weapon()
{
}

void Weapon::Fire(const Vec3& position, const Vec3& forward, Actor* owner)
{
	if (!m_refireStopwatch.HasDurationElapsed()) return;


	Vec3 positionALittleForward = position + (owner->GetForward() * 0.5f);
	if (m_definition->m_numProjectiles > 0) {
		ActorDefinition const* projectile = m_definition->m_projectileActorDefinition;

		for (int projectileIndex = 0; projectileIndex < m_definition->m_numProjectiles; projectileIndex++) {
			SpawnInfo spawnInfoProjectile = {};
			spawnInfoProjectile.m_definition = projectile;
			spawnInfoProjectile.m_position = positionALittleForward;
			spawnInfoProjectile.m_position.z = owner->m_definition->m_eyeHeight;

			Vec3 rndFwd = GetRandomDirectionInCone(forward, m_definition->m_projectileCone);
			rndFwd.Normalize();

			spawnInfoProjectile.m_orientation = owner->m_orientation;
			spawnInfoProjectile.m_velocity = rndFwd * m_definition->m_projectileSpeed;

			Actor* newProjectile = owner->GetMap()->SpawnActor(spawnInfoProjectile);
			newProjectile->m_owner = owner;
		}
	}
	else {
		RaycastFilter ignoreOwner = { owner };
		Vec3 raycastStartPos = owner->m_position;
		raycastStartPos.z = owner->m_definition->m_eyeHeight;

		for (int rayIndex = 0; rayIndex < m_definition->m_numRays; rayIndex++) {
			Vec3 rndFwd = GetRandomDirectionInCone(forward, m_definition->m_rayCone);


			RaycastResultDoomenstein closestImpact = owner->GetMap()->RaycastAll(raycastStartPos, rndFwd, m_definition->m_rayRange, ignoreOwner);

			if (closestImpact.m_didImpact && closestImpact.m_impactActor) {
				float damageDealt = rng.GetRandomFloatInRange(m_definition->m_rayDamage.m_min, m_definition->m_rayDamage.m_max);
				closestImpact.m_impactActor->HandleDamage(damageDealt);

				if (closestImpact.m_impactActor->m_isDead) owner->m_killCount++;

				Vec3 dispToActor = (closestImpact.m_impactPos - owner->m_position).GetNormalized();
				closestImpact.m_impactActor->AddForce(dispToActor * m_definition->m_rayImpulse);

				SpawnInfo spawnInfoBloodSplatter = {};
				spawnInfoBloodSplatter.m_definition = ActorDefinition::GetByName("BloodSplatter");
				spawnInfoBloodSplatter.m_position = closestImpact.m_impactPos;
				owner->GetMap()->SpawnActor(spawnInfoBloodSplatter);
				//DebugAddWorldPoint(closestImpact.m_impactPos, 0.05f, 0.25f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::USEDEPTH);
			}
			else {
				SpawnInfo spawnInfoBulletHit = {};
				spawnInfoBulletHit.m_definition = ActorDefinition::GetByName("BulletHit");
				spawnInfoBulletHit.m_position = closestImpact.m_impactPos;
				owner->GetMap()->SpawnActor(spawnInfoBulletHit);
				//DebugAddWorldPoint(closestImpact.m_impactPos, 0.1f, 1.0f, Rgba8::RED, Rgba8::RED, DebugRenderMode::USEDEPTH);
			}

		}

	}

	GAME_SOUND gameSoundID = GetSoundIDForSource(m_definition->m_fireSoundName);
	PlaySoundAt(gameSoundID, positionALittleForward, Vec3::ZERO);

	m_refireStopwatch.Start(m_definition->m_refireTime);
}

