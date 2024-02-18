#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

static Clock g_systemClock;

Clock::Clock()
{
	if (this != &g_systemClock) {
		g_systemClock.AddChild(this);
	}
}

Clock::Clock(Clock& parent) :
	m_parent(&parent)
{
}

void Clock::SetParent(Clock& parent)
{
	if (m_parent) {
		m_parent->RemoveChild(this);
	}
	m_parent = &parent;
	m_parent->AddChild(this);
}

void Clock::Pause()
{
	m_isPaused = true;
}

void Clock::Unpause()
{
	m_isPaused = false;
}

void Clock::TogglePause()
{
	m_isPaused = !m_isPaused;
}

void Clock::StepFrame()
{
	m_isPaused = false;
	m_pauseAfterFrame = true;
}

void Clock::SetTimeDilation(double dilationAmount)
{
	m_timeDilation = dilationAmount;
}

Clock::~Clock()
{
	for (int childClockIndex = 0; childClockIndex < m_children.size(); childClockIndex++) {
		Clock*& childClock = m_children[childClockIndex];
		if (!childClock) continue;
		if (m_parent) {
			childClock->SetParent(*m_parent);
		}
		else {
			if (childClock) {
				delete childClock;
				childClock = nullptr;
			}
		}
	}

	if (m_parent) {
		m_parent->RemoveChild(this);
	}
}

void Clock::SystemBeginFrame()
{
	g_systemClock.Tick();
}

Clock& Clock::GetSystemClock()
{
	return g_systemClock;
}

void Clock::Tick()
{
	double currentTime = GetCurrentTimeSeconds();
	double deltaTime = currentTime - m_lastUpdateTime;
	deltaTime = (deltaTime > 0.1f) ? 0.1f : deltaTime;
	Advance(deltaTime);
	m_lastUpdateTime = currentTime;
}

void Clock::Advance(double deltaTimeSeconds)
{
	double usedDeltaTimeSeconds = deltaTimeSeconds * m_timeDilation;
	if (m_isPaused) {
		usedDeltaTimeSeconds = 0;
	}
	else {
		m_frameCount++;
	}

	m_deltaTime = usedDeltaTimeSeconds;
	m_totalTime += m_deltaTime;

	for (int childClockIndex = 0; childClockIndex < m_children.size(); childClockIndex++) {
		Clock*& childClock = m_children[childClockIndex];
		if (childClock) {
			childClock->Advance(m_deltaTime);
		}
	}

	if (m_pauseAfterFrame) {
		m_pauseAfterFrame = false;
		m_isPaused = true;
	}
	m_lastUpdateTime = m_totalTime;
}

void Clock::AddChild(Clock* childClock)
{
	for (int childClockIndex = 0; childClockIndex < m_children.size(); childClockIndex++) {
		Clock*& childClockInVector = m_children[childClockIndex];

		if (childClock == childClockInVector) {
			return;
		}

		if (!childClockInVector) {
			childClockInVector = childClock;
			childClock->m_parent = this;
			return;
		}
	}

	m_children.push_back(childClock);
	childClock->m_parent = this;
}

void Clock::RemoveChild(Clock* childClock)
{
	for (int childClockIndex = 0; childClockIndex < m_children.size(); childClockIndex++) {
		Clock*& childClockInVector = m_children[childClockIndex];

		if (childClock == childClockInVector) {
			childClockInVector = nullptr;
		}
	}
}
