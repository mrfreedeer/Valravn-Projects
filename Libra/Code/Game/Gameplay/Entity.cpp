#include "Engine/Math/RaycastUtils.hpp"
#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Bullet.hpp"
#include "Game/Gameplay/Map.hpp"


Entity::Entity(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor) :
	m_map(pointerToGame),
	m_position(startingPosition),
	m_orientationDegrees(orientation),
	m_faction(faction),
	m_type(type),
	m_isActor(isActor),
	m_isProjectile(!isActor),
	m_heatMap(m_map->GetDimensions()),
	m_solidMap(m_map->GetDimensions()),
	m_rayCastLength(g_gameConfigBlackboard.GetValue("MAX_RAYCAST_LENGTH", 10.0f)),
	m_reachedGoal(true)
{
	m_map->GetSolidMapForEntity(m_solidMap, m_canSwim);
}

void Entity::RenderDebug() const
{
	g_theRenderer->BindTexture(nullptr);
	DebugDrawRing(m_position, m_physicsRadius, m_debugLineThickness, Rgba8::CYAN);
	DebugDrawRing(m_position, m_cosmeticsRadius, m_debugLineThickness, Rgba8::MAGENTA);

	Vec2 rightDir = GetForwardNormal();
	Vec2 leftDir = GetForwardNormal().GetRotated90Degrees();

	rightDir *= m_physicsRadius;
	leftDir *= m_physicsRadius;

	rightDir += m_position;
	leftDir += m_position;

	DebugDrawLine(m_position, rightDir, m_debugLineThickness, Rgba8::RED);
	DebugDrawLine(m_position, leftDir, m_debugLineThickness, Rgba8::GREEN);

}

Vec2 const Entity::GetForwardNormal() const
{
	Vec2 rightDir(1.0f, 0.0f);
	return rightDir.GetRotatedDegrees(m_orientationDegrees);
}

Entity::~Entity()
{
}

void Entity::Update(float deltaSeconds)
{
	m_position += m_velocity * deltaSeconds * (1.0f - m_rubbleSpeedPenalty);
	m_rubbleSpeedPenalty = 0.0f;
}

void Entity::UpdateOrientationWithHeatMap(float deltaSeconds)
{
	bool prevChasingPlayer = m_chasingPlayer;

	if (m_map->m_areHeatMapsDirty) {
		RecalculateHeatMap();
	}

	if (m_pathPoints.size() >= 2) {
		UpdatePathToGoal();
	}

	Entity* player = m_map->GetNearestEntityOfType(m_position, EntityType::PLAYER);
	float newOrientation = 0.0f;

	m_hasSightOfPlayer = false;
	if (m_map->IsAlive(player)) {
		m_hasSightOfPlayer = m_map->HasLineOfSight(m_position, player->m_position, ARBITRARILY_LARGE_VALUE);
	}

	m_wander = true;
	if (m_hasSightOfPlayer) {
		UpdateChasingGoalEntity(player); // m_wander could be changed within this function
	}

	if (m_wander) {
		if (m_reachedGoal) {
			SetNewGoal();
			IntVec2 nextWayPointCoords = m_pathPoints.at(m_pathPoints.size() - 1);
			m_nextWayPoint = m_map->GetPositionForTileCoords(nextWayPointCoords);
		}

	}

	Vec2 dispToNewWayPoint = m_nextWayPoint - m_position;
	newOrientation = GetTurnedTowardDegrees(m_orientationDegrees, dispToNewWayPoint.GetOrientationDegrees(), m_turnSpeed * deltaSeconds);

	m_orientationDegrees = newOrientation;

	// Check if goal was reached
	if (IsPointInsideDisc2D(m_goalPosition, m_position, m_physicsRadius)) {
		if (!m_hasSightOfPlayer) {
			m_chasingPlayer = false;
			m_wander = true;
		}
		m_reachedGoal = true;
		m_goalPosition = Vec2(-1.0f, -1.0f);
	}

	// Check if next way point was reached
	if (IsPointInsideDisc2D(m_position, m_nextWayPoint, 1.0f)) {
		if (!m_pathPoints.size() == 1) {
			m_pathPoints.pop_back();
		}
		IntVec2 nextWayPointCoords = m_pathPoints.at(m_pathPoints.size() - 1);
		m_nextWayPoint = m_map->GetPositionForTileCoords(nextWayPointCoords);
	}

	if (!prevChasingPlayer && prevChasingPlayer != m_chasingPlayer) {
		m_map->PlayDiscoveredSound(GetSoundBalanceToPlayer());
	}

}

