#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/IntRange.hpp"

int ParseXmlAttribute(XMLElement const& element, char const* attributeName, int defaultValue)
{
	return element.IntAttribute(attributeName, defaultValue);
}

char ParseXmlAttribute(XMLElement const& element, char const* attributeName, char defaultValue)
{
	if (element.Attribute(attributeName)) {
		return element.Attribute(attributeName)[0];
	}

	return defaultValue;
}

bool ParseXmlAttribute(XMLElement const& element, char const* attributeName, bool defaultValue)
{
	return element.BoolAttribute(attributeName, defaultValue);
}

float ParseXmlAttribute(XMLElement const& element, char const* attributeName, float defaultValue)
{
	return element.FloatAttribute(attributeName, defaultValue);
}


Rgba8 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Rgba8 const& defaultValue)
{

	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {
		Rgba8 color;
		color.SetFromText(attrValue);

		return color;
	}

	return defaultValue;
}

Vec2 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Vec2 const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {
		Vec2 parsedValue;
		parsedValue.SetFromText(attrValue);

		return parsedValue;
	}

	return defaultValue;
}

Vec3 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Vec3 const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {
		Vec3 parsedValue;
		parsedValue.SetFromText(attrValue);

		return parsedValue;
	}

	return defaultValue;
}

IntVec2 ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntVec2 const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {
		IntVec2 parsedValue;
		parsedValue.SetFromText(attrValue);

		return parsedValue;
	}

	return defaultValue;
}

IntVec3 ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntVec3 const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {
		IntVec3 parsedValue;
		parsedValue.SetFromText(attrValue);

		return parsedValue;
	}

	return defaultValue;
}

std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, std::string const& defaultValue)
{
	if (element.Attribute(attributeName)) {
		return element.Attribute(attributeName);
	}
	return defaultValue;
}

Strings ParseXmlAttribute(XMLElement const& element, char const* attributeName, Strings const& defaultValues)
{
	if (element.Attribute(attributeName)) {
		return SplitStringOnDelimiter(element.Attribute(attributeName), ',');
	}
	return defaultValues;
}

std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, char const* defaultValue)
{
	if (element.Attribute(attributeName)) {
		return element.Attribute(attributeName);
	}
	return defaultValue;
}

EulerAngles ParseXmlAttribute(XMLElement const& element, char const* attributeName, EulerAngles const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {

		EulerAngles newEulerAngle;
		newEulerAngle.SetFromText(attrValue);

		return newEulerAngle;
	}

	return defaultValue;
}

FloatRange ParseXmlAttribute(XMLElement const& element, char const* attributeName, FloatRange const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {

		FloatRange newFloatRange;
		newFloatRange.SetFromText(attrValue);

		return newFloatRange;
	}

	return defaultValue;
}

IntRange ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntRange const& defaultValue)
{
	char const* attrValue = element.Attribute(attributeName);
	if (attrValue) {

		IntRange newIntRange;
		newIntRange.SetFromText(attrValue);

		return newIntRange;
	}

	return defaultValue;
}
