#include <SDL.h>
#include "frameratecontroller.h"

using namespace Qk;

FramerateController::FramerateController()
	: m_targetFrametime(16.67 * 1000.0),
	  m_minSleepTime(4),
	  m_sleepFraction(0.85)
{

}

void FramerateController::StartFrameTimer()
{
	m_startTime = std::chrono::high_resolution_clock::now();
}

void FramerateController::StopFrameTimer()
{
	m_endTime = std::chrono::high_resolution_clock::now();
}

void FramerateController::SleepRemaining()
{
	unsigned int elapsed = ElapsedTimeMicroseconds();

	if (elapsed < m_targetFrametime)
	{
		int sleep = (int)(((double)(m_targetFrametime - elapsed) / 1000.0) * m_sleepFraction);

		if (sleep < 0)
			return;

		if ((unsigned int)sleep >= m_minSleepTime)
			SDL_Delay(sleep);

		do {
			m_endTime = std::chrono::high_resolution_clock::now();
		} while (ElapsedTimeMicroseconds() < m_targetFrametime);
	}
}

double FramerateController::ElapsedTimeMilliseconds()
{
	return (double)ElapsedTimeMicroseconds() / 1000.0;
}

void FramerateController::SetTargetFrameTime(double milliseconds)
{
	m_targetFrametime = (unsigned int)(milliseconds * 1000.0);
}

void FramerateController::SetSleepFraction(double fraction)
{
	m_sleepFraction = fraction;
}

void FramerateController::SetMinimumSleepTime(unsigned int milliseconds)
{
	m_minSleepTime = milliseconds;
}

unsigned int FramerateController::ElapsedTimeMicroseconds()
{
	return (unsigned int)std::chrono::duration_cast<std::chrono::microseconds>(m_endTime - m_startTime).count();
}
