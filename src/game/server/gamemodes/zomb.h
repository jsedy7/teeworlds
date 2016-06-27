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

public:
	CGameControllerZOMB(class CGameContext *pGameServer);
	virtual void Tick();
	virtual void Snap(i32 SnappingClientID);
	virtual void OnPlayerConnect(CPlayer *pPlayer);
};
