// LordSk
#include "zomb.h"
#include <io.h>
#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/protocol.h>
#include <game/server/entity.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

/* TODO:
 * - zombie pathfinding
 * - zombie types
 * - waves
 * - flag to resurect (get one flag to another to trigger a resurect)
 */

static char msgBuff__[256];
#define dbg_zomb_msg(...)\
	str_format(msgBuff__, 256, ##__VA_ARGS__);\
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "zomb", msgBuff__);

#define NO_TARGET (-1)

static i32 randInt(i32 min, i32 max)
{
	dbg_assert(max > min, "RandInt(min, max) max must be > min");
	return (min + (rand() / (float)RAND_MAX) * (max + 1 - min));
}

void CGameControllerZOMB::Init()
{
	for(u32 i = 0; i < m_ZombCount; ++i) {
		SpawnZombie(i);
	}
}

void CGameControllerZOMB::SpawnZombie(i32 zid)
{
	vec2 spawnPos = m_ZombSpawnPoint[0];
	m_ZombAlive[zid] = true;
	m_ZombHealth[zid] = 5;
	m_ZombCharCore[zid].Reset();
	m_ZombCharCore[zid].m_Pos = spawnPos;
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

struct Node
{
	ivec2 pos;
	f32 g, h, f;
	Node* pParent;

	Node() = default;
	Node(ivec2 pos_, i32 g_, i32 h_, Node* pPrevNode_):
		pos(pos_),
		g(g_),
		h(h_),
		f(g_ + h_),
		pParent(pPrevNode_)
	{}
};

inline Node* addToList_(Node** pNodeList, u32* pCount, Node* pNode)
{
	dbg_assert((*pCount)+1 < MAX_MAP_SIZE, "list is full");\
	pNodeList[(*pCount)++] = pNode;
	return pNodeList[(*pCount)-1];
}

#define addToList(list, node) addToList_(list, &list##Count, (node))

inline Node* addNode_(Node* pNodeList, u32* pCount, Node node)
{
	dbg_assert((*pCount)+1 < MAX_MAP_SIZE, "list is full");\
	pNodeList[(*pCount)++] = node;
	return &pNodeList[(*pCount)-1];
}

#define addNode(node) addNode_(nodeList, &nodeCount, (node))

inline Node* searchList_(Node** pNodeList, u32 count, ivec2 pos)
{
	for(u32 i = 0; i < count; ++i) {
		if(pNodeList[i]->pos == pos) {
			return pNodeList[i];
		}
	}
	return 0;
}

#define searchList(list, pos) searchList_(list, list##Count, pos)

static Node* openList[MAX_MAP_SIZE];
static Node* closedList[MAX_MAP_SIZE];
static u32 openListCount = 0;
static u32 closedListCount = 0;
static Node nodeList[MAX_MAP_SIZE];
static u32 nodeCount = 0;

vec2 CGameControllerZOMB::PathFind(vec2 start, vec2 end)
{
	ivec2 mStart(start.x / 32, start.y / 32);
	ivec2 mEnd(end.x / 32, end.y / 32);

	if(m_Map[mStart.y * m_MapWidth + mStart.x] != 0 ||
	   m_Map[mEnd.y * m_MapWidth + mEnd.x] != 0) {
		dbg_zomb_msg("Error: start and end point must be clear.");
	}

	openListCount = 0;
	closedListCount = 0;
	nodeCount = 0;

	addToList(openList, addNode(Node(mStart, 0, 0, 0)));

	u32 iterations = 0;
	bool searching = true;
	while(openListCount > 0 && searching) {
		// bubble sort
		bool sorting = true;
		while(sorting) {
			u32 changes = 0;
			for(u32 i = 1; i < openListCount; ++i) {
				if(openList[i]->f < openList[i-1]->f) {
					Node* temp = openList[i];
					openList[i] = openList[i-1];
					openList[i-1] = temp;
					++changes;
				}
			}
			sorting = (changes > 0);
		}

		//dbg_zomb_msg("sorted %d nodes %.1f", openListCount, openList[0]->f);

		// pop first node from open list and add it to closed list
		Node* pCurrent = openList[0];
		openList[0] = openList[openListCount-1];
		--openListCount;
		addToList(closedList, pCurrent);

		// successors
		ivec2 from = pCurrent->pos;
		if(pCurrent->pParent) {
			from = pCurrent->pParent->pos;
		}

		for(i32 x = -1; x < 2; ++x) {
			for(i32 y = -1; y < 2; ++y) {
				ivec2 succPos = pCurrent->pos + ivec2(x,y);
				if(succPos.x < 0 || succPos.x >= (i32)m_MapWidth ||
				   succPos.y < 0 || succPos.y >= (i32)m_MapHeight ||
				   (x == 0 && y == 0) || succPos == from ||
				   m_Map[succPos.y * m_MapWidth + succPos.x] != 0) {
					continue;
				}

				// found path
				if(succPos == mEnd) {
					addToList(closedList, addNode(Node(succPos, 0, 0, pCurrent)));
					searching = false;
					break;
				}

				f32 g = 1.f;
				if(x != 0 || y != 0) { // diagonal
					g = 2.f;
				}

				if(pCurrent->pParent) {
					g += pCurrent->pParent->g;
				}

				ivec2 d = mEnd - succPos;
				f32 h = dot(d, d);
				f32 f = g + h;

				// if successor is in any of the lists and its f < new f, skip
				Node* pSuccOpen = searchList(openList, succPos);
				if(pSuccOpen) {
					if(pSuccOpen->f > f) {
						pSuccOpen->f = f;
						pSuccOpen->pParent = pCurrent;
					}
					continue;
				}
				Node* pSuccClosed = searchList(closedList, succPos);
				if(pSuccClosed) {
					if(pSuccClosed->f > f) {
						pSuccClosed->f = f;
						pSuccClosed->pParent = pCurrent;
					}
					continue;
				}

				addToList(openList, addNode(Node(succPos, g, h, pCurrent)));
			}
		}

		// TODO: remove this safeguard
		++iterations;
		if(iterations > 2500) {
			break;
		}
	}

	Node* pCur = closedList[closedListCount-1];
	ivec2 next = pCur->pos;
	while(pCur && pCur->pParent) {
		next = pCur->pos;
		pCur = pCur->pParent;
		GameServer()->CreatePlayerSpawn(vec2(next.x * 32.f + 16.f, next.y * 32.f + 16.f));
	}

	dbg_zomb_msg("pathfind iterations: %d", iterations);
	return vec2(next.x * 32.f + 16.f, next.y * 32.f + 16.f);
}

CGameControllerZOMB::CGameControllerZOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "ZOMB";
	m_ZombCount = 1;
	m_ZombSpawnPointCount = 0;

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
		m_ZombPathFindClock[i] = PATHFIND_CLOCK_TIME;
	}
}

