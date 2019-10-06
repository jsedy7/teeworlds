#pragma once
#include "gamecore.h"
#include <base/tl/array.h>

struct CDuckCollision;
struct CGameContext;

struct CCoreExtra
{
	int m_HookedCurstomCoreID;
	int m_OldHookState;
	vec2 m_OldHookPos;
	float m_Radius;

	void Reset()
	{
		m_HookedCurstomCoreID = -1;
		m_OldHookState = HOOK_IDLE;
		m_Radius = 14;
	}
};

struct CDuckWorldCore
{
	struct CNetCoreCustomData
	{
		enum { NET_ID = 0x2001 };
		int m_ID;
		CCharacterCore m_Core;
		CCoreExtra m_Extra;
	};

	struct CNetCoreBaseExtraData
	{
		enum { NET_ID = 0x2002 };
		int m_ID;
		CCoreExtra m_Extra;
	};

	CDuckCollision* m_pCollision;
	CWorldCore* m_pBaseWorldCore;
	array<CCharacterCore> m_aCustomCores;
	array<CCoreExtra> m_aCoreExtras;

	void Init(CWorldCore* pBaseWorldCore, CDuckCollision* pDuckCollison);
	void Reset();
	void Tick();
	void CharacterCore_ExtraTick(CCharacterCore* pThis, CCoreExtra *pThisExtra, bool UseInput);
	void CustomCore_Tick(CCharacterCore* pThis, CCoreExtra *pThisExtra, bool UseInput);
	void CustomCore_Move(CCharacterCore* pThis, CCoreExtra *pThisExtra);
	int AddCustomCore(float Radius = -1);

	void SendAllCoreData(CGameContext* pGameServer);
	void RecvCoreCustomData(const CNetCoreCustomData& CoreData);
	void RecvCoreBaseExtraData(const CNetCoreBaseExtraData& CoreData);

	void Copy(const CDuckWorldCore* pOther);
};
