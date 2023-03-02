#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

class ActorDefinition;

class WeaponDefinition
{
public:
	bool LoadFromXmlElement( const XMLElement& element );

	std::string m_name;
	float m_refireTime = 0.5f;

	int m_numRays = 0;
	float m_rayCone = 0.0f;
	float m_rayRange = 40.0f;
	FloatRange m_rayDamage = FloatRange( 0.0f, 1.0f );
	float m_rayImpulse = 4.0f;

	int m_numProjectiles = 0;
	const ActorDefinition* m_projectileActorDefinition = nullptr;
	float m_projectileCone = 0.0f;
	float m_projectileSpeed = 40.0f;

	SpriteAnimDefinition* m_idleAnimationDefinition;
	SpriteAnimDefinition* m_attackAnimationDefinition;

	Shader* m_shader = nullptr;
	Texture* m_hudBaseTexture = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;
	Vec2 m_spriteSize = Vec2(1.0f, 1.0f);
	Vec2 m_spritePivot = Vec2(0.5f, 0.0f);
	Texture* m_reticleTexture = nullptr;
	Vec2 m_reticleSize = Vec2(1.0f, 1.0f);

	std::string m_fireSoundName;

	static void InitializeDefinitions( const char* path );
	static void DestroyDefinitions();
	static const WeaponDefinition* GetByName( const std::string& name );
	static std::vector<WeaponDefinition*> s_definitions;
};

