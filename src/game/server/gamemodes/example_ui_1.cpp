#include "example_ui_1.h"
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

struct ModNetID
{
	enum Enum {
		NPC_DIALOG_LINE=0x1,

	};
};

struct CModNetObj_NPCDialogLine
{
	enum { NET_ID = ModNetID::NPC_DIALOG_LINE };

	int m_NpcClientID; // NPC client ID
	char m_aDialogLine[128];
};

struct CNPC
{
	int m_ClientID;
	char m_aName[32];
	CCharacterCore m_Core;
	int m_aDialogLineID[MAX_PLAYERS];

	struct CDialogLine
	{
		char aLine[128];

		CDialogLine() {}
		CDialogLine(const char* pStr)
		{
			str_copy(aLine, pStr, sizeof(aLine));
		}
	};

	array<CDialogLine> m_aDialogLines;

	void Init(int ClientID, const char* pName, CGameContext* pGameServer)
	{
		m_ClientID = ClientID;
		str_copy(m_aName, pName, sizeof(m_aName));
		m_Core.Reset();
		m_Core.Init(&pGameServer->m_World.m_Core, pGameServer->Collision());
		pGameServer->m_World.m_Core.m_apCharacters[m_ClientID] = &m_Core;
	}

	CNetMsg_Sv_ClientInfo GetClientInfo()
	{
		CNetMsg_Sv_ClientInfo Info;
		Info.m_ClientID = m_ClientID;
		Info.m_Local = 0;
		Info.m_Team = 0;
		Info.m_pName = m_aName;
		Info.m_pClan = "NPC";
		Info.m_Country = 0;
		Info.m_Silent = true;

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			Info.m_aUseCustomColors[p] = 0;
			Info.m_aSkinPartColors[p] = 0xFFFFFFFF;
		}

		Info.m_apSkinPartNames[SKINPART_BODY] = "kitty";
		Info.m_apSkinPartNames[SKINPART_MARKING] = "whisker";
		Info.m_apSkinPartNames[SKINPART_DECORATION] = "";
		Info.m_apSkinPartNames[SKINPART_HANDS] = "standard";
		Info.m_apSkinPartNames[SKINPART_FEET] = "standard";
		Info.m_apSkinPartNames[SKINPART_EYES] = "standard";
		return Info;
	}

	void Tick(int CurTick, CGameContext* pGameServer)
	{
		m_Core.Tick(false);
		m_Core.Move();

		// wisdom colors
		CNetMsg_Sv_ClientDrop Drop;
		Drop.m_ClientID = m_ClientID;
		Drop.m_pReason = "";
		Drop.m_Silent = true;

		CNetMsg_Sv_ClientInfo Info = GetClientInfo();
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			Info.m_aUseCustomColors[p] = true;
			float Hue = (cosf((CurTick/(float)SERVER_TICK_SPEED + p) * pi * 2 * 0.1) + 1.0f) / 2.0f;
			vec3 Color = vec3(Hue, 1, 0.5);
			Info.m_aSkinPartColors[p] = 0xFF000000 | (int(Color.r * 0xff) << 16) | (int(Color.g * 0xff) << 8) | int(Color.b * 0xff);
		}

		Info.m_Silent = true;

		for(int i = 0; i < MAX_PLAYERS; i++)
		{
			if(pGameServer->m_apPlayers[i])
			{
				pGameServer->Server()->SendPackMsg(&Drop, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
				pGameServer->Server()->SendPackMsg(&Info, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
			}
		}
	}

	void Snap(int SnappingClient, IServer* pServer)
	{
		CNetObj_PlayerInfo *pPlayerInfo = (CNetObj_PlayerInfo *)pServer->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo));
		if(!pPlayerInfo) {
			return;
		}

		pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
		pPlayerInfo->m_Latency = -m_ClientID;
		pPlayerInfo->m_Score = -m_ClientID;

		CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(pServer->SnapNewItem(NETOBJTYPE_CHARACTER, m_ClientID, sizeof(CNetObj_Character)));
		if(!pCharacter)
			return;

		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);

		// set emote
		pCharacter->m_Emote = EMOTE_NORMAL;
		pCharacter->m_AmmoCount = 0;
		pCharacter->m_Health = 0;
		pCharacter->m_Armor = 0;
		pCharacter->m_TriggeredEvents = 0;

		pCharacter->m_Weapon = WEAPON_HAMMER;
		pCharacter->m_AttackTick = 0;
		pCharacter->m_Direction = 0;
	}

	void SendDialogLineTo(int ClientID, CGameContext* pGameServer)
	{
		CModNetObj_NPCDialogLine Line;
		Line.m_NpcClientID = m_ClientID;
		str_copy(Line.m_aDialogLine, m_aDialogLines[m_aDialogLineID[ClientID] % m_aDialogLines.size()].aLine, sizeof(Line.m_aDialogLine));
		pGameServer->SendDuckNetObj(Line, ClientID);
	}
};

CNPC TestNpc;

CGameControllerExampleUI1::CGameControllerExampleUI1(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXUI1";

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "data/mods/example_ui_1"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	TestNpc.Init(MAX_CLIENTS-g_Config.m_DbgDummies-1, "Dune the wise", GameServer());
	TestNpc.m_Core.m_Pos = vec2(1056, 800);
	TestNpc.m_aDialogLines.add("Hello adventurer!\nCome closer my dear.");
	TestNpc.m_aDialogLines.add("I must tell you about him, the DEMON.");
}

void CGameControllerExampleUI1::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();

	// send NPC client info
	CNetMsg_Sv_ClientInfo Info = TestNpc.GetClientInfo();
	Server()->SendPackMsg(&Info, MSGFLAG_VITAL|MSGFLAG_NORECORD, ClientID);

	// send dialog line
	TestNpc.SendDialogLineTo(ClientID, GameServer());
}

void CGameControllerExampleUI1::Tick()
{
	IGameController::Tick();
	TestNpc.Tick(Server()->Tick(), GameServer());
}

void CGameControllerExampleUI1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	TestNpc.Snap(SnappingClient, Server());
}

void CGameControllerExampleUI1::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
	if(MsgID == 0x1) {
		const char* pStr = pUnpacker->GetString(0);
		dbg_msg("duck", "'%s'", pStr);
	}
}
