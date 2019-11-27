#include "example_render_1.h"
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

CGameControllerExampleRender1::CGameControllerExampleRender1(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXRDR1";
	str_copy(g_Config.m_SvMap, "dm1", sizeof(g_Config.m_SvMap)); // force map

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "mods/example_render_1"))
	{
		dbg_msg("server", "failed to load duck mod");
	}
}

void CGameControllerExampleRender1::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
}

void CGameControllerExampleRender1::Tick()
{
	IGameController::Tick();
}

void CGameControllerExampleRender1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
}
