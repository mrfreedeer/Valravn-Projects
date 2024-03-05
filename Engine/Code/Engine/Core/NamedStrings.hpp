#pragma once
#include <map>
#include "Engine/Core/XmlUtils.hpp"
#include <string>

struct AABB2;
struct AABB3;
class NamedStrings {
public:
	NamedStrings();
	~NamedStrings();

	void PopulateFromXmlElementAttributes(tinyxml2::XMLElement const& element);
	void SetValue(std::string const& keyName, std::string const& newValue);
	std::string GetValue(std::string const& keyName, std::string const& defaultValue) const;
	bool GetValue(std::string const& keyName, bool defaultValue) const;
	int GetValue(std::string const& keyName, int defaultValue) const;
	float GetValue(std::string const& keyName, float defaultValue) const;
	double GetValue(std::string const& keyName, double defaultValue) const;
	std::string GetValue(std::string const& keyName, char const* defaultValue) const;
	Rgba8 GetValue(std::string const& keyName, Rgba8 const& defaultValue) const;
	Vec2 GetValue(std::string const& keyName, Vec2 const& defaultValue) const;
	IntVec2 GetValue(std::string const& keyName, IntVec2 const& defaultValue) const;
	IntVec3 GetValue(std::string const& keyName, IntVec3 const& defaultValue) const;
	IntRange GetValue(std::string const& keyName, IntRange const& defaultValue) const;
	FloatRange GetValue(std::string const& keyName, FloatRange const& defaultValue) const;
	AABB2 GetValue(std::string const& keyName, AABB2 const& defaultValue) const;
	AABB3 GetValue(std::string const& keyName, AABB3 const& defaultValue) const;

private:
	std::map <std::string, std::string> m_keyValuePairs;
};