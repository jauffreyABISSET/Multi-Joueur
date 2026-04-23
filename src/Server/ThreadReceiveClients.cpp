#include "pch.h"
#include "ThreadReceiveClients.h"
#include "../lib/Reseau/Client.h"
#include "../lib/Reseau/LoginPack.h"
#include "../lib/Reseau/MessagePack.h"

void ThreadReceiveClients::Function()
{
	while (MANAGER->mIsOpen)
	{
		sockaddr_in from;
		char buffer[MAX_BUFFER_SIZE];
		int bytes_received = MANAGER->mSock.ReceiveFrom(buffer, sizeof(buffer), from);

		if (bytes_received >= sizeof(Pack))
		{
			Pack* pPack = reinterpret_cast<Pack*>(buffer);

			switch (pPack->header)
			{
			case PackType::CS_PONG_PACK:
				HandleClientPong(pPack, from);
				break;

			case PackType::CS_LOGIN_ASK_PACK:
				HandleClientLogin(pPack, from);
				break;

			default:
				HandleClientPacks(buffer, from);
				break;
			}
		}
	}
}

int ThreadReceiveClients::HandleClient(sockaddr_in from, LoginAskPack p)
{
	int finalState = -1;

	auto& allClients = MANAGER->mAllClients;
	auto& clientsToAdd = MANAGER->mClientsToAdd;

	std::string msg = p.message;

	auto it = allClients.find(msg);

	if (it != allClients.end()) // existing user
	{
		finalState = EXISTING_USER_WITH_NEW_NAME;

		Client* pClient = it->second; // Client with the name

		if (pClient->isAFK == false)
			finalState = TRY_TO_STEAL_AN_ACTIVE_SESSION;
		else
		{
			finalState = STEAL_AN_AFK_SESSION;

			EnterCriticalSection(&MANAGER->mCS_clientToEditAccess);
			MANAGER->mClientsToEdit.push({ pClient, from });
			LeaveCriticalSection(&MANAGER->mCS_clientToEditAccess);
		}
	}
	else // new user
	{
		finalState = NEW_USER_WITH_NEW_NAME;

		for (auto it = allClients.begin(); it != allClients.end(); ++it) // verify if the new user has a registered IP
		{
			Client* pCurrent = it->second;

			if (from.sin_addr.s_addr == pCurrent->addr.sin_addr.s_addr && std::string(pCurrent->name) != std::string(msg)) // Same IP but not same Name
			{
				pCurrent->addr = sockaddr_in();
				finalState = NEW_USER_WITH_EXISTING_NAME;

				EnterCriticalSection(&MANAGER->mCS_clientToRemoveAccess);
				MANAGER->mClientsToRemove.push(pCurrent);
				LeaveCriticalSection(&MANAGER->mCS_clientToRemoveAccess);

				break;
			}
		}

		//Create a new client
		Client* pClient = new Client();
		pClient->name = msg;
		pClient->addr = from;

		EnterCriticalSection(&MANAGER->mCS_clientToAddAccess);
		MANAGER->mClientsToAdd.push(pClient);
		LeaveCriticalSection(&MANAGER->mCS_clientToAddAccess);
	}

	return finalState;
}

void ThreadReceiveClients::HandleClientPong(Pack* pPack, sockaddr_in from)
{
	EnterCriticalSection(&MANAGER->mCS_clientAccess);
	auto allClients = MANAGER->mAllClients;
	LeaveCriticalSection(&MANAGER->mCS_clientAccess);

	if (allClients.empty())
		return;

	Client* pClient = MANAGER->GetClient(from);

	if (pClient == nullptr)
	{
		std::cerr << "Client not found -> HandleClientPong() !\n";
		return;
	}

	if (IsPackMoreRecent(pPack->id, &pClient->lastPackIDSeen[(int)PackType::CS_PONG_PACK]))
	{
		pClient->lastPingTimer.ResetTime(); // Client is alive
	}
}

void ThreadReceiveClients::HandleClientLogin(Pack* pPack, sockaddr_in from)
{
	LoginAskPack* pLogin = reinterpret_cast<LoginAskPack*>(pPack);
	int result = HandleClient(from, *pLogin);
	std::string message = "";

	uint8_t valid = VALID_LOGIN;
	std::string username = pLogin->message;

	switch (result)
	{
	case NEW_USER_WITH_NEW_NAME:
		message = "New User " + username + " Verified !\n";
		break;

	case TRY_TO_STEAL_AN_ACTIVE_SESSION:
		message = username + " isn't AFK !\n";
		valid = INVALID_LOGIN;
		break;

	case STEAL_AN_AFK_SESSION:
		message = "Welcome back " + username + " !\n";
		break;
	}

	// ANSWER
	EnterCriticalSection(&MANAGER->mCS_Pack_ID);
	uint16_t id = GetNewPackID();
	LeaveCriticalSection(&MANAGER->mCS_Pack_ID);

	const int packSize = sizeof(LoginAnswerPack);

	LoginAnswerPack answer;
	answer.Set(id, PackType::SC_LOGIN_ANSWER_PACK, packSize);
	answer.SetMessage(message.c_str());
	answer.status = valid;

	char bufferAnswer[packSize];
	memcpy(bufferAnswer, &answer, packSize);

	MANAGER->mSock.SendTo(bufferAnswer, packSize, reinterpret_cast<sockaddr*>(&from), sizeof(from));
}

void ThreadReceiveClients::HandleClientPacks(char* buffer, sockaddr_in from)
{
	EnterCriticalSection(&MANAGER->mCS_clientAccess);
	auto allClients = MANAGER->mAllClients;
	LeaveCriticalSection(&MANAGER->mCS_clientAccess);

	if (allClients.empty())
		return;

	Client* pClient = MANAGER->GetClient(from);

	if (pClient == nullptr)
	{
		std::cerr << "Error finding the client !\n";
		return;
	}

	if (pClient->isAFK)
	{
		return;
	}

	char* queueBuffer = new char[MAX_BUFFER_SIZE];
	memcpy(queueBuffer, buffer, MAX_BUFFER_SIZE);

	EnterCriticalSection(&MANAGER->mCS_AddPacksToClientQueue);
	pClient->mPackQueue.push_back(queueBuffer);
	LeaveCriticalSection(&MANAGER->mCS_AddPacksToClientQueue);
}
