#include "pch.h"
#include "ThreadGameplayMessages.h"
#include "App.h"
#include "../lib/Reseau/Client.h"

void ThreadGameplayMessages::Function()
{
	uint32_t lastIDSeen = 0;
	int occurences = 0;

	while (APP.mIsRunning && APP.mIsLogged)
	{
		sockaddr_in from;

		char buffer[MAX_BUFFER_SIZE];

		EnterCriticalSection(&APP.mCS_SocketAccess);
		UDPSocket* pSock = APP.GetSocket();
		LeaveCriticalSection(&APP.mCS_SocketAccess);

		if (pSock == nullptr)
			return;

		int bytes_received = pSock->ReceiveFrom(buffer, sizeof(buffer), from);

		if (bytes_received >= sizeof(Pack))
		{
			Pack* pPack = reinterpret_cast<Pack*>(buffer);

			if (lastIDSeen == pPack->id)
				occurences++;

			lastIDSeen = pPack->id;

			if (occurences >= 10)
				continue; // Server is freezing

			char* queueBuffer = new char[MAX_BUFFER_SIZE];
			memcpy(queueBuffer, buffer, MAX_BUFFER_SIZE);

			EnterCriticalSection(&APP.mCS_PacksQueue);
			APP.mPacksQueue.push_back(queueBuffer);
			LeaveCriticalSection(&APP.mCS_PacksQueue);
		}
	}

	int a = 0;
}
