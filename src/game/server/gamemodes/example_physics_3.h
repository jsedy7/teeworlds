#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>
#include <game/duck_gamecore.h>

class CGameControllerExamplePhys3 : public IGameController
{
	bool HasEnoughPlayers() const { return true; }

	CDuckWorldCore m_DuckWorldCore;
	int m_BeePlgUID;

	void SpawnBeeAt(vec2 Pos);

public:
	CGameControllerExamplePhys3(class CGameContext *pGameServer);

	virtual void OnPlayerConnect(CPlayer* pPlayer);
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual void OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);
};
