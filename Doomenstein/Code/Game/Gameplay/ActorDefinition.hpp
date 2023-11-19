#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Game/Gameplay/Actor.hpp"
#include <string>
#include <map>

class WeaponDefinition;
class VertexBuffer;
class IndexBuffer;

enum class ActorBillboardType {
	NONE,
	FACING,
	ALIGNED
};

struct ModelAnimation {
	std::vector<VertexBuffer*> m_vertexBuffers;
	std::vector<IndexBuffer*> m_indexBuffers;
	std::vector<int> m_amountOfIndexes;
	int m_amountOfFrames = 0;
	float m_secondsPerFrame = 0.0f;
	bool m_scaledBySpeed = false;
	Texture* m_texture = nullptr;
	Material* m_material = nullptr;
};

//------------------------------------------------------------------------------------------------
class ActorDefinition
{
public:
	~ActorDefinition();
	bool LoadFromXmlElement( const XMLElement& element );
	std::string const GetSoundSourceForAction(std::string action) const;

	std::string m_name;
	std::vector<const WeaponDefinition*> m_weaponDefinitions;
	bool m_visible = true;
	bool m_dieOnSpawn = false;
	bool m_is3DModel = false;

	float m_physicsRadius = -1.0f;
	float m_physicsHeight = -1.0f;
	float m_walkSpeed = 0.0f;
	float m_runSpeed = 0.0f;
	float m_drag = 0.0f;
	float m_turnSpeed = 0.0f;
	FloatRange m_gravity = FloatRange::ZERO;
	float m_gravityRadius = 0.0f;
	float m_deathDamageInterval = 0.0f;
	FloatRange m_constantDamageOnDeath = FloatRange::ZERO;
	float m_deathDamageRadius = 0.0f;

	bool m_flying = false;
	bool m_simulated = false;
	bool m_collidesWithWorld = false;
	bool m_collidesWithActors = false;

	bool m_canBePossessed = false;
	float m_eyeHeight = 1.75f;
	float m_cameraFOVDegrees = 60.0f;

	bool m_aiEnabled = false;
	float m_sightRadius = 64.0f;
	float m_sightAngle = 180.0f;
	FloatRange m_meleeDamage = FloatRange();
	float m_meleeDelay = 1.0f;
	float m_meleeRange = 0.5f;
	bool m_hasPathing = false;

	float m_health = -1.0f;
	Faction m_faction = Faction::NEUTRAL;
	float m_corpseLifetime = 0.0f;
	bool m_dieOnCollide = false;
	FloatRange m_damageOnCollide = FloatRange::ZERO;
	float m_impulseOnCollide = 0.0f;


	static void InitializeDefinitions( const char* path );
	static void ClearDefinitions();
	static const ActorDefinition* GetByName( const std::string& name );
	static std::vector<ActorDefinition*> s_definitions;

	Vec2 m_appearanceSize = Vec2::ZERO;
	Vec2 m_pivot = Vec2::ZERO;
	ActorBillboardType m_billboardType = ActorBillboardType::NONE;
	bool m_renderDepth = true;
	bool m_renderLit = false;
	bool m_renderRoundedNormals = false;
	std::vector<SpriteAnimGroupDefinition const*> m_spriteAnimationGroups;
	std::map<std::string, std::string> m_soundsByAction;

	std::map<std::string, ModelAnimation> m_modelAnimationByAction;

	FloatRange m_spawnTimeRange = FloatRange::ZERO;
	std::string m_spawnCreatureName = "";
private:
	void SetFactionFromText(std::string const& text);
};

