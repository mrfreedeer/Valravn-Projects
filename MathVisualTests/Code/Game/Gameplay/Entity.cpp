#include "Game/Gameplay/Entity.hpp"
#include "Game/Gameplay/Game.hpp"

Entity::Entity(Game* pointerToGame, Vec3 const& startingWorldPosition):
	m_game(pointerToGame),
	m_position(startingWorldPosition)
{
}

Entity::~Entity()
{
	m_game = nullptr;
}

void Entity::Update(float deltaTime)
{
	m_position += m_velocity * deltaTime;
	m_orientation += m_angularVelocity * deltaTime;
}

Mat44 Entity::GetModelMatrix() const
{
	Mat44 model = m_orientation.GetMatrix_XFwd_YLeft_ZUp();
	model.SetTranslation3D(m_position);
	return model;
}

