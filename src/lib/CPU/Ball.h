#pragma once
#include "EntityWrapper.h"
#include "../Reseau/Pack.h"
#include "Timer.h"

#define MAX_BALLS 32

struct BallDataToSend
{
	XMFLOAT3 pos;
	uint32_t id;
	float radius;
	float lifeTime;
};

struct BallState
{
	BallDataToSend dat;
	XMFLOAT3 dir;
	float speed;

	Timer lifeTime;
	uint8_t ownerID;
};

struct BallGameFragmentPack : FragmentPack
{
	uint16_t count;
	BallDataToSend ballData[MAX_BALLS];
};

//CLIENT
class Ball : public EntityWrapper
{
	XMFLOAT3 mPos = {0, 0, 0};
	float mRadius = 0;
	cpu_mesh mMesh;
	cpu_material mMat;
	cpu_particle_emitter* m_pEmitter;

	float mLifeTime = 0;
public:
	Ball(float radius, XMFLOAT3 color, float lifeTime = BALL_LIFETIME);
	~Ball();

	void Initialize() override;
	void Update(float dt) override;

	cpu_particle_emitter* GetEmitter() { return m_pEmitter; }
};

