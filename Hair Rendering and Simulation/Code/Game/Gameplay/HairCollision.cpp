#include "Game/Gameplay/HairCollision.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Math/Vec3.hpp"
#include <vector>

constexpr int MAX_COLLISION_OBJECTS = 30;


struct CollisionObject {
	CollisionObject() = default;
	CollisionObject(Vec3 const position, float radius) :
		Position(position),
		Radius(radius) {}
	Vec3 Position;
	float Radius;
};

struct CollisionConstants {
	CollisionObject CollisionObjects[MAX_COLLISION_OBJECTS];
};

class CollisionHandler {
public:
	CollisionHandler() {
	s_collisionObjects.reserve(MAX_COLLISION_OBJECTS); 
	m_collisionsBuffer = new ConstantBuffer(g_theRenderer->m_device, sizeof(CollisionConstants));
	}

	~CollisionHandler() {
		delete m_collisionsBuffer;
	}

	void AddCollisionSphere(Vec3 const& position, float radius) {
		s_collisionObjects.emplace_back(position, radius);
	}

	ConstantBuffer* GetCollisionBuffer() const {
		CollisionConstants collisionConstants = {};

		s_collisionObjects.resize(MAX_COLLISION_OBJECTS);
		
		memcpy(&collisionConstants.CollisionObjects, &s_collisionObjects, sizeof(CollisionObject) * s_collisionObjects.size());
		g_theRenderer->CopyCPUToGPU(s_collisionObjects.data(), sizeof(collisionConstants), m_collisionsBuffer);
		
		return m_collisionsBuffer;
	}

	ConstantBuffer* m_collisionsBuffer = nullptr;
	static std::vector<CollisionObject> s_collisionObjects;
	
};

std::vector<CollisionObject> CollisionHandler::s_collisionObjects;

static CollisionHandler* s_collisionHandler = nullptr;

void AddHairCollisionSphere(Vec3 const& position, float radius)
{
	s_collisionHandler->AddCollisionSphere(position, radius);
}

ConstantBuffer* GetHairCollisionBuffer()
{
	return s_collisionHandler->GetCollisionBuffer();
}

void StartupHairCollision()
{
	s_collisionHandler = new CollisionHandler();
}

void ShutdownHairCollision()
{
	s_collisionHandler->s_collisionObjects.clear();
	delete s_collisionHandler;
	s_collisionHandler = nullptr;
}

void ClearCollisionObjects()
{
	s_collisionHandler->s_collisionObjects.clear();
}

