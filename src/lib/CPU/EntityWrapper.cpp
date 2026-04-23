#include "pch.h"
#include "EntityWrapper.h"

EntityWrapper::EntityWrapper(cpu_mesh* pMesh, cpu_material* pMaterial)
{
	Create(pMesh, pMaterial);
}

void EntityWrapper::Create(cpu_mesh* pMesh, cpu_material* pMaterial)
{
	m_pEntity = CPU.CreateEntity();
	m_pEntity->pMesh = pMesh;
	m_pEntity->pMaterial = pMaterial;
	m_pEntity->transform.SetPosition(0, 0, 0);
}

void EntityWrapper::AddCubeToMesh(cpu_mesh* pMesh, XMFLOAT3 _offset, XMFLOAT3 _scale, XMFLOAT3 _color)
{
	if (pMesh == nullptr)
	{
		std::cerr << "pMesh NULL\n";
		return;
	}

	XMFLOAT3 offset = _offset;
	XMFLOAT3 scale = _scale;

	XMFLOAT3 halfScale = { scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f };

	//Bottom points
	auto p1 = XMFLOAT3(offset.x - halfScale.x, offset.y - halfScale.y, offset.z - halfScale.z); // (-1, -1, -1)
	auto p2 = XMFLOAT3(offset.x + halfScale.x, offset.y - halfScale.y, offset.z - halfScale.z); // (1, -1, -1)
	auto p3 = XMFLOAT3(offset.x - halfScale.x, offset.y - halfScale.y, offset.z + halfScale.z); // (-1, -1, 1)
	auto p4 = XMFLOAT3(offset.x + halfScale.x, offset.y - halfScale.y, offset.z + halfScale.z); // (1, -1, 1)

	//Top points
	auto p5 = XMFLOAT3(offset.x - halfScale.x, offset.y + halfScale.y, offset.z - halfScale.z); // (-1, 1, -1)
	auto p6 = XMFLOAT3(offset.x + halfScale.x, offset.y + halfScale.y, offset.z - halfScale.z); // (1, 1, -1)
	auto p7 = XMFLOAT3(offset.x - halfScale.x, offset.y + halfScale.y, offset.z + halfScale.z); // (-1, 1, 1)
	auto p8 = XMFLOAT3(offset.x + halfScale.x, offset.y + halfScale.y, offset.z + halfScale.z); // (1, 1, 1)

	pMesh->AddFace(p1, p3, p4, p2, _color);
	pMesh->AddFace(p3, p7, p8, p4, _color);
	pMesh->AddFace(p5, p6, p8, p7, _color);
	pMesh->AddFace(p1, p2, p6, p5, _color);
	pMesh->AddFace(p1, p5, p7, p3, _color);
	pMesh->AddFace(p2, p4, p8, p6, _color);
}

void EntityWrapper::AddCylinderToMesh(cpu_mesh* pMesh, XMFLOAT3 _offset, float radius, int count, float depth, XMFLOAT3 color)
{
	if (count < 3)
		return;

	float halfDepth = depth * 0.5f;
	float step = XM_2PI / count;
	float angle = 0.0f;
	XMFLOAT3 p1, p2, p3;
	p1.x = 0.0f;
	p1.y = -halfDepth;
	p1.z = 0.0f;
	p2.y = -halfDepth;
	p3.y = -halfDepth;

	for (int i = 0; i < count; i++)
	{
		p2.x = cosf(angle) * radius;
		p2.z = sinf(angle) * radius;
		p3.x = cosf(angle + step) * radius;
		p3.z = sinf(angle + step) * radius;

		pMesh->AddTriangle(p1, p3, p2, color);

		XMFLOAT3 p4, p5, p6;
		p4 = p1;
		p4.y = halfDepth;
		p5 = p2;
		p5.y = halfDepth;
		p6 = p3;
		p6.y = halfDepth;

		pMesh->AddTriangle(p4, p5, p6, color);

		pMesh->AddFace(p2, p3, p6, p5, color);
		angle += step;
	}
}

void EntityWrapper::AddSphereToMesh(cpu_mesh* pMesh, XMFLOAT3 _offset, float _raduis, XMFLOAT3 _color)
{
	if (pMesh == nullptr)
	{
		std::cerr << "pMesh NULL\n";
		return;
	}

	XMFLOAT3 offset = _offset;
	float raduis = _raduis;
}

void EntityWrapper::ToggleVisibilty(bool state)
{
	m_pEntity->visible = state;
}

void EntityWrapper::Destroy()
{
	CPU.Release(m_pEntity);

	mToDestroy = true;
}

void EntityWrapper::SetPosition(XMFLOAT3 pos)
{
	SetPosition(pos.x, pos.y, pos.z);
}

void EntityWrapper::SetPosition(float x, float y, float z)
{
	m_pEntity->transform.SetPosition(x, y, z);
}

void EntityWrapper::Move(float dx, float dy, float dz)
{
	XMFLOAT3 pos = m_pEntity->transform.pos;

	m_pEntity->transform.SetPosition(pos.x + dx, pos.y + dy, pos.z + dz);
}

void EntityWrapper::SetRotation(float yaw, float pitch, float roll)
{
	m_pEntity->transform.SetYPR(yaw, pitch, roll);
}

void EntityWrapper::SetRotation(XMFLOAT3 YPR)
{
	SetRotation(YPR.x, YPR.y, YPR.z);
}

void EntityWrapper::SetScale(float x, float y, float z)
{
	SetScale(XMFLOAT3(x, y, z));
}

void EntityWrapper::SetScale(XMFLOAT3 scale)
{
	m_pEntity->transform.sca = scale;
}


