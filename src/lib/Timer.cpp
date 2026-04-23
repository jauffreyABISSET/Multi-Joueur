#include "pch.h"
#include "Timer.h"

void Timer::Init(float targetTime, bool countdownMode, bool isPaused, bool overflow)
{
	mTargetTime = targetTime;
	mCountdownMode = countdownMode;
	mIsPaused = isPaused;
	mOverflow = overflow;

	ResetTime();
}

void Timer::ResetTime()
{
	if (mCountdownMode == false)
		mTime = 0.f;
	else
		mTime = mTargetTime;
}

void Timer::Display()
{
	if (mCountdownMode == false)
	{
		std::cout << "Time : " << mTime << " / " << mTargetTime << std::endl;
	}
	else
	{
		std::cout << "Time Left : " << mTime << std::endl;
	}
}

void Timer::Update(float dt)
{
	if (mIsPaused)
		return;

	if (mCountdownMode == false)
	{
		if (mTime >= mTargetTime && mOverflow == false)
			mTime = mTargetTime;
		else
			mTime += dt;
	}
	else
	{
		if (mTime <= 0 && mOverflow == false)
			mTime = 0;
		else
			mTime -= dt;
	}
}

bool Timer::IsTimeOut()
{
	if (mCountdownMode == false)
		return mTime >= mTargetTime;

	else
		return mTime <= 0;
}
