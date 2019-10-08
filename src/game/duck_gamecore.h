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

	void Reset()
	{
		m_HookedCustomCoreUID = -1;
		m_OldHookState = HOOK_IDLE;
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
	int m_HookedCustomCoreUID;
	float m_Radius;
};

struct CNetObj_DuckCustomCore
{
	enum { NET_ID = 0x2001 };
	CCustomCore m_Core;
};

struct CNetObj_DuckCharCoreExtra
{
	enum { NET_ID = 0x2002 };
	CCoreExtra m_Extra;
};

struct CDuckWorldCore
{
	CDuckCollision* m_pCollision;
	CWorldCore* m_pBaseWorldCore;
	CCoreExtra m_aBaseCoreExtras[MAX_CLIENTS];
	array<CCustomCore> m_aCustomCores;
	int m_NextUID;

	void Init(CWorldCore* pBaseWorldCore, CDuckCollision* pDuckCollison);
	void Reset();
	void Tick();
	void CharacterCore_ExtraTick(CCharacterCore* pThis, CCoreExtra *pThisExtra, bool UseInput);
	void CustomCore_Tick(CCustomCore *pThis);
	void CustomCore_Move(CCustomCore *pThis);
	int AddCustomCore(float Radius = -1);
	void RemoveCustomCore(int ID);

	void Snap(CGameContext* pGameServer, int SnappingClient);

	void Copy(const CDuckWorldCore* pOther);
	int FindCustomCoreFromUID(int UID);
};
