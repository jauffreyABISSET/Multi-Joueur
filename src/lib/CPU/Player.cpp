#include "pch.h"
#include "Player.h"
#include "../lib/CPU/HealthBar.h"

Player::Player(float radius, XMFLOAT3 color) : EntityWrapper()
{
	mColor = color;
	SetTag(PLAYER);

	mMesh.CreateSphere(radius, 10, 10, color, color);
	AddCubeToMesh(&mMesh, { 0, radius, 0 }, { 0.75f, 0.75f, 0.75f }, color);

	CreateHealthBar({0, 0, 0}, {(PLAYER_RADIUS * 1.75), PLAYER_RADIUS * 0.50, 0.1});
	mMat.color = XMFLOAT3(1.f, 1.f, 1.f);
	Create(&mMesh, &mMat);

	Initialize();
}

Player::~Player()
{
	delete mHealthBar;
	CPU.Release(m_pEmitter);
	m_pEmitter = nullptr;
}

void Player::Initialize()
{	
	m_pEmitter = CPU.CreateParticleEmitter();
	m_pEmitter->density = 5000.0f;
	m_pEmitter->colorMin = mColor;
	m_pEmitter->spread = 1.f;
	m_pEmitter->colorMax = cpu::ToColor(255, 255, 255);
	m_pEmitter->durationMax = 1.2f;
}

void Player::SetEmitterState(bool state)
{
	if (state == true)
	{
		m_pEmitter->durationMin = 0.5f;
		m_pEmitter->durationMax = 1.2f;
	}
	else
	{
		m_pEmitter->durationMin = 0.f;
		m_pEmitter->durationMax = 0.f;
	}
}

void Player::Update(float dt)
{
}

void Player::CreateHealthBar(XMFLOAT3 offset, XMFLOAT3 scale)
{
	mHealthBar = new HealthBar(scale);
	mHealthBar->SetPosition(offset);
}

void Player::UpdateHealthBar()
{
	float ratio = (float)mHp / (float)mHpMax;
	ratio = std::clamp(ratio, 0.f, 1.f);

	float currentWidth = (PLAYER_RADIUS * 1.50) * ratio;

	XMFLOAT3 lifeColor = { 1.f - ratio, ratio, 0.f };
	XMFLOAT3 pos = GetEntity()->transform.pos;

	mHealthBar->SetPosition(pos.x, pos.y + PLAYER_RADIUS * 1.75, pos.z);
	mHealthBar->SetColor(lifeColor);
	mHealthBar->SetScale(currentWidth, PLAYER_RADIUS * 0.50, 0.1);
}

void Player::ToggleBarVisibilty(bool state)
{
	mHealthBar->ToggleVisibilty(state);
}
