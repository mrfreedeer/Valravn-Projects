#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/World.hpp"
#include "Game/Gameplay/Game.hpp"

Entity::Entity(Game* pointerToGame, Vec3 const& startingPosition, Vec3 const& startingVelocity, EulerAngles const& startingOrientation) :
	m_game(pointerToGame),
	m_position(startingPosition),
	m_velocity(startingVelocity),
	m_orientation(startingOrientation)
{
}

Entity::~Entity()
{
}

void Entity::MoveInDirection(Vec3 const& direction, float speed)
{
	AddForce(direction * speed * m_usedDrag);
}

void Entity::AddForce(Vec3 const& force)
{
	m_acceleration += force;
}

void Entity::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

void Entity::Jump()
{
	if (m_pushedFromBottom) {
		AddImpulse(Vec3(0.0f, 0.0f, m_gravity * 0.5f));
	}
}

void Entity::Update(float deltaSeconds)
{

	Vec3 dragVel = -m_velocity;

	if (m_movementMode == MovementMode::WALKING) {
		AddForce(Vec3(0.0f, 0.0f, -m_gravity));
		dragVel.z = 0.0f;
	}
	AddForce(dragVel * m_usedDrag);

	m_velocity += m_acceleration * deltaSeconds;

	if (m_movementMode != MovementMode::WALKING) {// Clamp Vertical so when using Q/E, there's no flying forever
		m_velocity.z = Clamp(m_velocity.z, -100.0f, 100.0f);
	}

	m_orientation += m_angularVelocity * deltaSeconds;

	PreventativePhysics(deltaSeconds);


	m_position += m_velocity * deltaSeconds;

	m_acceleration = Vec3::ZERO;

	if (m_pushedFromBottom) {
		m_usedDrag = m_groundDrag;
	}
	else {
		m_usedDrag = m_airDrag;
	}

}

void Entity::PreventativePhysics(float deltaSeconds)
{
	if (!m_isPreventativeEnabled || (m_movementMode == MovementMode::NOCLIP)) return;
	float usedTime = 0.0f;

	int raycastInsideBlockCount = 0; // Potentially, you could be inside a block and do a raycast algorithm 3 times
	while ((usedTime < deltaSeconds) && (m_velocity.GetLengthSquared() > 0.0f)) {
		SimpleMinerRaycast closestRaycastHit = GetClosestRaycastHit(deltaSeconds);
		if (closestRaycastHit.m_impactDist == 0)raycastInsideBlockCount++;

		if (closestRaycastHit.m_didImpact) {
			m_position += m_velocity * closestRaycastHit.m_impactFraction;
			usedTime += (deltaSeconds - usedTime) * closestRaycastHit.m_impactFraction;
			if (closestRaycastHit.m_impactNormal.x != 0.0f) {
				m_velocity.x = 0.0f;
			}
			if (closestRaycastHit.m_impactNormal.y != 0.0f) {
				m_velocity.y = 0.0f;
			}
			if (closestRaycastHit.m_impactNormal.z != 0.0f) {
				m_velocity.z = 0.0f;
				if (closestRaycastHit.m_impactNormal.z > 0.0f) {
					m_pushedFromBottom = true;
				}
			}

		}
		if (!closestRaycastHit.m_didImpact || raycastInsideBlockCount > 3)
		{
			usedTime = deltaSeconds;
		}



	}
}

void Entity::Render()
{
	if (!m_renderMesh) return;
	AABB3 debugAABB3 = GetBounds();

	std::vector<Vertex_PCU> verts;
	AddVertsForWireAABB3D(verts, debugAABB3, Rgba8::CYAN);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

void Entity::SetPhysicsMode(MovementMode newMode)
{
	m_movementMode = newMode;
}

void Entity::SetNextPhysicsMode()
{
	int newModeNum = ((int)m_movementMode + 1) % (int)MovementMode::NUM_MOVEMENT_MODES;
	m_movementMode = (MovementMode)newModeNum;
}

std::string Entity::GetPhysicsModeAsString() const
{
	switch (m_movementMode)
	{
	case MovementMode::WALKING:
		return "Walking";
		break;
	case MovementMode::FLYING:
		return "Flying";
		break;
	case MovementMode::NOCLIP:
		return "No Clip";
		break;
	default:
		break;
	}
	return std::string("Unknown");
}

AABB3 const Entity::GetBounds() const
{
	Vec3 min = m_position - (Vec3(m_width, m_width, m_height) * 0.5f);
	Vec3 max = m_position + (Vec3(m_width, m_width, m_height) * 0.5f);
	AABB3 bounds(min, max);
	return bounds;
}

SimpleMinerRaycast Entity::GetClosestRaycastHit(float deltaSeconds) const
{
	AABB3 physicsAABB3 = GetBounds();

	Vec3 corners[12];
	physicsAABB3.GetCorners(corners); // Counterclockwise front then back, starting and min

	corners[8] = corners[0];
	corners[8].z = m_position.z;

	corners[9] = corners[1];
	corners[9].z = m_position.z;

	corners[10] = corners[4];
	corners[10].z = m_position.z;

	corners[11] = corners[5];
	corners[11].z = m_position.z;

	SimpleMinerRaycast closestRaycast;
	closestRaycast.m_impactDist = FLT_MAX;

	float closesDotProd = -1.0f;
	World* world = m_game->GetWorld();


	Vec3 fwdNormal = m_velocity.GetNormalized();
	float length = (m_velocity * deltaSeconds).GetLength();

	for (int raycastInd = 0; raycastInd < 12; raycastInd++) {
		Vec3 const& raystart = corners[raycastInd];

		SimpleMinerRaycast cornerRaycast = world->RaycastVsTiles(raystart, fwdNormal, length);

		if (cornerRaycast.m_didImpact) {
			float dotProdWithVel = DotProduct3D(-cornerRaycast.m_impactNormal, fwdNormal);
			if ((cornerRaycast.m_impactDist < closestRaycast.m_impactDist) && (dotProdWithVel > closesDotProd)) {
				closestRaycast = cornerRaycast;
				closesDotProd = dotProdWithVel;
			}
		}
	}

	return closestRaycast;
}
