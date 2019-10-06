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
		CHARCORE,
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

struct CNetObj_DuckCharCore
{
	enum { NET_ID = ModNetID::CHARCORE };

	int m_ID;
	float m_X;
	float m_Y;
	float m_VelX;
	float m_VelY;
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
	/*CDuckCollision::CDynamicDisk Disk;
	Disk.m_Pos = vec2(500, 280);
	Disk.m_Vel = vec2(0, 0);
	Disk.m_Radius = 30;
	pCollision->SetDynamicDisk(0, Disk);*/

	m_DuckWorldCore.Init(&GameServer()->m_World.m_Core, pCollision);
	m_TestCoreID = m_DuckWorldCore.AddCustomCore(40);
	m_DuckWorldCore.m_aCustomCores[m_TestCoreID].m_Pos = vec2(500, 280);
}

void CGameControllerExamplePhys1::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();
}

void CGameControllerExamplePhys1::Tick()
{
	IGameController::Tick();
	m_DuckWorldCore.Tick();
	m_DuckWorldCore.SendAllCoreData(GameServer());

	/*CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	CDuckCollision::CDynamicDisk& Disk = *pCollision->GetDynamicDisk(0);
	Disk.Tick(pCollision, &GameServer()->m_World.m_Core);
	Disk.Move(pCollision, &GameServer()->m_World.m_Core);*/


}

void CGameControllerExamplePhys1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
}

void CGameControllerExamplePhys1::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
}
