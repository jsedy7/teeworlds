#include "xmas.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

CGameControllerXmas::CGameControllerXmas(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "HOHO";
	str_copy(g_Config.m_SvMap, "xmas1", sizeof(g_Config.m_SvMap)); // force dm1

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
}

void CGameControllerXmas::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{

}
