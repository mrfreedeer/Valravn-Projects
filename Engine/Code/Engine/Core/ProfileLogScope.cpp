#include "Engine/Core/ProfileLogScope.hpp"
#include <ctime>
#include <ratio>
#include <chrono>
#include <string>
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

uint64_t TimeGetHPC()
{
	auto timeNow = std::chrono::high_resolution_clock::now();
	auto timeInNanoSeconds = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow.time_since_epoch()).count();

	return timeInNanoSeconds;
}



std::string ConvertToReadableDurationString(uint64_t timeInNanoSeconds) {
	if (timeInNanoSeconds < 1000) {
		return Stringf("%lu ns", timeInNanoSeconds);
	}

	if (timeInNanoSeconds < 1'000'000) {
		return Stringf("%lu mus", timeInNanoSeconds / 1000);
	}

	if (timeInNanoSeconds < 1'000'000'000) {
		return Stringf("%lu ms", timeInNanoSeconds / 1'000'000);
	}

	return Stringf("%lu s", timeInNanoSeconds / 1'000'000'000);
}

ProfileLogScope::ProfileLogScope(char const* tag, bool logToConsole, uint64_t* reportTo) :
	m_tag(tag),
	m_reportTo(reportTo),
	m_logToConsole(logToConsole),
	m_start(TimeGetHPC())
{
}

ProfileLogScope::~ProfileLogScope()
{
	uint64_t dur = TimeGetHPC() - m_start;
	std::string readableDuration = m_tag;
	readableDuration += ": ";
	readableDuration += ConvertToReadableDurationString(dur);
	if (m_reportTo) {
		*m_reportTo += dur;
	}

	if (m_logToConsole) {
		g_theConsole->AddLine(DevConsole::INFO_MINOR_COLOR, readableDuration);
	}
}
