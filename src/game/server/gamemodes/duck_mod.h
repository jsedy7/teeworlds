#pragma once
#include <game/server/gamecontroller.h>

class CGameControllerDUCK : public IGameController
{
	template<typename T>
	void SendDukNetObj(const T& NetObj);

public:
	CGameControllerDUCK(class CGameContext *pGameServer);
	virtual void Tick();
	// add more virtual functions here if you wish
};
