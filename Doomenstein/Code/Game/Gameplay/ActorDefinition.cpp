#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Mesh.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"
#include "Game/Gameplay/WeaponDefinition.hpp"
#include "Game/Framework/GameCommon.hpp"

std::vector<ActorDefinition*> ActorDefinition::s_definitions;

ActorDefinition::~ActorDefinition()
{
	for (int spriteAnimGroupIndex = 0; spriteAnimGroupIndex < m_spriteAnimationGroups.size(); spriteAnimGroupIndex++) {
		SpriteAnimGroupDefinition const*& spriteAnimGroup = m_spriteAnimationGroups[spriteAnimGroupIndex];
		if (spriteAnimGroup) {
			delete spriteAnimGroup;
			spriteAnimGroup = nullptr;
		}
	}

	for (std::map<std::string, ModelAnimation>::iterator iter = m_modelAnimationByAction.begin(); iter != m_modelAnimationByAction.end(); iter++) {
		ModelAnimation& modelAnimation = iter->second;
		for (int bufferIndex = 0; bufferIndex < modelAnimation.m_indexBuffers.size(); bufferIndex++) {
			VertexBuffer*& vertexBuffer = modelAnimation.m_vertexBuffers[bufferIndex];
			IndexBuffer*& indexBuffer = modelAnimation.m_indexBuffers[bufferIndex];

			if (vertexBuffer) {
				delete vertexBuffer;
				vertexBuffer = nullptr;
			}

			if (indexBuffer) {
				delete indexBuffer;
				indexBuffer = nullptr;
			}
		}

	}
}

