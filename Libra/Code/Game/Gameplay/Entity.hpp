#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/HeatMaps.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Entity.hpp"

struct AABB2;
class Map;
class Bullet;
struct Vec2;

extern bool g_drawDebug;

enum class EntityFaction {
	GOOD,
	NEUTRAL,
	EVIL,
	NUM_FACTIONS
};

enum class EntityType {
	UNDEFINED = -1,
	WALLRUBBLE,
	ENEMYRUBBLE,
	ARIES,
	CAPRICORN,
	LEO,
	SCORPIO,
	PLAYER,
	BULLET,
	BOLT,
	EXPLOSION,
	FLAMETHROWER_BULLET,
	NUM_ENTITIES
};


class Entity {
protected:
	Entity(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type, bool isActor = true);

	virtual void RenderDebug() const;
	virtual float GetSoundBalanceToPlayer() const;

public:
	virtual ~Entity();

	virtual Vec2 const GetForwardNormal() const;
	virtual void Update(float deltaSeconds);
	virtual void UpdateOrientationWithHeatMap(float deltaSeconds);
	virtual void UpdateActionWithHeatMap(float deltaSeconds);
	virtual void Render() const = 0;
	virtual void RenderHealthBar() const;
	virtual void Die();
	virtual void TakeDamage();
	virtual bool IsActor() const { return m_isActor; }
	virtual bool IsAlive() const { return m_isAlive; }
	virtual bool IsProjectile() const { return m_isProjectile; }
	virtual void ReactToBullet(Bullet*& bullet);
	virtual void RubbleSlowDown();

private:
	virtual bool IsGoalOnNeighbourTile() const;
	virtual void UpdatePathToGoal();
	virtual void SetNewGoal();
	virtual void UpdateChasingGoalEntity(Entity const* goalEntity);
	virtual int GetAmountOfExplosions() const;
	virtual void RecalculateHeatMap();

public:
	Map* m_map = nullptr;
	Vec2 m_position;
	Vec2 m_velocity;
	Vec2 m_goalPosition = Vec2(-1.0f, -1.0f);
	Vec2 m_nextWayPoint = Vec2(-1.0f, -1.0f);

	float m_speed = 0.0f;
	float m_rubbleSpeedPenalty = 0.0f;
	float m_turnSpeed = 45.0f;
	float m_turnHalfAperture = 0.0f;

	float m_physicsRadius = 0.0f;
	float m_cosmeticsRadius = 0.0f;
	float m_orientationDegrees = 0.0f;
	float m_randomGoalOrientation = 0.0f;
	float m_rayCastLength = 0.0f;

	int m_health = 0;
	int m_maxHealth = 0;

	bool m_isGarbage = false;
	bool m_isAlive = true;
	bool m_isPushedByWalls = true;
	bool m_isPushedByEntities = true;
	bool m_pushesEntities = true;
	bool m_isHitByBullets = true;
	bool m_canSwim = false;
	bool m_reachedGoal = false;

	bool m_isActor = true;
	bool m_isProjectile = !m_isActor;
	bool m_hasSightOfPlayer = false;
	bool m_chasingPlayer = false;
	bool m_wander = false;
	bool m_move = false;
	EntityFaction m_faction = EntityFaction::NEUTRAL;
	EntityType m_type = EntityType::UNDEFINED;
	float m_debugLineThickness = g_gameConfigBlackboard.GetValue("DEBUG_LINE_THICKNESS", 0.02f);

	TileHeatMap m_heatMap;
	TileHeatMap m_solidMap;
	std::vector<IntVec2> m_pathPoints;

};
typedef std::vector<Entity*> EntityList;
