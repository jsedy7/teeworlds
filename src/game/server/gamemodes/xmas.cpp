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
	vec2 m_Pos;

	void Tick()
	{
	}

	void Snap(CGameContext* pServer, int SnappinClient)
	{
		const int reindeerCount = 6;

		CNetObj_Santa* pSanta = pServer->DuckSnapNewItem<CNetObj_Santa>(0);
		m_Pos.x = 3392;
		m_Pos.y = 1696;
		pSanta->m_PosX = m_Pos.x;
		pSanta->m_PosY = m_Pos.y;
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
}

void CGameControllerXmas::OnPlayerConnect(CPlayer* pPlayer)
{
}

void CGameControllerXmas::Tick()
{
	IGameController::Tick();
}

void CGameControllerXmas::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	santa.Snap(GameServer(), SnappingClient);
}

void CGameControllerXmas::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{

}
