#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <errno.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")

class TThread
{
	HANDLE mHandle = nullptr;
	bool mIsSuspended = false;
	LPTHREAD_START_ROUTINE mLpAdress = nullptr;

	static DWORD StartThread(LPVOID lpParameter)
	{
		if (TThread* t = static_cast<TThread*>(lpParameter))
		{
			t->Function();
		}

		return 0;
	}
public:
	TThread() = default;
	~TThread();
	void InitThread(LPTHREAD_START_ROUTINE lpAdress, bool suspended); // Use it if you have a Thread Function to link
	void InitThread(bool suspended); // Use it if you want to encapsulate the Thread functions, the child Function() will be called

	void SuspendTThread() { if (mIsSuspended == true)return; SuspendThread(mHandle); mIsSuspended = true; }
	void ResumeTThread() { if (mIsSuspended == false)return; ResumeThread(mHandle); mIsSuspended = false; }

	virtual void Function() {};

	HANDLE& GetHandle() { return mHandle; }
};