bool ActorDefinition::LoadFromXmlElement(const XMLElement& element)
{

	m_name = ParseXmlAttribute(element, "name", "unknown");
	std::string factionName = ParseXmlAttribute(element, "faction", "Neutral");
	SetFactionFromText(factionName);
	m_health = ParseXmlAttribute(element, "health", 200.0f);
	m_canBePossessed = ParseXmlAttribute(element, "canBePossessed", false);
	m_corpseLifetime = ParseXmlAttribute(element, "corpseLifetime", 1.0f);

	m_spawnTimeRange = ParseXmlAttribute(element, "spawnTimer", FloatRange::ZERO);
	m_spawnCreatureName = ParseXmlAttribute(element, "spawns", "");

	m_visible = ParseXmlAttribute(element, "visible", true);
	m_dieOnSpawn = ParseXmlAttribute(element, "dieOnSpawn", false);

	m_is3DModel = ParseXmlAttribute(element, "is3DModel", false);

	XMLElement const* collisionElem = element.FirstChildElement("Collision");


	if (collisionElem) {
		m_physicsRadius = ParseXmlAttribute(*collisionElem, "radius", 1.0f);
		m_physicsHeight = ParseXmlAttribute(*collisionElem, "height", 1.0f);
		m_collidesWithWorld = ParseXmlAttribute(*collisionElem, "collidesWithWorld", false);
		m_collidesWithActors = ParseXmlAttribute(*collisionElem, "collidesWithActors", false);
		m_impulseOnCollide = ParseXmlAttribute(*collisionElem, "impulseOnCollide", 0.0f);
		m_dieOnCollide = ParseXmlAttribute(*collisionElem, "dieOnCollide", false);
		m_damageOnCollide = ParseXmlAttribute(*collisionElem, "damageOnCollide", FloatRange::ZERO);
		
	}


	XMLElement const* physicsElem = element.FirstChildElement("Physics");
	if (physicsElem) {
		m_simulated = ParseXmlAttribute(*physicsElem, "simulated", false);
		m_walkSpeed = ParseXmlAttribute(*physicsElem, "walkSpeed", 1.0f);
		m_runSpeed = ParseXmlAttribute(*physicsElem, "runSpeed", 1.0f);
		m_turnSpeed = ParseXmlAttribute(*physicsElem, "turnSpeed", 180.0f);
		m_drag = ParseXmlAttribute(*physicsElem, "drag", 10.0f);
		m_flying = ParseXmlAttribute(*physicsElem, "flying", false);
		m_gravity = ParseXmlAttribute(*physicsElem, "gravity", FloatRange::ZERO);
		m_gravityRadius = ParseXmlAttribute(*physicsElem, "gravityRadius", 5.0f);
		m_constantDamageOnDeath = ParseXmlAttribute(*physicsElem, "constantDamageOnDeath", FloatRange::ZERO);
		m_deathDamageInterval = ParseXmlAttribute(*physicsElem, "deathDamageInterval", 1.0f);
		m_deathDamageRadius = ParseXmlAttribute(*physicsElem, "damageRadius", 0.0f);
	}

	XMLElement const* cameraElem = element.FirstChildElement("Camera");
	if (cameraElem) {
		m_eyeHeight = ParseXmlAttribute(*cameraElem, "eyeHeight", 0.5f);
		m_cameraFOVDegrees = ParseXmlAttribute(*cameraElem, "cameraFOV", 60.0f);
	}

	XMLElement const* inventoryElem = element.FirstChildElement("Inventory");
	if (inventoryElem) {
		XMLElement const* weapon = inventoryElem->FirstChildElement();
		while (weapon) {
			std::string weaponName = ParseXmlAttribute(*weapon, "name", "Pistol");
			m_weaponDefinitions.push_back(WeaponDefinition::GetByName(weaponName));
			weapon = weapon->NextSiblingElement();
		}
	}

	XMLElement const* aiElem = element.FirstChildElement("AI");
	if (aiElem) {
		m_aiEnabled = ParseXmlAttribute(*aiElem, "aiEnabled", false);
		m_sightRadius = ParseXmlAttribute(*aiElem, "sightRadius", 60.0f);
		m_sightAngle = ParseXmlAttribute(*aiElem, "sightAngle", 60.0f);
		m_meleeDamage = ParseXmlAttribute(*aiElem, "meleeDamage", FloatRange::ZERO_TO_ONE);
		m_meleeDelay = ParseXmlAttribute(*aiElem, "meleeRange", 0.5f);
		m_hasPathing = ParseXmlAttribute(*aiElem, "hasPathing", false);
	}

	XMLElement const* appearanceElem = element.FirstChildElement("Appearance");
	if (appearanceElem) {
		m_appearanceSize = ParseXmlAttribute(*appearanceElem, "size", Vec2::ONE);
		m_pivot = ParseXmlAttribute(*appearanceElem, "pivot", Vec2::ZERO);
		m_renderLit = ParseXmlAttribute(*appearanceElem, "renderLit", false);
		m_renderDepth = ParseXmlAttribute(*appearanceElem, "renderDepth", false);

		std::string billboardType = ParseXmlAttribute(*appearanceElem, "billboardType", "None");
		if (billboardType == "None") {
			m_billboardType = ActorBillboardType::NONE;
		}
		else if (billboardType == "Facing") {
			m_billboardType = ActorBillboardType::FACING;
		}
		else if (billboardType == "Aligned") {
			m_billboardType = ActorBillboardType::ALIGNED;
		}


		XMLElement const* currentAnimationGroupDef = appearanceElem->FirstChildElement("AnimationGroup");
		while (currentAnimationGroupDef) {
			std::string textureName = ParseXmlAttribute(*currentAnimationGroupDef, "spriteSheet", "");
			Texture* texture = g_theRenderer->CreateOrGetTextureFromFile(textureName.c_str());

			if (!texture) continue;

			SpriteAnimGroupDefinition* newSpriteAnimGroup = new SpriteAnimGroupDefinition(texture);
			newSpriteAnimGroup->LoadAnimationsFromXML(*currentAnimationGroupDef);
			m_spriteAnimationGroups.push_back(newSpriteAnimGroup);

			currentAnimationGroupDef = currentAnimationGroupDef->NextSiblingElement();
			g_theRenderer->CreateOrGetMaterial(newSpriteAnimGroup->GetMatName().c_str());
		}

	}

	XMLElement const* soundElements = element.FirstChildElement("Sounds");
	if (soundElements) {
		XMLElement const* soundElem = soundElements->FirstChildElement("Sound");
		while (soundElem) {
			std::string action = ParseXmlAttribute(*soundElem, "sound", "Walk");
			std::string source = ParseXmlAttribute(*soundElem, "name", "");

			m_soundsByAction[action] = source;
			soundElem = soundElem->NextSiblingElement();
		}
	}

	if (m_is3DModel) {
		XMLElement const* modelElements = element.FirstChildElement("Models");
		if (modelElements) {
			XMLElement const* animationModelElement = modelElements->FirstChildElement("AnimationModel");
			while (animationModelElement) {

				std::string name = ParseXmlAttribute(*animationModelElement, "name", "Unknown");
				float secondsPerFrame = ParseXmlAttribute(*animationModelElement, "secondsPerModel", 0.1f);
				bool scaleBySpeed = ParseXmlAttribute(*animationModelElement, "scaleBySpeed", false);

				XMLElement const* modelFile = animationModelElement->FirstChildElement("Model");

				std::vector<VertexBuffer*> vertexBuffers;
				std::vector<IndexBuffer*> indexBuffers;
				std::vector<int> amountOfIndexes;
				std::string textureName = ParseXmlAttribute(*animationModelElement, "texture", "");
				Texture* texture = nullptr;
				if (!textureName.empty()) {
					texture = g_theRenderer->CreateOrGetTextureFromFile(textureName.c_str());
				}
				std::string materialName = ParseXmlAttribute(*animationModelElement, "material", "");
				Material* material = nullptr;
				if (!materialName.empty()) {
					material = g_theRenderer->CreateOrGetMaterial(materialName.c_str());
				}

				BufferDesc vBufferDesc = {};
				vBufferDesc.data = nullptr;
				vBufferDesc.descriptorHeap = nullptr;
				vBufferDesc.memoryUsage = MemoryUsage::Dynamic;
				vBufferDesc.owner = g_theRenderer;
				vBufferDesc.size = sizeof(Vertex_PNCU);
				vBufferDesc.stride = sizeof(Vertex_PNCU);

				BufferDesc iBufferDesc = {};
				iBufferDesc.data = nullptr;
				iBufferDesc.descriptorHeap = nullptr;
				iBufferDesc.memoryUsage = MemoryUsage::Dynamic;
				iBufferDesc.owner = g_theRenderer;
				iBufferDesc.size = sizeof(unsigned int);
				iBufferDesc.stride = sizeof(unsigned int);

				while (modelFile) {
					std::string modelPath = ParseXmlAttribute(*modelFile, "path", "Unknown path");
					std::vector<Vertex_PNCU> verts;
					std::vector<unsigned int> indexes;

					LoadMeshFromPlyFile(modelPath, Rgba8::WHITE, verts, indexes);
					

					VertexBuffer* newVertexBuffer = new VertexBuffer(vBufferDesc);
					IndexBuffer* newIndexBuffer = new IndexBuffer(iBufferDesc);

					newVertexBuffer->CopyCPUToGPU(verts.data(), verts.size() * newVertexBuffer->GetStride());
					newIndexBuffer->CopyCPUToGPU(indexes.data(), indexes.size() * sizeof(unsigned int));

					vertexBuffers.push_back(newVertexBuffer);
					indexBuffers.push_back(newIndexBuffer);

					amountOfIndexes.push_back((int)indexes.size());

					modelFile = modelFile->NextSiblingElement();
				}

				ModelAnimation newModelAnim = {
					vertexBuffers,
					indexBuffers,
					amountOfIndexes,
					(int)indexBuffers.size(),
					secondsPerFrame,
					scaleBySpeed,
					texture,
					material
				};
				m_modelAnimationByAction[name] = newModelAnim;

				animationModelElement = animationModelElement->NextSiblingElement();
			}
		}
	}

	return true;
}