void Entity::UpdateChasingGoalEntity(Entity const* goalEntity)
{

	m_wander = false;
	m_chasingPlayer = true;
	IntVec2 lastKnownPlayerCoords = m_map->GetTileCoordsForPosition(m_goalPosition);
	IntVec2 playerCoords = m_map->GetTileCoordsForPosition(goalEntity->m_position);
	IntVec2 currentCoords = m_map->GetTileCoordsForPosition(m_position);


	if (lastKnownPlayerCoords != playerCoords) {
		m_map->GetHeatMapForEntity(m_heatMap, playerCoords, m_canSwim);

		float positionHeatMapValue = m_heatMap.GetValue(currentCoords);
		if (positionHeatMapValue >= ARBITRARILY_LARGE_VALUE) return;

		m_pathPoints = m_heatMap.GeneratePathToCoords(currentCoords, playerCoords);
		IntVec2 nextLowestCoords = m_pathPoints[(int)m_pathPoints.size() - 1];
		m_nextWayPoint = m_map->GetPositionForTileCoords(nextLowestCoords);
		m_goalPosition = goalEntity->m_position;
	}
}

void Entity::SetNewGoal()
{
	bool isNewGoalReachable = false;
	IntVec2 currentCoords = m_map->GetTileCoordsForPosition(m_position);
	while (!isNewGoalReachable) {
		IntVec2 nextCoords = m_solidMap.GetRandomValue(1.0f);
		m_goalPosition = m_map->GetPositionForTileCoords(nextCoords);
		m_map->GetHeatMapForEntity(m_heatMap, nextCoords, m_canSwim);
		m_reachedGoal = false;

		float valueForNewGoal = m_heatMap.GetValue(currentCoords);
		isNewGoalReachable = !(valueForNewGoal == ARBITRARILY_LARGE_VALUE);
	}
	IntVec2 goalCoords = m_map->GetTileCoordsForPosition(m_goalPosition);

	m_pathPoints = m_heatMap.GeneratePathToCoords(currentCoords, goalCoords);
}

void Entity::UpdateActionWithHeatMap(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	Vec2 fwd = GetForwardNormal();
	Vec2 dispToNextWayPoint = m_nextWayPoint - m_position;
	m_move = false;

	if (m_wander || GetAngleDegreesBetweenVectors2D(fwd, dispToNextWayPoint) <= m_turnHalfAperture) {
		m_move = true;
	}

	if (m_move) {
		m_velocity = fwd * m_speed;
	}
	else {
		m_velocity = Vec2::ZERO;
	}
}

void Entity::RenderHealthBar() const
{

	static float const s_healthBarHeight = g_gameConfigBlackboard.GetValue("HEALTH_BAR_HEIGHT", 0.1f);
	static float const s_healthBarLength = g_gameConfigBlackboard.GetValue("HEALTH_BAR_LENGTH", 1.0f);
	Vec2 renderPos = (Vec2(0.0f, 1.0f) * m_cosmeticsRadius) + m_position;
	AABB2 backgroundRed(Vec2::ZERO, Vec2(s_healthBarLength, s_healthBarHeight));

	float fillHealthLength = RangeMapClamped(static_cast<float>(m_health), 0.0f, static_cast<float>(m_maxHealth), 0.0f, s_healthBarLength);

	AABB2 fillHealthBox(Vec2::ZERO, Vec2(fillHealthLength, s_healthBarHeight));

	backgroundRed.SetCenter(renderPos);
	fillHealthBox.SetCenter(renderPos);

	backgroundRed.AlignABB2WithinBounds(fillHealthBox, Vec2(0.0f, 0.0f));

	g_theRenderer->BindTexture(nullptr);
	std::vector<Vertex_PCU> healthBarVerts;
	AddVertsForAABB2D(healthBarVerts, backgroundRed, Rgba8::RED);
	AddVertsForAABB2D(healthBarVerts, fillHealthBox, Rgba8::GREEN);

	g_theRenderer->DrawVertexArray(healthBarVerts);

}

