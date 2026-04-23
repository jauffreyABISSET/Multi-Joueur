#pragma once
#include "defines.h"
#include "Manager.h"

struct LoginAskPack;

enum HandleClientState
{
	NO_USER_TO_ADD,
	TRY_TO_STEAL_AN_ACTIVE_SESSION,
	STEAL_AN_AFK_SESSION,
	NEW_USER_WITH_NEW_NAME,
	NEW_USER_WITH_EXISTING_NAME,
	EXISTING_USER_WITH_NEW_NAME,
	EXISTING_USER_WIDTH_EXISTING_NAME,
};

class ThreadReceiveClients : public TThread
{

public:
	void Function() override;

	void HandleClientPong(Pack* pPack, sockaddr_in from);

	void HandleClientLogin(Pack* pPack, sockaddr_in from);
	int HandleClient(sockaddr_in client, LoginAskPack p);

	void HandleClientPacks(char* buffer, sockaddr_in from);
};

