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
		NPC_GIVE_ITEM=0x2,
	};
};

struct CModNetObj_NPCDialogLine
{
	enum { NET_ID = ModNetID::NPC_DIALOG_LINE };

	int m_NpcClientID; // NPC client ID
	char m_aDialogLine[128];
};

struct CModNetObj_NPCGiveItem
{
	enum { NET_ID = ModNetID::NPC_GIVE_ITEM };
};

struct CNPC
{
	int m_ClientID;
	char m_aName[32];
	CCharacterCore m_Core;
	int m_aDialogLineID[MAX_PLAYERS];
	bool m_aItemSent[MAX_PLAYERS];
	CGameContext* m_pGameServer;

	struct CDialogLine
	{
		char aLine[128];

		CDialogLine()
		{
		}

		CDialogLine(const char* pStr)
		{
			str_copy(aLine, pStr, sizeof(aLine));
		}
	};

	array<CDialogLine> m_aDialogLines;

	CNPC()
	{
		mem_zero(m_aItemSent, sizeof(m_aItemSent));
	}

	void Init(int ClientID, const char* pName, CGameContext* pGameServer)
	{
		m_ClientID = ClientID;
		m_pGameServer = pGameServer;
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

	void Tick(int CurTick)
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
			if(m_pGameServer->m_apPlayers[i])
			{
				m_pGameServer->Server()->SendPackMsg(&Drop, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
				m_pGameServer->Server()->SendPackMsg(&Info, MSGFLAG_VITAL|MSGFLAG_NORECORD, i);
			}
		}
	}

	void Snap(int SnappingClient)
	{
		IServer* pServer = m_pGameServer->Server();
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

		pCharacter->m_Emote = EMOTE_NORMAL;
		if(pServer->Tick() % 250 < 6)
			pCharacter->m_Emote = EMOTE_BLINK;

		pCharacter->m_AmmoCount = 0;
		pCharacter->m_Health = 0;
		pCharacter->m_Armor = 0;
		pCharacter->m_TriggeredEvents = 0;

		pCharacter->m_Weapon = WEAPON_HAMMER;
		pCharacter->m_AttackTick = 0;
		pCharacter->m_Direction = 0;
		pCharacter->m_Angle = 0;

		// look at client
		CCharacter* pChar = m_pGameServer->GetPlayerChar(SnappingClient);
		if(pChar) {
			pCharacter->m_Angle = angle(pChar->GetPos() - m_Core.m_Pos) * 256;
		}
	}

	void SendDialogLineTo(int ClientID)
	{
		CModNetObj_NPCDialogLine Line;
		Line.m_NpcClientID = m_ClientID;
		str_copy(Line.m_aDialogLine, m_aDialogLines[m_aDialogLineID[ClientID] % m_aDialogLines.size()].aLine, sizeof(Line.m_aDialogLine));
		m_pGameServer->SendDuckNetObj(Line, ClientID);
	}

	void NextLine(int ClientID)
	{
		int OldLineID = m_aDialogLineID[ClientID];
		m_aDialogLineID[ClientID] = min(m_aDialogLineID[ClientID]+1, m_aDialogLines.size()-1);

		if(OldLineID != m_aDialogLineID[ClientID] && m_aDialogLineID[ClientID] == 3)
		{
			CModNetObj_NPCGiveItem Item;
			m_pGameServer->SendDuckNetObj(Item, ClientID);
		}
	}
};

CNPC TestNpc;

CGameControllerExampleUI1::CGameControllerExampleUI1(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "EXUI1";
	str_copy(g_Config.m_SvMap, "dm1", sizeof(g_Config.m_SvMap)); // force dm1

	// load duck mod
	if(!Server()->LoadDuckMod("", "", "data/mods/example_ui_1"))
	{
		dbg_msg("server", "failed to load duck mod");
	}

	TestNpc.Init(MAX_CLIENTS-g_Config.m_DbgDummies-1, "Dune the wise", GameServer());
	TestNpc.m_Core.m_Pos = vec2(1056, 800);
	TestNpc.m_aDialogLines.add("Hello adventurer!\nCome closer my dear.");
	TestNpc.m_aDialogLines.add("What I must tell you is of utmost importance.");
	TestNpc.m_aDialogLines.add("But first, take this powerful item with you.");
	TestNpc.m_aDialogLines.add("...");

	mem_zero(m_aInteractKeyPressed, sizeof(m_aInteractKeyPressed));
}

void CGameControllerExampleUI1::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();

	// send NPC client info
	CNetMsg_Sv_ClientInfo Info = TestNpc.GetClientInfo();
	Server()->SendPackMsg(&Info, MSGFLAG_VITAL|MSGFLAG_NORECORD, ClientID);

	// send dialog line
	TestNpc.m_aDialogLineID[ClientID] = 0;
	TestNpc.SendDialogLineTo(ClientID);
}

void CGameControllerExampleUI1::Tick()
{
	IGameController::Tick();
	TestNpc.Tick(Server()->Tick());

	for(int ClientID = 0; ClientID < MAX_PLAYERS; ClientID++)
	{
		if(GameServer()->m_apPlayers[ClientID] && m_aInteractKeyPressed[ClientID])
		{
			CCharacter* pChar = GameServer()->GetPlayerChar(ClientID);
			if(pChar)
			{
				float dist = distance(TestNpc.m_Core.m_Pos, pChar->GetPos());
				if(dist < 400)
				{
					TestNpc.NextLine(ClientID);
					TestNpc.SendDialogLineTo(ClientID);
				}
			}

			m_aInteractKeyPressed[ClientID] = false;
		}
	}
}

void CGameControllerExampleUI1::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	TestNpc.Snap(SnappingClient);
}

void CGameControllerExampleUI1::OnDuckMessage(int MsgID, CUnpacker *pUnpacker, int ClientID)
{
	dbg_msg("duck", "DuckMessage :: NetID = 0x%x", MsgID);
	if(MsgID == 0x1)
	{
		m_aInteractKeyPressed[ClientID] = true;
	}
}
