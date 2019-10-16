#include "example_physics_3.h"
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
		BEE=0x1,
	};
};


struct CBee
{
	int m_CoreUID[2];
	CDuckWorldCore* m_pWorld;
	int m_Health;

	void Create(CDuckWorldCore* pWorld, vec2 Pos, int BeePlgUID)
	{
		m_pWorld = pWorld;

		CCustomCore* pCore1 = m_pWorld->AddCustomCore(35);
		pCore1->m_PlgUID = BeePlgUID;
		pCore1->m_Pos = Pos;

		CCustomCore* pCore2 = m_pWorld->AddCustomCore(35);
		pCore2->m_PlgUID = BeePlgUID;
		pCore2->m_Pos = Pos + vec2(70, 0);

		CDuckPhysJoint Joint;
		Joint.m_CustomCoreUID1 = pCore1->m_UID;
		Joint.m_CustomCoreUID2 = pCore2->m_UID;
		Joint.m_Force1 = 10;
		Joint.m_Force2 = 2;
		m_pWorld->m_aJoints.add(Joint);

		m_CoreUID[0] = pCore1->m_UID;
		m_CoreUID[1] = pCore2->m_UID;

		m_Health = 10;
	}

	void Snap(CGameContext* pGameServer, int SnappinClient)
	{
		CCustomCore* pCore1 = m_pWorld->FindCustomCoreFromUID(m_CoreUID[0]);
		vec2 Pos = pCore1->m_Pos + vec2(70, 0);

		if(NetworkClipped(pGameServer, SnappinClient, Pos))
			return;


	}
};

#define MAX_BEES 64
static CBee m_aBees[MAX_BEES];
static int m_BeeCount = 0;

void CGameControllerExamplePhys3::SpawnBeeAt(vec2 Pos)
{
	dbg_assert(m_BeeCount < MAX_BEES, "way too many bees dude");

	CBee Bee;
	Bee.Create(&m_DuckWorldCore, Pos, m_BeePlgUID);
	m_aBees[m_BeeCount++] = Bee;
}

CGameControllerExamplePhys3::CGameControllerExamplePhys3(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXPHYS3";
	str_copy(g_Config.m_SvMap, "duck_ex_phys_3", sizeof(g_Config.m_SvMap)); // force map

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "data/mods/example_physics_3"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	m_DuckWorldCore.Init(&GameServer()->m_World.m_Core, pCollision);
	CPhysicsLawsGroup* pPlg = m_DuckWorldCore.AddPhysicLawsGroup();
	//pPlg->m_Gravity = 0.0;
	//pPlg->m_Elasticity = 0.9;
	pPlg->m_AirFriction = 0.95;
	pPlg->m_GroundFriction = 1.0;

	m_BeePlgUID = pPlg->m_UID;
	//m_BeePlgUID = -1;
	m_BeeCount = 0;

	SpawnBeeAt(vec2(1344, 680));
}

void CGameControllerExamplePhys3::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();
}

void CGameControllerExamplePhys3::Tick()
{
	IGameController::Tick();
	m_DuckWorldCore.Tick();
}

void CGameControllerExamplePhys3::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	m_DuckWorldCore.Snap(GameServer(), SnappingClient);
}

void CGameControllerExamplePhys3::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
}
