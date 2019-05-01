#pragma once
#include <game/server/gamecontroller.h>

class CGameControllerTEST : public IGameController
{
public:
	CGameControllerTEST(class CGameContext *pGameServer);
	virtual void Tick();
	// add more virtual functions here if you wish
};
