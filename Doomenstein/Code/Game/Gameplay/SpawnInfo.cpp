#include "Game/Gameplay/SpawnInfo.hpp"
#include "Game/Gameplay/ActorDefinition.hpp"

SpawnInfo::SpawnInfo(const ActorDefinition* definition, const Vec3& position, const EulerAngles& orientation, const Vec3& velocity):
	m_definition(definition),
	m_position(position),
	m_orientation(orientation),
	m_velocity(velocity)
{
}

SpawnInfo::SpawnInfo(const char* definitionName, const Vec3& position, const EulerAngles& orientation, const Vec3& velocity):
	m_definition(ActorDefinition::GetByName(definitionName)),
	m_position(position),
	m_orientation(orientation),
	m_velocity(velocity)
{
}

bool SpawnInfo::LoadFromXmlElement(const XMLElement& element)
{
	std::string actorDefName = ParseXmlAttribute(element, "actor", "Unnamed");
	m_definition = ActorDefinition::GetByName(actorDefName);
	m_position = ParseXmlAttribute(element, "position", Vec3::ZERO);
	m_orientation = ParseXmlAttribute(element, "orientation", EulerAngles::ZERO);

	return true;
}
