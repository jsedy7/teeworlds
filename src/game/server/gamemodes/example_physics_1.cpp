#include "example_physics_1.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

// TODO: move
inline bool NetworkClipped(CGameContext* pGameServer, int SnappingClient, vec2 CheckPos)
{
	float dx = pGameServer->m_apPlayers[SnappingClient]->m_ViewPos.x-CheckPos.x;
	float dy = pGameServer->m_apPlayers[SnappingClient]->m_ViewPos.y-CheckPos.y;

	if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
		return 1;

	if(distance(pGameServer->m_apPlayers[SnappingClient]->m_ViewPos, CheckPos) > 1100.0f)
		return 1;
}

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
	m_DuckWorldCore.Init(&GameServer()->m_World.m_Core, pCollision);
	CPhysicsLawsGroup* pPlg = m_DuckWorldCore.AddPhysicLawsGroup();
	//pPlg->m_Gravity = 0.0;
	//pPlg->m_Elasticity = 0.9;
	pPlg->m_AirFriction = 1.0;
	pPlg->m_GroundFriction = 1.0;

	CCustomCore* pTestCore1 = m_DuckWorldCore.AddCustomCore(40);
	pTestCore1->SetPhysicLawGroup(*pPlg);
	pTestCore1->m_Pos = vec2(500, 280);

	CCustomCore* pTestCore2 = m_DuckWorldCore.AddCustomCore(30);
	pTestCore2->SetPhysicLawGroup(*pPlg);
	pTestCore2->m_Pos = vec2(300, 280);

	CDuckPhysJoint Joint;
	Joint.m_CustomCoreUID1 = pTestCore1->m_UID;
	Joint.m_CustomCoreUID2 = pTestCore2->m_UID;
	Joint.m_Force1 = 2;
	Joint.m_Force2 = 0.1;
	m_DuckWorldCore.m_aJoints.add(Joint);

	/*for(int i = 0; i < 100; i++)
	{
		int CoreID = m_DuckWorldCore.AddCustomCore(15 + (random_int() % 25));
		m_DuckWorldCore.m_aCustomCores[CoreID].m_Pos = vec2(500 + (random_int() % 25), 280);
	}*/
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

	/*if(random_int() % 40 == 0)
	{
		int CoreID = m_DuckWorldCore.AddCustomCore(15 + (random_int() % 25));
		m_DuckWorldCore.m_aCustomCores[CoreID].m_Pos = vec2(500 + (random_int() % 25), 280);
	}

	if(random_int() % 50 == 0 && m_DuckWorldCore.m_aCustomCores.size() > 0)
	{
		m_DuckWorldCore.RemoveCustomCore(random_int() % m_DuckWorldCore.m_aCustomCores.size());
	}*/
}

void CGameControllerExamplePhys1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	m_DuckWorldCore.Snap(GameServer(), SnappingClient);
}

void CGameControllerExamplePhys1::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
}
