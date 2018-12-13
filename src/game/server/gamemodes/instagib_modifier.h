#pragma once

#include <base/vmath.h>
#include <engine/shared/protocol.h>

class CCharacter;
class CGameContext;
class CLaser;
struct CNetObj_Character;

struct CInstagibModifier
{
	CGameContext* m_pGameServer = nullptr;
	bool m_Activated = false;
	bool m_ConfigLaserJump = true;
	bool m_ConfigShield = true;
	char m_GameType[64];
	int m_ShieldCD[MAX_CLIENTS] = {0};

	void ScanGametypeForActivation(CGameContext* pGameServer, char* pGameTypeStr);
	bool IsActivated() const { return m_Activated; }
	void OnInit(char *pGameType);
	void OnTick();
	void OnCharacterSpawn(CCharacter* pChar);
	void OnCharacterFireLaser(CCharacter* pChar);
	bool OnCharacterTakeDamage(CCharacter* pChar, int Weapon, vec2 Force, int FromCID);
	void CharacterDoWeaponSwitch(CCharacter* pChar);
	void OnCharacterSnap(CCharacter* pChar, CNetObj_Character* pNetChar);

	bool LaserDoBounce(CLaser *pLaser);
};
