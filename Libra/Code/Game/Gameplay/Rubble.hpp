#pragma once
#include "Game/Gameplay/Entity.hpp"



class Rubble : public Entity {
public:
	Rubble(Map* pointerToGame, Vec2 const& startingPosition, float orientation, EntityFaction faction, EntityType type);
	~Rubble();

	virtual void Render() const;
	virtual void Update(float deltaSeconds);
	virtual void RenderHealthBar() const override{}
private:
	Texture const* m_texture = nullptr;
	float m_size = 0.0f;
};