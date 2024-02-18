#pragma once
#include <vector>


class Clock
{
public:
	Clock();
	explicit Clock(Clock& parent);
	~Clock();
	Clock(const Clock& copy) = delete;

	void		SetParent(Clock& parent);

	void		Pause();
	void		Unpause();
	void		TogglePause();
	void		StepFrame();
	void		SetTimeDilation(double dilationAmount);

	double		GetDeltaTime() const { return m_deltaTime; };
	double		GetTotalTime() const { return m_totalTime; };
	size_t		GetFrameCount() const { return m_frameCount; };
	bool		IsPaused() const { return m_isPaused; };
	double		GetTimeDilation() const { return m_timeDilation; };

public:
	static void	SystemBeginFrame();
	static Clock& GetSystemClock();

protected:
	void		Tick();
	void		Advance(double deltaTimeSeconds);
	void		AddChild(Clock* childClock);
	void		RemoveChild(Clock* childClock);

protected:
	Clock* m_parent = nullptr;
	std::vector<Clock*>	m_children;

	double		m_lastUpdateTime = 0.0;
	double		m_totalTime = 0.0;
	double		m_deltaTime = 0.0;
	size_t		m_frameCount = 0;

	double		m_timeDilation = 1.0;
	bool		m_isPaused = false;
	bool		m_pauseAfterFrame = false;

};

