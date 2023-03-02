#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Stopwatch.hpp"

class WeaponDefinition;
class Actor;

class Weapon
{
public:
	Weapon(WeaponDefinition const* definition );
	~Weapon();

	void Fire( const Vec3& position, const Vec3& forward, Actor* owner );

	WeaponDefinition const* m_definition = nullptr;
	Stopwatch m_refireStopwatch;
};
