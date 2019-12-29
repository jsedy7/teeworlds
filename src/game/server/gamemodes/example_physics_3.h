#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>
#include <game/duck_gamecore.h>

class CGameControllerExamplePhys3 : public IGameController
{
	bool HasEnoughPlayers() const { return true; }

	int m_BeePlgUID;
	int m_HivePlgUID;

	void SpawnBeeAt(vec2 Pos);
	void Reset();

public:
	CGameControllerExamplePhys3(class CGameContext *pGameServer);

	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual void OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);
	virtual void OnReset();

	friend struct CHive;
};
