#pragma once

#include <chrono>

namespace Qk
{
	class FramerateController
	{
	public:
		FramerateController();

		void StartFrameTimer();
		void StopFrameTimer();
		void SleepRemaining();
		double ElapsedTimeMilliseconds();

		void SetTargetFrameTime(double milliseconds);
		void SetSleepFraction(double fraction);
		void SetMinimumSleepTime(unsigned int milliseconds);

	private:
		unsigned int ElapsedTimeMicroseconds();

	private:
		unsigned int m_targetFrametime;
		double m_sleepFraction;
		unsigned int m_minSleepTime;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_endTime;
	};
}