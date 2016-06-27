#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>

#define MAX_SURVIVORS 4
#define MAX_ZOMBS (MAX_CLIENTS - MAX_SURVIVORS)

class CGameControllerZOMB : public IGameController
{
	int m_ZombCount;
	CCharacterCore m_ZombCharCore[MAX_ZOMBS];

public:
	CGameControllerZOMB(class CGameContext *pGameServer);
	virtual void Tick();
	virtual void Snap(int SnappingClientID);
	virtual void OnPlayerConnect(CPlayer *pPlayer);
};
