#pragma once
#include "EntityWrapper.h"
#include "../Reseau/Pack.h"

class Wall;
class Bush;

class Arena
{
	XMFLOAT3 mPos = { 0, 0, 0 };
	std::vector<Wall*>  mWalls;
	cpu_mesh mMesh;
	cpu_material mMat;
	float thickness = PLAYER_RADIUS;

public:
	Arena();
	~Arena();
	void CreateWall(XMFLOAT3 offset, float length, bool horizontal);
	void Wall_Up();
	void Wall_Right();
	void Wall_Left();
	void Wall_Down();
};

