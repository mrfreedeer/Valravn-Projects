#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <vector>

class Game;

class Entity {
public:
	Entity(Game* pointerToGame, Vec3 const& startingWorldPosition);
	virtual ~Entity();

	virtual void Update(float deltaTime);
	virtual void Render() const = 0;
	
	virtual Mat44 GetModelMatrix() const;

public:
	Vec3 m_position = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	EulerAngles m_orientation = {};
	EulerAngles m_angularVelocity = {};

protected:
	std::vector<Vertex_PCU> m_verts;
	Game* m_game = nullptr;
	Rgba8 m_modelColor = Rgba8::WHITE;
};

typedef std::vector<Entity*> EntityList;