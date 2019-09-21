#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>

class CGameControllerTEST : public IGameController
{
	template<typename T>
	void SendDukNetObj(const T& NetObj, int CID);

public:
	CGameControllerTEST(class CGameContext *pGameServer);
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	virtual void OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);
};
