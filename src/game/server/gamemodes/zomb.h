#pragma once
#include <game/server/gamecontroller.h>
#include <engine/console.h>
#include <game/gamecore.h>
#include <stdint.h>

#define MAX_SURVIVORS 4
#define MAX_ZOMBS (MAX_CLIENTS - MAX_SURVIVORS)
#define MAX_MAP_SIZE 1024 * 1024
#define MAX_WAVES 128
#define MAX_SPAWN_QUEUE 256

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef float f32;
typedef double f64;

class CGameControllerZOMB : public IGameController
{
	bool m_DoInit;
	i32 m_Tick;
	u32 m_ZombCount;
	CCharacterCore m_ZombCharCore[MAX_ZOMBS];
	CNetObj_PlayerInput m_ZombInput[MAX_ZOMBS];
	i32 m_ZombActiveWeapon[MAX_ZOMBS];
	i32 m_ZombAttackTick[MAX_ZOMBS];

	// prevents superposition of damage stars
	i32 m_ZombDmgTick[MAX_ZOMBS];
	i32 m_ZombDmgAngle[MAX_ZOMBS];

	bool m_ZombAlive[MAX_ZOMBS];
	i32 m_ZombHealth[MAX_ZOMBS];
	u8 m_ZombType[MAX_ZOMBS];
	u8 m_ZombBuff[MAX_ZOMBS];

	i32 m_ZombSurvTarget[MAX_ZOMBS];
	vec2 m_ZombDestination[MAX_ZOMBS];

	i32 m_ZombPathFindClock[MAX_ZOMBS];
	i32 m_ZombAttackClock[MAX_ZOMBS];
	i32 m_ZombJumpClock[MAX_ZOMBS];
	i32 m_ZombAirJumps[MAX_ZOMBS];
	i32 m_ZombEnrageClock[MAX_ZOMBS];
	i32 m_ZombHookClock[MAX_ZOMBS];
	i32 m_ZombHookGrabClock[MAX_ZOMBS];
	i32 m_ZombExplodeClock[MAX_ZOMBS];

	i32 m_ZombChargeClock[MAX_ZOMBS];
	i32 m_ZombChargingClock[MAX_ZOMBS];
	vec2 m_ZombChargeVel[MAX_ZOMBS];
	bool m_ZombChargeHit[MAX_ZOMBS][MAX_CLIENTS];

	u8 m_Map[MAX_MAP_SIZE];
	u32 m_MapWidth;
	u32 m_MapHeight;

	vec2 m_ZombSpawnPoint[64];
	u32 m_ZombSpawnPointCount;

	struct SpawnCmd {
		u8 type;
		u8 isElite;
	};

	SpawnCmd m_WaveData[MAX_WAVES][MAX_SPAWN_QUEUE];
	u32 m_WaveSpawnCount[MAX_WAVES];
	u32 m_WaveCount;
	u32 m_CurrentWave;
	u32 m_SpawnCmdID;
	i32 m_SpawnClock;
	i32 m_WaveWaitClock;
	u32 m_ZombGameState;


#ifdef CONF_DEBUG
	ivec2 m_DbgPath[256];
	u32 m_DbgPathLen;

	struct DbgLine {
		ivec2 start, end;
	};

	DbgLine m_DbgLines[256];
	u32 m_DbgLinesCount;

	void DebugPathAddPoint(ivec2 p);
	void DebugLine(ivec2 s, ivec2 e);
#endif

	void Init();
	void SpawnZombie(i32 zid, u8 type, bool isElite);
	void KillZombie(i32 zid, i32 killerCID);
	void SwingHammer(i32 zid, u32 dmg, f32 knockback);

	inline bool InMapBounds(const ivec2& pos);
	inline bool IsBlocked(const ivec2& pos);
	bool JumpStraight(const ivec2& start, const ivec2& dir, const ivec2& goal,
					  i32* out_pJumps);
	bool JumpDiagonal(const ivec2& start, const ivec2& dir, const ivec2& goal,
					  i32* out_pJumps);
	vec2 PathFind(vec2 start, vec2 end);

	void SendZombieInfos(i32 zid, i32 CID);

	void HandleMovement(u32 zid, const vec2& targetPos);
	void HandleHook(u32 zid, f32 targetDist, bool targetLOS);
	void HandleBoomer(u32 zid, f32 targetDist, bool targetLOS);
	void HandleBull(u32 zid, const vec2& targetPos, f32 targetDist, bool targetLOS);
	void HandleMudge(u32 zid, const vec2& targetPos, bool targetLOS);
	void HandleHunter(u32 zid, const vec2& targetPos, f32 targetDist, bool targetLOS);

	static void ConZombStart(IConsole::IResult *pResult, void *pUserData);
	void StartZombGame(u32 startingWave = 0);
	void GameWon();
	void ChatMessage(const char* msg);
	void AnnounceWave(u32 waveID);

public:
	CGameControllerZOMB(class CGameContext *pGameServer);
	void Tick();
	void Snap(i32 SnappingClientID);
	void OnPlayerConnect(CPlayer *pPlayer);
	bool IsFriendlyFire(int ClientID1, int ClientID2) const;
	bool OnEntity(int Index, vec2 Pos);
	bool HasEnoughPlayers() const;
	void ZombTakeDmg(i32 CID, vec2 Force, i32 Dmg, int From, i32 Weapon);
};

#define IsControllerZomb(GameServer) (GameServer->GameType()[0] == 'Z' && \
	GameServer->GameType()[1] == 'O' &&\
	GameServer->GameType()[2] == 'M' &&\
	GameServer->GameType()[3] == 'B')

inline bool IsSurvivor(i32 CID) {
	return (CID < MAX_SURVIVORS);
}

inline bool IsZombie(i32 CID) {
	return (CID >= MAX_SURVIVORS);
}
