#pragma once
#include "Game/Gameplay/ActorUID.hpp"

class Map;
class Actor;

class Controller
{
	friend class Actor;

public:
	Controller();
	virtual ~Controller();

	virtual void Update( float deltaSeconds );
	virtual void Possess( Actor* actor );
	Actor* GetActor() const;

protected:

	ActorUID m_actorUID = ActorUID::INVALID;
	Map* m_map = nullptr;
};

