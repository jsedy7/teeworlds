// LordSk
#include "zomb.h"
#include <io.h>
#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/protocol.h>
#include <game/server/entity.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

/* TODO:
 * - zombie pathfinding
 * - zombie types
 * - waves
 * - flag to resurect (get one flag to another to trigger a resurect)
 */

#define dbg_zomb_msg(...)\
	char msgBuff__[256];\
	str_format(msgBuff__, 256, ##__VA_ARGS__);\
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "zomb", msgBuff__);

#define NO_TARGET (-1)

i32 CGameControllerZOMB::RandInt(i32 min, i32 max)
{
	dbg_assert(max > min, "RandInt(min, max) max must be > min");
	return (min + (rand() / (float)RAND_MAX) * (max + 1 - min));
}

void CGameControllerZOMB::SpawnZombie(i32 zid)
{
	m_ZombAlive[zid] = true;
	m_ZombHealth[zid] = 5;
	GameServer()->m_World.m_Core.m_apCharacters[zid + MAX_SURVIVORS] = &m_ZombCharCore[zid];
}

void CGameControllerZOMB::KillZombie(i32 zid, i32 killerCID)
{
	m_ZombAlive[zid] = false;
	GameServer()->m_World.m_Core.m_apCharacters[zid + MAX_SURVIVORS] = 0;
	GameServer()->CreateSound(m_ZombCharCore[zid].m_Pos, SOUND_PLAYER_DIE);
	GameServer()->CreateSound(m_ZombCharCore[zid].m_Pos, SOUND_PLAYER_PAIN_LONG);
	GameServer()->CreateDeath(m_ZombCharCore[zid].m_Pos, 0);
	// TODO: send a kill message?
}

CGameControllerZOMB::CGameControllerZOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "ZOMB";
	m_ZombCount = MAX_ZOMBS;

	// get map info
	mem_zero(m_Map, MAX_MAP_SIZE);
	CMapItemLayerTilemap* pGameLayer = GameServer()->Layers()->GameLayer();
	m_MapWidth = pGameLayer->m_Width;
	m_MapHeight = pGameLayer->m_Height;
	CTile* pTiles = (CTile*)(GameServer()->Layers()->Map()->GetData(pGameLayer->m_Data));
	u32 count = m_MapWidth * m_MapHeight;
	for(u32 i = 0; i < count; ++i) {
		if(pTiles[i].m_Index == TILE_SOLID ||
		   pTiles[i].m_Index == TILE_NOHOOK) {
			m_Map[i] = 1;
		}
	}

	// init zombies
	mem_zero(m_ZombInput, sizeof(CNetObj_PlayerInput) * MAX_ZOMBS);
	mem_zero(m_ZombActiveWeapon, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombAttackTick, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombDmgTick, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombDmgAngle, sizeof(i32) * MAX_ZOMBS);
	memset(m_ZombSurvTarget, NO_TARGET, sizeof(i32) * MAX_ZOMBS);

	for(u32 i = 0; i < m_ZombCount; ++i) {
		CCharacterCore& core = m_ZombCharCore[i];
		core.Reset();
		core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
		core.m_Pos = vec2(250 + 50 * i, 250);

		SpawnZombie(i);
	}
}

void CGameControllerZOMB::Tick()
{
	m_Tick = Server()->Tick();
	IGameController::Tick();

	for(u32 i = 0; i < m_ZombCount; ++i) {
		CNetObj_PlayerInput& zi = m_ZombInput[i];
		if(m_ZombSurvTarget[i] == NO_TARGET) {
			// roam around
			zi.m_Direction = i%2 ? -1 : 1;
			zi.m_Jump = m_Tick%50 > 25 ? 1 : 0;
		}
	}

	for(u32 i = 0; i < m_ZombCount; ++i) {
		m_ZombCharCore[i].m_Input = m_ZombInput[i];
		m_ZombCharCore[i].Tick(true);
		m_ZombCharCore[i].Move();
		m_ZombInput[i] = m_ZombCharCore[i].m_Input;
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
		if(!m_ZombAlive[i]) {
			continue;
		}

		u32 zombCID = MAX_SURVIVORS + i;

		CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->
				SnapNewItem(NETOBJTYPE_PLAYERINFO, zombCID, sizeof(CNetObj_PlayerInfo)));
		if(!pPlayerInfo) {
			dbg_zomb_msg("Error: failed to SnapNewItem(NETOBJTYPE_PLAYERINFO)");
			return;
		}

		pPlayerInfo->m_PlayerFlags = PLAYERFLAG_READY;
		pPlayerInfo->m_Latency = zombCID;
		pPlayerInfo->m_Score = zombCID;

		CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->
				SnapNewItem(NETOBJTYPE_CHARACTER, zombCID, sizeof(CNetObj_Character)));
		if(!pCharacter) {
			dbg_zomb_msg("Error: failed to SnapNewItem(NETOBJTYPE_CHARACTER)");
			return;
		}

		pCharacter->m_Tick = m_Tick;
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
		u32 zombID = MAX_SURVIVORS + i;
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

	dbg_zomb_msg("sending zombie info to %d", pPlayer->GetCID());
}

bool CGameControllerZOMB::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2) {
		return false;
	}

	// both survivors
	if(IsSurvivor(ClientID1) && IsSurvivor(ClientID2)) {
		return false;
	}

	// both zombies
	if(IsZombie(ClientID1) && IsZombie(ClientID2)) {
		return false;
	}

	return true;
}

void CGameControllerZOMB::ZombTakeDmg(i32 CID, vec2 Force, i32 Dmg, int From, i32 Weapon)
{
	// don't take damage from other zombies
	if(IsZombie(From)) {
		return;
	}

	u32 zid = CID - MAX_SURVIVORS;
	dbg_zomb_msg("%d taking %d dmg", CID, Dmg);

	// make sure that the damage indicators doesn't group together
	++m_ZombDmgAngle[zid];
	if(Server()->Tick() < m_ZombDmgTick[zid]+25) {
		GameServer()->CreateDamageInd(m_ZombCharCore[zid].m_Pos, m_ZombDmgAngle[zid]*0.25f, Dmg);
	}
	else {
		m_ZombDmgAngle[zid] = 0;
		GameServer()->CreateDamageInd(m_ZombCharCore[zid].m_Pos, 0, Dmg);
	}
	m_ZombDmgTick[zid] = Server()->Tick();

	m_ZombHealth[zid] -= Dmg;
	if(m_ZombHealth[zid] <= 0) {
		KillZombie(zid, -1);
		dbg_zomb_msg("%d died", CID);
	}
}
