#include "xmas.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

struct CNetObj_Santa
{
	enum { NET_ID = 0x1 };
	float m_PosX;
	float m_PosY;
};

struct CSanta
{
	CDuckWorldCore* m_pWorld;
	CCustomCore* m_pSantaCore;
	CCustomCore* m_pBagCore;
	int m_PlgFlyUID;

	void Init(CDuckWorldCore* pWorld)
	{
		m_pWorld = pWorld;

		m_pBagCore = m_pWorld->AddCustomCore(50);
		m_pSantaCore = m_pWorld->AddCustomCore(20);

		CPhysicsLawsGroup* pPlgFly = m_pWorld->AddPhysicLawsGroup();
		pPlgFly->m_AirAccel = 1;
		pPlgFly->m_AirFriction = 1;
		pPlgFly->m_Gravity = 0;

		m_PlgFlyUID = pPlgFly->m_UID;
		m_pSantaCore->SetPhysicLawGroup(pPlgFly);
		m_pBagCore->SetPhysicLawGroup(pPlgFly);

		m_pSantaCore->m_Pos.x = 5280;
		m_pSantaCore->m_Pos.y = 1696;
		m_pSantaCore->m_Vel = vec2(0, 0);
	}

	void Start()
	{
		m_pSantaCore->m_Pos.x = 5280;
		m_pSantaCore->m_Pos.y = 1696;
		m_pSantaCore->m_Vel = vec2(-10, 0);
		m_pBagCore->m_PlgUID = m_PlgFlyUID;
	}

	void Tick()
	{
		if(m_pSantaCore->m_Pos.x < 3300)
		{
			if(m_pBagCore->m_PlgUID != -1)
			{
				dbg_msg("xmas", "DROP");
				m_pBagCore->m_PlgUID = -1;
				m_pBagCore->m_Vel = vec2(0,-10);
			}
		}
		else
		{
			m_pBagCore->m_Pos = m_pSantaCore->m_Pos + vec2(120, -20);
			m_pBagCore->m_Vel = m_pSantaCore->m_Vel;
		}

	}

	void Snap(CGameContext* pServer, int SnappinClient)
	{
		//CNetObj_Santa* pSanta = pServer->DuckSnapNewItem<CNetObj_Santa>(0);
		//pSanta->m_PosX = m_pSantaCore->m_Pos.x;
		//pSanta->m_PosY = m_pSantaCore->m_Pos.y;
	}
};

static CSanta santa;

CGameControllerXmas::CGameControllerXmas(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "HOHO";
	str_copy(g_Config.m_SvMap, "xmas", sizeof(g_Config.m_SvMap)); // force dm1

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "mods/xmas"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	CDuckCollision* pCollision = (CDuckCollision*)GameServer()->Collision();
	m_DuckWorldCore.Init(&GameServer()->m_World.m_Core, pCollision);

	santa.Init(&m_DuckWorldCore);
}

void CGameControllerXmas::OnPlayerConnect(CPlayer* pPlayer)
{
}

void CGameControllerXmas::Tick()
{
	IGameController::Tick();

	m_DuckWorldCore.Tick();
	santa.Tick();
}

void CGameControllerXmas::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	m_DuckWorldCore.Snap(GameServer(), SnappingClient);
	santa.Snap(GameServer(), SnappingClient);
}

void CGameControllerXmas::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	if(MsgID == 0x1)
	{
		dbg_msg("xmas", "START");
		santa.Start();
	}
}
