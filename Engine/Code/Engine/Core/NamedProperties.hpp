#pragma once
#include <map>
#include <string>
#include <typeinfo>
#include "Engine/Core/StringUtils.hpp"

class NamedPropertyBase {
public:
	virtual ~NamedPropertyBase() = default;

	virtual NamedPropertyBase* GetClone() const = 0;

	virtual bool IsType(std::type_info const& type) const = 0;
};

template<typename T_Value>
class NamedProperty : public NamedPropertyBase {
public:
	T_Value m_data;

	NamedPropertyBase* GetClone() const {
		NamedProperty<T_Value>* clone = new NamedProperty<T_Value>();
		clone->m_data = m_data;
		return clone;
	}

	virtual bool IsType(std::type_info const& type) const override { return (typeid(m_data).hash_code() == type.hash_code()); }
};


class NamedProperties {
public:
	NamedProperties() = default;
	NamedProperties(NamedProperties const& otherNamedProperties);
	~NamedProperties();


	template<typename T_Value>
	inline T_Value GetValue(std::string const& name, T_Value const& defaultValue) const;
	template<typename T_Value>
	inline void SetValue(std::string const& name, T_Value const& value);

	inline std::string GetValue(std::string const& name, char const* defaultValue) const;
	inline void SetValue(std::string const& name, char const* value);

	void operator=(NamedProperties const& otherNamedProperties);

	std::map<std::string, NamedPropertyBase*> m_keyValuePairs;
};

template<typename T_Value>
inline T_Value NamedProperties::GetValue(std::string const& name, T_Value const& defaultValue) const
{
	std::map<std::string, NamedPropertyBase*>::const_iterator it;
	bool shouldBreak = false;
	for (it = m_keyValuePairs.begin(); (it != m_keyValuePairs.end()) && !shouldBreak;) {
		shouldBreak = (AreStringsEqualCaseInsensitive(name, it->first));
		if (!shouldBreak)it++;
	}

	if (it != m_keyValuePairs.end()) {
		NamedPropertyBase const* anyTypeProperty = it->second;

		if (anyTypeProperty->IsType(typeid(defaultValue))) {
			NamedProperty<T_Value> const* propertyAsCorrectType = reinterpret_cast<NamedProperty<T_Value> const*>(anyTypeProperty);
			return propertyAsCorrectType->m_data;
		}
		else {
			return defaultValue;
		}

	}

	return defaultValue;
}

template<typename T_Value>
inline void NamedProperties::SetValue(std::string const& name, T_Value const& value)
{
	std::map<std::string, NamedPropertyBase*>::iterator it;
	bool shouldBreak = false;
	for (it = m_keyValuePairs.begin(); (it != m_keyValuePairs.end()) && !shouldBreak;) {
		shouldBreak = (AreStringsEqualCaseInsensitive(name, it->first));
		if (!shouldBreak)it++;
	}

	bool propertyNeedsToBeCreated = !(it != m_keyValuePairs.end());
	if (it != m_keyValuePairs.end()) {
		NamedPropertyBase* anyTypeProperty = it->second;

		if (anyTypeProperty->IsType(typeid(value))) {
			NamedProperty<T_Value>* propertyAsCorrectType = reinterpret_cast<NamedProperty<T_Value>*>(anyTypeProperty);
			propertyAsCorrectType->m_data = value;
			return;
		}
		else {
			delete anyTypeProperty;
			propertyNeedsToBeCreated = true;
		}
	}

	if (propertyNeedsToBeCreated) {
		NamedProperty<T_Value>* newProperty = new NamedProperty<T_Value>();
		newProperty->m_data = value;

		m_keyValuePairs[name] = newProperty;

	}
}


inline std::string NamedProperties::GetValue(std::string const& name, char const* defaultValue) const
{
	return GetValue<std::string>(name, std::string(defaultValue));
}

inline void NamedProperties::SetValue(std::string const& name, char const* value)
{
	SetValue<std::string>(name, value);
}

