#pragma once
#include "Pack.h"

struct LoginAskPack : Pack
{
	char message[USERNAME_MAX_SIZE];

	void SetMessage(std::string txt)
	{
		size_t size = txt.size();

		if (size >= USERNAME_MAX_SIZE)
		{
			std::cerr << "Too long username : it will be cut !\n";
			size = USERNAME_MAX_SIZE - 1;
		}

		memcpy(message, txt.c_str(), size);
		message[size] = '\0';
	}
};


constexpr uint8_t INVALID_LOGIN = 0;
constexpr uint8_t VALID_LOGIN = 1;

struct LoginAnswerPack : Pack
{
	char message[MSG_MAX_SIZE];
	uint8_t status; // if 0, the message isn't positive

	void SetMessage(std::string txt)
	{
		size_t size = txt.size();

		if (size >= MSG_MAX_SIZE)
		{
			std::cerr << "Too long message : it will be cut !\n";
			size = MSG_MAX_SIZE - 1;
		}

		memcpy(message, txt.c_str(), size);
		message[size] = '\0';
	}
};
