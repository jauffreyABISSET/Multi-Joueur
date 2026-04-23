#pragma once
#include "../defines.h"
#include "../lib/CPU/EntityWrapper.h"

class Bush : public EntityWrapper
{
	cpu_mesh mMesh;
	cpu_material mMat;

public:
	Bush(float radius);
	void Initialize() override;
	void Destroy() override;
	void Update(float dt) override;
};

