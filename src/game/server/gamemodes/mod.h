/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H
#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/dm.h>

// you can subclass GAMECONTROLLER_CTF, GAMECONTROLLER_TDM etc if you want
// todo a modification with their base as well.
class CGameControllerMOD : public CGameControllerDM
{
	void SendDummyInfo(int DummyCID, int ToCID);

public:
	CGameControllerMOD(class CGameContext *pGameServer);
	virtual void OnPlayerConnect(class CPlayer* pPlayer);
	virtual void Tick();
	// add more virtual functions here if you wish
	virtual void Snap(int SnapCID);
};
#endif
