#include "example_physics_1.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

struct ModNetID
{
	enum Enum {
		DISK=0x1,
	};
};

struct CNetObj_ModDynamicDisk
{
	enum { NET_ID = ModNetID::DISK };

	int m_Id;
	int m_Flags;
	float m_PosX;
	float m_PosY;
	float m_VelX;
	float m_VelY;
	float m_Radius;
	float m_HookForce;
};

CGameControllerExamplePhys1::CGameControllerExamplePhys1(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXPHYS1";
	str_copy(g_Config.m_SvMap, "dm1", sizeof(g_Config.m_SvMap)); // force dm1

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "data/mods/example_physics_1"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	CDuckCollision::CDynamicDisk Disk;
	Disk.m_Pos = vec2(700, 280);
	Disk.m_Vel = vec2(0, 0);
	Disk.m_Radius = 30;
	pCollision->SetDynamicDisk(0, Disk);
}

void CGameControllerExamplePhys1::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();
}

void CGameControllerExamplePhys1::Tick()
{
	IGameController::Tick();

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	CDuckCollision::CDynamicDisk& Disk = *pCollision->GetDynamicDisk(0);
	Disk.Tick(pCollision, &GameServer()->m_World.m_Core);
	Disk.Move(pCollision, &GameServer()->m_World.m_Core);

	for(int p = 0; p < MAX_PLAYERS; p++)
	{
		if(GameServer()->m_apPlayers[p])
		{
			if(GameServer()->m_apPlayers[p]->IsDummy())
				continue;

			CNetObj_ModDynamicDisk NetDisk;
			NetDisk.m_Id = 0;
			NetDisk.m_Flags = Disk.m_Flags;
			NetDisk.m_PosX = Disk.m_Pos.x;
			NetDisk.m_PosY = Disk.m_Pos.y;
			NetDisk.m_VelX = Disk.m_Vel.x;
			NetDisk.m_VelY = Disk.m_Vel.y;
			NetDisk.m_Radius = Disk.m_Radius;
			NetDisk.m_HookForce = Disk.m_HookForce;
			GameServer()->SendDuckNetObj(NetDisk, p);
		}
	}
}

void CGameControllerExamplePhys1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
}

void CGameControllerExamplePhys1::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
}
