#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>

class CGameControllerExampleUI1 : public IGameController
{
	bool HasEnoughPlayers() const { return true; }
public:
	CGameControllerExampleUI1(class CGameContext *pGameServer);

	virtual void OnPlayerConnect(CPlayer* pPlayer);
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual void OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);
};
