#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Game/Gameplay/WeaponDefinition.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"

std::vector<WeaponDefinition*> WeaponDefinition::s_definitions;

bool WeaponDefinition::LoadFromXmlElement(XMLElement const& element)
{
	m_name = ParseXmlAttribute(element, "name", "Unnamed");
	m_refireTime = ParseXmlAttribute(element, "refireTime", 1.0f);
	m_numRays = ParseXmlAttribute(element, "numRays", 0);
	m_rayCone = ParseXmlAttribute(element, "rayCone", 0.0f);
	m_rayRange = ParseXmlAttribute(element, "rayRange", 0.0f);
	m_rayDamage = ParseXmlAttribute(element, "rayDamage", FloatRange::ZERO_TO_ONE);

	m_numProjectiles = ParseXmlAttribute(element, "numProjectiles", 0);
	std::string projectileDefName = ParseXmlAttribute(element, "projectileActor", "Unnamed");
	m_projectileActorDefinition = ActorDefinition::GetByName(projectileDefName);
	m_projectileCone = ParseXmlAttribute(element, "projectileCone", 0.0f);
	m_projectileSpeed = ParseXmlAttribute(element, "projectileSpeed", 0.0f);

	XMLElement const* hudElement = element.FirstChildElement("HUD");
	std::string materialName = ParseXmlAttribute(*hudElement, "material", "Default");
	std::string baseTexturePath = ParseXmlAttribute(*hudElement, "baseTexture", "");
	std::string reticleTexturePath = ParseXmlAttribute(*hudElement, "reticleTexture", "");


	m_material = g_theRenderer->CreateOrGetMaterial(materialName.c_str());
	m_hudBaseTexture = g_theRenderer->CreateOrGetTextureFromFile(baseTexturePath.c_str());
	m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(reticleTexturePath.c_str());
	m_reticleSize = ParseXmlAttribute(*hudElement, "reticleSize", Vec2::ONE);
	m_spriteSize = ParseXmlAttribute(*hudElement, "spriteSize", Vec2::ONE);
	m_spritePivot = ParseXmlAttribute(*hudElement, "spritePivot", Vec2::ONE);


	XMLElement const* animationsElement = hudElement->FirstChildElement("Animation");
	std::string spriteSheetPath = ParseXmlAttribute(*animationsElement, "spriteSheet", "");
	IntVec2 spriteSheeeLayout = ParseXmlAttribute(*animationsElement, "cellCount", IntVec2::ZERO);
	m_spriteSheet = new SpriteSheet(*g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str()), spriteSheeeLayout);

	while (animationsElement) {
		std::string animationName = ParseXmlAttribute(*animationsElement, "name", "");
		int startFrame = ParseXmlAttribute(*animationsElement, "startFrame", 0);
		int endFrame = ParseXmlAttribute(*animationsElement, "endFrame", 0);
		int durationFrames = abs(endFrame - startFrame) + 1;
		float secondsPerFrame = ParseXmlAttribute(*animationsElement, "secondsPerFrame", 1.0f);
		std::string animationMaterialPath = ParseXmlAttribute(*animationsElement, "material", "Default");

		Material* material = g_theRenderer->CreateOrGetMaterial(animationMaterialPath.c_str());
		if (animationName == "Idle") {
			m_idleAnimationDefinition = new SpriteAnimDefinition(*m_spriteSheet, startFrame, endFrame, durationFrames * secondsPerFrame, SpriteAnimPlaybackType::ONCE, Vec3::ZERO, material);
		}
		else {
			m_attackAnimationDefinition = new SpriteAnimDefinition(*m_spriteSheet, startFrame, endFrame, durationFrames * secondsPerFrame, SpriteAnimPlaybackType::ONCE, Vec3::ZERO, material);
		}

		animationsElement = animationsElement->NextSiblingElement();
	}

	XMLElement const* soundGroupElement = element.FirstChildElement("Sounds");
	XMLElement const* soundElement = soundGroupElement->FirstChildElement("Sound");

	while (soundElement) {
		m_fireSoundName = ParseXmlAttribute(*soundElement, "name", "");
		soundElement = soundElement->NextSiblingElement();
	}

	return true;
}

void WeaponDefinition::InitializeDefinitions(const char* path)
{
	tinyxml2::XMLDocument weaponDefDoc;
	XMLError loadFileStatus = weaponDefDoc.LoadFile(path);

	GUARANTEE_OR_DIE(loadFileStatus == XMLError::XML_SUCCESS, "COULD NOT LOAD WEAPON DEFINITION XML DOCUMENT");

	XMLElement const* weaponDefSet = weaponDefDoc.FirstChildElement("Definitions");
	while (weaponDefSet) {

		XMLElement const* weaponDef = weaponDefSet->FirstChildElement("WeaponDefinition");
		while (weaponDef) {
			WeaponDefinition* newWeaponDef = new WeaponDefinition();
			newWeaponDef->LoadFromXmlElement(*weaponDef);

			s_definitions.push_back(newWeaponDef);
			weaponDef = weaponDef->NextSiblingElement();
		}

		weaponDefSet = weaponDefSet->NextSiblingElement();
	}
}

void WeaponDefinition::DestroyDefinitions()
{
	for (int defIndex = 0; defIndex < s_definitions.size(); defIndex++) {
		WeaponDefinition*& weaponDef = s_definitions[defIndex];
		if (weaponDef) {
			delete weaponDef->m_idleAnimationDefinition;
			weaponDef->m_idleAnimationDefinition = nullptr;

			delete weaponDef->m_attackAnimationDefinition;
			weaponDef->m_attackAnimationDefinition = nullptr;

			delete weaponDef;
			weaponDef = nullptr;
		}
	}

	s_definitions.clear();
}

const WeaponDefinition* WeaponDefinition::GetByName(const std::string& name)
{
	for (int weaponDefIndex = 0; weaponDefIndex < s_definitions.size(); weaponDefIndex++) {
		WeaponDefinition const* weaponDef = s_definitions[weaponDefIndex];

		if (weaponDef->m_name == name) {
			return weaponDef;
		}
	}

	return nullptr;
}
