#pragma once
#define UNUSED(x) (void)(x);
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Network/Network.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/ProfileLogScope.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <string>

class NamedProperties;
class EventSystem;
class DevConsole;

extern NamedStrings g_gameConfigBlackboard;

constexpr float ARBITRARILY_LARGE_VALUE = FLT_MAX;
constexpr int ARBITRARILY_LARGE_INT_VALUE = INT_MAX;
extern DevConsole* g_theConsole;
extern EventSystem* g_theEventSystem;
extern JobSystem* g_theJobSystem;
extern NetworkSystem* g_theNetwork;


enum class MemoryUsage {
	Default,	// Buffer made to read from multiple times but CPU cannot write to. 
	Upload,		// Buffer that can be read/write from both GPU and CPU. 
};


#pragma warning(disable : 26812)