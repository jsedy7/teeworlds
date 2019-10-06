#pragma once
#include "gamecore.h"
#include <base/tl/array.h>

struct CDuckCollision;
struct CGameContext;

struct CCharacterCoreAddInfo
{
	int m_HookedAddCharChore;
	int m_OldHookState;
	vec2 m_OldHookPos;

	void Reset()
	{
		m_HookedAddCharChore = -1;
		m_OldHookState = HOOK_IDLE;
	}
};

struct CDuckWorldCore
{
	struct CNetCoreAddiData
	{
		enum { NET_ID = 0x2001 };
		int m_ID;
		CCharacterCore m_Core;
		CCharacterCoreAddInfo m_AddInfo;
	};

	struct CNetCoreBaseExtraData
	{
		enum { NET_ID = 0x2002 };
		int m_ID;
		CCharacterCoreAddInfo m_AddInfo;
	};

	CDuckCollision* m_pCollision;
	CWorldCore* m_pBaseWorldCore;
	array<CCharacterCore> m_aAdditionalCharCores;
	array<CCharacterCoreAddInfo> m_aAdditionalCharCoreInfos;

	void Init(CWorldCore* pBaseWorldCore, CDuckCollision* pDuckCollison);
	void Reset();
	void Tick();
	void CCharacterCore_Tick(CCharacterCore* pThis, CCharacterCoreAddInfo *pThisAddInfo, bool UseInput);
	int AddCharCore();

	void SendAllCoreData(CGameContext* pGameServer);
	void RecvCoreAddiData(const CNetCoreAddiData& CoreData);
	void RecvCoreBaseExtraData(const CNetCoreBaseExtraData& CoreData);

	void Copy(const CDuckWorldCore* pOther);
};
