#include "example_render_1.h"
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

CGameControllerExampleRender1::CGameControllerExampleRender1(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXRDR1";

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
