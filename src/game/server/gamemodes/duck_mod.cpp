#include "duck_mod.h"

CGameControllerDUCK::CGameControllerDUCK(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "DUCK";

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void CGameControllerDUCK::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();
}
