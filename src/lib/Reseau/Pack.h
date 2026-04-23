#pragma once
#include <iostream>
#define MSG_MAX_SIZE 128
#define USERNAME_MAX_SIZE 16

static inline uint16_t currentPackID = 0;

static inline uint16_t GetNewPackID()
{
	return currentPackID++;
}

enum class PackType : uint8_t 
{
	UNDEFINED_PACK,
	MESSAGE_PACK,

	//Client To Server
	CS_PONG_PACK,
	CS_LOGIN_ASK_PACK,
	CS_INPUT_PACK,

	//Server To Client
	SC_PING_PACK,
	SC_LOGIN_ANSWER_PACK,
	SC_BALLGAME_FRAGMENTS_PACK,
	SC_PLAYERGAME_FRAGMENT_PACK,
	SC_EVENT_MESSAGE_PACK,

	Count,
};

struct Pack // + 6 octets
{
	uint16_t id;
	PackType header;
	uint16_t size;

	Pack()
	{
		id = -1;
		header = PackType::UNDEFINED_PACK;
		size = -1;
	}

	void Set(uint16_t _id, PackType _header, uint16_t _size)
	{
		id = _id;
		header = _header;
		size = _size;
	}
};

struct FragmentPack : Pack
{
	uint16_t fullPackID;
	uint16_t fragmentIndex;
	uint16_t fragmentCount;
	uint32_t serverTick;

	void SetFragment(uint16_t _fullPackID, uint16_t _fragIndex, uint16_t fragsCount, uint16_t _serverTick)
	{
		fullPackID = _fullPackID;
		fragmentIndex = _fragIndex;
		fragmentCount = fragsCount;
		serverTick = _serverTick;
	}
};

static inline bool IsPackMoreRecent(uint16_t packID, uint16_t* lastSeen)
{
	if (packID == *lastSeen)
		return false;

	if ((uint16_t)(packID - *lastSeen) < 32768) // modulo 65335
	{
		*lastSeen = packID;

		return true;
	}

	return false;
}