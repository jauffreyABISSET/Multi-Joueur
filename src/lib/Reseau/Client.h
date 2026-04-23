#pragma once
#include "defines.h"
#include <list>
#include <string>
#include "../Timer.h"
#include "../../lib/CPU/Player.h"

inline static int currentID = 0;

struct Client
{
	uint32_t id;
	std::string name;
	sockaddr_in addr;

	uint16_t lastPackIDSeen[(int)PackType::Count];
	std::list<char*> mPackQueue;

	Timer cleanClientTimer;
	Timer lastPingTimer;
	bool isAFK = false;

	PlayerState playerState;

	Client()
	{
		id = currentID;
		currentID++;

		for (size_t i = 0; i < (int)PackType::Count; i++)
		{
			lastPackIDSeen[i] = 0;
		}

		mPackQueue = {};

		lastPingTimer.Init(MAX_WAITING_TIME, false);
	}
};

