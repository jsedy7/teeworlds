#pragma once
#include "gamecore.h"
#include <base/tl/array.h>

struct CDuckCollision;
struct CGameContext;

struct CNetObj_DuckCustomCore
{
	enum { NET_ID = 0x2001 };

	int m_UID;
	int m_PosX;
	int m_PosY;
	int m_VelX;
	int m_VelY;
	int m_IsHooked;
	int m_HookPosX;
	int m_HookPosY;
	int m_HookedCustomCoreUID;
	int m_Radius;
};

struct CNetObj_DuckCharCoreExtra
{
	enum { NET_ID = 0x2002 };
	int m_HookedCustomCoreUID;
};

struct CCharCoreExtra
{
	int m_HookedCustomCoreUID;
	int m_OldHookState;

	void Reset()
	{
		m_HookedCustomCoreUID = -1;
		m_OldHookState = HOOK_IDLE;
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
	int m_UID;
	vec2 m_Pos;
	vec2 m_Vel;
	vec2 m_HookPos;
	bool m_IsHooked;
	int m_HookedCustomCoreUID;
	float m_Radius;

	void Read(const CNetObj_DuckCustomCore& NetObj)
	{
		m_UID = NetObj.m_UID;
		m_Pos.x = NetObj.m_PosX / 256.f;
		m_Pos.y = NetObj.m_PosY / 256.f;
		m_Vel.x = NetObj.m_VelX / 256.f;
		m_Vel.y = NetObj.m_VelY / 256.f;
		m_HookPos.x = NetObj.m_HookPosX / 256.f;
		m_HookPos.y = NetObj.m_HookPosY / 256.f;
		m_IsHooked = NetObj.m_IsHooked;
		m_HookedCustomCoreUID = NetObj.m_HookedCustomCoreUID;
		m_Radius = NetObj.m_Radius / 256.f;
	}

	void Write(CNetObj_DuckCustomCore* pNetObj)
	{
		pNetObj->m_UID = m_UID;
		pNetObj->m_PosX = m_Pos.x * 256.f;
		pNetObj->m_PosY = m_Pos.y * 256.f;
		pNetObj->m_VelX = m_Vel.x * 256.f;
		pNetObj->m_VelY = m_Vel.y * 256.f;
		pNetObj->m_HookPosX = m_HookPos.x * 256.f;
		pNetObj->m_HookPosY = m_HookPos.y * 256.f;
		pNetObj->m_IsHooked = m_IsHooked;
		pNetObj->m_HookedCustomCoreUID = m_HookedCustomCoreUID;
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
		m_Pos = vec2(0,0);
		m_Vel = vec2(0,0);
		m_HookPos = vec2(0,0);
		m_IsHooked = false;
		m_HookedCustomCoreUID = -1;
		m_Radius = 20;
	}
};

struct CDuckWorldCore
{
	CDuckCollision* m_pCollision;
	CWorldCore* m_pBaseWorldCore;
	CCharCoreExtra m_aBaseCoreExtras[MAX_CLIENTS];
	array<CCustomCore> m_aCustomCores;
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
