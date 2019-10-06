#pragma once
#include "gamecore.h"
#include <base/tl/array.h>

struct CDuckCollision;
struct CGameContext;

struct CCoreExtra
{
	int m_HookedCustomCoreUID;
	int m_OldHookState;
	vec2 m_OldHookPos;
	float m_Radius;

	void Reset()
	{
		m_HookedCustomCoreUID = -1;
		m_OldHookState = HOOK_IDLE;
		m_Radius = 14;
	}
};

struct CCustomCore
{
	int m_UID;
	vec2 m_Pos;
	vec2 m_Vel;

	vec2 m_HookPos;
	vec2 m_HookDir;
	int m_HookTick;
	int m_HookState;
	int m_HookedPlayer;

	CCoreExtra m_Extra;
};

struct CDuckWorldCore
{
	struct CNetCoreCustomData
	{
		enum { NET_ID = 0x2001 };
		int m_ID;
		int m_Tick;
		CCustomCore m_Core;
	};

	struct CNetCoreBaseExtraData
	{
		enum { NET_ID = 0x2002 };
		int m_ID;
		CCoreExtra m_Extra;
	};

	struct CNetClear
	{
		enum { NET_ID = 0x2003 };
	};

	CDuckCollision* m_pCollision;
	CWorldCore* m_pBaseWorldCore;
	CCoreExtra m_aBaseCoreExtras[MAX_CLIENTS];
	array<CCustomCore> m_aCustomCores;
	int m_NextUID;

	void Init(CWorldCore* pBaseWorldCore, CDuckCollision* pDuckCollison);
	void Reset();
	void Tick();
	void CharacterCore_ExtraTick(CCharacterCore* pThis, CCoreExtra *pThisExtra, bool UseInput);
	void CustomCore_Tick(CCustomCore *pThis, CCoreExtra *pThisExtra);
	void CustomCore_Move(CCustomCore *pThis, CCoreExtra *pThisExtra);
	int AddCustomCore(float Radius = -1);
	void RemoveCustomCore(int ID);

	void SendAllCoreData(CGameContext* pGameServer);
	void RecvCoreCustomData(const CNetCoreCustomData& CoreData);
	void RecvCoreBaseExtraData(const CNetCoreBaseExtraData& CoreData);
	void RecvClear(const CNetClear& NetClear);

	void Copy(const CDuckWorldCore* pOther);
	int FindCustomCoreFromUID(int UID);
};
