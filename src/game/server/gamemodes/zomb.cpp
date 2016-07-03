// LordSk
#include "zomb.h"
#include <io.h>
#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/server/entity.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

/* TODO:
 * - zombie types
 * - Bull zombie (charges survivors)
 * - waves
 * - flag to resurect (get one flag to another to trigger a resurect)
 */

static char msgBuff__[256];
#define dbg_zomb_msg(...)\
	memset(msgBuff__, 0, sizeof(msgBuff__));\
	str_format(msgBuff__, 256, ##__VA_ARGS__);\
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "zomb", msgBuff__);

#define NO_TARGET (-1)
#define SecondsToTick(sec) (sec * SERVER_TICK_SPEED)

enum {
	SKINPART_BODY = 0,
	SKINPART_MARKING,
	SKINPART_DECORATION,
	SKINPART_HANDS,
	SKINPART_FEET,
	SKINPART_EYES
};

enum {
	ZTYPE_BASIC = 0,
	ZTYPE_TANK
};

static const u32 g_ZombMaxHealth[] = {
	8, // ZTYPE_BASIC
	40 // ZTYPE_TANK
};

static const f32 g_ZombAttackSpeed[] = {
	2.f,
	1.f
};

enum {
	BUFF_ENRAGED = 1,
	BUFF_HEALING = (1 << 1),
	BUFF_ARMORED = (1 << 2),
	BUFF_ELITE = (1 << 3)
};

enum {
	ZOMBIE_GROUNDJUMP = 1,
	ZOMBIE_AIRJUMP,
	ZOMBIE_BIGJUMP
};

static i32 randInt(i32 min, i32 max)
{
	dbg_assert(max > min, "RandInt(min, max) max must be > min");
	return (min + (rand() / (float)RAND_MAX) * (max + 1 - min));
}

void CGameControllerZOMB::Init()
{
	for(u32 i = 0; i < m_ZombCount; ++i) {
		SpawnZombie(i, ZTYPE_BASIC, false);
	}
}

