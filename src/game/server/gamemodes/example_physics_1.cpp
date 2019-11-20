#include "example_physics_1.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

CGameControllerExamplePhys1::CGameControllerExamplePhys1(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXPHYS1";
	str_copy(g_Config.m_SvMap, "dm1", sizeof(g_Config.m_SvMap)); // force dm1

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "mods/example_physics_1"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	m_DuckWorldCore.Init(&GameServer()->m_World.m_Core, pCollision);

	CPhysicsLawsGroup* pPlg = m_DuckWorldCore.AddPhysicLawsGroup();
	pPlg->m_AirFriction = 1.0;
	pPlg->m_GroundFriction = 1.0;

	CCustomCore* pTestCore1 = m_DuckWorldCore.AddCustomCore(40);
	pTestCore1->SetPhysicLawGroup(pPlg);
	pTestCore1->m_Pos = vec2(500, 280);

	CCustomCore* pTestCore2 = m_DuckWorldCore.AddCustomCore(30);
	pTestCore2->SetPhysicLawGroup(pPlg);
	pTestCore2->m_Pos = vec2(300, 280);

	CCustomCore* pTestCore3 = m_DuckWorldCore.AddCustomCore(30);
	pTestCore3->SetPhysicLawGroup(pPlg);
	pTestCore3->m_Pos = vec2(400, 280);

	CDuckPhysJoint Joint;
	Joint.m_CustomCoreUID1 = pTestCore1->m_UID;
	Joint.m_CustomCoreUID2 = pTestCore2->m_UID;
	Joint.m_Force1 = 2;
	Joint.m_Force2 = 0.1;
	m_DuckWorldCore.m_aJoints.add(Joint);
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
}

void CGameControllerExamplePhys1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	m_DuckWorldCore.Snap(GameServer(), SnappingClient);
}

void CGameControllerExamplePhys1::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
}
