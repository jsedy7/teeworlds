#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>
#include <game/duck_gamecore.h>

class CGameControllerXmas : public IGameController
{
	bool HasEnoughPlayers() const { return true; }

public:
	CGameControllerXmas(class CGameContext *pGameServer);

	virtual void OnReset();
	virtual void OnPlayerConnect(CPlayer* pPlayer);
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual void OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	void OnCharacterHammerHit(vec2 Pos);
};
