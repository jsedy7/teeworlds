#include "example_render_1.h"
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

CGameControllerExampleRender1::CGameControllerExampleRender1(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXRDR1";
	str_copy(m_aDuckMod, "example_render_1", sizeof(m_aDuckMod));
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
