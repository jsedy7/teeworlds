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
		return true;

	if(distance(pGameServer->m_apPlayers[SnappingClient]->m_ViewPos, CheckPos) > 1100.0f)
		return true;
	return false;
}

struct CNetObj_Bee
{
	enum { NET_ID = 0x1 };
	int m_Core1ID;
	int m_Core2ID;
	int m_Health;
};

struct CBee
{
	int m_CoreUID[2];
	CDuckWorldCore* m_pWorld;
	int m_Health;
	int m_BeeID;

	void Create(CDuckWorldCore* pWorld, vec2 Pos, int BeePlgUID, int BeeID)
	{
		m_pWorld = pWorld;

		CCustomCore* pCore1 = m_pWorld->AddCustomCore(26.25);
		pCore1->m_PlgUID = BeePlgUID;
		pCore1->m_Pos = Pos;

		CCustomCore* pCore2 = m_pWorld->AddCustomCore(26.25);
		pCore2->m_PlgUID = BeePlgUID;
		pCore2->m_Pos = Pos + vec2(36, 0);

		CDuckPhysJoint Joint;
		Joint.m_CustomCoreUID1 = pCore1->m_UID;
		Joint.m_CustomCoreUID2 = pCore2->m_UID;
		Joint.m_Force1 = 4;
		Joint.m_Force2 = 2;
		m_pWorld->m_aJoints.add(Joint);

		m_CoreUID[0] = pCore1->m_UID;
		m_CoreUID[1] = pCore2->m_UID;

		m_Health = 10;
		m_BeeID = BeeID;
	}

	void Tick()
	{
		CCustomCore* pCore1 = m_pWorld->FindCustomCoreFromUID(m_CoreUID[0]);
		CCustomCore* pCore2 = m_pWorld->FindCustomCoreFromUID(m_CoreUID[1]);
		pCore1->m_Vel.y -= 1.8;
		pCore2->m_Vel.y -= 1.6;
	}

	void Snap(CGameContext* pGameServer, int SnappinClient)
	{
		int Core1ID, Core2ID;
		CCustomCore* pCore1 = m_pWorld->FindCustomCoreFromUID(m_CoreUID[0], &Core1ID);
		CCustomCore* pCore2 = m_pWorld->FindCustomCoreFromUID(m_CoreUID[1], &Core2ID);
		vec2 Pos = pCore1->m_Pos + vec2(70, 0);

		if(NetworkClipped(pGameServer, SnappinClient, Pos))
			return;

		CNetObj_Bee* pBee = pGameServer->DuckSnapNewItem<CNetObj_Bee>(m_BeeID);
		pBee->m_Core1ID = Core1ID;
		pBee->m_Core2ID = Core2ID;
		pBee->m_Health = m_Health;

		//dbg_msg("bee", "%d %d %d", pBee->m_Core1ID, pBee->m_Core2ID, pBee->m_Health);
	}
};

#define MAX_BEES 64
static CBee m_aBees[MAX_BEES];
static bool m_aBeeIsAlive[MAX_BEES];

void CGameControllerExamplePhys3::SpawnBeeAt(vec2 Pos)
{
	int BeeID = -1;

	for(int i = 0; i < MAX_BEES; i++)
	{
		if(!m_aBeeIsAlive[i]) {
			BeeID = i;
			break;
		}
	}

	dbg_assert(BeeID != -1, "way too many bees dude");

	CBee Bee;
	Bee.Create(&m_DuckWorldCore, Pos, m_BeePlgUID, BeeID);
	m_aBees[BeeID] = Bee;
	m_aBeeIsAlive[BeeID] = true;
}

CGameControllerExamplePhys3::CGameControllerExamplePhys3(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXPHYS3";
	str_copy(g_Config.m_SvMap, "duck_ex_phys_3", sizeof(g_Config.m_SvMap)); // force map

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "mods/example_physics_3"))
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
	mem_zero(m_aBeeIsAlive, sizeof(m_aBeeIsAlive));

	SpawnBeeAt(vec2(1344, 680));
	//SpawnBeeAt(vec2(1344, 780));
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

	for(int i = 0; i < MAX_BEES; i++)
	{
		if(m_aBeeIsAlive[i]) {
			m_aBees[i].Tick();
		}
	}
}

void CGameControllerExamplePhys3::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	m_DuckWorldCore.Snap(GameServer(), SnappingClient);

	for(int i = 0; i < MAX_BEES; i++)
	{
		if(m_aBeeIsAlive[i]) {
			m_aBees[i].Snap(GameServer(), SnappingClient);
		}
	}
}

void CGameControllerExamplePhys3::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
}
