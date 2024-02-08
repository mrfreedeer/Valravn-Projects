#pragma once
class Game;
class Entity;

class Controller {
public:
	Controller(Game* pointerToGame);
	virtual ~Controller();

	virtual void Update(float deltaSeconds) = 0;

	virtual void Possess(Entity& entity);
	virtual void Unpossess();

protected:
	Game* m_game = nullptr;
	Entity* m_possessedEntity = nullptr;
};