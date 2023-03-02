#include "Game/Gameplay/AI.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/Game.hpp"

AI::AI(Map* pointerToMap) :
	m_heatmap(pointerToMap->m_dimensions)
{
	m_map = pointerToMap;
}

AI::~AI()
{
}

void AI::Update(float deltaSeconds)
{
	Actor* possessedActor = GetActor();
	if (possessedActor) {
		Actor* closestEnemy = m_map->GetClosestVisibleEnemy(possessedActor);
		if (closestEnemy) {
			UpdateNormal(deltaSeconds, *closestEnemy);
			m_hasSightOfPlayer = true;
			m_hasReachedGoal = true;
			m_lastKnownPlayerPosition = closestEnemy->m_position;
		}
		else {
			if (possessedActor->m_definition->m_hasPathing) {
				UpdatePathing(deltaSeconds);
			}
		}
	}

	Controller::Update(deltaSeconds);
}

void AI::UpdateNormal(float deltaSeconds, Actor& closestEnemy)
{
	Actor* possessedActor = GetActor();

	if (possessedActor->m_isDead) return;
	Vec3 dirToEnemy = closestEnemy.m_position - possessedActor->m_position;

	float distSqrToEnemy = GetDistanceSquared3D(possessedActor->m_position, closestEnemy.m_position);
	float sumOfRadius = possessedActor->GetPhysicsRadius() + closestEnemy.GetPhysicsRadius();
	float sumOfRadiusSqrTol = (sumOfRadius * sumOfRadius) * 1.1f;
	if (distSqrToEnemy > (sumOfRadiusSqrTol)) {
		float approachSpeed = RangeMapClamped(distSqrToEnemy, 100.0f, sumOfRadiusSqrTol, possessedActor->m_definition->m_runSpeed, possessedActor->m_definition->m_walkSpeed);
		possessedActor->MoveInDirection(dirToEnemy, approachSpeed);
	}

	float const& meleeRange = possessedActor->m_definition->m_meleeRange;
	float meleeRangeWithPhysicsRadius = (possessedActor->GetPhysicsRadius() + meleeRange);

	if (distSqrToEnemy <= (meleeRangeWithPhysicsRadius * meleeRangeWithPhysicsRadius)) {
		if (m_meleeStopwatch.HasDurationElapsed()) {
			float damageDealt = rng.GetRandomFloatInRange(possessedActor->m_definition->m_meleeDamage);
			closestEnemy.HandleDamage(damageDealt);
			m_meleeStopwatch.Start(&m_map->GetGame()->m_clock, possessedActor->m_definition->m_meleeDelay);
			possessedActor->PlayAnimation(ActorAnimationName::ATTACK);

		}
	}

	float& yawDeg = possessedActor->m_orientation.m_yawDegrees;

	float newYawDeg = GetTurnedTowardDegrees(yawDeg, dirToEnemy.GetAngleAboutZDegrees(), possessedActor->m_definition->m_turnSpeed * deltaSeconds);
	yawDeg = newYawDeg;

}

void AI::UpdatePathing(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Actor* possessedActor = GetActor();
	if (possessedActor) {
		IntVec2 currentCoords = m_map->GetCoordsForPosition(possessedActor->m_position);
		if (m_hasReachedGoal) {
			IntVec2 randomGoal = IntVec2::ZERO; 

			if (m_hasSightOfPlayer) {
				randomGoal = m_map->GetCoordsForPosition(m_lastKnownPlayerPosition);
			}
			else {
				randomGoal = m_map->m_solidMap.GetRandomValue(1.0f);
			}

			m_map->PopulateDistanceField(m_heatmap, randomGoal, FLT_MAX);
			m_hasReachedGoal = false;
			m_pathPoints = m_heatmap.GeneratePathToCoords(currentCoords, randomGoal);
			m_goalPosition = m_map->GetPositionForTileCoords(randomGoal);
			IntVec2 nextWayPointCoords = m_pathPoints.at(m_pathPoints.size() - 1);
			m_nextWaypoint = m_map->GetPositionForTileCoords(nextWayPointCoords);
		}
		else {
			Vec3 dispToNextWayPoint = m_nextWaypoint - possessedActor->m_position;
			float& yawDeg = possessedActor->m_orientation.m_yawDegrees;

			float newYawDeg = GetTurnedTowardDegrees(yawDeg, dispToNextWayPoint.GetAngleAboutZDegrees(), possessedActor->m_definition->m_turnSpeed * deltaSeconds);
			yawDeg = newYawDeg;

			possessedActor->MoveInDirection(dispToNextWayPoint, possessedActor->m_definition->m_walkSpeed);

			if (IsPointInsideSphere(possessedActor->m_position, m_goalPosition, 0.5f)) {
				m_hasReachedGoal = true;
			}

			if (IsPointInsideSphere(possessedActor->m_position, m_nextWaypoint, 0.5f)) {
				if (!(m_pathPoints.size() == 1)) {
					m_pathPoints.pop_back();
				}
				IntVec2 nextWayPointCoords = m_pathPoints.at(m_pathPoints.size() - 1);
				m_nextWaypoint = m_map->GetPositionForTileCoords(nextWayPointCoords);
			}
		}
	}

	m_hasSightOfPlayer = false;
}
