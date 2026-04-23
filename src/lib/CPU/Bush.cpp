#include "pch.h"
#include "Bush.h"

Bush::Bush(float radius)
{
	SetTag(ARENA);
	XMFLOAT3 color = { 0,1,0 };
	AddCylinderToMesh(&mMesh, {0,0,0}, radius, 15, PLAYER_RADIUS * 2.f, color);
	Create(&mMesh, &mMat);
}

void Bush::Initialize()
{

}

void Bush::Destroy()
{

}

void Bush::Update(float dt)
{

}
