// LordSk
#include "zomb.h"
#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/protocol.h>
#include <game/server/entity.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#define dgb_zomb_msg(...) GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "zomb", ##__VA_ARGS__);

CGameControllerZOMB::CGameControllerZOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "ZOMB";
	m_ZombCount = MAX_ZOMBS;

	// init zombies
	mem_zero(m_ZombInput, sizeof(CNetObj_PlayerInput) * MAX_ZOMBS);
	mem_zero(m_ZombActiveWeapon, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombAttackTick, sizeof(i32) * MAX_ZOMBS);

	for(u32 i = 0; i < m_ZombCount; ++i) {
		u32 zombCID = MAX_CLIENTS - i - 1;
		CCharacterCore& core = m_ZombCharCore[i];
		core.Reset();
		core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
		core.m_Pos = vec2(250 + 50 * i, 250);
		GameServer()->m_World.m_Core.m_apCharacters[zombCID] = &core;
	}
}

void CGameControllerZOMB::Tick()
{
	IGameController::Tick();

	for(u32 i = 0; i < m_ZombCount; ++i) {
		m_ZombCharCore[i].m_Input = m_ZombInput[i];
		m_ZombCharCore[i].Tick(true);
		m_ZombCharCore[i].Move();
	}
}

void CGameControllerZOMB::Snap(i32 SnappingClientID)
{
	IGameController::Snap(SnappingClientID);
	CEntity* pChar = (CEntity*)GameServer()->GetPlayerChar(SnappingClientID);
	if(!pChar || pChar->NetworkClipped(SnappingClientID)) {
		return;
	}

	// send zombie player and character infos
	for(u32 i = 0; i < m_ZombCount; ++i) {
		u32 zombCID = MAX_CLIENTS - 1 - i;

		CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->
				SnapNewItem(NETOBJTYPE_PLAYERINFO, zombCID, sizeof(CNetObj_PlayerInfo)));
		if(!pPlayerInfo) {
			dgb_zomb_msg("Error: failed to SnapNewItem(NETOBJTYPE_PLAYERINFO)");
			return;
		}

		pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
		pPlayerInfo->m_Latency = zombCID;
		pPlayerInfo->m_Score = zombCID;

		CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->
				SnapNewItem(NETOBJTYPE_CHARACTER, zombCID, sizeof(CNetObj_Character)));
		if(!pCharacter) {
			dgb_zomb_msg("Error: failed to SnapNewItem(NETOBJTYPE_CHARACTER)");
			return;
		}

		pCharacter->m_Tick = Server()->Tick();
		pCharacter->m_Emote = EMOTE_NORMAL;
		pCharacter->m_TriggeredEvents = m_ZombCharCore[i].m_TriggeredEvents;
		pCharacter->m_Weapon = m_ZombActiveWeapon[i];
		pCharacter->m_AttackTick = m_ZombAttackTick[i];

		m_ZombCharCore[i].Write(pCharacter);
	}
}

void CGameControllerZOMB::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);

	// send zombie client informations
	for(u32 i = 0; i < m_ZombCount; ++i) {
		u32 zombID = MAX_CLIENTS - 1 - i;
		CNetMsg_Sv_ClientInfo NewClientInfoMsg;
		NewClientInfoMsg.m_ClientID = zombID;
		NewClientInfoMsg.m_Local = 0;
		NewClientInfoMsg.m_Team = 0;
		NewClientInfoMsg.m_pName = "zombie";
		NewClientInfoMsg.m_pClan = "";
		NewClientInfoMsg.m_Country = 0;
		for(i32 p = 0; p < 6; p++)
		{
			NewClientInfoMsg.m_apSkinPartNames[p] = "standard";
			NewClientInfoMsg.m_aUseCustomColors[p] = 0;
			NewClientInfoMsg.m_aSkinPartColors[p] = 0;
		}

		Server()->SendPackMsg(&NewClientInfoMsg, MSGFLAG_VITAL|MSGFLAG_NORECORD, pPlayer->GetCID());
	}

	dgb_zomb_msg("Sending test zombie info");
}
