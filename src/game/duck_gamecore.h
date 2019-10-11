#pragma once
#include "gamecore.h"
#include <base/tl/array.h>

struct CDuckCollision;
struct CGameContext;

struct CNetObj_DuckCustomCore
{
	enum { NET_ID = 0x2001 };

	int m_UID;
	int m_PlgUID;
	int m_Flags;
	int m_PosX;
	int m_PosY;
	int m_VelX;
	int m_VelY;
	int m_Radius;
};

struct CNetObj_DuckCharCoreExtra
{
	enum { NET_ID = 0x2002 };
	int m_HookedCustomCoreUID;
};

struct CNetObj_DuckPhysicsLawsGroup
{
	enum { NET_ID = 0x2003 };
	int m_UID;
	int m_AirFriction;
	int m_GroundFriction;
	int m_AirMaxSpeed;
	int m_GroundMaxSpeed;
	int m_AirAccel;
	int m_GroundAccel;
	int m_Gravity;
	int m_Elasticity;
};

struct CCharCoreExtra
{
	int m_HookedCustomCoreUID;

	void Reset()
	{
		m_HookedCustomCoreUID = -1;
	}

	void Read(const CNetObj_DuckCharCoreExtra& NetObj)
	{
		Reset();
		m_HookedCustomCoreUID = NetObj.m_HookedCustomCoreUID;
	}

	void Write(CNetObj_DuckCharCoreExtra* pNetObj)
	{
		pNetObj->m_HookedCustomCoreUID = m_HookedCustomCoreUID;
	}
};

struct CCustomCore
{
	enum
	{
		FLAG_CHAR_COLLIDE  = 0x1,
		FLAG_CHAR_HOOK     = 0x2,
		FLAG_CORE_COLLIDE  = 0x4,
	};

	int m_UID;
	int m_PlgUID; // physics laws group unique ID
	int m_Flags;
	vec2 m_Pos;
	vec2 m_Vel;
	float m_Radius;

	void Read(const CNetObj_DuckCustomCore& NetObj)
	{
		m_UID = NetObj.m_UID;
		m_PlgUID = NetObj.m_PlgUID;
		m_Flags = NetObj.m_Flags;
		m_Pos.x = NetObj.m_PosX / 256.f;
		m_Pos.y = NetObj.m_PosY / 256.f;
		m_Vel.x = NetObj.m_VelX / 256.f;
		m_Vel.y = NetObj.m_VelY / 256.f;
		m_Radius = NetObj.m_Radius / 256.f;
	}

	void Write(CNetObj_DuckCustomCore* pNetObj)
	{
		pNetObj->m_UID = m_UID;
		pNetObj->m_PlgUID = m_PlgUID;
		pNetObj->m_Flags = m_Flags;
		pNetObj->m_PosX = m_Pos.x * 256.f;
		pNetObj->m_PosY = m_Pos.y * 256.f;
		pNetObj->m_VelX = m_Vel.x * 256.f;
		pNetObj->m_VelY = m_Vel.y * 256.f;
		pNetObj->m_Radius = m_Radius * 256.f;
	}

	inline void Quantize()
	{
		CNetObj_DuckCustomCore NetObj;
		Write(&NetObj);
		Read(NetObj);
	}

	void Reset()
	{
		m_UID = -1;
		m_PlgUID = -1;
		m_Flags = FLAG_CHAR_COLLIDE | FLAG_CHAR_HOOK | FLAG_CORE_COLLIDE;
		m_Pos = vec2(0,0);
		m_Vel = vec2(0,0);
		m_Radius = 20;
	}
};

struct CPhysicsLawsGroup
{
	int m_UID;
	float m_AirFriction;
	float m_GroundFriction;
	float m_AirMaxSpeed;
	float m_GroundMaxSpeed;
	float m_AirAccel;
	float m_GroundAccel;
	float m_Gravity;
	float m_Elasticity;

	void Read(const CNetObj_DuckPhysicsLawsGroup& NetObj)
	{
		m_UID = NetObj.m_UID;
		m_AirFriction = NetObj.m_AirFriction / 256.f;
		m_GroundFriction = NetObj.m_GroundFriction / 256.f;
		m_AirMaxSpeed = NetObj.m_AirMaxSpeed / 256.f;
		m_GroundMaxSpeed = NetObj.m_GroundMaxSpeed / 256.f;
		m_AirAccel = NetObj.m_AirAccel / 256.f;
		m_GroundAccel = NetObj.m_GroundAccel / 256.f;
		m_Gravity = NetObj.m_Gravity / 256.f;
		m_Elasticity = NetObj.m_Elasticity / 256.f;
	}

	void Write(CNetObj_DuckPhysicsLawsGroup* pNetObj)
	{
		pNetObj->m_UID = m_UID;
		pNetObj->m_AirFriction = m_AirFriction * 256;
		pNetObj->m_GroundFriction = m_GroundFriction * 256;
		pNetObj->m_AirMaxSpeed = m_AirMaxSpeed * 256;
		pNetObj->m_GroundMaxSpeed = m_GroundMaxSpeed * 256;
		pNetObj->m_AirAccel = m_AirAccel * 256;
		pNetObj->m_GroundAccel = m_GroundAccel * 256;
		pNetObj->m_Gravity = m_Gravity * 256;
		pNetObj->m_Elasticity = m_Elasticity * 256;
	}
};

struct CDuckWorldCore
{
	CDuckCollision* m_pCollision;
	CWorldCore* m_pBaseWorldCore;
	CCharCoreExtra m_aBaseCoreExtras[MAX_CLIENTS];
	array<CCustomCore> m_aCustomCores;
	array<CPhysicsLawsGroup> m_aPhysicsLawsGroups;
	int m_NextUID;

	void Init(CWorldCore* pBaseWorldCore, CDuckCollision* pDuckCollison);
	void Reset();
	void Tick();
	void CharacterCore_ExtraTick(CCharacterCore* pThis, CCharCoreExtra *pThisExtra, bool UseInput);
	void CustomCore_Tick(CCustomCore *pThis);
	void CustomCore_Move(CCustomCore *pThis);
	int AddCustomCore(float Radius = -1);
	void RemoveCustomCore(int ID);

	void Snap(CGameContext* pGameServer, int SnappingClient);

	void Copy(const CDuckWorldCore* pOther);
	int FindCustomCoreFromUID(int UID);
};
