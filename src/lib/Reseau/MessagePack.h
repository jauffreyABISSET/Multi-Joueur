#pragma once
#include "Pack.h"
#include <string>
#include <iostream>

struct MessagePack : Pack
{
	char message[MSG_MAX_SIZE];

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