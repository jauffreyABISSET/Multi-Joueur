#include "defines.h"
#include <iostream>

namespace Maths
{
	//Algebra
	inline void AddXMFLOAT3(XMFLOAT3& targetPos, XMFLOAT3 v)
	{
		targetPos.x += v.x;
		targetPos.y += v.y;
		targetPos.z += v.z;
	}

	inline XMFLOAT3 GetDirectionVector(const XMFLOAT3& p1, const XMFLOAT3& p2)
	{
		return XMFLOAT3(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
	}

	inline float GetNorm(const XMFLOAT3& v) // return the norm of the vector 
	{
		return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}
	inline void SelfNormalize(XMFLOAT3& v) // normalize the vector
	{
		float norm = GetNorm(v);

		if (norm == 0)
		{
			return;
		}

		v.x /= norm;
		v.y /= norm;
		v.z /= norm;
	}
	inline XMFLOAT3 Normalize(const XMFLOAT3& v) // return the norm of the vector
	{
		float norm = GetNorm(v);

		if (norm == 0)
		{
			std::cerr << "norm = 0 ! Impossible\n";
			return XMFLOAT3(0, 0, 0);
		}

		return XMFLOAT3(v.x / norm, v.y / norm, v.z / norm);
	}
	
	//Collisions
	inline void RepulseSphereSphere(XMFLOAT3& pos1, float r1, XMFLOAT3& pos2, float r2)
	{
		float dx = pos2.x - pos1.x;
		float dy = pos2.y - pos1.y;
		float dz = pos2.z - pos1.z;

		float sqrDist = dx * dx + dy * dy + dz * dz;
		float sqrMinDist = (r1 + r2) * (r1 + r2);

		if (sqrDist > sqrMinDist)
			return;

		float dist = sqrt(sqrDist);
		float minDist = sqrt(sqrMinDist);

		float overlap = minDist - dist;
		float halfOverlap = overlap * 0.5f;

		XMFLOAT3 normal = { dx, dy, dz };
		Maths::SelfNormalize(normal);

		if (dist == 0)
			normal = { 1, 0, 0 };

		XMFLOAT3 repulseVectorInvert = { -normal.x * halfOverlap, -normal.y * halfOverlap, -normal.z * halfOverlap };
		XMFLOAT3 repulseVector = { normal.x * halfOverlap, normal.y * halfOverlap, normal.z * halfOverlap };

		AddXMFLOAT3(pos1, repulseVectorInvert);
		AddXMFLOAT3(pos2, repulseVector);
	}

	inline bool IsCollisionSphereSphere(const XMFLOAT3& pos1, float r1, const XMFLOAT3& pos2, float r2) // r = Radius
	{
		float dx = pos2.x - pos1.x;
		float dy = pos2.y - pos1.y;
		float dz = pos2.z - pos1.z;

		float sqrDist = dx * dx + dy * dy + dz * dz;

		if (sqrDist < (r1 + r2) * (r1 + r2))
			return true;

		return false;
	}
	inline bool IsSphereInsideCube(const XMFLOAT3& spherePos, float sphereRadius, const XMFLOAT3& cubePos, const XMFLOAT3& scale) // check if a cube contains entirely a sphere
	{
		float halfX = scale.x * 0.5f;
		float halfY = scale.y * 0.5f;
		float halfZ = scale.z * 0.5f;

		XMFLOAT3 min = { cubePos.x - halfX, cubePos.y - halfY, cubePos.z - halfZ };
		XMFLOAT3 max = { cubePos.x + halfX, cubePos.y + halfY, cubePos.z + halfZ };

		if (spherePos.x - sphereRadius < min.x || spherePos.y - sphereRadius < min.y || spherePos.z - sphereRadius < min.z)
			return false;

		if (spherePos.x + sphereRadius > max.x || spherePos.y + sphereRadius > max.y || spherePos.z + sphereRadius > max.z)
			return false;

		return true;
	}
	inline void LockSphereInsideCube(XMFLOAT3& spherePos, float sphereRadius, const XMFLOAT3& cubePos, const XMFLOAT3& scale) // Prevent a sphere to exit a cube
	{
		float diameter = sphereRadius * 2.f;

		if (diameter > scale.x || diameter > scale.y || diameter > scale.z)
		{
			std::cerr << "Sphere too big to be contain in this cube\n";
			return;
		}

		float halfX = scale.x * 0.5f;
		float halfY = scale.y * 0.5f;
		float halfZ = scale.z * 0.5f;

		XMFLOAT3 min = { cubePos.x - halfX, cubePos.y - halfY, cubePos.z - halfZ };
		XMFLOAT3 max = { cubePos.x + halfX, cubePos.y + halfY, cubePos.z + halfZ };

		if (spherePos.x - sphereRadius < min.x)
			spherePos.x = min.x + sphereRadius;
		else if (spherePos.x + sphereRadius > max.x)
			spherePos.x = max.x - sphereRadius;

		if (spherePos.y - sphereRadius < min.y)
			spherePos.y = min.y + sphereRadius;
		else if (spherePos.y + sphereRadius > max.y)
			spherePos.y = max.y - sphereRadius;

		if (spherePos.z - sphereRadius < min.z)
			spherePos.z = min.z + sphereRadius;
		else if (spherePos.z + sphereRadius > max.z)
			spherePos.z = max.z - sphereRadius;
	}

	inline XMFLOAT3 GetWorldForwardFromYPR(const XMFLOAT3& localForward, const XMFLOAT3& YPR)
	{
		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(YPR.x, YPR.y, YPR.z);

		XMVECTOR vLocalForward = XMVectorSet(localForward.x, localForward.y, localForward.z, 0);

		XMVECTOR worldForward = XMVector3TransformCoord(vLocalForward, rotation);
		
		XMFLOAT3 dir = {};
		XMStoreFloat3(&dir, worldForward); // Convert

		return dir;
	}
}