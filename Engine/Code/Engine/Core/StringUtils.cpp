#include "Engine/Core/StringUtils.hpp"
#include <stdarg.h>
#include <locale>
#include <algorithm>


//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;


//-----------------------------------------------------------------------------------------------
const std::string Stringf(char const* format, ...)
{
	char textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH];
	va_list variableArgumentList;
	va_start(variableArgumentList, format);
	vsnprintf_s(textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList);
	va_end(variableArgumentList);
	textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	return std::string(textLiteral);
}


//-----------------------------------------------------------------------------------------------
const std::string Stringf(int maxLength, char const* format, ...)
{
	char textLiteralSmall[STRINGF_STACK_LOCAL_TEMP_LENGTH];
	char* textLiteral = textLiteralSmall;
	if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
		textLiteral = new char[maxLength];

	va_list variableArgumentList;
	va_start(variableArgumentList, format);
	vsnprintf_s(textLiteral, maxLength, _TRUNCATE, format, variableArgumentList);
	va_end(variableArgumentList);
	textLiteral[maxLength - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	std::string returnValue(textLiteral);
	if (maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
		delete[] textLiteral;

	return returnValue;
}

Strings SplitStringOnDelimiter(const std::string& originalString, char delimiterToSplitOn)
{
	Strings resultStrings;

	std::string currentString;
	for (int stringIndex = 0; stringIndex < originalString.size(); stringIndex++) {
		if (originalString[stringIndex] == delimiterToSplitOn) {
			resultStrings.push_back(currentString);
			currentString.clear();
		}
		else {
			currentString.push_back(originalString[stringIndex]);
		}
	}

	resultStrings.push_back(currentString);
	return resultStrings;
}


Strings SplitStringOnSpace(std::string const& originalString) {
	Strings resultStrings;

	std::string currentString;
	for (int stringIndex = 0; stringIndex < originalString.size(); stringIndex++) {
		if (std::isspace(originalString[stringIndex])) {
			resultStrings.push_back(currentString);
			currentString.clear();
		}
		else {
			currentString.push_back(originalString[stringIndex]);
		}
	}

	resultStrings.push_back(currentString);
	RemoveEmptyStrings(resultStrings);

	resultStrings.resize(resultStrings.size());
	return resultStrings;
}

void RemoveEmptyStrings(Strings& originalStrings)
{
	for (std::vector<std::string>::iterator it = originalStrings.begin(); it != originalStrings.end(); ) {
		if (it->empty()) {
			it = originalStrings.erase(it);
		}
		else {
			it++;
		}
	}
}

bool AreStringsEqualCaseInsensitive(std::string const& stringA, std::string const& stringB)
{
	return !_stricmp(stringA.c_str(), stringB.c_str());
}

bool IsStringAllWhitespace(std::string const& str)
{
	for (int index = 0; index < str.size(); index++) {
		if (!std::isspace(str[index])) return false;
	}
	return true;
}

inline void TrimLeft(std::string& str) {
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](char ch) {
		return !std::isspace(ch);
		})
	);
}

inline void TrimRight(std::string& str) {
	str.erase(std::find_if(str.rbegin(), str.rend(), [](char ch) {
		return !std::isspace(ch);
		}).base(), str.end());
}

inline void TrimString(std::string& str)
{
	TrimLeft(str);
	TrimRight(str);
}

std::string TrimStringCopy(std::string const& str)
{
	std::string trimmedStr = str;
	TrimLeft(trimmedStr);
	TrimRight(trimmedStr);

	return trimmedStr;
}

bool ContainsString(std::string const& baseStr, std::string const& otherString)
{
	bool containsString = (baseStr.find(otherString) != std::string::npos);
	return containsString;
}

bool ContainsStringCaseInsensitive(std::string const& baseStr, std::string const& otherString)
{
	std::string lowerCaseStr = otherString;
	std::string lowerCaseBase = baseStr;
	
	auto lowerLambda = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };


	std::transform(lowerCaseStr.begin(), lowerCaseStr.end(), lowerCaseStr.begin(),lowerLambda);
	std::transform(lowerCaseBase.begin(), lowerCaseBase.end(), lowerCaseBase.begin(),lowerLambda);

	bool containsString = (lowerCaseBase.find(lowerCaseStr) != std::string::npos);
	return containsString;
}

