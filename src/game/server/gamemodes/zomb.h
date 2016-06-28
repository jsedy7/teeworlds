#pragma once
#include <game/server/gamecontroller.h>
#include <game/gamecore.h>
#include <stdint.h>

#define MAX_SURVIVORS 4
#define MAX_ZOMBS (MAX_CLIENTS - MAX_SURVIVORS)

typedef int32_t i32;
typedef uint32_t u32;

class CGameControllerZOMB : public IGameController
{
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

	void SpawnZombie(i32 zid);
	void KillZombie(i32 zid, i32 killerCID);

public:
	CGameControllerZOMB(class CGameContext *pGameServer);
	void Tick();
	void Snap(i32 SnappingClientID);
	void OnPlayerConnect(CPlayer *pPlayer);
	bool IsFriendlyFire(int ClientID1, int ClientID2) const;
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