std::string const ActorDefinition::GetSoundSourceForAction(std::string action) const
{
	std::map<std::string, std::string>::const_iterator iter = m_soundsByAction.find(action);
	if (iter != m_soundsByAction.end()) {
		return iter->second;
	}
	return "";

}

void ActorDefinition::InitializeDefinitions(const char* path)
{
	tinyxml2::XMLDocument actorDefXMLDoc;

	XMLError actorDefLoadStatus = actorDefXMLDoc.LoadFile(path);
	GUARANTEE_OR_DIE(actorDefLoadStatus == XMLError::XML_SUCCESS, "ACTOR DEFINITIONS XML DOES NOT EXIST OR CANNOT BE FOUND");

	XMLElement const* multipleActorDefs = actorDefXMLDoc.FirstChildElement("Definitions");
	while (multipleActorDefs) {
		XMLElement const* currentActorDef = multipleActorDefs->FirstChildElement();

		while (currentActorDef) {
			ActorDefinition* actorDef = new ActorDefinition();
			actorDef->LoadFromXmlElement(*currentActorDef);
			s_definitions.push_back(actorDef);

			currentActorDef = currentActorDef->NextSiblingElement();
		}

		multipleActorDefs = multipleActorDefs->NextSiblingElement();
	}
}

void ActorDefinition::ClearDefinitions()
{
	for (int actorDefIndex = 0; actorDefIndex < s_definitions.size(); actorDefIndex++) {
		ActorDefinition*& actorDef = s_definitions[actorDefIndex];
		if (actorDef) {
			delete actorDef;
			actorDef = nullptr;
		}
	}

	s_definitions.clear();
}

const ActorDefinition* ActorDefinition::GetByName(const std::string& name)
{
	for (int actorDefIndex = 0; actorDefIndex < s_definitions.size(); actorDefIndex++) {
		ActorDefinition* actorDef = s_definitions[actorDefIndex];
		if (actorDef) {
			if (actorDef->m_name == name) {
				return actorDef;
			}
		}
	}

	return nullptr;
}

void ActorDefinition::SetFactionFromText(std::string const& text)
{
	if (text == "Marine") {
		m_faction = Faction::MARINE;
	}

	if (text == "Neutral") {
		m_faction = Faction::NEUTRAL;
	}

	if (text == "Demon") {
		m_faction = Faction::DEMON;
	}
}
