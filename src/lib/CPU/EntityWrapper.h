#pragma once
#include "../defines.h"

enum Tag
{
	NONE,
	PLAYER,
	BALL,
	ARENA,
	HEALTHBAR,
};

class EntityWrapper
{
public:
	EntityWrapper() = default;
	EntityWrapper(cpu_mesh* pMesh, cpu_material* pMaterial);
	~EntityWrapper() { Destroy(); }

	virtual void Initialize() = 0;
	virtual void Destroy();
	virtual void Update(float dt) = 0;

	cpu_entity* GetEntity() { return m_pEntity; }

	const XMFLOAT3& GetPosition() const { return m_pEntity->transform.pos; }
	void SetPosition(XMFLOAT3 pos);
	void SetPosition(float x, float y, float z);
	void Move(float dx, float dy, float dz);
	void SetRotation(float yaw, float pitch = 0.0f, float roll = 0.0f);
	void SetRotation(XMFLOAT3 YPR);
	void SetScale(float x, float y, float z);
	void SetScale(XMFLOAT3 scale);

	void SetTag(Tag tag) { mTag = tag; }
	bool IsTag(Tag tag) const { return mTag == tag; }

	bool mToDestroy = false;
public:
	void Create(cpu_mesh* pMesh, cpu_material* pMaterial);
	void AddCubeToMesh(cpu_mesh* pMesh, XMFLOAT3 _offset, XMFLOAT3 _scale, XMFLOAT3 _color);
	void AddCylinderToMesh(cpu_mesh* pMesh, XMFLOAT3 _offset, float radius, int count, float depth, XMFLOAT3 color);
	void AddSphereToMesh(cpu_mesh* pMesh, XMFLOAT3 _offset, float _raduis, XMFLOAT3 _color);

	void ToggleVisibilty(bool state);

	cpu_entity* m_pEntity = nullptr;
	Tag mTag = NONE;
};



