#pragma once
#include "../defines.h"
#include "../lib/CPU/EntityWrapper.h"

class Wall : public EntityWrapper
{
	cpu_mesh mMesh;
	cpu_material mMat;

public:
	Wall(float length, bool horizontal);
	void Initialize() override;
	void Destroy() override;
	void Update(float dt) override;
};

