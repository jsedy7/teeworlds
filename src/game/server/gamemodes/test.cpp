#include "test.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

typedef uint32_t u32;
typedef int32_t i32;

struct ModNetID
{
	enum Enum {
		TEST=0,
		DEBUG_RECT,
		MAP_RECT_SET_SOLID,

		HOOK_BLOCK,
		DYNAMIC_DISK,

		_COUNT
	};
};

struct CNetObj_DebugRect
{
	enum { NET_ID = ModNetID::DEBUG_RECT };

	int id;
	float x;
	float y;
	float w;
	float h;
	u32 color;
};

struct CNetObj_HookBlock
{
	enum { NET_ID = ModNetID::HOOK_BLOCK };

	int m_Id;
	int m_Flags;
	float m_PosX;
	float m_PosY;
	float m_VelX;
	float m_VelY;
	float m_Width;
	float m_Height;
};

struct CNetObj_DynamicDisk
{
	enum { NET_ID = ModNetID::DYNAMIC_DISK };

	int m_Id;
	int m_Flags;
	float m_PosX;
	float m_PosY;
	float m_VelX;
	float m_VelY;
	float m_Radius;
	float m_HookForce;
};

//#define CHARCHORE_TEST

static CCharacterCore TestCore;

CGameControllerTEST::CGameControllerTEST(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "TEST";

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "data/mods/test"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();

#ifdef CHARCHORE_TEST
	GameServer()->m_World.m_Core.m_apCharacters[63] = &TestCore;
	TestCore.Init(&GameServer()->m_World.m_Core, pCollision);
	TestCore.m_Pos = vec2(700, 280);
#else
	CDuckCollision::CDynamicDisk Disk;
	Disk.m_Pos = vec2(700, 280);
	Disk.m_Vel = vec2(0, 0);
	Disk.m_Radius = 30;
	pCollision->SetDynamicDisk(0, Disk);
#endif
}

void CGameControllerTEST::Tick()
{
	IGameController::Tick();

	double Time = time_get()/(double)time_freq();
	double a = sin(Time) * 0.5 + 0.5;

	vec2 HookBlockPos = vec2(800 - 200 * a, 512);
	vec2 HookBlockSize = vec2(100 + 400 * a, 64);
	vec2 HookBlockVel = vec2(0, 0);

	CDuckCollision::CStaticBlock SolidBlock;
	SolidBlock.m_Pos = HookBlockPos;
	SolidBlock.m_Size = HookBlockSize;
	SolidBlock.m_Flags = CCollision::COLFLAG_SOLID;

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	pCollision->SetStaticBlock(0, SolidBlock);

#ifdef CHARCHORE_TEST
	TestCore.m_Pos.y = 280;
	TestCore.m_Vel = vec2(sign(sin(Time)) * 10.f, 0);
	TestCore.Tick(false);
	TestCore.Move();
#else
	CDuckCollision::CDynamicDisk& Disk = *pCollision->GetDynamicDisk(0);
	Disk.m_Vel = vec2(sign(sin(Time)) * 7.f, 0);
	Disk.Tick(pCollision, &GameServer()->m_World.m_Core);
	Disk.Move(pCollision, &GameServer()->m_World.m_Core);
#endif


	for(int p = 0; p < MAX_PLAYERS; p++)
	{
		if(GameServer()->m_apPlayers[p])
		{
			if(GameServer()->m_apPlayers[p]->IsDummy())
				continue;

			CNetObj_DebugRect Rect;
			Rect.id = 0;
			Rect.x = HookBlockPos.x;
			Rect.y = HookBlockPos.y;
			Rect.w = HookBlockSize.x;
			Rect.h = HookBlockSize.y;
			Rect.color = 0xffff00ff;
			GameServer()->SendDuckNetObj(Rect, p);

			CNetObj_HookBlock NetHookBlock;
			NetHookBlock.m_Id = 0;
			NetHookBlock.m_Flags = CCollision::COLFLAG_SOLID;
			NetHookBlock.m_PosX = HookBlockPos.x;
			NetHookBlock.m_PosY = HookBlockPos.y;
			NetHookBlock.m_VelX = HookBlockVel.x;
			NetHookBlock.m_VelY = HookBlockVel.y;
			NetHookBlock.m_Width = HookBlockSize.x;
			NetHookBlock.m_Height = HookBlockSize.y;
			GameServer()->SendDuckNetObj(NetHookBlock, p);

#ifdef CHARCHORE_TEST
#else
			CNetObj_DynamicDisk NetDisk;
			NetDisk.m_Id = 0;
			NetDisk.m_Flags = Disk.m_Flags;
			NetDisk.m_PosX = Disk.m_Pos.x;
			NetDisk.m_PosY = Disk.m_Pos.y;
			NetDisk.m_VelX = Disk.m_Vel.x;
			NetDisk.m_VelY = Disk.m_Vel.y;
			NetDisk.m_Radius = Disk.m_Radius;
			NetDisk.m_HookForce = Disk.m_HookForce;
			GameServer()->SendDuckNetObj(NetDisk, p);
#endif
		}
	}
}

void CGameControllerTEST::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

#ifdef CHARCHORE_TEST
	CNetObj_PlayerInfo *pPlayerInfo = (CNetObj_PlayerInfo *)Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, 63, sizeof(CNetObj_PlayerInfo));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
	pPlayerInfo->m_Latency = 0;
	pPlayerInfo->m_Score = 0;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, 63, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	TestCore.Write(pCharacter);
#endif

	// this gets invalidated
	/*
	CNetObj_DebugRect* pRect2 = (CNetObj_DebugRect*)Server()->SnapNewItem(NUM_NETOBJTYPES + CNetObj_DebugRect::NET_ID, 0, sizeof(CNetObj_DebugRect));

	pRect2->id = 0;
	pRect2->x = 0;
	pRect2->y = 0;
	pRect2->w = 1000;
	pRect2->h = 1000;
	pRect2->color = 0xff0000ff;
	*/
}

void CGameControllerTEST::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
	if(MsgID == 0x1) {
		const char* pStr = pUnpacker->GetString(0);
		dbg_msg("duck", "'%s'", pStr);
	}
}
