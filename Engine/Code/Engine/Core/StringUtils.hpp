#pragma once
//-----------------------------------------------------------------------------------------------
#include <string>
#include <vector>

typedef std::vector<std::string> Strings;

//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... );
const std::string Stringf( int maxLength, char const* format, ... );

Strings SplitStringOnDelimiter(const std::string& originalString, char delimiterToSplitOn);
Strings SplitStringOnSpace(const std::string& originalString);
void RemoveEmptyStrings(Strings& originalStrings);

bool AreStringsEqualCaseInsensitive(std::string const& stringA, std::string const& stringB);
bool IsStringAllWhitespace(std::string const& str);
inline void TrimString(std::string& str);
std::string TrimStringCopy(std::string const& str);
bool ContainsString(std::string const& baseStr, std::string const& otherString);
bool ContainsStringCaseInsensitive(std::string const& baseStr, std::string const& otherString);