/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "mod.h"
#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

#define PLAYER_COUNT 10

void CGameControllerMOD::SendDummyInfo(int DummyCID, int ToCID)
{
	enum
	{
		SKINPART_BODY = 0,
		SKINPART_MARKING,
		SKINPART_DECORATION,
		SKINPART_HANDS,
		SKINPART_FEET,
		SKINPART_EYES,
		NUM_SKINPARTS,
	};

	CNetMsg_Sv_ClientDrop Msg;
	Msg.m_ClientID = DummyCID;
	Msg.m_pReason = "";
	Msg.m_Silent = 1;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, ToCID);

	// then update
	CNetMsg_Sv_ClientInfo nci;
	nci.m_ClientID = DummyCID;
	nci.m_Local = 0;
	nci.m_Team = DummyCID&1;
	nci.m_pClan = "";
	nci.m_Country = 0;
	nci.m_Silent = 1;
	nci.m_pName = "dummy";
	nci.m_apSkinPartNames[SKINPART_DECORATION] = "standard";
	nci.m_aUseCustomColors[SKINPART_DECORATION] = 0;
	nci.m_apSkinPartNames[SKINPART_EYES] = "standard";
	nci.m_aUseCustomColors[SKINPART_EYES] = 0;
	nci.m_apSkinPartNames[SKINPART_MARKING] = "standard";
	nci.m_aUseCustomColors[SKINPART_MARKING] = 0;
	nci.m_apSkinPartNames[SKINPART_BODY] = "standard";
	nci.m_aUseCustomColors[SKINPART_BODY] = 0;
	nci.m_apSkinPartNames[SKINPART_HANDS] = "standard";
	nci.m_aUseCustomColors[SKINPART_HANDS] = 0;
	nci.m_apSkinPartNames[SKINPART_FEET] = "standard";
	nci.m_aUseCustomColors[SKINPART_FEET] = 0;

	//dbg_msg("dummy", "(DummyCID=%d CID=%d)", DummyCID, ToCID);
	Server()->SendPackMsg(&nci, MSGFLAG_VITAL|MSGFLAG_NORECORD, ToCID);
}

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: CGameControllerDM(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "MOD";
	m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void CGameControllerMOD::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	dbg_msg("dummy", "OnPlayerConnect(CID=%d)", pPlayer->GetCID());

	for(int i = PLAYER_COUNT; i < MAX_CLIENTS; i++)
	{
		SendDummyInfo(i, pPlayer->GetCID());
	}
}

void CGameControllerMOD::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	CGameControllerDM::Tick();
}

void CGameControllerMOD::Snap(int SnapCID)
{
	CGameControllerDM::Snap(SnapCID);

	if(SnapCID < PLAYER_COUNT)
	{
		vec2 Pos = vec2(100, 100);
		if(GameServer()->m_apPlayers[SnapCID]->GetCharacter())
			Pos = GameServer()->m_apPlayers[SnapCID]->GetCharacter()->GetPos();

		for(int i = PLAYER_COUNT; i < MAX_CLIENTS; i++)
		{
			CNetObj_PlayerInfo *pPlayerInfo = (CNetObj_PlayerInfo *)Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, i, sizeof(CNetObj_PlayerInfo));
			if(!pPlayerInfo)
				return;

			pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
			pPlayerInfo->m_Latency = i;
			pPlayerInfo->m_Score = i;

			CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, i, sizeof(CNetObj_Character)));
			if(!pCharacter)
				return;

			CCharacterCore Core;
			Core.Reset();
			Core.m_Pos = Pos + vec2(64 * (i%10), 64* (i/10));
			Core.Write(pCharacter);
		}
	}
}
