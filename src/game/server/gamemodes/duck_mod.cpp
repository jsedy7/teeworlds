#include "duck_mod.h"
#include <stdint.h>
#include <game/server/gamecontext.h>

CGameControllerDUCK::CGameControllerDUCK(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "DUCK";

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

typedef uint8_t u8;
typedef uint16_t u16;

struct CNetObj_Test
{
	uint16_t ClientID;
	float Value1;
};

inline u16 toHexStr(u8 val)
{
	u16 hex = (val & 15) << 8 | (val >> 4);
	u16 mask = ((hex + 0x0606) >> 4) & 0x0101;
	hex |= 0x3030;
	hex += 0x07 * mask;
	return hex;
}

void PackNetObjAsStr(u8 NetObjID, void* pNetObj, int NetObjSize, char* pOutStr, int StrBuffSize)
{
	dbg_assert(StrBuffSize >= (NetObjSize*2+4+1), "String buffer too small");
	pOutStr[0] = 0x44;
	pOutStr[1] = 0x55;
	pOutStr[2] = 0x4B;
	pOutStr[3] = NetObjID + 0x21;

	int c = 4;
	for(int i = 0; i < NetObjSize; i++)
	{
		u8 Hex = ((u8*)pNetObj)[i];
		u16 HexStr = toHexStr(Hex);
		pOutStr[c++] = HexStr & 0xFF;
		pOutStr[c++] = HexStr >> 8;
	}

	pOutStr[c] = 0; // null terminate
}

void CGameControllerDUCK::Tick()
{
	IGameController::Tick();

	static int CooldownTick = 0;
	CooldownTick++;

	if(CooldownTick >= SERVER_TICK_SPEED)
	{
		CooldownTick = 0;
		CNetObj_Test Test;
		Test.ClientID = random_int();
		Test.Value1 = (double)random_int() / (int)0x7FFFFFFF;

		char aPackedStr[256];
		PackNetObjAsStr(0, &Test, sizeof(Test), aPackedStr, sizeof(aPackedStr));
		GameServer()->SendBroadcast(aPackedStr, 0);
		dbg_msg("duck_mod", "sending ClientID=%d Value1=%g", Test.ClientID, Test.Value1);
	}
}
