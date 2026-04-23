#pragma once
#include "Pack.h"

enum class Input : uint8_t
{
	NONE		= 0x00,
	LEFT		= 0x01,
	RIGHT		= 0x02,
	MOUSELEFT	= 0x04,
	FORWARD		= 0x08,
	BACKWARD	= 0x10,
	SHIFT		= 0x20,

	//BOTH		= MOUSELEFT | FORWARD,
};

struct InputPack : Pack
{
	XMFLOAT3 dir;
	uint8_t input;

	InputPack() { input = (uint8_t)Input::NONE; dir = { 0, 0, 0 }; }
};
