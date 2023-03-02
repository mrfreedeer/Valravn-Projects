#include "Engine/Core/NamedProperties.hpp"

NamedProperties::NamedProperties(NamedProperties const& otherNamedProperties)
{
	std::map<std::string, NamedPropertyBase*> const& otherProperties = otherNamedProperties.m_keyValuePairs;

	std::map<std::string, NamedPropertyBase*>::const_iterator it;
	for (it = otherProperties.begin(); (it != otherProperties.end());) {
		m_keyValuePairs[it->first] = it->second->GetClone();

	}
}

NamedProperties::~NamedProperties()
{
	for (auto& [key, value] : m_keyValuePairs) {
		if (value) {
			delete value;
			value = nullptr;
		}
	}
}

void NamedProperties::operator=(NamedProperties const& otherNamedProperties)
{
	std::map<std::string, NamedPropertyBase*> const& otherProperties = otherNamedProperties.m_keyValuePairs;

	std::map<std::string, NamedPropertyBase*>::const_iterator it;
	for (it = otherProperties.begin(); (it != otherProperties.end()); it++) {
		m_keyValuePairs[it->first] = it->second->GetClone();

	}
}
