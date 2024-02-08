#include "Game/Gameplay/Controller.hpp"
#include "Game/Gameplay/Map.hpp"
#include "Game/Gameplay/Actor.hpp"

Controller::Controller()
{
}

Controller::~Controller()
{
}

void Controller::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
}

void Controller::Possess(Actor* actor)
{
	if (actor && actor->CanBePossessed()) {
		m_actorUID = actor->m_uid;
		actor->OnPossessed(this);
	}
}

Actor* Controller::GetActor() const
{
	return m_map->GetActorByUID(m_actorUID);
}
