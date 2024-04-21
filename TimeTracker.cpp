#include "TimeTracker.h"

#include <iostream>

#pragma optimize ("", off)

// -------------------------------------------------------------- //

TimeTracker::TimeTracker()
	: mStartTime()
	, mTakingInputs(true)
	, mRunningTotal(0.0)
	, mMeasurementsTaken(0)
{

}

// -------------------------------------------------------------- //

TimeTracker::~TimeTracker()
{

}

// -------------------------------------------------------------- //

void TimeTracker::StartTiming()
{
	mStartTime = std::chrono::high_resolution_clock::now();
}

// -------------------------------------------------------------- //

void TimeTracker::AddMeasurement()
{
	if (!mTakingInputs)
		return;

	std::chrono::time_point<std::chrono::high_resolution_clock> endTime  = std::chrono::high_resolution_clock::now();
	std::chrono::microseconds                                   duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - mStartTime);

	double durationDouble = (double)duration.count() / 1000000.0;

	mRunningTotal += durationDouble;
	mMeasurementsTaken++;
}

// -------------------------------------------------------------- //

void TimeTracker::StopTiming()
{
	mTakingInputs = false;
}

// -------------------------------------------------------------- //

void TimeTracker::OutputAverageTime()
{
	if (mMeasurementsTaken == 0)
	{
		std::cout << "N/A" << std::endl;
		return;
	}
	
	double average = mRunningTotal / (double)mMeasurementsTaken;

	std::cout << average << std::endl;
}

// -------------------------------------------------------------- //

void TimeTracker::CombineTimes(TimeTracker& other)
{
	mMeasurementsTaken += other.mMeasurementsTaken;
	mRunningTotal      += other.mRunningTotal;
}

// -------------------------------------------------------------- //