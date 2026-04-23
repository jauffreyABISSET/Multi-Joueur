#pragma once
#include "EntityWrapper.h"
#include "../Reseau/Pack.h"
#include "../Timer.h"

#define MAX_PLAYERS 16

class HealthBar;

struct PlayerDataToSend
{
	XMFLOAT3 pos;
	XMFLOAT3 rot;
	uint8_t hp;

	uint8_t isAlive;
	uint16_t killCount;

	char username[USERNAME_MAX_SIZE];

	void SetData(XMFLOAT3 _pos, XMFLOAT3 _rot, uint8_t _hp, bool _isAlive)
	{
		pos = _pos;
		rot = _rot;
		hp = _hp;

		isAlive = (uint8_t)_isAlive;
	}

	void SetUsername(std::string name)
	{
		size_t size = name.size();

		if (size >= USERNAME_MAX_SIZE)
		{
			std::cerr << "Too long username : it will be cut !\n";
			size = USERNAME_MAX_SIZE - 1;
		}

		memcpy(username, name.c_str(), size);
		username[size] = '\0';
	}
};

struct PlayerState
{
	PlayerDataToSend dat;
	Timer reloadTimer;
	Timer respawnTime;
};

struct PlayerGamePack : FragmentPack
{
	uint16_t count;
	PlayerDataToSend playerData[MAX_PLAYERS];
};

class Player : public EntityWrapper
{
	XMFLOAT3 mPos = { 0, 0, 0 };
	uint8_t mHp;
	uint16_t mKillCount = 0;
	uint8_t mHpMax = PLAYER_MAX_HP;

	HealthBar*  mHealthBar;
	cpu_mesh mMesh;
	cpu_material mMat;
	XMFLOAT3 mColor;
	cpu_particle_emitter* m_pEmitter = nullptr;
	char username[USERNAME_MAX_SIZE];

public:
	Player() = default;
	Player(float radius, XMFLOAT3 color);
	~Player();
	void Initialize() override;
	void SetEmitterState(bool state);

	void Update(float dt) override;
	void CreateHealthBar(XMFLOAT3 offset, XMFLOAT3 scale);
	void UpdateHealthBar();
	void SetHP(uint8_t hp) {mHp = hp;}
	void SetKillCount(uint16_t k) { mKillCount = k; }
	uint16_t GetKillCount() const { return mKillCount; }
	void ToggleBarVisibilty(bool state);
	uint8_t GetHPMax() const { return mHpMax; }
	uint8_t GetHP() const { return mHp; }
	std::string GetUsername() const { return std::string(username); }
	cpu_particle_emitter* GetEmitter() { return m_pEmitter; }
};

