#pragma once

#include <chrono>
#include <vector>

class TimeTracker
{
public:
	TimeTracker();
	~TimeTracker();

	void StartTiming();
	void AddMeasurement();
	void StopTiming();

	void OutputAverageTime();

	void Clear() 
	{
		mRunningTotal      = 0.0;
		mMeasurementsTaken = 0;
		mTakingInputs      = true;
	}

	void CombineTimes(TimeTracker& other);

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
	double                                                      mRunningTotal;
	unsigned int                                                mMeasurementsTaken;

	bool                                                        mTakingInputs;
};