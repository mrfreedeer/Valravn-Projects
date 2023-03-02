#pragma once
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"

typedef tinyxml2::XMLElement XMLElement;
typedef tinyxml2::XMLError XMLError;
typedef tinyxml2::XMLDocument XMLDoc;

struct EulerAngles;
struct FloatRange;
struct IntRange;

int ParseXmlAttribute(XMLElement const& element, char const* attributeName, int defaultValue = 0);
char ParseXmlAttribute(XMLElement const& element, char const* attributeName, char defaultValue);
bool ParseXmlAttribute(XMLElement const& element, char const* attributeName, bool defaultValue = false);
float ParseXmlAttribute(XMLElement const& element, char const* attributeName, float defaultValue = 0.0f);
Rgba8 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Rgba8 const& defaultValue = Rgba8::WHITE);
Vec2 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Vec2 const& defaultValue = Vec2::ZERO);
Vec3 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Vec3 const& defaultValue = Vec3::ZERO);
IntVec2 ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntVec2 const& defaultValue = IntVec2::ZERO);
IntVec3 ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntVec3 const& defaultValue = IntVec3::ZERO);
std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, std::string const& defaultValue);
Strings ParseXmlAttribute(XMLElement const& element, char const* attributeName, Strings const& defaultValues = Strings());
std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, char const* defaultValue);
EulerAngles ParseXmlAttribute(XMLElement const& element, char const* attributeName, EulerAngles const& defaultValue);
FloatRange ParseXmlAttribute(XMLElement const& element, char const* attributeName, FloatRange const& defaultValue);
IntRange ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntRange const& defaultValue);
