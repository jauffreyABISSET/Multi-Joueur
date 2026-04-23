#pragma once
#include "../defines.h"
#include "../lib/CPU/EntityWrapper.h"

class HealthBar : public EntityWrapper
{
	cpu_mesh mMesh;
	cpu_material mMat;
	XMFLOAT3 mHealthBar_Scale;

public:
	HealthBar(XMFLOAT3 healthBar_Scale);
	void SetColor(XMFLOAT3 _color);
	void Initialize() override;
	void Destroy() override;
	void Update(float dt) override;
};

