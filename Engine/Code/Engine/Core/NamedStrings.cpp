#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"

NamedStrings::NamedStrings()
{
}

NamedStrings::~NamedStrings()
{
}

void NamedStrings::PopulateFromXmlElementAttributes(XMLElement const& element)
{
	tinyxml2::XMLAttribute const* attribute = element.FirstAttribute();

	while (attribute) {
		SetValue(attribute->Name(), attribute->Value());
		attribute = attribute->Next();
	}
}

void NamedStrings::SetValue(std::string const& keyName, std::string const& newValue)
{
	m_keyValuePairs[keyName] = newValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, std::string const& defaultValue) const
{
	for (std::map<std::string, std::string>::const_iterator iter = m_keyValuePairs.begin(); iter != m_keyValuePairs.end(); iter++) {
		if (AreStringsEqualCaseInsensitive(keyName, iter->first)) {
			return iter->second;
		}
	}
	return defaultValue;
}

bool NamedStrings::GetValue(std::string const& keyName, bool defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());

	if (!valueAsString.empty()) {
		bool isTrue = AreStringsEqualCaseInsensitive(valueAsString, "true");
		isTrue = isTrue || AreStringsEqualCaseInsensitive(valueAsString, "t");
		isTrue = isTrue || valueAsString == "1";

		return isTrue;
	}
	
	return defaultValue;
}

int NamedStrings::GetValue(std::string const& keyName, int defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		return stoi(valueAsString);
	}
	return defaultValue;
}

float NamedStrings::GetValue(std::string const& keyName, float defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		return std::stof(valueAsString);
	}
	return defaultValue;
}


double NamedStrings::GetValue(std::string const& keyName, double defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		return std::stod(valueAsString);
	}
	return defaultValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, char const* defaultValue) const
{
	for (std::map<std::string, std::string>::const_iterator iter = m_keyValuePairs.begin(); iter != m_keyValuePairs.end(); iter++) {
		if (AreStringsEqualCaseInsensitive(keyName, iter->first)) {
			return iter->second;
		}
	}
	return defaultValue;
}

Rgba8 NamedStrings::GetValue(std::string const& keyName, Rgba8 const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		Rgba8 value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}
	return defaultValue;
}

Vec2 NamedStrings::GetValue(std::string const& keyName, Vec2 const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		Vec2 value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

IntVec2 NamedStrings::GetValue(std::string const& keyName, IntVec2 const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		IntVec2 value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

IntVec3 NamedStrings::GetValue(std::string const& keyName, IntVec3 const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		IntVec3 value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

IntRange NamedStrings::GetValue(std::string const& keyName, IntRange const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		IntRange value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

FloatRange NamedStrings::GetValue(std::string const& keyName, FloatRange const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		FloatRange value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

AABB2 NamedStrings::GetValue(std::string const& keyName, AABB2 const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		AABB2 value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

AABB3 NamedStrings::GetValue(std::string const& keyName, AABB3 const& defaultValue) const
{
	std::string valueAsString = GetValue(keyName, std::string());
	if (!valueAsString.empty()) {
		AABB3 value;
		value.SetFromText(valueAsString.c_str());
		return value;
	}

	return defaultValue;
}

