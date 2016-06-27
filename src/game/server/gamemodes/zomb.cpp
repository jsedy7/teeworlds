// LordSk
#include "zomb.h"
#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/protocol.h>
#include <game/server/entity.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#define dbg_cout(...) GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "zomb", ##__VA_ARGS__);

CGameControllerZOMB::CGameControllerZOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "ZOMB";
	m_ZombCount = 4;

	for(int i = 0; i < m_ZombCount; ++i) {
		int zombID = MAX_CLIENTS - i - 1;
		CCharacterCore& core = m_ZombCharCore[i];
		core.Reset();
		core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
		core.m_Pos = vec2(250 + 100 * i, 250);
		GameServer()->m_World.m_Core.m_apCharacters[zombID] = &core;
	}
}

void CGameControllerZOMB::Tick()
{
	IGameController::Tick();


}

void CGameControllerZOMB::Snap(int SnappingClientID)
{
	IGameController::Snap(SnappingClientID);
	CEntity* pChar = (CEntity*)GameServer()->GetPlayerChar(SnappingClientID);
	if(!pChar || pChar->NetworkClipped(SnappingClientID)) {
		return;
	}

	// send zombie player and character infos
	for(int i = 0; i < m_ZombCount; ++i) {
		int zombID = MAX_CLIENTS - 1 - i;

		CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->
				SnapNewItem(NETOBJTYPE_PLAYERINFO, zombID, sizeof(CNetObj_PlayerInfo)));
		if(!pPlayerInfo) {
			dbg_cout("Error: failed to SnapNewItem(NETOBJTYPE_PLAYERINFO)");
			return;
		}

		pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
		pPlayerInfo->m_Latency = zombID;
		pPlayerInfo->m_Score = zombID;

		CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->
				SnapNewItem(NETOBJTYPE_CHARACTER, zombID, sizeof(CNetObj_Character)));
		if(!pCharacter) {
			dbg_cout("Error: failed to SnapNewItem(NETOBJTYPE_CHARACTER)");
			return;
		}

		pCharacter->m_Tick = Server()->Tick();

		pCharacter->m_Emote = EMOTE_HAPPY;

		pCharacter->m_AmmoCount = 0;
		pCharacter->m_Health = 10;
		pCharacter->m_Armor = 0;

		//pCharacter->m_TriggeredEvents = m_TriggeredEvents;
		//pCharacter->m_Weapon = m_ActiveWeapon;
		//pCharacter->m_AttackTick = m_AttackTick;

		m_ZombCharCore[i].Write(pCharacter);
	}
}

void CGameControllerZOMB::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);

	// send zombie client informations
	for(int i = 0; i < m_ZombCount; ++i) {
		int zombID = MAX_CLIENTS - 1 - i;
		CNetMsg_Sv_ClientInfo NewClientInfoMsg;
		NewClientInfoMsg.m_ClientID = zombID;
		NewClientInfoMsg.m_Local = 0;
		NewClientInfoMsg.m_Team = 0;
		NewClientInfoMsg.m_pName = "zombie";
		NewClientInfoMsg.m_pClan = "";
		NewClientInfoMsg.m_Country = 0;
		for(int p = 0; p < 6; p++)
		{
			NewClientInfoMsg.m_apSkinPartNames[p] = "standard";
			NewClientInfoMsg.m_aUseCustomColors[p] = 0;
			NewClientInfoMsg.m_aSkinPartColors[p] = 0;
		}

		Server()->SendPackMsg(&NewClientInfoMsg, MSGFLAG_VITAL|MSGFLAG_NORECORD, pPlayer->GetCID());
	}

	dbg_cout("Sending test zombie info");
}
