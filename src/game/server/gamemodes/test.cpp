#include "test.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <game/my_protocol.h>

// TODO: move this
template<typename T>
void CGameControllerTEST::SendDukNetObj(const T& NetObj, int CID)
{
	CMsgPacker Msg(NETMSG_DUCK_NETOBJ, false);
	Msg.AddInt((int)T::NET_ID);
	Msg.AddInt((int)sizeof(NetObj));
	Msg.AddRaw(&NetObj, sizeof(NetObj));
	Server()->SendMsg(&Msg, MSGFLAG_VITAL, CID);
}

//static CCharacterCore TestCore;

CGameControllerTEST::CGameControllerTEST(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "TEST";

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "data/mods/test"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	//GameServer()->m_World.m_Core.m_apCharacters[63] = &TestCore;
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

	CDuckCollision::CDynamicDisk Disk;
	Disk.m_Pos = vec2(250, 250);
	Disk.m_Vel = vec2(0, 0);
	Disk.m_Radius = 50;
	pCollision->SetDynamicDisk(0, Disk);

	pCollision->Tick();

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
			SendDukNetObj(Rect, p);

			CNetObj_HookBlock NetHookBlock;
			NetHookBlock.m_Id = 0;
			NetHookBlock.m_Flags = CCollision::COLFLAG_SOLID;
			NetHookBlock.m_PosX = HookBlockPos.x;
			NetHookBlock.m_PosY = HookBlockPos.y;
			NetHookBlock.m_VelX = HookBlockVel.x;
			NetHookBlock.m_VelY = HookBlockVel.y;
			NetHookBlock.m_Width = HookBlockSize.x;
			NetHookBlock.m_Height = HookBlockSize.y;
			SendDukNetObj(NetHookBlock, p);

			CNetObj_DynamicDisk NetDisk;
			NetDisk.m_Id = 0;
			NetDisk.m_Flags = Disk.m_Flags;
			NetDisk.m_PosX = Disk.m_Pos.x;
			NetDisk.m_PosY = Disk.m_Pos.y;
			NetDisk.m_VelX = Disk.m_Vel.x;
			NetDisk.m_VelY = Disk.m_Vel.y;
			NetDisk.m_Radius = Disk.m_Radius;
			NetDisk.m_HookForce = Disk.m_HookForce;
			SendDukNetObj(NetDisk, p);
		}
	}
}

void CGameControllerTEST::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	/*CNetObj_PlayerInfo *pPlayerInfo = (CNetObj_PlayerInfo *)Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, 63, sizeof(CNetObj_PlayerInfo));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
	pPlayerInfo->m_Latency = 0;
	pPlayerInfo->m_Score = 0;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, 63, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	TestCore.m_Pos = vec2(250, 250);
	TestCore.m_Vel = vec2(0, 0);
	TestCore.Write(pCharacter);*/
}
