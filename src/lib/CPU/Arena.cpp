#include "pch.h"
#include "Arena.h"
#include "../lib/CPU/Wall.h"
#include "../lib/CPU/Bush.h"

Arena::Arena()
{
	Wall_Down();
	Wall_Left();
	Wall_Right();
	Wall_Up();
}

Arena::~Arena()
{
	for (Wall* w : mWalls)
	{
		delete w;
	}

	mWalls.clear();
}

void Arena::CreateWall(XMFLOAT3 offset, float length, bool horizontal)
{
	Wall* pWall = new Wall(length, horizontal);
	pWall->SetPosition(offset);
	mWalls.push_back(pWall);
}

void Arena::Wall_Up()
{
	CreateWall({ 0 ,ARENA_SCALE.y * 0.5f,0 }, ARENA_SCALE.x, true);
}
void Arena::Wall_Right()
{
	CreateWall({ ARENA_SCALE.x * 0.5f , 0 ,0 }, ARENA_SCALE.y, false);
}
void Arena::Wall_Left()
{
	CreateWall({ -ARENA_SCALE.x * 0.5f, 0, 0 }, ARENA_SCALE.y, false);
}
void Arena::Wall_Down()
{
	CreateWall({ 0, -ARENA_SCALE.y * 0.5f, 0 }, ARENA_SCALE.x, true);
}