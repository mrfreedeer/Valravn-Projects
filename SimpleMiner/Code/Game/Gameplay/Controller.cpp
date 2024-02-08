#include "Game/Gameplay/Controller.hpp"

Controller::Controller(Game* pointerToGame) :
	m_game(pointerToGame)
{
}

Controller::~Controller()
{
}

void Controller::Possess(Entity& entity)
{
	m_possessedEntity = &entity;
}

void Controller::Unpossess()
{
}
 

