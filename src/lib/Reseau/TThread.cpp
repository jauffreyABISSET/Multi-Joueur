#include "pch.h"
#include "TThread.h"

TThread::~TThread()
{
	if (mHandle)
	{
		WaitForSingleObject(mHandle, INFINITE);
		CloseHandle(mHandle);
		mHandle = nullptr;
	}
}

void TThread::InitThread(LPTHREAD_START_ROUTINE lpAdress, bool suspended)
{
	mLpAdress = lpAdress;

	DWORD flag = 0;

	mIsSuspended = suspended;

	if (suspended)
		flag = CREATE_SUSPENDED;

	mHandle = CreateThread(
		NULL,
		0,
		mLpAdress, //lpAdress
		this, // param
		flag,
		NULL
	);
}

void TThread::InitThread(bool suspended)
{
	mLpAdress = nullptr;

	DWORD flag = 0;

	mIsSuspended = suspended;

	if (suspended)
		flag = CREATE_SUSPENDED;

	mHandle = CreateThread(
		NULL,
		0,
		StartThread, //lpAdress
		this, // param
		flag,
		NULL
	);
}
