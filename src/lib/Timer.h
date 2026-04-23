#pragma once

class Timer
{
	float mTargetTime = -1.f;
	float mTime = 0.f;

	bool mOverflow = true; // time continues to pass even if targetTime reached
	bool mCountdownMode = false; // time decreasing until 0, resetted to targetTime

	bool mIsPaused = false; // pause the timer
public:
	Timer() = default;

	void Init(float targetTime, bool countdownMode = false, bool isPaused = false, bool overflow = false);

	void SetTargetTime(float time) { mTargetTime = time; }
	void ResetTime();
	void Pause() { mIsPaused = true; }
	void Resume() { mIsPaused = false; }

	const float& GetTime() const { return mTime; }

	void Display();

	void Update(float dt);

	bool IsTimeOut();
};

