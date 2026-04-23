#include "pch.h"
#include "Wall.h"

Wall::Wall(float length, bool horizontal)
{
	SetTag(ARENA);

	XMFLOAT3 color = { 1,1,1 };
	mMesh.CreateCube(1, color);
	mMat.color = XMFLOAT3(1.f, 1.f, 1.f);
	Create(&mMesh, &mMat);

	float thickness = 0.1f;
	XMFLOAT3 finalScale = { thickness, thickness, thickness * PLAYER_RADIUS * 2 };
	float finalLength = length * 0.5f;

	if (horizontal)
		finalScale.x = finalLength;
	else
		finalScale.y = finalLength;

	SetScale(finalScale);
}

void Wall::Initialize()
{

}

void Wall::Destroy()
{

}

void Wall::Update(float dt)
{

}