void CGameControllerZOMB::Tick()
{
	static bool init = true;
	if(init) {
		Init();
		init = false;
	}

	m_Tick = Server()->Tick();
	IGameController::Tick();

	for(u32 i = 0; i < m_ZombCount; ++i) {
		CNetObj_PlayerInput& zi = m_ZombInput[i];

		// TODO: find target
		if(GameServer()->GetPlayerChar(0)) {
			m_ZombSurvTarget[i] = 0;
		}
		else {
			m_ZombSurvTarget[i] = NO_TARGET;
		}

		if(m_ZombSurvTarget[i] == NO_TARGET) {
			// roam around
			zi.m_Direction = i%2 ? -1 : 1;
			zi.m_Jump = m_Tick%50 > 25 ? 1 : 0;
		}
		else {
			vec2 pos = m_ZombCharCore[i].m_Pos;
			vec2 targetPos = GameServer()->GetPlayerChar(m_ZombSurvTarget[i])->GetPos();
			vec2 to = pos;
			if(--m_ZombPathFindClock[i] <= 0) {
				to = PathFind(pos, targetPos);
				m_ZombPathFindClock[i] = PATHFIND_CLOCK_TIME;
			}

			zi.m_Direction = -1;
			if(pos.x < to.x) {
				zi.m_Direction = 1;
			}

			if(abs(to.x - pos.x) < 2.f) {
				zi.m_Direction = 0;
			}

			zi.m_Jump = 0;
			if(to.y < pos.y && abs(to.y - pos.y) > 10.f) {
				zi.m_Jump = 1;
			}
		}
	}

	for(u32 i = 0; i < m_ZombCount; ++i) {
		m_ZombCharCore[i].m_Input = m_ZombInput[i];
		if(m_ZombCharCore[i].m_Input.m_Jump) {
			m_ZombCharCore[i].m_Jumped = 0;
		}
		m_ZombCharCore[i].Tick(true);
		m_ZombCharCore[i].Move();
		//dbg_zomb_msg("m_Jumped: %d", m_ZombCharCore[i].m_Jumped);
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

bool CGameControllerZOMB::OnEntity(int Index, vec2 Pos)
{
	bool r = IGameController::OnEntity(Index, Pos);

	if(Index == ENTITY_SPAWN_BLUE || Index == ENTITY_SPAWN) {
		m_ZombSpawnPoint[m_ZombSpawnPointCount++] = Pos;
	}

	return r;
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
