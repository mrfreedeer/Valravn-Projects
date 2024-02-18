#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/Clock.hpp"

Stopwatch::Stopwatch():
	m_clock(&Clock::GetSystemClock())
{
}

Stopwatch::Stopwatch(double duration):
	m_duration(duration),
	m_clock(&Clock::GetSystemClock())
{
	m_startTime = m_clock->GetTotalTime();
}

Stopwatch::Stopwatch(const Clock* clock, double duration):
	m_duration(duration),
	m_clock(clock)
{
	m_startTime = m_clock->GetTotalTime();
}

void Stopwatch::Start(double duration)
{
	m_duration = duration;
	m_startTime = m_clock->GetTotalTime();
}

void Stopwatch::Start(const Clock* clock, double duration)
{
	m_clock = clock;
	m_duration = duration;
	m_startTime = m_clock->GetTotalTime();
}

void Stopwatch::Restart()
{
	m_startTime = m_clock->GetTotalTime();
}

void Stopwatch::Stop()
{
	m_duration = 0;
}

void Stopwatch::Pause()
{
	m_startTime = m_startTime - m_clock->GetTotalTime();
}

void Stopwatch::Resume()
{
	m_startTime += m_clock->GetTotalTime();
}

double Stopwatch::GetElapsedTime() const
{
	if (m_duration == 0) return 0;
	if (m_startTime < 0) return -m_startTime;

	double elapsed = m_clock->GetTotalTime() - m_startTime;
	return elapsed;
}

float Stopwatch::GetElapsedFraction() const
{
	double elapsed = GetElapsedTime();
	if ((elapsed == 0.0f) && (m_duration == 0.0)) return 0.0;
	return static_cast<float>(elapsed / m_duration);
}

bool Stopwatch::IsStopped() const
{
	return m_duration == 0;
}

bool Stopwatch::IsPaused() const
{
	return m_startTime < 0;
}

bool Stopwatch::HasDurationElapsed() const
{
	double elapsed = GetElapsedTime();
	return elapsed >= m_duration;
}

bool Stopwatch::CheckDurationElapsedAndDecrement()
{
	if (m_duration == 0.0) return false;

	float elapsedFraction = GetElapsedFraction();
	if ((elapsedFraction < 1.0f) && !IsPaused())return false;

	m_startTime += m_duration;
	return true;
}
