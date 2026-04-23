#include "pch.h"
#include "Ball.h"
#include "../../../lib/cpulib/include/cpu-engine/cpu-engine.h"

Ball::Ball(float radius, XMFLOAT3 color, float lifeTime) : EntityWrapper()
{
	SetTag(BALL);

	mRadius = radius;

	mLifeTime = lifeTime;

	mMesh.CreateSphere(radius, 5, 5, color, color);
	mMat.color = XMFLOAT3(1.f, 1.f, 1.f);

	Create(&mMesh, &mMat);

	Initialize();
}

Ball::~Ball()
{
	CPU.Release(m_pEmitter);
	m_pEmitter = nullptr;
}

void Ball::Initialize()
{
	m_pEmitter = CPU.CreateParticleEmitter();
	m_pEmitter->density = 500 * mRadius;
	m_pEmitter->spread = 10.f * mRadius;
	m_pEmitter->colorMin = cpu::ToColor(255, 0, 0);
	m_pEmitter->colorMax = cpu::ToColor(255, 255, 0);
	m_pEmitter->durationMax = 0.5f;
}

void Ball::Update(float dt)
{
	if (mLifeTime < 0)
	{
		mToDestroy = true;
	}
	else
		mLifeTime -= dt;
}
