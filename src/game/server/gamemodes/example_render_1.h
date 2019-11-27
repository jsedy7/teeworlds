#pragma once
#include <game/server/gamecontroller.h>

class CGameControllerExampleRender1 : public IGameController
{
	bool HasEnoughPlayers() const { return true; }

public:
	CGameControllerExampleRender1(class CGameContext *pGameServer);

	virtual void OnPlayerConnect(CPlayer* pPlayer);
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};
