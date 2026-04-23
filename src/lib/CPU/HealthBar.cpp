#include "pch.h"
#include "HealthBar.h"

HealthBar::HealthBar(XMFLOAT3 healthBar_Scale)
{
	SetTag(PLAYER);
	mHealthBar_Scale = healthBar_Scale;

	XMFLOAT3 color = { 1,1,1 };

	AddCubeToMesh(&mMesh, { 0,0,0 }, healthBar_Scale, color);

	mMat.color = XMFLOAT3(1.f, 1.f, 1.f);

	Create(&mMesh, &mMat);
}

void HealthBar::SetColor(XMFLOAT3 _color)
{
	mMat.color = _color;
}

void HealthBar::Initialize()
{
}

void HealthBar::Destroy()
{
}

void HealthBar::Update(float dt)
{
}