void Entity::Die()
{
	m_isGarbage = true;
	m_isAlive = false;

	bool isBulletOrExplosion = (m_type == EntityType::BULLET) || (m_type == EntityType::FLAMETHROWER_BULLET) || (m_type == EntityType::BOLT) || (m_type == EntityType::EXPLOSION);

	if (!isBulletOrExplosion) {

		float soundBalanceToPlayer = GetSoundBalanceToPlayer();

		PlaySound(GAME_SOUND::ENEMY_DIED, 1.0f, false, soundBalanceToPlayer);

		if (m_type != EntityType::PLAYER) {
			m_map->SpawnNewEntity(EntityType::ENEMYRUBBLE, EntityFaction::NEUTRAL, m_position, m_orientationDegrees);
		}
	}

	if (m_type != EntityType::EXPLOSION && m_type != EntityType::FLAMETHROWER_BULLET) {
		bool isAnyKindOfBullet = (m_type == EntityType::BULLET) || (m_type == EntityType::BOLT);

		EntityFaction usedFaction;

		if (isAnyKindOfBullet) {
			usedFaction = EntityFaction::NEUTRAL;
		}
		else {
			usedFaction = m_faction;
		}

		int amountOfExplosions = GetAmountOfExplosions();

		for (int explosionIndex = 0; explosionIndex < amountOfExplosions; explosionIndex++) {
			m_map->SpawnNewEntity(EntityType::EXPLOSION, usedFaction, m_position, m_orientationDegrees);
		}
		
	}

}

void Entity::TakeDamage()
{
	if (!m_isAlive) return;
	m_health--;
	if (m_health <= 0) {
		Die();
	}
}

void Entity::ReactToBullet(Bullet*& bullet)
{
	bullet->Die();
	if (m_type == EntityType::PLAYER) {
		PlaySound(GAME_SOUND::PLAYER_HIT);
	}
	else {
		float soundBalanceToPlayer = GetSoundBalanceToPlayer();
		PlaySound(GAME_SOUND::ENEMY_HIT, 1.0f, false, soundBalanceToPlayer);
	}
	TakeDamage();
}

void Entity::RubbleSlowDown()
{
	if (m_rubbleSpeedPenalty < 0.5f) {
		m_rubbleSpeedPenalty += 0.1f;
	}
}

bool Entity::IsGoalOnNeighbourTile() const
{
	IntVec2 currentCoords = m_map->GetTileCoordsForPosition(m_position);
	IntVec2 goalCoords = m_map->GetTileCoordsForPosition(m_goalPosition);
	IntVec2 dispToGoal = goalCoords - currentCoords;

	if ((abs(dispToGoal.x) <= 1 && abs(dispToGoal.y) <= 1)) {
		return true;
	}
	return false;
}

void Entity::UpdatePathToGoal()
{
	IntVec2 const& secondNextWayPoint = m_pathPoints[(int)m_pathPoints.size() - 2];
	Vec2 secondNextWayPointPos = m_map->GetPositionForTileCoords(secondNextWayPoint);

	Vec2 fwdToSecondNextWayPoint = (secondNextWayPointPos - m_position).GetNormalized();
	float distanceToSecondNextWaypoint = GetDistance2D(m_position, secondNextWayPointPos);


	bool doesCircleFitToNextWaypoint = m_map->DoesCircleFitToPosition(m_position, fwdToSecondNextWayPoint, distanceToSecondNextWaypoint + 0.1f, m_physicsRadius, !m_canSwim);

	if (doesCircleFitToNextWaypoint) {
		m_pathPoints.pop_back();
		IntVec2 nextWayPointCoords = m_pathPoints[(int)m_pathPoints.size() - 1];
		m_nextWayPoint = m_map->GetPositionForTileCoords(nextWayPointCoords);
	}

}

int Entity::GetAmountOfExplosions() const
{
	static int amountExplosionsGood = g_gameConfigBlackboard.GetValue("EXPLOSION_AMOUNT_GOOD", 10);
	static int amountExplosionsNeutral = g_gameConfigBlackboard.GetValue("EXPLOSION_AMOUNT_NEUTRAL", 2);
	static int amountExplosionsEvil = g_gameConfigBlackboard.GetValue("EXPLOSION_AMOUNT_EVIL", 5);

	switch (m_faction)
	{
	case EntityFaction::GOOD:
		return amountExplosionsGood;
		break;
	case EntityFaction::EVIL:
		return amountExplosionsEvil;
		break;
	default:
		return amountExplosionsNeutral;
		break;
	}
}

void Entity::RecalculateHeatMap()
{
	IntVec2 goalCoords = m_map->GetTileCoordsForPosition(m_goalPosition);
	m_map->GetHeatMapForEntity(m_heatMap, goalCoords, m_canSwim);
}

float Entity::GetSoundBalanceToPlayer() const
{
	Entity* nearestPlayer = m_map->GetNearestEntityOfType(m_position, EntityType::PLAYER);
	float soundBalanceToPlayer = 0.0f;

	if (nearestPlayer) {
		Vec2 dispFromPlayerToEnemy = m_position - nearestPlayer->m_position;
		Vec2 playerForward = nearestPlayer->GetForwardNormal();

		soundBalanceToPlayer = DotProduct2D(playerForward, dispFromPlayerToEnemy);
	}

	return soundBalanceToPlayer;
}