void CGameControllerZOMB::SpawnZombie(i32 zid, u32 type, bool isElite)
{
	m_ZombAlive[zid] = true;
	m_ZombType[zid] = type;
	m_ZombHealth[zid] = g_ZombMaxHealth[type];
	m_ZombBuff[zid] = 0;

	if(isElite) {
		m_ZombHealth[zid] *= 2;
		m_ZombBuff[zid] |= BUFF_ELITE;
	}

	vec2 spawnPos = m_ZombSpawnPoint[zid%m_ZombSpawnPointCount];
	m_ZombCharCore[zid].Reset();
	m_ZombCharCore[zid].m_Pos = spawnPos;
	m_ZombCharCore[zid].m_Pos.x += (-MAX_ZOMBS/2 + zid); // prevent them from being stuck
	GameServer()->m_World.m_Core.m_apCharacters[zid + MAX_SURVIVORS] = &m_ZombCharCore[zid];

	m_ZombInput[zid] = CNetObj_PlayerInput();
	m_ZombSurvTarget[zid] = NO_TARGET;
	m_ZombAttackClock[zid] = 0;
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

void CGameControllerZOMB::SwingHammer(i32 zid, u32 dmg, f32 knockback)
{
	const vec2& zpos = m_ZombCharCore[zid].m_Pos;
	GameServer()->CreateSound(zpos, SOUND_HAMMER_FIRE);

	vec2 hitDir = normalize(vec2(m_ZombInput[zid].m_TargetX, m_ZombInput[zid].m_TargetY));
	vec2 hitPos = zpos + hitDir * 21.f; // NOTE: different reach per zombie type?

	CCharacter *apEnts[MAX_CLIENTS];

	// NOTE: same here for radius
	i32 count = GameServer()->m_World.FindEntities(hitPos, 14.f, (CEntity**)apEnts,
					MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	for(i32 i = 0; i < count; ++i) {
		CCharacter *pTarget = apEnts[i];
		const vec2& targetPos = pTarget->GetPos();

		if(GameServer()->Collision()->IntersectLine(hitPos, targetPos, NULL, NULL)) {
			continue;
		}

		// set his velocity to fast upward (for now)
		if(length(targetPos-hitPos) > 0.0f) {
			GameServer()->CreateHammerHit(targetPos-normalize(targetPos-hitPos)*14.f);
		}
		else {
			GameServer()->CreateHammerHit(hitPos);
		}

		vec2 kbDir;
		if(length(targetPos - zpos) > 0.0f) {
			kbDir = normalize(targetPos - zpos);
		}
		else {
			kbDir = vec2(0.f, -1.f);
		}

		vec2 kbForce = vec2(0.f, -1.f) + normalize(kbDir + vec2(0.f, -1.1f)) * knockback;

		pTarget->TakeDamage(kbForce, dmg, MAX_SURVIVORS + zid, WEAPON_HAMMER);
	}

	m_ZombAttackTick[zid] = m_Tick;
}

struct Node
{
	ivec2 pos;
	f32 g, f;
	Node* pParent;

	Node() = default;
	Node(ivec2 pos_, f32 g_, f32 h_, Node* pParent_):
		pos(pos_),
		g(g_),
		f(g_ + h_),
		pParent(pParent_)
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

#define addNode(node) addNode_(nodeList, &nodeListCount, (node))

inline Node* searchList_(Node* pNodeList, u32 count, ivec2 pos)
{
	for(u32 i = 0; i < count; ++i) {
		if(pNodeList[i].pos == pos) {
			return &pNodeList[i];
		}
	}
	return 0;
}

#define searchList(list, pos) searchList_(list, list##Count, pos)

static i32 compareNodePtr(const void* a, const void* b)
{
	const Node* na = *(const Node**)a;
	const Node* nb = *(const Node**)b;
	if(na->f < nb->f) return -1;
	if(na->f > nb->f) return 1;
	return 0;
}

inline bool CGameControllerZOMB::InMapBounds(const ivec2& pos)
{
	return (pos.x >= 0 && pos.x < (i32)m_MapWidth &&
			pos.y >= 0 && pos.y < (i32)m_MapHeight);
}

inline bool CGameControllerZOMB::IsBlocked(const ivec2& pos)
{
	return (!InMapBounds(pos) || m_Map[pos.y * m_MapWidth + pos.x] != 0);
}

static Node* openList[MAX_MAP_SIZE];
static Node* closedList[MAX_MAP_SIZE];
static u32 openListCount = 0;
static u32 closedListCount = 0;
static Node nodeList[MAX_MAP_SIZE];
static u32 nodeListCount = 0;

bool CGameControllerZOMB::JumpStraight(const ivec2& start, const ivec2& dir, const ivec2& goal,
									   i32* out_pJumps)
{
	ivec2 jumpPos = start;
	ivec2 forcedCheckDir[2]; // places to check for walls

	if(dir.x != 0) {
		forcedCheckDir[0] = ivec2(0, 1);
		forcedCheckDir[1] = ivec2(0, -1);
	}
	else {
		forcedCheckDir[0] = ivec2(1, 0);
		forcedCheckDir[1] = ivec2(-1, 0);
	}

	*out_pJumps = 0;
	while(1) {
		if(jumpPos == goal) {
			return true;
		}

		// hit a wall, diregard jump
		if(!InMapBounds(jumpPos) || IsBlocked(jumpPos)) {
			return false;
		}

		// forced neighours check
		ivec2 wallPos = jumpPos + forcedCheckDir[0];
		ivec2 freePos = jumpPos + forcedCheckDir[0] + dir;
		if(IsBlocked(wallPos) && !IsBlocked(freePos)) {
			return true;
		}

		wallPos = jumpPos + forcedCheckDir[1];
		freePos = jumpPos + forcedCheckDir[1] + dir;
		if(IsBlocked(wallPos) && !IsBlocked(freePos)) {
			return true;
		}

		jumpPos += dir;
		++(*out_pJumps);
	}

	return false;
}

bool CGameControllerZOMB::JumpDiagonal(const ivec2& start, const ivec2& dir, const ivec2& goal,
									   i32* out_pJumps)
{
	ivec2 jumpPos = start;
	ivec2 forcedCheckDir[2]; // places to check for walls
	forcedCheckDir[0] = ivec2(-dir.x, 0);
	forcedCheckDir[1] = ivec2(0, -dir.y);

	ivec2 straigthDirs[] = {
		ivec2(dir.x, 0),
		ivec2(0, dir.y)
	};

	*out_pJumps = 0;
	while(1) {
		if(jumpPos == goal) {
			return true;
		}

		// check veritcal/horizontal
		for(u32 i = 0; i < 2; ++i) {
			i32 j;
			if(JumpStraight(jumpPos, straigthDirs[i], goal, &j)) {
				return true;
			}
		}

		// hit a wall, diregard jump
		if(!InMapBounds(jumpPos) || IsBlocked(jumpPos)) {
			return false;
		}

		// forced neighours check
		ivec2 wallPos = jumpPos + forcedCheckDir[0];
		ivec2 freePos = jumpPos + forcedCheckDir[0] + ivec2(0, dir.y);
		if(IsBlocked(wallPos) && !IsBlocked(freePos)) {
			return true;
		}

		wallPos = jumpPos + forcedCheckDir[1];
		freePos = jumpPos + forcedCheckDir[1] + ivec2(dir.x, 0);
		if(IsBlocked(wallPos) && !IsBlocked(freePos)) {
			return true;
		}

		jumpPos += dir;
		++(*out_pJumps);
	}

	return false;
}

vec2 CGameControllerZOMB::PathFind(vec2 start, vec2 end)
{
	ivec2 mStart(start.x / 32, start.y / 32);
	ivec2 mEnd(end.x / 32, end.y / 32);

	if(mStart == mEnd) {
		return end;
	}

	if(IsBlocked(mStart) || IsBlocked(mStart)) {
		dbg_zomb_msg("Error: PathFind() start and end point must be clear.");
		return end;
	}

	bool pathFound = false;
	openListCount = 0;
	closedListCount = 0;
	nodeListCount = 0;

#ifdef CONF_DEBUG
	m_DbgPathLen = 0;
	m_DbgLinesCount = 0;
	u32 maxIterations = g_Config.m_DbgPathFindIterations;
#endif

	Node* pFirst = addNode(Node(mStart, 0, 0, nullptr));
	addToList(openList, pFirst);

	u32 iterations = 0;
	while(openListCount > 0 && !pathFound) {
		qsort(openList, openListCount, sizeof(Node*), compareNodePtr);

		// pop first node from open list and add it to closed list
		Node* pCurrent = openList[0];
		openList[0] = openList[openListCount-1];
		--openListCount;
		addToList(closedList, pCurrent);

		// neighbours
		ivec2 nbDir[8];
		u32 nbCount = 0;
		const ivec2& cp = pCurrent->pos;

		// first node (TODO: move this out of the loop)
		if(!pCurrent->pParent) {
			for(i32 x = -1; x < 2; ++x) {
				for(i32 y = -1; y < 2; ++y) {
					ivec2 dir(x, y);
					ivec2 succPos = cp + dir;
					if((x == 0 && y == 0) ||
					   !InMapBounds(succPos) ||
					   IsBlocked(succPos)) {
						continue;
					}
					nbDir[nbCount++] = dir;
				}
			}
		}
		else {
			ivec2 fromDir(clamp(cp.x - pCurrent->pParent->pos.x, -1, 1),
						  clamp(cp.y - pCurrent->pParent->pos.y, -1, 1));



			// straight (add fromDir + adjacent diags)
			if(fromDir.x == 0 || fromDir.y == 0) {
				if(!IsBlocked(cp + fromDir)) {
					nbDir[nbCount++] = fromDir;

					if(fromDir.x == 0) {
						ivec2 diagR(1, fromDir.y);
						if(!IsBlocked(cp + diagR)) {
							nbDir[nbCount++] = diagR;
						}
						ivec2 diagL(-1, fromDir.y);
						if(!IsBlocked(cp + diagL)) {
							nbDir[nbCount++] = diagL;
						}
					}
					else if(fromDir.y == 0) {
						ivec2 diagB(fromDir.x, 1);
						if(!IsBlocked(cp + diagB)) {
							nbDir[nbCount++] = diagB;
						}
						ivec2 diagT(fromDir.x, -1);
						if(!IsBlocked(cp + diagT)) {
							nbDir[nbCount++] = diagT;
						}
					}
				}
			}
			// diagonal (add diag + adjacent straight dirs + forced neighbours)
			else {
				ivec2 stX(fromDir.x, 0);
				ivec2 stY(0, fromDir.y);

				if(!IsBlocked(cp + fromDir) &&
				   (!IsBlocked(cp + stX) || !IsBlocked(cp + stY))) {
					nbDir[nbCount++] = fromDir;
				}
				if(!IsBlocked(cp + stX)) {
					nbDir[nbCount++] = stX;
				}
				if(!IsBlocked(cp + stY)) {
					nbDir[nbCount++] = stY;
				}

				// forced neighbours
				ivec2 fn1(-stX.x, stY.y);
				if(IsBlocked(cp - stX) && !IsBlocked(cp + stY) && !IsBlocked(cp + fn1)) {
					nbDir[nbCount++] = fn1;
				}
				ivec2 fn2(stX.x, -stY.y);
				if(IsBlocked(cp - stY) && !IsBlocked(cp + stX) && !IsBlocked(cp + fn2)) {
					nbDir[nbCount++] = fn2;
				}
			}
		}

		// jump
		for(u32 n = 0; n < nbCount; ++n) {
			const ivec2& succDir = nbDir[n];
			ivec2 succPos = pCurrent->pos + succDir;

			if(succDir.x == 0 || succDir.y == 0) {
				i32 jumps;
				if(JumpStraight(succPos, succDir, mEnd, &jumps)) {
					ivec2 jumpPos = succPos + succDir * jumps;
					f32 g = pCurrent->g + jumps;
					f32 h = abs(mEnd.x - jumpPos.x) + abs(mEnd.y - jumpPos.y);

					if(jumpPos == mEnd) {
						addToList(closedList, addNode(Node(jumpPos, 0, 0, pCurrent)));
						pathFound = true;
						break;
					}
					else {
						Node* pSearchNode = searchList(nodeList, jumpPos);
						if(pSearchNode) {
							f32 f = g + h;
							if(f < pSearchNode->f) {
								pSearchNode->f = f;
								pSearchNode->g = g;
								pSearchNode->pParent = pCurrent;
							}
						}
						else {
							Node* pJumpNode = addNode(Node(jumpPos, g, h, pCurrent));
							addToList(openList, pJumpNode);
						}
					}
				}
			}
			else {
				i32 jumps = 0;
				if(JumpDiagonal(succPos, succDir, mEnd, &jumps)) {
					ivec2 jumpPos = succPos + succDir * jumps;
					f32 g = pCurrent->g + jumps * 2.f;
					f32 h = abs(mEnd.x - jumpPos.x) + abs(mEnd.y - jumpPos.y);

					if(jumpPos == mEnd) {
						addToList(closedList, addNode(Node(jumpPos, 0, 0, pCurrent)));
						pathFound = true;
						break;
					}
					else {
						Node* pSearchNode = searchList(nodeList, jumpPos);
						if(pSearchNode) {
							f32 f = g + h;
							if(f < pSearchNode->f) {
								pSearchNode->f = f;
								pSearchNode->g = g;
								pSearchNode->pParent = pCurrent;
							}
						}
						else {
							Node* pJumpNode = addNode(Node(jumpPos, g, h, pCurrent));
							addToList(openList, pJumpNode);
						}
					}
				}
			}
		}

		// TODO: remove this safeguard
		++iterations;
#ifdef CONF_DEBUG
		if(iterations > maxIterations) {
			break;
		}
#endif
	}

	Node* pCur = closedList[closedListCount-1];
	ivec2 next = pCur->pos;
	while(pCur && pCur->pParent) {
		next = pCur->pos;
		pCur = pCur->pParent;
	}

	return vec2(next.x * 32.f + 16.f, next.y * 32.f + 16.f);
}

#ifdef CONF_DEBUG
void CGameControllerZOMB::DebugPathAddPoint(ivec2 p)
{
	if(m_DbgPathLen >= 256) return;
	m_DbgPath[m_DbgPathLen++] = p;
}

void CGameControllerZOMB::DebugLine(ivec2 s, ivec2 e)
{
	if(m_DbgLinesCount >= 256) return;
	u32 id = m_DbgLinesCount++;
	m_DbgLines[id].start = s;
	m_DbgLines[id].end = e;
}
#endif

CGameControllerZOMB::CGameControllerZOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "ZOMB";
	m_ZombCount = MAX_ZOMBS;
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
	mem_zero(m_ZombPathFindClock, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombDestination, sizeof(vec2) * MAX_ZOMBS);
	mem_zero(m_ZombAttackClock, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombJumpClock, sizeof(i32) * MAX_ZOMBS);
	mem_zero(m_ZombAirJumps, sizeof(i32) * MAX_ZOMBS);

	for(u32 i = 0; i < m_ZombCount; ++i) {
		m_ZombCharCore[i].Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	}

	m_DoInit = true;
}

void CGameControllerZOMB::Tick()
{
	if(m_DoInit) {
		Init();
		m_DoInit = false;
	}

	m_Tick = Server()->Tick();
	IGameController::Tick();

	for(u32 i = 0; i < m_ZombCount; ++i) {
		if(!m_ZombAlive[i]) continue;
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
			vec2& dest = m_ZombDestination[i];
			if(--m_ZombPathFindClock[i] <= 0) {
				dest = PathFind(pos, targetPos);
				m_ZombPathFindClock[i] = SecondsToTick(0.1f);
			}

			zi.m_Direction = -1;
			if(pos.x < dest.x) {
				zi.m_Direction = 1;
			}

			// grounded?
			f32 phyzSize = 28.f;
			bool grounded = (GameServer()->Collision()->CheckPoint(pos.x+phyzSize/2, pos.y+phyzSize/2+5) ||
							 GameServer()->Collision()->CheckPoint(pos.x-phyzSize/2, pos.y+phyzSize/2+5));
			for(u32 i = 0; i < MAX_CLIENTS; ++i) {
				CCharacterCore* pCore = GameServer()->m_World.m_Core.m_apCharacters[i];
				if(!pCore) {
					continue;
				}

				if(pos.y < pCore->m_Pos.y &&
				   abs(pos.x - pCore->m_Pos.x) < phyzSize &&
				   abs(pos.y - pCore->m_Pos.y) < (phyzSize*1.5f)) {
					grounded = true;
					break;
				}
			}

			if(grounded) {
				m_ZombAirJumps[i] = 2;
			}

			// jump
			zi.m_Jump = 0;
			if(--m_ZombJumpClock[i] <= 0 && dest.y < pos.y) {
				f32 yDist = abs(dest.y - pos.y);
				if(yDist > 10.f) {
					if(grounded) {
						zi.m_Jump = ZOMBIE_GROUNDJUMP;
						m_ZombJumpClock[i] = SecondsToTick(0.5f);
					}
					else if(m_ZombAirJumps[i] > 0) {
						zi.m_Jump = ZOMBIE_AIRJUMP;
						m_ZombJumpClock[i] = SecondsToTick(0.5f);
						--m_ZombAirJumps[i];
					}
				}
				if(yDist > 300.f) {
					zi.m_Jump = ZOMBIE_BIGJUMP;
					m_ZombJumpClock[i] = SecondsToTick(1.0f);
				}
			}

			zi.m_TargetX = targetPos.x - pos.x;
			zi.m_TargetY = targetPos.y - pos.y;

			// attack!
			if(--m_ZombAttackClock[i] <= 0 && distance(pos, targetPos) < 56.f) {
				SwingHammer(i, 0, 2.f);
				m_ZombAttackClock[i] = SecondsToTick(1.f / g_ZombAttackSpeed[m_ZombType[i]]);
			}
		}
	}

	for(u32 i = 0; i < m_ZombCount; ++i) {
		if(!m_ZombAlive[i]) continue;
		m_ZombCharCore[i].m_Input = m_ZombInput[i];

		// handle jump
		m_ZombCharCore[i].m_Jumped = 0;
		i32 jumpTriggers = 0;
		if(m_ZombCharCore[i].m_Input.m_Jump) {
			f32 jumpStr = 14.f;
			if(m_ZombCharCore[i].m_Input.m_Jump == ZOMBIE_BIGJUMP) {
				jumpStr = 28.f;
				jumpTriggers |= COREEVENTFLAG_AIR_JUMP;
				m_ZombCharCore[i].m_Jumped |= 3;
			}
			else if(m_ZombCharCore[i].m_Input.m_Jump == ZOMBIE_AIRJUMP) {
				jumpTriggers |= COREEVENTFLAG_AIR_JUMP;
				m_ZombCharCore[i].m_Jumped |= 3;
			}
			else {
				jumpTriggers |= COREEVENTFLAG_GROUND_JUMP;
				m_ZombCharCore[i].m_Jumped |= 1;
			}

			m_ZombCharCore[i].m_Vel.y = -jumpStr;
			m_ZombCharCore[i].m_Input.m_Jump = 0;
		}

		m_ZombCharCore[i].Tick(true);

		m_ZombCharCore[i].m_TriggeredEvents |= jumpTriggers;
		f32 xDist = abs(m_ZombDestination[i].x - m_ZombCharCore[i].m_Pos.x);
		if(xDist < 5.f) {
			m_ZombCharCore[i].m_Vel.x = (m_ZombDestination[i].x - m_ZombCharCore[i].m_Pos.x);
		}

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

		// eyes
		pCharacter->m_Emote = EMOTE_NORMAL;
		if(m_ZombBuff[i]&BUFF_ENRAGED) {
			pCharacter->m_Emote = EMOTE_SURPRISE;
		}

		pCharacter->m_TriggeredEvents = m_ZombCharCore[i].m_TriggeredEvents;
		pCharacter->m_Weapon = m_ZombActiveWeapon[i];
		pCharacter->m_AttackTick = m_ZombAttackTick[i];

		m_ZombCharCore[i].Write(pCharacter);
	}

#ifdef CONF_DEBUG
	i32 tick = Server()->Tick();
	u32 laserID = 0;

	// debug path
	for(u32 i = 1; i < m_DbgPathLen; ++i) {
		CNetObj_Laser *pObj = (CNetObj_Laser*)Server()->SnapNewItem(NETOBJTYPE_LASER,
									 laserID++, sizeof(CNetObj_Laser));
		if(!pObj)
			return;

		pObj->m_X = m_DbgPath[i-1].x * 32 + 16;
		pObj->m_Y = m_DbgPath[i-1].y * 32 + 16;
		pObj->m_FromX = m_DbgPath[i].x * 32 + 16;
		pObj->m_FromY = m_DbgPath[i].y * 32 + 16;
		pObj->m_StartTick = tick;
	}

	// debug lines
	for(u32 i = 1; i < m_DbgLinesCount; ++i) {
		CNetObj_Laser *pObj = (CNetObj_Laser*)Server()->SnapNewItem(NETOBJTYPE_LASER,
									 laserID++, sizeof(CNetObj_Laser));
		if(!pObj)
			return;

		pObj->m_X = m_DbgLines[i].start.x * 32 + 16;
		pObj->m_Y = m_DbgLines[i].start.y * 32 + 16;
		pObj->m_FromX = m_DbgLines[i].end.x * 32 + 16;
		pObj->m_FromY = m_DbgLines[i].end.y * 32 + 16;
		pObj->m_StartTick = tick;
	}
#endif
}

inline i32 PackColor(i32 hue, i32 sat, i32 lgt, i32 alpha = 255)
{
	return ((alpha << 24) + (hue << 16) + (sat << 8) + lgt);
}

void CGameControllerZOMB::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);

	// send zombie client informations
	for(u32 i = 0; i < m_ZombCount; ++i) {
		u32 zombCID = MAX_SURVIVORS + i;
		CNetMsg_Sv_ClientInfo nci;
		nci.m_ClientID = zombCID;
		nci.m_Local = 0;
		nci.m_Team = 0;
		nci.m_pClan = "";
		nci.m_Country = 0;
		nci.m_apSkinPartNames[SKINPART_DECORATION] = "standard";
		nci.m_aUseCustomColors[SKINPART_DECORATION] = 0;
		nci.m_aSkinPartColors[SKINPART_DECORATION] = 0;

		// hands and feets
		i32 brown = PackColor(28, 77, 13);
		i32 red = PackColor(0, 255, 0);

		i32 handFeetColor = brown;
		if(m_ZombBuff[i]&BUFF_ENRAGED) {
			handFeetColor = red;
		}

		nci.m_apSkinPartNames[SKINPART_HANDS] = "standard";
		nci.m_aUseCustomColors[SKINPART_HANDS] = 1;
		nci.m_aSkinPartColors[SKINPART_HANDS] = handFeetColor;
		nci.m_apSkinPartNames[SKINPART_FEET] = "standard";
		nci.m_aUseCustomColors[SKINPART_FEET] = 1;
		nci.m_aSkinPartColors[SKINPART_FEET] = handFeetColor;

		nci.m_apSkinPartNames[SKINPART_EYES] = "zombie_eyes";
		nci.m_aUseCustomColors[SKINPART_EYES] = 0;
		nci.m_aSkinPartColors[SKINPART_EYES] = 0;

		nci.m_aUseCustomColors[SKINPART_MARKING] = 0;
		nci.m_aSkinPartColors[SKINPART_MARKING] = 0;

		if(m_ZombBuff[i]&BUFF_ENRAGED) {
			nci.m_apSkinPartNames[SKINPART_MARKING] = "enraged";
		}
		else {
			nci.m_apSkinPartNames[SKINPART_MARKING] = "standard";
		}

		nci.m_aUseCustomColors[SKINPART_BODY] = 0;
		nci.m_aSkinPartColors[SKINPART_BODY] = 0;

		switch(m_ZombType[i]) {
			case ZTYPE_BASIC:
				nci.m_pName = "zombie";
				nci.m_apSkinPartNames[SKINPART_BODY] = i%2 ? "zombie1" : "zombie2";
				break;

			default: break;
		}

		Server()->SendPackMsg(&nci, MSGFLAG_VITAL|MSGFLAG_NORECORD, pPlayer->GetCID());
	}

	dbg_zomb_msg("sending zombie info to %d", pPlayer->GetCID());
}

bool CGameControllerZOMB::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2) {
		return true;
	}

	// both survivors
	if(IsSurvivor(ClientID1) && IsSurvivor(ClientID2)) {
		return true;
	}

	// both zombies
	if(IsZombie(ClientID1) && IsZombie(ClientID2)) {
		return true;
	}

	return false;
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
